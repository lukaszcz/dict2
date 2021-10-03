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

#ifndef LIST_H
#define LIST_H

#include "utils.h"

/* Init & Cleanup */

void list_init();
void list_cleanup();

/* Lists */

struct Dict_struct;

typedef struct Struct_list{
  union {
    int entry_line_idx;
    /* entry_line_idx is supposed to contain the index of a line
    containing an entry with a given keyword. The index is with
    respect to some predefined file_t. */
    struct Dict_struct *dict;
    char *str;
    struct Struct_list *lst;
    int value;
    void *data;
  } u;
  struct Struct_list *next;
} list_t;

typedef int (*list_node_cmp_t)(const list_t **, const list_t **);
typedef void (*list_node_func_t)(list_t *);
typedef int (*list_node_pred_t)(list_t *);
typedef list_t *(*list_node_copy_t)(const list_t *);

list_t *list_node_new();
list_t *list_node_copy(list_t *node);
/* list_node_free frees a single node; node may not be NULL */
void list_node_free(list_t *node);


unsigned list_length(list_t *list);

// Returns the last node of a list.
list_t *list_last(list_t *list);

list_t *list_copy(list_t *list);

list_t *list_copy_2(list_t *list, list_node_copy_t node_copy);

/* Copies list1 and appends list2 to it.
   Returns a copy of list1 (with list2 appended),
   or list2 if list1 is NULL. */
list_t *list_copy1_append(list_t *list1, list_t *list2);

/* Appends list2 to list1. Returns list1 with list2
   appended or list2 if list1 is NULL. */
list_t *list_append(list_t *list1, list_t *list2);

list_t *list_sort(list_t *list, list_node_cmp_t cmp);

list_t *list_unique_2(list_t *list, list_node_cmp_t cmp,
                      list_node_func_t node_free);

static inline list_t *list_unique(list_t *list, list_node_cmp_t cmp)
{
  return list_unique_2(list, cmp, list_node_free);
}

list_t *list_filter_2(list_t *list, list_node_pred_t pred,
                      list_node_func_t node_free);

static inline list_t *list_filter(list_t *list, list_node_pred_t pred)
{
  return list_filter_2(list, pred, list_node_free);
}

// Applies the given function to each node of the list.
void list_for_each(list_t *list, list_node_func_t func);

/* list_free frees the whole list; list may be NULL */
void list_free(list_t *list);

void list_free_2(list_t *list, list_node_func_t node_free);


static inline list_t *strlist_node_new(const char *str)
{
  list_t *node = list_node_new();
  node->u.str = xstrdup(str);
  return node;
}

static inline list_t *strlist_node_copy(list_t *node)
{
  assert (node != NULL);
  list_t *node2 = list_node_new();
  node2->u.str = xstrdup(node->u.str);
  return node2;
}

static inline void strlist_node_free(list_t *node)
{
  assert (node != NULL);
  free(node->u.str);
  list_node_free(node);
}

list_t *strlist_copy(list_t *list);
/* Tries to validates a given list of strings. Truncates invalid
   strings so as to preserve their valid prefixes. If some string
   cannot be truncated in this way, then returns zero, otherwise
   nonzero. When an invalid UTF-8 string is encountered then prints
   a warning message on stderr. */
int strlist_utf8_validate(list_t *list);
/* strlist_free frees a list of dynamically allocated strings */
void strlist_free(list_t *list);

static inline void node_strlist_free(list_t *node)
{
  assert (node != NULL);
  strlist_free(node->u.lst);
  list_node_free(node);
}

static inline int node_not_strlist_utf8_validate(list_t *node)
{
  assert (node != NULL);
  return !strlist_utf8_validate(node->u.lst);
}

#endif
