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
#include <regex.h>
#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "hashtable.h"
#include "options.h"
#include "utils.h"
#include "list.h"
#include "strutils.h"
#include "cache.h"
#include "wforms.h"
#include "dictionary.h"

/* Used internally by several functions. */
static dict_t *current_dict;
static list_t *searched_text_variants_lst = NULL;

static int lst_cmp(const list_t **pnode1, const list_t **pnode2);
static list_t *node_line_idx_to_entry_list(const list_t *node);
/* Prepends the results of searching str in dict to lst. */
static list_t *search_prepend(dict_t *dict, const char *str, list_t *lst);
/* Prepends str to the list of strings - lst */
static list_t *strlist_prepend(const char *str, list_t *lst);
static list_t *strlist_convert(const char *str, list_t *lst,
                               int i, const char *conv);
/* Prepends ae -> ä, ue -> ü, etc. If ignore_str is false then str itself
   (unmodified) is not prepended. i is the start index at which to start
   looking for possible conversions in str (though if str is prepended,
   then it's prepended starting from index 0 - i.e. the whole of it). */
static list_t *strlist_prepend_de_conversions(const char *str,
                                              list_t *lst, int i,
                                              int ignore_str);
static list_t *strlist_prepend_umlaut_conversions(list_t *lst);
static list_t *strlist_prepend_case_conversions(list_t *lst);

static list_t *dict_search_exact(dict_t *dict, const char *what);
static list_t *dict_search_regex(dict_t *dict, const char *regex);

/* Returns nonzero on success. */
static int dict_create_hashtable(dict_t *dict, file_t *file,
                                 int dict_num, int i);

/************************************************************************/

static int lst_cmp(const list_t **pnode1, const list_t **pnode2)
{
  int s1_len, s2_len;
  int ss1_len, ss2_len;
  char *s1;
  char *s2;
  char *ss1;
  char *ss2;
  char *is1;
  char *is2;
  int cmp1, cmp2;
  char c1, c2;
  list_t *lst1;
  list_t *lst2;
  list_t *lst;

  assert (pnode1 != NULL);
  assert (*pnode1 != NULL);
  assert ((*pnode1)->u.lst != NULL);
  assert ((*pnode1)->u.lst->u.str != NULL);
  assert (pnode2 != NULL);
  assert (*pnode2 != NULL);
  assert ((*pnode2)->u.lst != NULL);
  assert ((*pnode2)->u.lst->u.str != NULL);

  /* cmp1 = lexicographical comparison (if the key entries are equal, then
    compare by the second entries (fields), etc) */
  lst1 = (*pnode1)->u.lst;
  lst2 = (*pnode2)->u.lst;
  s1 = lst1->u.str;
  s2 = lst2->u.str;
  cmp1 = g_utf8_collate(s1, s2);
  if (cmp1 == 0)
  {
    lst1 = lst1->next;
    lst2 = lst2->next;
    while (cmp1 == 0 && lst1 != NULL && lst2 != NULL)
    {
      cmp1 = g_utf8_collate(lst1->u.str, lst2->u.str);
      lst1 = lst1->next;
      lst2 = lst2->next;
    }
    if (cmp1 == 0)
    {
      if (lst1 == NULL && lst2 == NULL)
      {
        return 0;
      }
      else if (lst1 == NULL)
      {
        cmp1 = -1;
      }
      else
      {
        assert (lst2 == NULL);
        cmp1 = 1;
      }
    }
  }

  /* variants of the searched text may differ from the text typed by the
    user only by the case of the first letter in a word or by
    umlaut-conversions */

  /* check if s1/s2 is equal to one of the searched text variants */
  lst = searched_text_variants_lst;
  while (lst != NULL)
  {
    if (strcmp(s1, lst->u.str) == 0)
    {
      if (strcmp(s2, lst->u.str) == 0)
      {
        return cmp1;
      }
      else
      {
        return -1;
      }
    }
    else if (strcmp(s2, lst->u.str) == 0)
    {
      return 1;
    }
    lst = lst->next;
  } /* end while (lst != NULL) */

  s1_len = strlen(s1);
  s2_len = strlen(s2);
  ss1 = (char *) trim_brackets(s1, s1_len, &ss1_len);
  ss2 = (char *) trim_brackets(s2, s2_len, &ss2_len);
  /* remove a leading 'to' */
  if (ss1_len > 3 && ss1[0] == 't' && ss1[1] == 'o' && ss1[2] == ' ')
  {
    ss1_len -= 3;
    ss1 += 3;
  }
  if (ss2_len > 3 && ss2[0] == 't' && ss2[1] == 'o' && ss2[2] == ' ')
  {
    ss2_len -= 3;
    ss2 += 3;
  }
  c1 = ss1[ss1_len];
  ss1[ss1_len] = '\0';
  c2 = ss2[ss2_len];
  ss2[ss2_len] = '\0';

  lst = searched_text_variants_lst;
  while (lst != NULL)
  {
    if (strcmp(ss1, lst->u.str) == 0)
    {
      if (strcmp(ss2, lst->u.str) == 0)
      {
        ss1[ss1_len] = c1;
        ss2[ss2_len] = c2;
        return cmp1;
      }
      else
      {
        ss1[ss1_len] = c1;
        ss2[ss2_len] = c2;
        return -1;
      }
    }
    else if (strcmp(ss2, lst->u.str) == 0)
    {
      ss1[ss1_len] = c1;
      ss2[ss2_len] = c2;
      return 1;
    }
    lst = lst->next;
  } /* end while (lst != NULL) */

  /* if s1/s2 consists of multiple words, then check whether one of searched
  text variants is one of the words of s1/s2*/
  ss1[ss1_len] = c1;
  ss2[ss2_len] = c2;
  if (strchr(s1, ' ') != NULL || strchr(s2, ' ') != NULL)
  {
    lst = searched_text_variants_lst;
    while (lst != NULL)
    {
      int len = strlen(lst->u.str);
      is1 = strstr(s1, lst->u.str);
      is2 = strstr(s2, lst->u.str);
      if (is1 != NULL)
      {
        if ((is1 != s1 && isalpha(*(is1 - 1))) || isalpha(is1[len]))
        {
          is1 = NULL;
        }
      }
      if (is2 != NULL)
      {
        if ((is2 != s2 && isalpha(*(is2 - 1))) || isalpha(is2[len]))
        {
          is2 = NULL;
        }
      }
      if (is1 != NULL)
      {
        if (is2 != NULL)
        {
          return cmp1;
        }
        else
        {
          return -1;
        }
      }
      else if (is2 != NULL)
      {
        return 1;
      }
      lst = lst->next;
    } /* end while (lst != NULL) */
  }
  ss1[ss1_len] = '\0';
  ss2[ss2_len] = '\0';

  cmp2 = g_utf8_collate(ss1, ss2);
  ss1[ss1_len] = c1;
  ss2[ss2_len] = c2;
  return (cmp2 == 0) ? cmp1 : cmp2;
}

