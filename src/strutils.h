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

#ifndef STRUTILS_H
#define STRUTILS_H

/* Initialization & cleanup */
void strutils_init();
void strutils_cleanup();

/* String conversions */

/* The conversion functions return the converted string
  (which points to an internal static buffer) or NULL on
  failure. */
char *conv_utf8_to_iso_8859_15(const char *s, size_t s_len);
char *conv_iso_8859_15_to_utf8(const char *s, size_t s_len);
/* Converts html character entities within the string to
  corresponding UTF-8 characters. */
char *conv_html_to_utf8(const char *s, long s_len);

/* String functions. */

// Returns s with all things in any kind of brackets at the
// end or at the beginning of the string skipped. Skips leading
// and closing whitespace as well. Stores the length of the
// result in ss_len. s doesn't have to be null-terminated, and
// the pointer returned points inside s.
// Example: "(bad (bad)) good [bad] {{bad} bad}" is transformed
// to "good"
const char *trim_brackets(const char *s, int s_len, int *ss_len);
// The same as above, but trims spaces only, and leaves brackets intact.
const char *trim_spaces(const char *s, int s_len, int *ss_len);
// Skips whitespace starting from the position i in s. Returns the next
// non-whitespace position or s_len if the whole string starting from i
// is whitespace.
int skip_ws(const char *s, int s_len, int i);
// Reads characters until the next whitespace. Stores them in w, which is
// expected to be at least w_len bytes long. w always gets null-terminated.
int read_word(const char *s, int s_len, int i, char *w, int w_len);
/* xstrncpy is essentially the same as strncpy, except that it does not
   pad the remainder of dest with zeroes when strlen(src) < size. It may be
   useful to avoid this overhead when a short string is copied into a buffer
   with large length upper bound. Another difference is that the 0 byte is
   always present as a string terminator. */
char *xstrncpy(char *dest, const char *src, size_t size);
/* Reverses a string in-place. Returns the argument. */
char *strrev(char *str);
int is_en_vowel(int c);
int is_en_consonant(int c);

#endif
