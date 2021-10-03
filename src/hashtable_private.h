/* Copyright (C) 2002, 2004 Christopher Clark <firstname.lastname@cl.cam.ac.uk> */
/* Copyright (C) 2006-2007 Łukasz Czajka <firstname.lastname@students.mimuw.edu.pl> */

#ifndef __HASHTABLE_PRIVATE_CWC22_H__
#define __HASHTABLE_PRIVATE_CWC22_H__

#include "file.h"
#include "list.h"
#include "hashtable.h"

/*****************************************************************************/

struct entry
{
  long s_off; /* the offset of the key with respect to some memory mapped file
    - the file containing the words of the dictionary with which this
  hashtable is associated (see dict.h); a pointer to the file is kept in
  the hashtable in file_start;
    s_off + file_start gives a pointer to the key. */
  long s_len; /* the length of the key */
  size_t h; /* the value of the hash function for the key */
  void *v; /* the value - the list of dictionary entries associated with the
              key (see dict.h) is obtained by passing this to
              hashtable_get_list_from_value */
  struct entry *next;
};

struct hashtable {
    unsigned int tablelength;
    struct entry **table; /* extra_off should be added to pointers
                             in this table */
    unsigned int entrycount;
    unsigned int loadlimit;
    unsigned int primeindex;
    const char *file_start; /* A pointer to the start of the mmapped file
                            where the keys reside. */
    unsigned long extra_off;
    /* An offset that should be added to all entry pointers
       to get a valid pointer. This is non-zero only if
       is_cached is true. */
    file_t *cache_file;
    /* cache_file: Nonzero iff the hashtable is cached on disk - i.e. the
    entries are actually mmapped (see cache.h). This file is different from
    file_start - these are two separate files. cache_file is used to cache
    entry structures, file_start holds the keys. */
    list_t *lst;
    /* lst: the list most recently returned by hashtable_search;
       it is dynamically allocated; this field is used only if the hashtable
       is cached, otherwise it's NULL */
};

/*****************************************************************************/
unsigned int
hash(struct hashtable *h, const char *s, int s_len);

list_t *
hashtable_read_cached_list(struct hashtable *h, void *v);

/*****************************************************************************/
/* indexFor */
static inline unsigned int
indexFor(unsigned int tablelength, unsigned int hashvalue) {
    return (hashvalue % tablelength);
}

static inline list_t *
hashtable_get_list_from_value(struct hashtable *h, void *v)
{
  if (h->cache_file == NULL)
  {
    return (list_t *) v;
  }
  else
  {
    return hashtable_read_cached_list(h, v);
  }
}

/* Only works if tablelength == 2^N */
/*static inline unsigned int
indexFor(unsigned int tablelength, unsigned int hashvalue)
{
    return (hashvalue & (tablelength - 1u));
}
*/

/*****************************************************************************/
/*#define freekey(X) free(X)*/
#define freekey(X) ;


/*****************************************************************************/

#endif /* __HASHTABLE_PRIVATE_CWC22_H__*/

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