static list_t *node_line_idx_to_entry_list(const list_t *node)
{
  list_t *node2;
  assert (current_dict != NULL);
  assert (node != NULL);
  node2 = list_node_new();
  node2->u.lst = line_idx_to_entry_list(current_dict, node->u.entry_line_idx);
  return node2;
}

static list_t *search_prepend(dict_t *dict, const char *str, list_t *lst)
{
  list_t *lst2;
  assert (str != NULL);
  if (!dict->converted)
  {
    str = conv_utf8_to_iso_8859_15(str, strlen(str));
  }
  lst2 = (list_t*) hashtable_search(dict->hash, str, strlen(str));
  return list_copy1_append(lst2, lst);
}

static list_t *strlist_prepend(const char *str, list_t *lst)
{
  if (strlen(str) != 0)
  {
    list_t *node = list_node_new();
    node->u.str = xstrdup(str);
    node->next = lst;
    return node;
  }
  else
  {
    return lst;
  }
}

static list_t *strlist_convert(const char *str, list_t *lst,
                               int i, const char *conv)
{
  int len;
  char str2[MAX_STR_LEN + 1];
  assert (strlen(str) + strlen(conv) <= MAX_STR_LEN + 1);
  str2[MAX_STR_LEN] = '\0';
  len = strlen(conv);
  strncpy(str2, str, i - 1);
  strcpy(str2 + i - 1, conv);
  strcpy(str2 + i - 1 + len, str + i + 1);
  lst = strlist_prepend_de_conversions(str2, lst,
                                       i - 1 + len, 0);
  return lst;
}

