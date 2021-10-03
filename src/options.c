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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "paths.h"
#include "list.h"
#include "options.h"

list_t *opt_autoload_list = NULL;
list_t *opt_active_list = NULL;
int opt_german_umlaut_conversion = 1;
int opt_ignore_case = 1;
int opt_search_forms = 1;
int opt_search_inflections = 1;
int opt_search_stem = 0;
int opt_caching = 1;
int opt_cache_min_file_size = 512 * 1024;


void options_set_defaults()
{
  options_cleanup();
  opt_autoload_list = list_node_new();
  opt_autoload_list->u.str = xstrdup(path_honig_txt);
  opt_autoload_list->next = list_node_new();
  opt_autoload_list->next->u.str = xstrdup(path_irregular_verbs_de_txt);
  opt_autoload_list->next->next = NULL;
  options_add_active("dict.cc (de -> en)");
  options_add_active("dict.cc (en -> de)");
  options_add_active("Mr Honey's Dictionaries (en -> de)");
  options_add_active("Mr Honey's Dictionaries (de -> en)");
  options_add_active("List of German irregular verbs.");
  opt_ignore_case = 1;
  opt_german_umlaut_conversion = 1;
  opt_search_forms = 1;
  opt_search_inflections = 1;
  opt_search_stem = 0;
  opt_caching = 1;
  opt_cache_min_file_size = 512 * 1024;
}

void options_read_from_file(const char *path)
{
  FILE *f;
  int i, len;
  char str2[MAX_STR_LEN];
  char str[MAX_STR_LEN + 1];
  str[MAX_STR_LEN] = '\0';
  assert (path != NULL);

  f = fopen(path, "r");
  if (f == NULL)
  {
    snprintf(str, MAX_STR_LEN, "Cannot open file: %s", path);
    syserr(str);
    return;
  }
  options_set_defaults();
  options_autoload_list_clear();
  options_active_list_clear();
  while (fgets(str, MAX_STR_LEN, f) != NULL)
  {
    i = 0;
    while (isspace(str[i]))
    {
      ++i;
    }
    if (str[i] == '\0')
    {
      continue;
    }
    len = 0;
    while (isalnum(str[i + len]) || str[i + len] == '_')
    {
      ++len;
    }
    if (!isspace(str[i + len]))
    {
      error("Bad configuration file format.");
      continue;
    }
    str[i + len] = '\0';
    if (strcmp(str + i, "autoload") == 0)
    {
      if (sscanf(str + i + len + 1, "%s", str2) != 1)
      {
        fprintf(stderr, "Bad configuration file format.");
        continue;
      }
      options_add_autoload_file_last(str2);
    }
    else if (strcmp(str + i, "active") == 0)
    {
      int j = 0;
      while (j < MAX_STR_LEN - 1 && str[i + j + len + 1] != '\n' && str[i + j + len + 1] != '\0')
      {
        str2[j] = str[i + j + len + 1];
        ++j;
      }
      str2[j] = 0;
      options_add_active(str2);
    }
    else if (strcmp(str + i, "german_umlaut_conversion") == 0)
    {
      if (sscanf(str + i + len + 1, "%d", &opt_german_umlaut_conversion) != 1)
      {
        fprintf(stderr, "Bad configuration file format.");
        continue;
      }
    }
    else if (strcmp(str + i, "ignore_case") == 0)
    {
      if (sscanf(str + i + len + 1, "%d", &opt_ignore_case) != 1)
      {
        fprintf(stderr, "Bad configuration file format.");
        continue;
      }
    }
    else if (strcmp(str + i, "search_forms") == 0)
    {
      if (sscanf(str + i + len + 1, "%d", &opt_search_forms) != 1)
      {
        fprintf(stderr, "Bad configuration file format.");
        continue;
      }
    }
    else if (strcmp(str + i, "search_inflections") == 0)
    {
      if (sscanf(str + i + len + 1, "%d", &opt_search_inflections) != 1)
      {
        fprintf(stderr, "Bad configuration file format.");
        continue;
      }
    }
    else if (strcmp(str + i, "search_stem") == 0)
    {
      if (sscanf(str + i + len + 1, "%d", &opt_search_stem) != 1)
      {
        fprintf(stderr, "Bad configuration file format.");
        continue;
      }
    }
    else if (strcmp(str + i, "caching") == 0)
    {
      if (sscanf(str + i + len + 1, "%d", &opt_caching) != 1)
      {
        fprintf(stderr, "Bad configuration file format.");
        continue;
      }
    }
    else if (strcmp(str + i, "cache_min_file_size") == 0)
    {
      if (sscanf(str + i + len + 1, "%d", &opt_cache_min_file_size) != 1)
      {
        fprintf(stderr, "Bad configuration file format.");
        continue;
      }
    }
  } // end while fgets
  fclose(f);
}

