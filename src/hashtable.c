/* Copyright (C) 2004 Christopher Clark <firstname.lastname@cl.cam.ac.uk> */
/* Copyright (C) 2006-2007 Łukasz Czajka <firstname.lastname@students.mimuw.edu.pl> */

#include "hashtable.h"
#include "hashtable_private.h"
#include "fnv.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/*
Credit for primes table: Aaron Krowne
 http://br.endernet.org/~akrowne/
 http://planetmath.org/encyclopedia/GoodHashTablePrimes.html
*/
static const unsigned int primes[] = {
53, 97, 193, 389,
769, 1543, 3079, 6151,
12289, 24593, 49157, 98317,
196613, 393241, 786433, 1572869,
3145739, 6291469, 12582917, 25165843,
50331653, 100663319, 201326611, 402653189,
805306457, 1610612741
};
const unsigned int prime_table_length = sizeof(primes)/sizeof(primes[0]);
const float max_load_factor = 0.65;

/*****************************************************************************/
struct hashtable *
hashtable_create(unsigned int minsize, const char *file_start)
{
    struct hashtable *h;
    unsigned int pindex, size = primes[0];
    /* Check requested hashtable isn't too large */
    if (minsize > (1u << 30)) return NULL;
    /* Enforce size as prime */
    for (pindex=0; pindex < prime_table_length; pindex++) {
        if (primes[pindex] > minsize) { size = primes[pindex]; break; }
    }
    h = (struct hashtable *)malloc(sizeof(struct hashtable));
    if (NULL == h) return NULL; /*oom*/
    h->table = (struct entry **)malloc(sizeof(struct entry*) * size);
    if (NULL == h->table) { free(h); return NULL; } /*oom*/
    memset(h->table, 0, size * sizeof(struct entry *));
    h->tablelength  = size;
    h->primeindex   = pindex;
    h->entrycount   = 0;
    h->loadlimit    = (unsigned int) ceil(size * max_load_factor);
    h->file_start   = file_start;
    h->extra_off    = 0;
    h->cache_file   = NULL;
    h->lst          = NULL;
    return h;
}

/*****************************************************************************/
unsigned int
hash(struct hashtable *h, const char *s, int s_len)
{
    /* Aim to protect against poor hash functions by adding logic here
     * - logic taken from java 1.4 hashtable source */
    unsigned int i = hash_str(s, s_len);
    i += ~(i << 9);
    i ^=  ((i >> 14) | (i << 18)); /* >>> */
    i +=  (i << 4);
    i ^=  ((i >> 10) | (i << 22)); /* >>> */
    return i;
}

list_t *
hashtable_read_cached_list(struct hashtable *h, void *v)
{
  int *vv;
  list_t *node;

  assert (h != NULL);
  assert (h->cache_file != NULL);

  list_free(h->lst);
  h->lst = list_node_new();
  vv = (int *) (((char *) v) + h->extra_off);
  assert (*vv != 0);
  node = h->lst;
  node->u.entry_line_idx = *vv;
  ++vv;
  while (*vv != 0)
  {
    node->next = list_node_new();
    node = node->next;
    node->u.entry_line_idx = *vv;
    ++vv;
  }
  node->next = NULL;

  return h->lst;
}

/*****************************************************************************/
static int
hashtable_expand(struct hashtable *h)
{
    /* Double the size of the table to accomodate more entries */
    struct entry **newtable;
    struct entry *e;
    struct entry **pE;
    unsigned int newsize, i, index;

    assert (h != NULL);
    assert (h->cache_file == NULL);

    /* Check we're not hitting max capacity */
    if (h->primeindex == (prime_table_length - 1)) return 0;
    newsize = primes[++(h->primeindex)];

    newtable = (struct entry **)malloc(sizeof(struct entry*) * newsize);
    if (NULL != newtable)
    {
        memset(newtable, 0, newsize * sizeof(struct entry *));
        /* This algorithm is not 'stable'. ie. it reverses the list
         * when it transfers entries between the tables */
        for (i = 0; i < h->tablelength; i++) {
            while (NULL != (e = h->table[i])) {
                h->table[i] = e->next;
                index = indexFor(newsize,e->h);
                e->next = newtable[index];
                newtable[index] = e;
            }
        }
        free(h->table);
        h->table = newtable;
    }
    /* Plan B: realloc instead */
    else
    {
        newtable = (struct entry **)
                   realloc(h->table, newsize * sizeof(struct entry *));
        if (NULL == newtable) { (h->primeindex)--; return 0; }
        h->table = newtable;
        memset(newtable[h->tablelength], 0, newsize - h->tablelength);
        for (i = 0; i < h->tablelength; i++) {
            for (pE = &(newtable[i]), e = *pE; e != NULL; e = *pE) {
                index = indexFor(newsize,e->h);
                if (index == i)
                {
                    pE = &(e->next);
                }
                else
                {
                    *pE = e->next;
                    e->next = newtable[index];
                    newtable[index] = e;
                }
            }
        }
    }
    h->tablelength = newsize;
    h->loadlimit   = (unsigned int) ceil(newsize * max_load_factor);
    return -1;
}

