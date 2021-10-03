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
#include <glib.h>
#include "utils.h"
#include "list.h"

/* Static variables */

static list_t *list_pool = NULL;
static int list_pool_size = 0;

#ifdef DEBUG
static int lst_new = 0;
static int lst_free = 0;
#endif

/* Init & Cleanup */

void list_init()
{
}

void list_cleanup()
{
  list_t *pn;
#ifdef DEBUG
  fprintf(stderr, "\nlst_new = %d\n", lst_new);
  fprintf(stderr, "lst_free = %d\n", lst_free);
#endif
  while (list_pool != NULL)
  {
    pn = list_pool->next;
    free(list_pool);
    list_pool = pn;
  }
}

/* Lists */

list_t *list_node_new()
{
  list_t *r;
#ifdef DEBUG
  ++lst_new;
#endif
  if (list_pool != NULL)
  {
    r = list_pool;
    list_pool = list_pool->next;
    list_pool_size--;
    return r;
  }
  else
  {
    return (list_t*) xmalloc(sizeof(list_t));
  }
}

list_t *list_node_copy(list_t *node)
{
  list_t *node2 = list_node_new();
  node2->u = node->u;
  return node2;
}

void list_node_free(list_t *node)
{
#ifdef DEBUG
  ++lst_free;
#endif
  if (list_pool_size < MAX_LIST_POOL_SIZE)
  {
    node->next = list_pool;
    list_pool = node;
    list_pool_size++;
  }
  else
  {
    free(node);
  }
}

unsigned list_length(list_t *list)
{
  unsigned len = 0;
  while (list != NULL)
  {
    ++len;
    list = list->next;
  }
  return len;
}

list_t *list_last(list_t *list)
{
  if (list != NULL)
  {
    while (list->next != NULL)
    {
      list = list->next;
    }
  }
  return list;
}

list_t *list_copy(list_t *list)
{
  list_t *lst;
  list_t *l;
  if (list != NULL)
  {
    lst = list_node_new();
    lst->u = list->u;
    list = list->next;
    l = lst;
    while (list != NULL)
    {
      l->next = list_node_new();
      l = l->next;
      l->u = list->u;
      list = list->next;
    }
    l->next = NULL;
    return lst;
  }
  else
  {
    return NULL;
  }
}

list_t *list_copy_2(list_t *list, list_node_copy_t node_copy)
{
  list_t *lst;
  list_t *l;
  if (list != NULL)
  {
    lst = node_copy(list);
    list = list->next;
    l = lst;
    while (list != NULL)
    {
      l->next = node_copy(list);
      l = l->next;
      list = list->next;
    }
    l->next = NULL;
    return lst;
  }
  else
  {
    return NULL;
  }
}

list_t *list_copy1_append(list_t *list1, list_t *list2)
{
  list_t *lst;
  list_t *l;
  if (list1 != NULL)
  {
    lst = list_node_new();
    lst->u = list1->u;
    l = lst;
    list1 = list1->next;
    while (list1 != NULL)
    {
      l->next = list_node_new();
      l = l->next;
      l->u = list1->u;
      list1 = list1->next;
    }
    l->next = list2;
    return lst;
  }
  else
  {
    return list2;
  }
}

list_t *list_append(list_t *list1, list_t *list2)
{
  list_t *lst;
  if (list1 != NULL)
  {
    lst = list1;
    while (lst->next != NULL)
    {
      lst = lst->next;
    }
    lst->next = list2;
    return list1;
  }
  else
  {
    return list2;
  }
}

typedef int (*cmp_t)(const void *, const void *);

list_t *list_sort(list_t *list, list_node_cmp_t cmp)
{
  // Would it be profitable to change it to some kind of merge-sort?
  list_t **array;
  int len, i;

  len = list_length(list);
  if (len == 0)
  {
    return NULL;
  }
  array = (list_t **) xmalloc(len * sizeof(list_t *));
  for (i = 0; i < len; list = list->next, ++i)
  {
    assert (list != NULL);
    array[i] = list;
  }
  qsort(array, len, sizeof(list_t *), (cmp_t) cmp);
  for (i = 0; i < len - 1; ++i)
  {
    array[i]->next = array[i + 1];
  }
  array[len - 1]->next = NULL;
  list = array[0];
  free(array);
  return list;
}

list_t *list_unique_2(list_t *list, list_node_cmp_t cmp,
                      list_node_func_t node_free)
{
  list_t *lst;
  list_t *prev;
  if (list != NULL)
  {
    lst = list->next;
    prev = list;
    while (lst != NULL)
    {
      assert (prev->next == lst);
      if (cmp((const list_t **) &prev,
          (const list_t **) &lst) != 0)
      {
        prev = lst;
      }
      else
      {
        prev->next = lst->next;
        node_free(lst);
      }
      lst = prev->next;
    }
  }
  return list;
}

list_t *list_filter_2(list_t *list, list_node_pred_t pred,
                      list_node_func_t node_free)
{
  list_t *list2 = NULL;
  list_t *lst = NULL;
  list_t *ln = NULL;
  while (list != NULL && pred(list))
  {
    ln = list->next;
    node_free(list);
    list = ln;
  }
  lst = list2 = list;
  if (list != NULL)
  {
    list = list->next;
    while (list != NULL)
    {
      assert (lst->next == list);
      if (pred(list))
      {
        lst->next = list->next;
        node_free(list);
        list = lst->next;
      }
      else
      {
        lst = list;
        list = list->next;
      }
    }
  }
  assert (lst == NULL || lst->next == NULL);
  return list2;
}

void list_for_each(list_t *list, list_node_func_t func)
{
  while (list != NULL)
  {
    func(list);
    list = list->next;
  }
}

void list_free(list_t *list)
{
  list_t *pn;
  while (list != NULL)
  {
    pn = list->next;
    list_node_free(list);
    list = pn;
  }
}

void list_free_2(list_t *list, list_node_func_t node_free)
{
  list_t *pn;
  while (list != NULL)
  {
    pn = list->next;
    node_free(list);
    list = pn;
  }
}

list_t *strlist_copy(list_t *list)
{
  list_t *list2;
  list_t *lst;
  if (list != NULL)
  {
    list2 = strlist_node_new(list->u.str);
    lst = list2;
    list = list->next;
    while (list != NULL)
    {
      lst->next = strlist_node_new(list->u.str);
      lst = lst->next;
      list = list->next;
    }
    lst->next = NULL;
    return list2;
  }
  else
  {
    return NULL;
  }
}

int strlist_utf8_validate(list_t *list)
{
  char *end;
  const char *s;
  while (list != NULL)
  {
    s = list->u.str;
    if (!g_utf8_validate(s, -1, (const gchar **) &end))
    {
      fprintf(stderr, "WARNING: Invalid UTF-8 string.\n");
      if (end != s)
      {
        *end = '\0';
      }
      else
      {
        return 0;
      }
    }
    list = list->next;
  }
  return 1;
}

/* strlist_free frees a list of dynamically allocated strings */
void strlist_free(list_t *list)
{
  list_t *ln;
  while (list != NULL)
  {
    ln = list->next;
    free(list->u.str);
    list_node_free(list);
    list = ln;
  }
}
