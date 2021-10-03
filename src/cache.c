/***************************************************************************
 *   Copyright (C) 2007 by Łukasz Czajka   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/* For a description of the cache file format see: docs/cache.pdf
   (you will probably need to run 'make pdf' first) */

#include <sys/types.h>
#include <dirent.h>
#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "hashtable.h"
#include "hashtable_private.h"
#include "paths.h"
#include "list.h"
#include "strutils.h"
#include "utils.h"
#include "cache.h"

static char cache_file_path[MAX_NAME_LEN * 3];

static void get_cache_file_path(file_t *file, int dict_num)
{
  int len;

  assert (file != 0);

  len = strlen(path_cache_dir);
  strcpy(cache_file_path, path_cache_dir);
  cache_file_path[len] = '/';
  ++len;
  strcpy(cache_file_path + len, file->path);
  strcpy(cache_file_path + len, basename(cache_file_path + len));
  len = strlen(cache_file_path);
  sprintf(cache_file_path + len, "%u.cache", dict_num);
}

int cache_load(dict_t *dict, int dict_num)
{
  char *d;
  struct hashtable *h;
  file_t *file;
  time_t t1, t2;

  assert (progress_max > 0);
  assert (progress_notifier != NULL);
  assert (dict != NULL);
  assert (dict->file != NULL);

  get_cache_file_path(dict->file, dict_num);
  t1 = file_mtime(dict->file->path);
  t2 = file_mtime(cache_file_path);

  if (t1 != 0 && t2 != 0 && t1 < t2)
  {
    file = file_load(cache_file_path);
    if (file == NULL)
    {
      return 0;
    }
    ++file->ref;
    d = (char *) file->data;
    dict->size = *((int *) d);
    h = (struct hashtable *) xmalloc(sizeof(struct hashtable));
    h->extra_off = (unsigned long) d;
    h->tablelength = *((int *) (d + sizeof(int)));
    h->entrycount = *((int *) (d + 2 * sizeof(int)));
    h->loadlimit = *((int *) (d + 3 * sizeof(int)));
    h->primeindex = *((int *) (d + 4 * sizeof(int)));
    h->table = (struct entry **) (((char *) *((struct entry ***) (d + 5 * sizeof(int)))) + h->extra_off);
    h->file_start = dict->file->data;
    h->cache_file = file;
    h->lst = NULL;
    dict->hash = h;

    /* perform some rudimentary data correctness checks */
    if (h->entrycount > h->loadlimit || h->loadlimit > h->tablelength)
    {
      free(h);
      --file->ref;
      file_unload(file);
      return 0;
    }

    return 1;
  }
  else
  {
    return 0;
  }
}

