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

#ifndef FILE_H
#define FILE_H

#include "limits.h"

typedef struct{
  int converted; /* converted: see header_t below */
  const char *data; /* File data. */
  size_t length; /* The length of the data array (in bytes). */
  int fd; /* Used internally. */
  int ref; /* ref: the number of dict_t's using the file */
  char *path; /* the path of the file in the filesystem */
} file_t;

typedef struct{
  int converted;
  /* converted is nonzero if the file is converted to utf8 - UTF8 is
  present in the first line of the file. If this magic string is not
  found then the file is assumed to be a file downloaded directly from
  www.dict.cc - encoding ISO-8859-15, no header, two dictionaries
  (German-English, English-German). */
  int entries_num;
  char langs[MAX_DICT_ENTRIES][MAX_STR_LEN + 1];
  char name[MAX_NAME_LEN + 1];
  /* name is the name stored in the file - not the name of the file */
  int dicts_num; /* the number of dictionaries in the file */
  int keys_num[MAX_DICTS_IN_FILE];
  int keys[MAX_DICTS_IN_FILE][MAX_DICT_ENTRIES];
    /* which entries are keys */
  int size[MAX_DICTS_IN_FILE];
    /* approximate dictionary sizes */
} file_header_t;

typedef struct{
  char str[MAX_ENTRY_LEN + 1];
  /* str is automatically converted to valid UTF-8,
  regardless whether the file is in UTF-8 or not
  (but only if needs_utf8 is passed to file_read_line) */
  const char *s;
  /* Points to the entry string in a separate read-only memory area.
     Not zero-terminated. Length s_len.*/
  int s_len;
} file_entry_t;

extern file_entry_t file_entry[MAX_DICT_ENTRIES];
extern int file_entries_read;
extern file_header_t file_header;

/* Loads a given file into memory.
  NOTE: This may be implemented with memory maps,
  so always use file_unload to dispose the allocated
  memory (and not C free!). Returns NULL on failure. On success
  the newly created file_t object has reference count zero. */
file_t *file_load(const char *path);
/* Unloads the file. Should be called only if the reference
   count is zero. */
void file_unload(file_t *file);
/* Returns the input position after the header or -1 on error.
  Modifies file_header. */
int file_read_header(file_t *file);
/* Returns the next input position.
  Modifies file_entry and file_entries_read.
  If needs_utf8 is nonzero then the str field in file_entry
  is a valid UTF8 string, otherwise it's invalid. */
int file_read_line(file_t *file, int i, int needs_utf8);
/* Skips to the next line without actually reading the
  current one. */
int file_skip_line(file_t *file, int i);

/* NOTE: For details of error handling see utils.h */

#endif