static list_t *strlist_prepend_de_conversions(const char *str,
                                              list_t *lst, int i,
                                              int ignore_str)
{
  if (str[i] != '\0')
  {
    char c = '\0';
    while (str[i] != '\0')
    {
      if (str[i] == 's' && c == 's')
      {
        lst = strlist_convert(str, lst, i, "ß");
        c = '\0';
      }
      else if (str[i] == 'e')
      {
        if (c == 'a')
        {
          lst = strlist_convert(str, lst, i, "ä");
          c = '\0';
        }
        else if (c == 'o')
        {
          lst = strlist_convert(str, lst, i, "ö");
          c = '\0';
        }
        else if (c == 'u')
        {
          lst = strlist_convert(str, lst, i, "ü");
          c = '\0';
        }
        else
        {
          c = str[i];
        }
      }
      else
      {
        c = str[i];
      }
      ++i;
    }
  }
  if (!ignore_str)
  {
    return strlist_prepend(str, lst);
  }
  else
  {
    return lst;
  }
}

static list_t *strlist_prepend_umlaut_conversions(list_t *lst)
{
  list_t *lst2;
  if (opt_german_umlaut_conversion)
  {
    lst2 = lst;
    while (lst2 != NULL)
    {
      lst = strlist_prepend_de_conversions(lst2->u.str, lst, 0, 1);
      lst2 = lst2->next;
    }
  }
  return lst;
}

static list_t *strlist_prepend_case_conversions(list_t *lst)
{
  int len;
  list_t *lst2;
  gunichar c;
  const char *s;
  char str[MAX_STR_LEN + 1];
  str[MAX_STR_LEN] = '\0';

  if (opt_ignore_case)
  {
    lst2 = lst;
    while (lst2 != NULL)
    {
      s = g_utf8_next_char(lst2->u.str);
      c = g_utf8_get_char(lst2->u.str);
      if (g_unichar_isupper(c))
      {
        c = g_unichar_tolower(c);
        len = g_unichar_to_utf8(c, str);
        xstrncpy(str + len, s, MAX_STR_LEN - len);
        lst = strlist_prepend(str, lst);
      }
      else if (g_unichar_islower(c))
      {
        c = g_unichar_toupper(c);
        len = g_unichar_to_utf8(c, str);
        xstrncpy(str + len, s, MAX_STR_LEN - len);
        lst = strlist_prepend(str, lst);
      }
      lst2 = lst2->next;
    }
  }
  return lst;
}

static void create_searched_text_variants_lst(const char *what,
                                              const char *lang)
{
  list_t *lst1;
  list_t *lst;

  strlist_free(searched_text_variants_lst);
  searched_text_variants_lst = strlist_node_new(what);
  searched_text_variants_lst->next = NULL;
  searched_text_variants_lst =
      strlist_prepend_case_conversions(searched_text_variants_lst);
  if (strcmp(lang, "de") == 0)
  {
    searched_text_variants_lst =
        strlist_prepend_umlaut_conversions(searched_text_variants_lst);
  }
  // make the original text go first
  lst = searched_text_variants_lst;
  assert (lst != NULL);
  if (lst->next != NULL)
  {
    lst1 = lst;
    lst = lst->next;
    while (lst->next != NULL)
    {
      lst1 = lst;
      lst = lst->next;
    }
    lst1->next = NULL;
    lst->next = searched_text_variants_lst;
    searched_text_variants_lst = lst;
  }
}

static list_t *dict_search_exact(dict_t *dict, const char *what)
{
  list_t *lst;
  list_t *lst2;
  list_t *lst3;
  list_t *node;
  list_t *list;
  const char *ss;
  const char *s;
  int ss_len;
  int i, j;

  assert (dict != NULL);
  assert (dict->hash != NULL);
  assert (dict->file != NULL);

  lst2 = strlist_prepend(what, NULL);
  lst2 = strlist_prepend_case_conversions(lst2);
  if (strcmp(dict->langs[0], "de") == 0)
  {
    lst2 = strlist_prepend_umlaut_conversions(lst2);
  }

  lst = NULL;
  list = lst2;
  while (lst2 != NULL)
  {
    lst = search_prepend(dict, lst2->u.str, lst);
    lst2 = lst2->next;
  }

  lst2 = NULL;
  while (lst != NULL)
  {
    file_read_line(dict->file, lst->u.entry_line_idx, 1);
    for (i = 0; i < dict->keys_num; ++i)
    {
      j = dict->entry_order[i];
      /* We have to use the *.str field because the file may not be in
         UTF-8, but the searched strings always are. */
      ss = trim_brackets(file_entry[j].str, strlen(file_entry[j].str),
                         &ss_len);
      lst3 = list;
      while (lst3 != NULL)
      {
        s = lst3->u.str;
        lst3 = lst3->next;
        if (strlen(s) == ss_len && memcmp(s, ss, ss_len) == 0)
        {
          node = list_node_new();
          node->next = lst2;
          node->u.entry_line_idx = lst->u.entry_line_idx;
          lst2 = node;
          break;
        }
      }
    }
    lst = lst->next;
  }
  strlist_free(list);
  return lst2;
}

