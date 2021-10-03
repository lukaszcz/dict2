/* Copyright (C) 2002, 2004 Christopher Clark  <firstname.lastname@cl.cam.ac.uk> */
/* Copyright (C) 2006-2007 Łukasz Czajka <firstname.lastname@students.mimuw.edu.pl> */

#include "hashtable.h"
#include "hashtable_private.h"
#include "hashtable_itr.h"
#include <stdlib.h> /* defines NULL */
#include <string.h> /* for memcmp */

/*****************************************************************************/
/* hashtable_iterator    - iterator constructor */

struct hashtable_itr *
hashtable_iterator(struct hashtable *h)
{
    unsigned int i, tablelength;
    struct hashtable_itr *itr = (struct hashtable_itr *)
        malloc(sizeof(struct hashtable_itr));
    if (NULL == itr) return NULL;
    itr->h = h;
    itr->e = NULL;
    itr->parent = NULL;
    tablelength = h->tablelength;
    itr->index = tablelength;
    if (0 == h->entrycount) return itr;

    for (i = 0; i < tablelength; i++)
    {
        if (NULL != h->table[i])
        {
            itr->e = h->table[i];
            itr->index = i;
            break;
        }
    }
    return itr;
}

/*****************************************************************************/
/* key      - return the key of the (key,value) pair at the current position */
/* value    - return the value of the (key,value) pair at the current position */

const char *
hashtable_iterator_key_s(struct hashtable_itr *i)
{ return i->e->s_off + i->h->file_start; }

int
    hashtable_iterator_key_s_len(struct hashtable_itr *i)
{ return i->e->s_len; }


/*****************************************************************************/
/* advance - advance the iterator to the next element
 *           returns zero if advanced to end of table */

int
hashtable_iterator_advance(struct hashtable_itr *itr)
{
    unsigned int j,tablelength;
    struct entry **table;
    struct entry *next;
    unsigned long extra_off;
    if (NULL == itr->e) return 0; /* stupidity check */

    extra_off = itr->h->extra_off;
    next = (struct entry *) (((char *) itr->e->next) + extra_off);
    if (((char *) next) - extra_off != NULL)
    {
        itr->parent = itr->e;
        itr->e = next;
        return -1;
    }
    tablelength = itr->h->tablelength;
    itr->parent = NULL;
    if (tablelength <= (j = ++(itr->index)))
    {
        itr->e = NULL;
        return 0;
    }
    table = itr->h->table;
    while (NULL == (next = table[j]))
    {
        if (++j >= tablelength)
        {
            itr->index = tablelength;
            itr->e = NULL;
            return 0;
        }
    }
    itr->index = j;
    itr->e = (struct entry *) (((char *) next) + extra_off);
    return -1;
}

/*****************************************************************************/
/* remove - remove the entry at the current iterator position
 *          and advance the iterator, if there is a successive
 *          element.
 *          If you want the value, read it before you remove:
 *          beware memory leaks if you don't.
 *          Returns zero if end of iteration. */

int
hashtable_iterator_remove(struct hashtable_itr *itr)
{
    struct entry *remember_e, *remember_parent;
    int ret;

    assert (itr->h->cache_file == NULL);

    /* Do the removal */
    if (NULL == (itr->parent))
    {
        /* element is head of a chain */
        itr->h->table[itr->index] = itr->e->next;
    } else {
        /* element is mid-chain */
        itr->parent->next = itr->e->next;
    }
    /* itr->e is now outside the hashtable */
    remember_e = itr->e;
    itr->h->entrycount--;

    /* Advance the iterator, correcting the parent */
    remember_parent = itr->parent;
    ret = hashtable_iterator_advance(itr);
    if (itr->parent == remember_e) { itr->parent = remember_parent; }
    free(remember_e);
    return ret;
}

/*****************************************************************************/
int /* returns zero if not found */
hashtable_iterator_search(struct hashtable_itr *itr,
                          struct hashtable *h, const char *s, int s_len)
{
    struct entry *e, *parent;
    unsigned int hashvalue, index;
    unsigned long extra_off;
    const char *fs;

    fs = itr->h->file_start;
    extra_off = itr->h->extra_off;

    hashvalue = hash(h,s,s_len);
    index = indexFor(h->tablelength,hashvalue);

    e = (struct entry *) (((char *) h->table[index]) + extra_off);
    parent = NULL;
    while (((char *) e) - extra_off != NULL)
    {
        /* Check hash value to short circuit heavier comparison */
        if ((hashvalue == e->h) && e->s_len == s_len &&
             memcmp(s, e->s_off + fs, s_len) == 0)
        {
            itr->index = index;
            itr->e = e;
            itr->parent = parent;
            itr->h = h;
            return -1;
        }
        parent = e;
        e = (struct entry *) (((char *) e->next) + extra_off);
    }
    return 0;
}


/*
 * Copyright (c) 2002, 2004, Christopher Clark
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