void options_save_to_file(const char *path)
{
  FILE *f;
  list_t *lst;
  char str[MAX_STR_LEN + 1];
  str[MAX_STR_LEN] = '\0';
  assert (path != NULL);

  f = fopen(path, "w");
  if (f == NULL)
  {
    snprintf(str, MAX_STR_LEN, "Cannot open file: %s", path);
    syserr(str);
    return;
  }
  lst = opt_autoload_list;
  while (lst != NULL)
  {
    fprintf(f, "autoload %s\n", lst->u.str);
    lst = lst->next;
  }
  lst = opt_active_list;
  while (lst != NULL)
  {
    fprintf(f, "active %s\n", lst->u.str);
    lst = lst->next;
  }
  fprintf(f, "german_umlaut_conversion %d\n", opt_german_umlaut_conversion);
  fprintf(f, "ignore_case %d\n", opt_ignore_case);
  fprintf(f, "search_forms %d\n", opt_search_forms);
  fprintf(f, "search_inflections %d\n", opt_search_inflections);
  fprintf(f, "search_stem %d\n", opt_search_stem);
  fprintf(f, "caching %d\n", opt_caching);
  fprintf(f, "cache_min_file_size %d\n", opt_cache_min_file_size);
  fclose(f);
}

void options_cleanup()
{
  options_autoload_list_clear();
  options_active_list_clear();
}

void options_add_autoload_file(const char *path)
{
  list_t *lst;
  lst = list_node_new();
  lst->u.str = xstrdup(path);
  lst->next = opt_autoload_list;
  opt_autoload_list = lst;
}

void options_add_autoload_file_last(const char *path)
{
  list_t *lst;
  list_t *node;
  node = list_node_new();
  node->u.str = xstrdup(path);
  node->next = NULL;
  if (opt_autoload_list != NULL)
  {
    lst = opt_autoload_list;
    while (lst->next != NULL)
    {
      lst = lst->next;
    }
    lst->next = node;
  }else
  {
    opt_autoload_list = node;
  }
}

void options_autoload_list_clear()
{
  list_t *lst;
  lst = opt_autoload_list;
  while (lst != NULL)
  {
    free(lst->u.str);
    lst = lst->next;
  }
  list_free(opt_autoload_list);
  opt_autoload_list = NULL;
}

int options_check_active(const char *dict_name)
{
  list_t *lst = opt_active_list;
  while (lst != NULL)
  {
    if (strcmp(lst->u.str, dict_name) == 0)
      return 1;
    lst = lst->next;
  }
  return 0;
}

void options_add_active(const char *dict_name)
{
  if (!options_check_active(dict_name))
  {
    list_t* node = list_node_new();
    node->u.str = xstrdup(dict_name);
    node->next = opt_active_list;
    opt_active_list = node;
  }
}

void options_remove_active(const char *dict_name)
{
  list_t *lst = opt_active_list;
  list_t *prev = NULL;
  while (lst != NULL)
  {
    if (strcmp(lst->u.str, dict_name) == 0)
    {
      if (prev != NULL)
      {
        prev->next = lst->next;
      } else
      {
        opt_active_list = lst->next;
      }
      free(lst->u.str);
      list_node_free(lst);
      return;
    }
    prev = lst;
    lst = lst->next;
  }
}

void options_active_list_clear()
{
  list_t *lst;
  lst = opt_active_list;
  while (lst != NULL)
  {
    free(lst->u.str);
    lst = lst->next;
  }
  list_free(opt_active_list);
  opt_active_list = NULL;
}