static list_t *dict_search_regex(dict_t *dict, const char *regex)
{
  regex_t reg;
  int err, i, step, nexti, prev_i, j, k;
  char error_buf[MAX_STR_LEN + 1];
  list_t *lst;
  list_t *node;

  assert (dict != NULL);
  assert (dict->file != NULL);
  assert (dict->hash != NULL);
  if ((err = regcomp(&reg, regex, REG_EXTENDED | REG_NOSUB |
       (opt_ignore_case ? REG_ICASE : 0))) != 0)
  {
    regerror(err, &reg, error_buf, MAX_STR_LEN);
    error_buf[MAX_STR_LEN] = '\0';
    error(error_buf);
    regfree(&reg);
    return NULL;
  }
  i = file_read_header(dict->file);
  step = dict->file->length / progress_max;
  nexti = step;
  lst = NULL;
  while (i < dict->file->length)
  {
    if (i >= nexti)
    {
      if (progress_notifier() == 0)
      {
        regfree(&reg);
        return lst;
      }
      nexti += step;
    }
    prev_i = i;
    i = file_read_line(dict->file, i, 1);
    for (j = 0; j < dict->keys_num; ++j)
    {
      k = dict->entry_order[j];
      if (regexec(&reg, file_entry[k].str, 0, 0, 0) == 0)
      { /* match found */
        node = list_node_new();
        node->next = lst;
        node->u.entry_line_idx = prev_i;
        lst = node;
      }
    }
  } // end main loop
  regfree(&reg);
  return lst;
}

/* Returns non-zero on success (even if the hashtable was partially read). */
static int dict_create_hashtable(dict_t *dict, file_t *file,
                                 int dict_num, int i)
{
  const char *s;
  const char *ss;
  int s_len, ss_len;
  list_t *lst;
  list_t *node;
  int step, nexti;
  int j, k, length;
  int line_idx;
  const char *file_start = dict->file->data;

  dict->hash = hashtable_create(file_header.size[dict_num], file_start);
  if (dict->hash == NULL)
  {
    fatal("Error loading file - cannot create a hashtable.");
    return 0;
  }
  assert (progress_max > 0);
  dict->size = 0;
  length = file->length;
  step = length / progress_max;
  nexti = step;
  while (i < length && i != -1)
  {
    if (i >= nexti)
    {
      if (progress_notifier() == 0)
      {
        return 0;
      }
      nexti += step;
    }
    line_idx = i;
    i = file_read_line(file, i, 0);
    if (file_entries_read == 0)
    {
      continue;
    }
    else if (file_entries_read < dict->entries_num || i == -1)
    {
      error("Bad file format. Dictionary partially read.");
      ++file->ref;
      return 1;
    }
    ++dict->size;
    for (k = 0; k < dict->keys_num; ++k)
    {
      j = dict->entry_order[k];
      s = file_entry[j].s;
      s_len = file_entry[j].s_len;
      assert (s_len > 0);
      /* insert all keywords plus the whole entry */
      /* skip things in various kinds of brackets */
      ss = trim_brackets(s, s_len, &ss_len);
      lst = (list_t*) hashtable_search(dict->hash, ss, ss_len);
      node = list_node_new();
      node->u.entry_line_idx = line_idx;
      if (lst == 0)
      {
        node->next = NULL;
        hashtable_insert(dict->hash, ss - file_start, ss_len, node);
      }
      else
      {
        node->next = lst->next;
        lst->next = node;
      }
      j = 0;
      assert (j < s_len || !isspace(s[0]));
      while (j < s_len)
      {
        ss = s + j;
        while (j < s_len && !(isspace(s[j]) || ispunct(s[j])))
        {
         /* NOTE: We cannot simply use isalnum as we would possibly
          skip some alphanumeric UTF-8 characters. Alternatively, we
          could use UTF-8 character manipulation functions, but
          this would be too complicated and wouldn't work with ISO
          charater sets. */
          ++j;
        }
        ss_len = s + j - ss;
        /* Don't hash too short keywords to avoid cluttering the table. */
        if ((dict->converted &&
                    g_utf8_strlen(ss, ss_len) >= MIN_KEYWORD_CHARS) ||
                    (!dict->converted && ss_len >= MIN_KEYWORD_CHARS))
        {
          lst = (list_t*) hashtable_search(dict->hash, ss, ss_len);
          if (lst == 0 || (lst->u.entry_line_idx != line_idx &&
                     (lst->next == 0 ||
                     lst->next->u.entry_line_idx != line_idx)))
          { /* avoid duplicate entries */
            node = list_node_new();
            node->u.entry_line_idx = line_idx;
            if (lst == 0)
            {
              node->next = NULL;
              hashtable_insert(dict->hash, ss - file_start, ss_len, node);
            }
            else
            {
              node->next = lst->next;
              lst->next = node;
            }
          }
        }
        while (j < s_len && (isspace(s[j]) || ispunct(s[j])))
        {
          ++j;
        }
      } /* end while (j < s_len) */
    } /* end for each key */
  } /* end main loop while (i < length) */
  return 1;
}

