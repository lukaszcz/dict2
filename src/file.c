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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "utils.h"
#include "strutils.h"
#include "file.h"

file_entry_t file_entry[MAX_DICT_ENTRIES];
int file_entries_read = 0;
file_header_t file_header;

file_t *file_load(const char *path)
{
  file_t *r;
  char str[MAX_STR_LEN + 1];
  str[MAX_STR_LEN] = '\0';
  r = (file_t*) xmalloc(sizeof(file_t));
  r->fd = open(path, O_RDONLY);
  if (r->fd == -1)
  {
    snprintf(str, MAX_STR_LEN, "Cannot open file: %s", path);
    syserr(str);
    free(r);
    return 0;
  }
  r->length = lseek(r->fd, 0, SEEK_END);
  if (r->length == -1)
  {
    syserr("File read error");
    close(r->fd);
    free(r);
    return 0;
  }
  lseek(r->fd, 0, SEEK_SET);
  r->data = (const char*) mmap(0, r->length, PROT_READ, MAP_PRIVATE, r->fd, 0);
  if (r->data == MAP_FAILED)
  {
    syserr("mmap failed");
    close(r->fd);
    free(r);
    return 0;
  }
  if (r->length > 4 && r->data[0] == 'U' && r->data[1] == 'T' &&
      r->data[2] == 'F' && r->data[3] == '8' && r->data[4] == '\n')
  {
    r->converted = 1;
  }
  else
  {
    r->converted = 0;
  }
  r->ref = 0;
  r->path = xstrdup(path);
  return r;
}

void file_unload(file_t *file)
{
  if (file != NULL)
  {
    assert (file->ref == 0);
    free(file->path);
    munmap((void *) file->data, file->length);
    if (close(file->fd) == -1)
    {
      syserr("Cannot close file");
    }
    free(file);
  }
}

int file_read_header(file_t *file)
{
  int i, j, k;
  const char *s;

  assert (file != NULL);
  assert (file->data != NULL);

  if (file->converted == 0)
  { /* not a standard converted file - assume ISO-8859-15 encoding,
      two dictionaries */
    file_header.converted = 0;
    file_header.dicts_num = 2;
    file_header.entries_num = 2;
    strcpy(file_header.name, "dict.cc");
    file_header.keys_num[0] = 1;
    file_header.keys_num[1] = 1;
    file_header.keys[0][0] = 0;
    file_header.keys[1][0] = 1;
    file_header.size[0] = 100000;
    file_header.size[1] = 100000;
    i = 0;
  }
  else
  { /* converted, standard file */
    file_header.converted = 1;
    s = file->data;
    i = 0;
    i = file_read_line(file, i, 1);
    if (file_entries_read != 1 ||
        strcmp(file_entry[0].str, "UTF8") != 0)
    {
      error("Bad file format");
      return -1;
    }
    i = file_read_line(file, i, 1);
    k = file_header.entries_num = file_entries_read;
    if (k == 0)
    {
      error("Bad file format");
      return -1;
    }
    /* first line - languages */
    for (j = 0; j < k; ++j)
    {
      xstrncpy(file_header.langs[j], file_entry[j].str, MAX_STR_LEN);
      file_header.langs[j][MAX_STR_LEN] = '\0';
    }
    /* dictionary name */
    i = file_read_line(file, i, 1);
    if (file_entries_read != 2 || strcmp(file_entry[0].str, "name") != 0)
    {
      error("Bad file format");
      return -1;
    }
    xstrncpy(file_header.name, file_entry[1].str, MAX_NAME_LEN);
    file_header.name[MAX_NAME_LEN] = '\0';
    /* the number of dictionaries */
    i = file_read_line(file, i, 1);
    if (file_entries_read != 2 || strcmp(file_entry[0].str, "dicts_num") != 0)
    {
      error("Bad file format");
      return -1;
    }
    file_header.dicts_num = atoi(file_entry[1].str);
    if (file_header.dicts_num > MAX_DICTS_IN_FILE)
    {
      error("Bad file format");
      return -1;
    }
    i = file_read_line(file, i, 1);
    for (j = 0; j < file_header.dicts_num; ++j)
    {
      if (file_entries_read < 2 || strcmp(file_entry[0].str, "keys") != 0)
      {
        error("Bad file format");
        return -1;
      }
      file_header.keys_num[j] = file_entries_read - 1;
      for (k = 0; k < file_entries_read - 1; ++k)
      {
        file_header.keys[j][k] = atoi(file_entry[k + 1].str);
      }
      i = file_read_line(file, i, 1);
      if (file_entries_read == 2 && strcmp(file_entry[0].str, "size") == 0)
      {
        file_header.size[j] = atoi(file_entry[1].str);
        if (((unsigned) file_header.size[j]) > MAX_DICT_SIZE)
        {
          file_header.size[j] = MAX_DICT_SIZE;
        }
        i = file_read_line(file, i, 1);
      }
      else
      {
        file_header.size[j] = 128;
      }
    }
    if (file_entries_read != 1 || strcmp(file_entry[0].str, "eoh") != 0)
    {
      error("Bad file format");
      return -1;
    }
  } /* end else if converted file */
  return i;
}