int
hashtable_is_cached(struct hashtable *h)
{
  return h->cache_file != NULL;
}

/*****************************************************************************/
unsigned int
hashtable_count(struct hashtable *h)
{
    return h->entrycount;
}

/*****************************************************************************/
int
hashtable_insert(struct hashtable *h, int s_off, int s_len, list_t *v)
{
    /* This method allows duplicate keys - but they shouldn't be used */
    unsigned int index;
    struct entry *e;

    assert (h != NULL);
    assert (h->cache_file == NULL);

    if (++(h->entrycount) > h->loadlimit)
    {
        /* Ignore the return value. If expand fails, we should
         * still try cramming just this value into the existing table
         * -- we may not have memory for a larger table, but one more
         * element may be ok. Next time we insert, we'll try expanding again.*/
        hashtable_expand(h);
    }
    e = (struct entry *)malloc(sizeof(struct entry));
    if (NULL == e) { --(h->entrycount); return 0; } /*oom*/
    e->h = hash(h,s_off + h->file_start,s_len);
    index = indexFor(h->tablelength,e->h);
    e->s_off = s_off;
    e->s_len = s_len;
    e->v = v;
    e->next = h->table[index];
    h->table[index] = e;
    return -1;
}

/*****************************************************************************/
list_t * /* returns value associated with key */
hashtable_search(struct hashtable *h, const char *s, int s_len)
{
    struct entry *e;
    unsigned int hashvalue, index;
    unsigned long extra_off;
    const char *fs;
    hashvalue = hash(h,s,s_len);
    index = indexFor(h->tablelength,hashvalue);
    fs = h->file_start;
    extra_off = h->extra_off;
    e = (struct entry *) (((char *) h->table[index]) + extra_off);
    while (((char *) e) - extra_off != NULL)
    {
        /* Check hash value to short circuit heavier comparison */
        if ((hashvalue == e->h) && s_len == e->s_len &&
             memcmp(s, e->s_off + fs, s_len) == 0)
        {
          return hashtable_get_list_from_value(h, e->v);
        }
        e = (struct entry *) (((char *) e->next) + extra_off);
    }
    return NULL;
}

/*****************************************************************************/
list_t * /* returns value associated with key */
    hashtable_remove(struct hashtable *h, const char *s, int s_len)
{
    struct entry *e;
    struct entry **pE;
    void *v;
    unsigned int hashvalue, index;
    const char *fs;

    assert (h != NULL);
    assert (h->cache_file == NULL);

    fs = h->file_start;
    hashvalue = hash(h,s,s_len);
    index = indexFor(h->tablelength,hash(h,s,s_len));
    pE = &(h->table[index]);
    e = *pE;
    while (NULL != e)
    {
        /* Check hash value to short circuit heavier comparison */
        if ((hashvalue == e->h) && e->s_len == s_len &&
             memcmp(s, e->s_off + fs, s_len) == 0)
        {
            *pE = e->next;
            h->entrycount--;
            v = e->v;
            free(e);
            return hashtable_get_list_from_value(h, v);
        }
        pE = &(e->next);
        e = e->next;
    }
    return NULL;
}

/*****************************************************************************/
/* destroy */
void
hashtable_destroy(struct hashtable *h)
{
    if (h->cache_file == NULL)
    {
      unsigned int i;
      struct entry *e, *f;
      struct entry **table = h->table;
      assert (h->extra_off == 0);
      for (i = 0; i < h->tablelength; i++)
      {
        e = table[i];
        while (NULL != e)
        {
          f = e;
          e = e->next;
          list_free((list_t *) f->v);
          free(f);
        }
      }
      assert (h->lst == NULL);
      free(h->table);
    }
    else
    {
      list_free(h->lst);
      if (--h->cache_file->ref == 0)
      {
        file_unload(h->cache_file);
      }
    }
    free(h);
}

/*
 * Copyright (c) 2002, Christopher Clark
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * * Neither the name of the original author; nor the names of any contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