void cache_save(dict_t *dict, int dict_num)
{
  FILE *f;
  int size, i, line_idx;
  int zero = 0;
  void *null_ptr = NULL;
  struct hashtable *hash;
  struct entry *e;
  void **htab;
  void *htab_off;
  void *ptr;
  unsigned long pos;
  unsigned long lists_pos;
  list_t *lst;
  file_t *file;

  assert (dict != NULL);
  assert (dict->file != NULL);
  assert (dict->hash != NULL);
  assert ( ! hashtable_is_cached(dict->hash));

  file = dict->file;
  get_cache_file_path(file, dict_num);

  f = fopen(cache_file_path, "wb");
  if (f != NULL)
  {
    notifier("cache_start");
    /* write Header */
    if (fwrite(&dict->size, sizeof(dict->size), 1, f) != 1)
    {
      syserr("Error writing cache file (1)");
      fclose(f);
      cache_clear();
      return;
    }
    hash = dict->hash;
    if (fwrite(&hash->tablelength, sizeof(hash->tablelength), 1, f) != 1)
    {
      syserr("Error writing cache file (2)");
      fclose(f);
      cache_clear();
      return;
    }
    if (fwrite(&hash->entrycount, sizeof(hash->entrycount), 1, f) != 1)
    {
      syserr("Error writing cache file (3)");
      fclose(f);
      cache_clear();
      return;
    }
    if (fwrite(&hash->loadlimit, sizeof(hash->loadlimit), 1, f) != 1)
    {
      syserr("Error writing cache file (4)");
      fclose(f);
      cache_clear();
      return;
    }
    if (fwrite(&hash->primeindex, sizeof(hash->primeindex), 1, f) != 1)
    {
      syserr("Error writing cache file (5)");
      fclose(f);
      cache_clear();
      return;
    }
    size = hash->tablelength;
    htab = (void **) xmalloc(size * sizeof(void *));
    htab_off = (void *) (ftell(f) + sizeof(htab_off));
    if (fwrite(&htab_off, sizeof(htab_off), 1, f) != 1)
    {
      syserr("Error writing cache file (6)");
      fclose(f);
      cache_clear();
      return;
    }

    /* make space for Table */
    if (fseek(f, size * sizeof(struct entry *), SEEK_CUR) == -1)
    {
      syserr("Error writing cache file (7)");
      free(htab);
      fclose(f);
      cache_clear();
      return;
    }

/*    for (i = 0; i < size * sizeof(struct entry *); ++i)
    {
      if (fputc(0, f) == EOF)
      {
        syserr("Error writing cache file (7)");
        free(htab);
        fclose(f);
        cache_clear();
        return;
      }
    }*/

    /* write Entries */
    lists_pos = ftell(f) + hash->entrycount * sizeof(struct entry);
    pos = lists_pos;
    for (i = 0; i < size; ++i)
    {
      e = hash->table[i];
      if (e != NULL)
      {
        htab[i] = (void *) ftell(f);
        while (e != NULL)
        {
          if (fwrite(&e->s_off, sizeof(e->s_off), 1, f) != 1)
          {
            syserr("Error writing cache file (8)");
            free(htab);
            fclose(f);
            cache_clear();
            return;
          }
          if (fwrite(&e->s_len, sizeof(e->s_len), 1, f) != 1)
          {
            syserr("Error writing cache file (9)");
            free(htab);
            fclose(f);
            cache_clear();
            return;
          }
          if (fwrite(&e->h, sizeof(e->h), 1, f) != 1)
          {
            syserr("Error writing cache file (10)");
            free(htab);
            fclose(f);
            cache_clear();
            return;
          }
          ptr = (void *) pos;
          if (fwrite(&ptr, sizeof(ptr), 1, f) != 1)
          {
            syserr("Error writing cache file (11)");
            free(htab);
            fclose(f);
            cache_clear();
            return;
          }
          if (e->next != NULL)
          {
            ptr = (void *) (ftell(f) + sizeof(ptr));
            if (fwrite(&ptr, sizeof(ptr), 1, f) != 1)
            {
              syserr("Error writing cache file (12)");
              free(htab);
              fclose(f);
              cache_clear();
              return;
            }
          }
          else
          {
            if (fwrite(&null_ptr, sizeof(null_ptr), 1, f) != 1)
            {
              syserr("Error writing cache file (13)");
              free(htab);
              fclose(f);
              cache_clear();
              return;
            }
          }
          lst = (list_t*) e->v;
          pos += (list_length(lst) + 1) * sizeof(int);
          e = e->next;
        } // end while
      }
      else // not e != NULL
      {
        htab[i] = NULL;
      }
    } // end for

    /* write Lists */
    assert (ftell(f) == lists_pos);
    for (i = 0; i < size; ++i)
    {
      e = hash->table[i];
      if (e != NULL)
      {
        while (e != NULL)
        {
          lst = (list_t*) e->v;
          assert (lst != NULL);
          while (lst != NULL)
          {
            line_idx = lst->u.entry_line_idx;
            if (line_idx == 0)
            {
              line_idx = 1;
            }
            if (fwrite(&line_idx, sizeof(line_idx), 1, f) != 1)
            {
              syserr("Error writing cache file (14)");
              free(htab);
              fclose(f);
              cache_clear();
              return;
            }
            lst = lst->next;
          }
          if (fwrite(&zero, sizeof(zero), 1, f) != 1)
          {
            syserr("Error writing cache file (15)");
            free(htab);
            fclose(f);
            cache_clear();
            return;
          }
          e = e->next;
        } // end while (e != NULL)
      }
    } // end for
    assert (ftell(f) == pos);

    /* write Table */
    if (fseek(f, (unsigned long) htab_off, SEEK_SET) == -1)
    {
      syserr("Error writing cache file (16)");
      free(htab);
      fclose(f);
      cache_clear();
      return;
    }
    for (i = 0; i < size; ++i)
    {
      if (fwrite(&htab[i], sizeof(htab[i]), 1, f) != 1)
      {
        syserr("Error writing cache file (17)");
        free(htab);
        fclose(f);
        cache_clear();
        return;
      }
    }
    free(htab);

#ifdef DEBUG
    fprintf(stderr, "\nhtab_off = 0x%lX\n", (unsigned long) htab_off);
    fprintf(stderr, "lists_pos = 0x%lX\n", lists_pos);
    fprintf(stderr, "size = %d\n", size);
#endif

    fclose(f);

    notifier("cache_finish");
  }
}

void cache_clear()
{
  DIR *dir;
  struct dirent *d;
  char path[MAX_NAME_LEN * 2];
  int len = strlen(path_cache_dir);

  strcpy(path, path_cache_dir);
  path[len] = '/';
  ++len;
  dir = opendir(path_cache_dir);
  if (dir == NULL)
  {
    return;
  }
  while ((d = readdir(dir)) != NULL)
  {
    strcpy(path + len, d->d_name);
    remove(path);
  }
  closedir(dir);
}
