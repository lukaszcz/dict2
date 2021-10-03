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

#ifndef RBTREE_H
#define RBTREE_H

#include <stdlib.h>

typedef enum { rb_red, rb_black } color_t;

typedef struct RBNODE{
  struct RBNODE* left;
  struct RBNODE* right;
  void* key;
  color_t color;
} rbnode_t;

typedef int (*rb_cmp_fun_t)(void*, void*);
typedef void (*rb_fun_t)(void*);

typedef struct{
  rbnode_t* root;
  rbnode_t* nil;
  size_t bh; // the black-height
  rbnode_t** stack;
  size_t stack_size;
  rb_cmp_fun_t cmp;
  size_t size;
} rbtree_t;

rbtree_t* rb_new(rb_cmp_fun_t cmp);
void rb_free(rbtree_t* tree);
/* Returns a node containing x or 0 if not found. */
rbnode_t* rb_search(rbtree_t* tree, void* x);
void rb_insert(rbtree_t* tree, void* x);
/* If x is not found in tree, then does nothing. */
void rb_delete(rbtree_t* tree, void* x);
rbnode_t* rb_minimum(rbtree_t* tree);
/* Returns tree1 U {x} U tree2. Destroys tree1 and
   tree2. Precondition: y <= x <= z, where y is any key from tree1, z
   is any key from tree2. */
rbtree_t* rb_join(rbtree_t* tree1, void* x, rbtree_t* tree2);
/* Performs fun on all elements of the tree. */
void rb_for_all(rbtree_t* tree, rb_fun_t fun);
/* Returns true if tree is empty. */
int rb_empty(rbtree_t* tree);

void rb_test();

#endif