int file_read_line(file_t *file, int i, int needs_utf8)
{
  /* The simple approach of ignoring UTF-8 characters here is valid
    since no ASCII character can be a part of a multibyte non-ASCII
    UTF-8 character. See man utf8. Note that it also works with ISO
    character sets. */
  int j, was_prev_colon;
  const char *s;
  int k, length;
  const char *str;

  assert (file != NULL);
  assert (i < file->length);
  assert (file->data != NULL);

  s = file->data;
  length = file->length;

  while (i < length && (isspace(s[i]) || s[i] == '#'))
  {
    while (i < length && isspace(s[i]))
    {
      ++i;
    }
    if (i < length && s[i] == '#')
    { /* a comment */
      while (i < length && s[i] != '\n')
      {
        ++i;
      }
    }
  }
  k = 0;
  while (k < MAX_DICT_ENTRIES)
  {
    while (i < length && s[i] != '\n' && isspace(s[i]))
    {
      ++i;
    }
    j = 0;
    was_prev_colon = 0;
    file_entry[k].s = s + i;
    while (i < length && j < MAX_ENTRY_LEN && s[i] != '\n')
    {
      if (s[i] == ':')
      {
        if (was_prev_colon)
        {
          ++i;
          --j;
          break;
        }
        was_prev_colon = 1;
      }
      else
      {
        was_prev_colon = 0;
      }
      file_entry[k].str[j] = s[i];
      ++i;
      ++j;
    }
    file_entry[k].str[j] = '\0';
    --j;
    while (j >= 0 && isspace(file_entry[k].str[j]))
    {
      --j;
    }
    ++j;
    file_entry[k].str[j] = '\0';
    file_entry[k].s_len = j;
    ++k;
    if (j == 0)
    {
      --k;
    }
    if (i == length || s[i] == '\n')
    {
      break;
    }
  } /* end while (k < MAX_DICT_ENTRIES) */
  if (!file->converted && needs_utf8)
  {
    for (j = 0; j < k; ++j)
    {
      str = conv_iso_8859_15_to_utf8(file_entry[j].str,
                                     strlen(file_entry[j].str));
      if (str == NULL)
      {
        return -1;
      }
      xstrncpy(file_entry[j].str, str, MAX_ENTRY_LEN);
      file_entry[j].str[MAX_ENTRY_LEN] = '\0';
      str = conv_html_to_utf8(file_entry[j].str,
                              strlen(file_entry[j].str));
      if (str == NULL)
      {
        return -1;
      }
      xstrncpy(file_entry[j].str, str, MAX_ENTRY_LEN);
      file_entry[j].str[MAX_ENTRY_LEN] = '\0';
    }
  }
  file_entries_read = k;
  return i + 1;
}

int file_skip_line(file_t *file, int i)
{
  const char *s;
  int cnt, was_hash, was_non_space;
  assert (file != NULL);
  assert (i < file->length);
  assert (file->data != NULL);
  s = file->data;
  cnt = 1;
  was_hash = 0;
  was_non_space = 0;
  while (i < file->length && cnt > 0)
  {
    if (s[i] == '\n')
    {
      was_hash = 0;
      if (was_non_space)
      {
        --cnt;
      }
      was_non_space = 0;
    }else if (was_hash == 0 && s[i] == '#')
    {
      was_hash = 1;
      ++cnt;
    }else if (!isspace(s[i]))
    {
      was_non_space = 1;
    }
    ++i;
  }
  return i;
}
