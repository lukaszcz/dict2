/* Copyright (C) 2002 Christopher Clark <firstname.lastname@cl.cam.ac.uk> */
/* Copyright (C) 2006-2007 Łukasz Czajka <firstname.lastname@students.mimuw.edu.pl> */

#ifndef __HASHTABLE_CWC22_H__
#define __HASHTABLE_CWC22_H__

#include "list.h"

struct hashtable;

/*
  The hashtable stores offsets to keys, which are strings. Keys are not null-
  terminated. Their length is stored together with their offsets. Offsets are
  with respect to file_start - a pointer to the mmapped file where _all_ the
  strings from the hashtable are stored. file_start is passed as a parameter
  to hashtable_create.
  A hashtable may be cached. Cached hashtables are created from within
  cache.c. hashtable_create creates only non-cached hashtables. This is
  because it makes sense only to cache a hashtable together with its
  associated dict_t.
  The values associated with the keys are lists of entry_line_idx
  (see dict_t).

  NOTE: A hashtable may store strings (as keys) from only _one_ mmapped file.
 */

/*****************************************************************************
 * hashtable_create

 * @name                    hashtable_create
 * @param   minsize         minimum initial size of hashtable
 * @param   file_start      a pointer to the mmapped file which holds
                            the strings that may be stored in this
                            hashtable
 * @return                  newly created non-cached hashtable or NULL on
                            failure
 */

struct hashtable *
hashtable_create(unsigned int minsize, const char *file_start);

/* Returns nonzero if the hashtable is cached. */
int
hashtable_is_cached(struct hashtable *h);

/*****************************************************************************
 * hashtable_insert
  Precondition: ! hashtable_is_cached(h)

 * @name        hashtable_insert
 * @param   h   the hashtable to insert into
 * @param   s_off   the offset of the key
 * @param   s_len  the length of the key
 * @param   v   the value - claims ownership
 * @return      non-zero for successful insertion
 *
 * This function will cause the table to expand if the insertion would take
 * the ratio of entries to table size over the maximum load factor.
 *
 * This function does not check for repeated insertions with a duplicate key.
 * The data, however, is expected to be unique, so if you are not sure
 * whether the key already exists in the hashtable, use hashtable_search
 * first.
 */

int
hashtable_insert(struct hashtable *h, int s_off, int s_len, list_t *v);

/*****************************************************************************
 * hashtable_search

 * @name        hashtable_search
 * @param   h   the hashtable to search
 * @param   s      the key - a character string - does not claim ownership;
 *                 s need not be zero-terminated
 * @param   s_len  the length of s
 * @return      the list associated with the key, or NULL if none found
 * The returned value may point to some internally preallocated data (if the
 * hashtable is cached). This data may only be read and not modified. It is
 * also invalidated with any call to any of the hashtable_* functions.
 * If the hashtable is not cached, then the returned value is a normal
 * list, and may be modified.
 */

list_t *
hashtable_search(struct hashtable *h, const char *s, int s_len);

/*****************************************************************************
 * hashtable_remove
  Precondition: ! hashtable_is_cached(h)

 * @name        hashtable_remove
 * @param   h   the hashtable to remove the item from
 * @param   s      the key - a character string - does not claim ownership;
 *                 s need not be zero-terminated
 * @param   s_len  the length of s
 * @return      the value associated with the key, or NULL if none found
 */

list_t * /* returns value */
hashtable_remove(struct hashtable *h, const char *s, int s_len);


/*****************************************************************************
 * hashtable_count

 * @name        hashtable_count
 * @param   h   the hashtable
 * @return      the number of items stored in the hashtable
 */
unsigned int
hashtable_count(struct hashtable *h);


/*****************************************************************************
 * hashtable_destroy

 * @name        hashtable_destroy
 * @param   h   the hashtable
 * @param       free_values     whether to call 'free' on the remaining values
 *
 * This function may be used to destroy non-cached as well as cached
 * hashtables.
 */

void
hashtable_destroy(struct hashtable *h);

#endif /* __HASHTABLE_CWC22_H__ */

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