dict_t *dict_create(file_t *file, int dict_num)
{
  dict_t *dict;
  int i, j, k, kk, len0, len1, len2, found, success;

  assert (file != NULL);

  if (file->length == 0)
  {
    error("Empty file");
    return NULL;
  }

  dict = (dict_t*) xmalloc(sizeof(dict_t));
  dict->file = file;
  i = file_read_header(file);
  if (i == -1)
  {
    free(dict);
    return NULL;
  }
  assert (dict_num < file_header.dicts_num);
  dict->converted = file_header.converted;
  dict->entries_num = file_header.entries_num;
  dict->keys_num = file_header.keys_num[dict_num];
  if (dict->keys_num > dict->entries_num)
  {
    error("Bad file format.");
    free(dict);
    return NULL;
  }
  for (j = 0; j < dict->keys_num; ++j)
  {
    dict->entry_order[j] = file_header.keys[dict_num][j];
  }
  for (k = 0; k < dict->entries_num; ++k)
  {
    found = 0;
    for (kk = 0; kk < file_header.keys_num[dict_num]; ++kk)
    {
      if (file_header.keys[dict_num][kk] == k)
      {
        found = 1; break;
      }
    }
    if (!found)
    {
      assert (j < dict->entries_num);
      dict->entry_order[j++] = k;
    }
  }
  assert (j == dict->entries_num);
  if (dict->converted)
  {
    for (j = 0; j < dict->entries_num; ++j)
    {
      assert (dict->entry_order[j] < dict->entries_num);
      assert (dict->entry_order[j] >= 0);
      xstrncpy(dict->langs[j], file_header.langs[dict->entry_order[j]],
               MAX_NAME_LEN);
      dict->langs[j][MAX_NAME_LEN] = '\0';
    }
    if (dict->entries_num == 2)
    {
      len1 = strlen(dict->langs[0]);
      len2 = strlen(dict->langs[1]);
      len0 = strlen(file_header.name);
      strcpy(dict->name, file_header.name);
      if (len0 + len1 + len2 + 7 < MAX_NAME_LEN)
      {
        dict->name[len0] = ' ';
        dict->name[len0 + 1] = '(';
        strcpy(dict->name + len0 + 2, dict->langs[0]);
        dict->name[len0 + len1 + 2] = ' ';
        dict->name[len0 + len1 + 3] = '-';
        dict->name[len0 + len1 + 4] = '>';
        dict->name[len0 + len1 + 5] = ' ';
        strcpy(dict->name + len0 + len1 + 6, dict->langs[1]);
        dict->name[len0 + len1 + len2 + 6] = ')';
        dict->name[len0 + len1 + len2 + 7] = '\0';
      }
    }
    else
    {
      strcpy(dict->name, file_header.name);
    }
  } /* end if converted */

  if (!cache_load(dict, dict_num))
  {
    success = dict_create_hashtable(dict, file, dict_num, i);
    if (success && opt_caching &&
        file_size(file->path) >= opt_cache_min_file_size)
    {
      cache_save(dict, dict_num);
    }
  }
  else
  {
    success = 1;
  }

  if (!dict->converted)
  {
    if (hashtable_search(dict->hash, "schlecht", 8) == NULL)
    { /* en -> de */
      strcpy(dict->langs[0], "en");
      strcpy(dict->langs[1], "de");
      strcpy(dict->name, "dict.cc (en -> de)");
    }
    else
    { /* de -> en */
      strcpy(dict->langs[0], "de");
      strcpy(dict->langs[1], "en");
      strcpy(dict->name, "dict.cc (de -> en)");
    }
  }
  dict->keywords_num = hashtable_count(dict->hash);
  ++file->ref;
  if (success)
  {
    return dict;
  }
  else
  {
    dict_free(dict);
    return NULL;
  }
}

