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

#ifndef DICTIONARY_H
#define DICTIONARY_H

#include "limits.h"
#include "file.h"
#include "list.h"
#include "hashtable.h"


typedef struct Dict_struct{
  struct hashtable *hash;
  /* The hashtable maps keywords (strings pointing into some mmaped file)
  to lists of entry line indices (list_t with entry_line_idx being the
  valid field in the union - see list.h), i.e. a list of indices of the
  lines which contain a given keyword. */
  char name[MAX_NAME_LEN + 1];
  char langs[MAX_DICT_ENTRIES][MAX_NAME_LEN + 1];
  /* NOTE: Hashtable entries and dictionary entries are two different things.
  Dictionary entries are the parts that one in the dictionary file is broken
  into, eg. if "gut :: good" is a line in the dictionary file, then "gut" and
  "good" are two entries belonging to this line */
  int entries_num;
  int entry_order[MAX_DICT_ENTRIES];
  /* entry_order[i] contains the number of the entry that
  is to appear as i-th */
  int keys_num;
  /* keys_num: how many of the entries are keys?
     keys always come first in the entry_order table */
  file_t *file;
  int converted;
  /* converted: see file_header_t for a detailed description */
  int size;
  /* size: the number of entries */
  int keywords_num;
  /* keywords_num: the overall number of keywords hashed */
} dict_t;

typedef list_t *keyword_handle_t;

typedef enum{SEARCH_KEYWORD, SEARCH_EXACT, SEARCH_REGEX} search_t;

/* dict_create and dict_search use progress_* variables from utils.h,
   so they should be set to sensible values before calling these two
   functions */

/* Creates a dictionary numbered dict_num in file.
  Returns NULL on failure. On success increases the
  reference count of the file given as an argument.*/
dict_t *dict_create(file_t *file, int dict_num);
/* All string parameters are assumed to be valid UTF-8.
  The list returned is a list of lists of dynamically allocated strings and
  should be freed by the caller (using list_free_2(list, node_strlist_free)).
  The results returned are neither sorted nor unique and even not guaranteed
  to be valid UTF-8. One should probably apply list_sort, list_unique_2 and
  list_filer to them.*/
list_t *dict_search(dict_t *dict, const char *what, search_t search_type);
/* It is more efficient to use this function when searching the same keyword
   in multiple dictionaries. A keyword handle may be then allocated once
   for all the dictionaries, dict_search_keyword called for each dictionary
   separately, and finally the keyword handle freed. */
list_t *dict_search_keyword(dict_t *dict, keyword_handle_t handle);
/* Allocates a handle for keyword. The handle may then be passed to
   dict_search_keyword. lang is the language keyword is in. */
keyword_handle_t dict_keyword_handle_new(const char *keyword,
                                         const char *lang);
void dict_keyword_handle_free(keyword_handle_t handle);
/* Frees the dictionary. Decreases the reference count of the associated
  file. If it drops to zero then frees the file as well. */
void dict_free(dict_t *dict);

/* Returns a list of entries present at a given line. line_idx is assumed to
   indicate a valid line with an appropriate number of entries. */
list_t *line_idx_to_entry_list(dict_t *dict, int line_idx);

/* Sorts results of the most recent search (the argument most recently
   passed to dict_search is taken into account) and deletes duplicate
   entries. Returns the sorted list. */
list_t *sort_search_results(list_t *lst);

#endif
