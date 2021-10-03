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

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include "rbtree.h"

#define MAXNUM 100

static int cmp_int(void *x1, void *x2)
{
  return x1 - x2;
}

void rb_test()
{
  rbtree_t* tree = rb_new(cmp_int);
  rbtree_t* tree2 = rb_new(cmp_int);
  rbnode_t* node;
  long i;

  srand(time(0));

  for (i = 0; i < MAXNUM; ++i)
    {
      rb_insert(tree, (void *) (long)(rand() % MAXNUM));
    }

  for (i = 0; i < MAXNUM; ++i)
    {
      node = rb_minimum(tree);
      printf("%ld ", (long) node->key);
      rb_delete(tree, (void *) node->key);
    }

  printf("\n");
  printf("stack_size: %zu\n", tree->stack_size);
  if (tree->root != tree->nil)
    {
      printf("ERROR!!!\n");
    }

  for (i = 0; i < MAXNUM; ++i)
    {
      rb_insert(tree, (void *) i);
    }

  for (i = MAXNUM - 1; i >= 0; --i)
    {
      node = rb_search(tree, (void *) i);
      printf("%ld ", (long) node->key);
      rb_delete(tree, (void *) node->key);
    }

  printf("\n");
  printf("stack_size: %zu\n", tree->stack_size);
  if (tree->root != tree->nil)
    {
      printf("ERROR!!!\n");
    }

  for (i = 0; i < MAXNUM * 3; ++i)
    {
      rb_insert(tree, (void *) (long)(rand() % (MAXNUM * 3)));
    }

  for (i = 0; i < MAXNUM; ++i)
    {
      rb_insert(tree2, (void *) -(long)(rand() % MAXNUM + 2));
    }

  printf("\n");
  printf("bh1: %zu, bh2: %zu\n", tree2->bh, tree->bh);
  tree = rb_join(tree2, (void *) -1, tree);

  printf("\n");
  printf("stack_size: %zu\n", tree->stack_size);
  for (i = 0; i < MAXNUM * 4 + 1; ++i)
    {
      node = rb_minimum(tree);
      printf("%ld ", (long) node->key);
      rb_delete(tree, (void *) node->key);
    }
  printf("\n");

  rb_free(tree);
}