list_t *dict_search(dict_t *dict, const char *what, search_t search_type)
{
  list_t *lst1;
  list_t *lst;
  keyword_handle_t handle;

  switch(search_type){
    case SEARCH_KEYWORD:
      handle = dict_keyword_handle_new(what, dict->langs[0]);
      lst = dict_search_keyword(dict, handle);
      dict_keyword_handle_free(handle);
      return lst;
    case SEARCH_REGEX:
      create_searched_text_variants_lst(what, dict->langs[0]);
      lst1 = dict_search_regex(dict, what);
      break;
    case SEARCH_EXACT:
      create_searched_text_variants_lst(what, dict->langs[0]);
      lst1 = dict_search_exact(dict, what);
      break;
    default:
      fatal("Programming error - unknown search type.");
      break;
  };
  current_dict = dict;
  lst = list_copy_2(lst1, node_line_idx_to_entry_list);
  list_free(lst1);
  return lst;
}

list_t *dict_search_keyword(dict_t *dict, keyword_handle_t handle)
{
  list_t *lst;
  list_t *lst2;

  lst = handle;
  lst2 = NULL;
  while (lst != NULL)
  {
    lst2 = search_prepend(dict, lst->u.str, lst2);
    lst = lst->next;
  }

  current_dict = dict;
  lst = list_copy_2(lst2, node_line_idx_to_entry_list);
  list_free(lst2);

  return lst;
}

keyword_handle_t dict_keyword_handle_new(const char *keyword,
                                         const char *lang)
{
  list_t *lst2;
  list_t *lst;
  char str[MAX_STR_LEN + 1];
  int i, single_word;
  str[MAX_STR_LEN] = '\0';

  create_searched_text_variants_lst(keyword, lang);

  lst = strlist_prepend(keyword, NULL);
  if (strcmp(lang, "de") == 0)
  {
    lst = strlist_prepend_umlaut_conversions(lst);
  }
  if (opt_ignore_case)
  {
    lst2 = lst;
    while (lst2 != NULL)
    {
      lst2->u.str[0] = tolower(lst2->u.str[0]);
      lst2 = lst2->next;
    }
  }
  /* perform the following conversions for single words only */
  i = 0;
  single_word = 1;
  while (keyword[i] != '\0')
  {
    if (isspace(keyword[i]))
    {
      single_word = 0;
      break;
    }
    ++i;
  }
  if (single_word)
  {
    lst = wforms_add(lst, lang);
  }
  lst = strlist_prepend_case_conversions(lst);

#ifdef DEBUG
  fprintf(stderr, "dict_keyword_handle_new: kwds = %d\n", list_length(lst));
#endif

  return lst;
}

void dict_keyword_handle_free(keyword_handle_t handle)
{
  strlist_free(handle);
}

void dict_free(dict_t *dict)
{
  assert (dict != NULL);
  assert (dict->hash != NULL);
  assert (dict->file != NULL);

  hashtable_destroy(dict->hash);

  if (--dict->file->ref == 0)
  {
    file_unload(dict->file);
  }
  free(dict);

  strlist_free(searched_text_variants_lst);
  searched_text_variants_lst = NULL;
}

list_t *line_idx_to_entry_list(dict_t *dict, int line_idx)
{
  list_t *list;
  list_t *lst;
  int i;
  assert (dict != NULL);
  assert (dict->file != NULL);
  assert (line_idx < dict->file->length);

  file_read_line(dict->file, line_idx, 1);
  assert (file_entries_read == dict->entries_num);

  lst = list_node_new();
  lst->u.str = xstrdup(file_entry[dict->entry_order[0]].str);
  list = lst;
  for (i = 1; i < file_entries_read; ++i)
  {
    lst->next = list_node_new();
    lst = lst->next;
    lst->u.str = xstrdup(file_entry[dict->entry_order[i]].str);
  }
  lst->next = NULL;
  return list;
}

list_t *sort_search_results(list_t *lst)
{
  lst = list_filter(lst, node_not_strlist_utf8_validate);
  lst = list_sort(lst, lst_cmp);
  lst = list_unique_2(lst, lst_cmp, node_strlist_free);
  return lst;
}
