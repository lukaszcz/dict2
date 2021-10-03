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

#include <ctype.h>
#include <string.h>
#include <iconv.h>
#include <glib.h>
#include <stdlib.h>
#include <search.h>

#include "utils.h"
#include "strutils.h"

static iconv_t cdesc_utf8_to_iso;
static iconv_t cdesc_iso_to_utf8;

static char conv_buffer[MAX_STR_LEN + 1];

/* HTML character entities */
#define ENTITIES 4
static char *key[] = {"quot", "amp", "lt", "gt", "nbsp", "iexcl", "cent",
                      "pound", "curren", "yen", "brvbar", "sect", "uml",
                      "copy", "ordf", "laquo", "not", "shy", "reg", "macr",
                      "deg", "plusmn", "sup2", "sup3", "acute", "micro",
                      "para", "middot", "cedil", "sup1", "ordm", "raquo",
                      "frac14", "frac12", "frac34", "iquest", "Ntidle",
                      "Ograve", "Oacute", "Ocirc", "Otidle", "Ouml", "times",
                      "Oslash", "Ugrave", "Uacute", "Ucirc", "Uuml",
                      "Yacute", "THORN", "szlig", "agrave", "aacute",
                      "acirc", "atidle", "auml", "aring", "aelig", "ccedil",
                      "egrave", "eacute", "ecirc", "euml", "igrave",
                      "iacute", "icirc", "iuml", "eth", "ntidle", "ograve",
                      "oacute", "ocirc", "otidle", "ouml", "divide",
                      "oslash", "ugrave", "uacute", "ucirc", "uuml",
                      "yacute", "thorn", "yuml"};
static int data[] = {34, 38, 60, 62, 160, 161, 162,
                     163, 164, 165, 166, 167, 168,
                     169, 170, 171, 172, 173, 174, 175,
                     176, 177, 178, 179, 180, 181,
                     182, 183, 184, 185, 186, 187,
                     188, 189, 190, 191, 209,
                     210, 211, 212, 213, 214, 215,
                     216, 217, 218, 219, 220,
                     221, 222, 223, 224, 225,
                     226, 227, 228, 229, 230, 231,
                     232, 233, 234, 235, 235,
                     237, 238, 239, 240, 241, 242,
                     243, 244, 245, 246, 247,
                     248, 249, 250, 251, 252,
                     253, 254, 255};

void strutils_init()
{
  ENTRY e;
  int i;
  cdesc_iso_to_utf8 = iconv_open("UTF-8", "ISO-8859-15");
  if (cdesc_iso_to_utf8 == (iconv_t) -1)
  {
    syserr("Cannot open character conversion descriptor (ISO-8859-15 => UTF-8).");
  }
  cdesc_utf8_to_iso = iconv_open("ISO-8859-15", "UTF-8");
  if (cdesc_utf8_to_iso == (iconv_t) -1)
  {
    syserr("Cannot open character conversion descriptor (UTF-8 => ISO-8859-15).");
  }

  hcreate(100);

  for (i = 0; i < ENTITIES; ++i)
  {
    e.key = key[i];
    e.data = (void *) data[i];
    hsearch(e, ENTER);
  }
}

void strutils_cleanup()
{
  iconv_close(cdesc_utf8_to_iso);
  iconv_close(cdesc_iso_to_utf8);
  hdestroy();
}

/* Character set conversions */

/* stores in result a string representing the UCS unicode character c in the
  UTF-8 encoding; returns result; result should be at least 7 bytes long;
  result will be zero-terminated. */
static char *ucs_to_utf8(wchar_t c, char *result)
{
  if (c <= 0x7f)
  {
    result[0] = c;
    result[1] = 0;
  }
  else if (c <= 0x7ff)
  {
    result[0] = 0xc0 | (c >> 6);
    result[1] = 0x80 | (c & 0x3f);
    result[2] = 0;
  }
  else if (c <= 0xffff)
  {
    result[0] = 0xe0 | (c >> 12);
    result[1] = 0x80 | ((c >> 6) & 0x3f);
    result[2] = 0x80 | (c & 0x3f);
    result[3] = 0;
  }
  else if (c <= 0x1fffff)
  {
    result[0] = 0xf0 | (c >> 18);
    result[1] = 0x80 | ((c >> 12) & 0x3f);
    result[2] = 0x80 | ((c >> 6) & 0x3f);
    result[3] = 0x80 | (c & 0x3f);
    result[4] = 0;
  }
  else if (c <= 0x3ffffff)
  {
    result[0] = 0xf8 | (c >> 24);
    result[1] = 0x80 | ((c >> 18) & 0x3f);
    result[2] = 0x80 | ((c >> 12) & 0x3f);
    result[3] = 0x80 | ((c >> 6) & 0x3f);
    result[4] = 0x80 | (c & 0x3f);
    result[5] = 0;
  }
  else
  {
    result[0] = 0xfc | (c >> 30);
    result[1] = 0x80 | ((c >> 24) & 0x3f);
    result[2] = 0x80 | ((c >> 18) & 0x3f);
    result[3] = 0x80 | ((c >> 12) & 0x3f);
    result[4] = 0x80 | ((c >> 6) & 0x3f);
    result[5] = 0x80 | (c & 0x3f);
    result[6] = 0;
  }

  return result;
}

char *conv_utf8_to_iso_8859_15(const char *s, unsigned s_len)
{
  char *out = conv_buffer;
  unsigned out_len = MAX_STR_LEN;
  iconv(cdesc_iso_to_utf8, NULL, NULL, NULL, NULL); /* reset state */
  if (iconv(cdesc_utf8_to_iso, (char **) &s, &s_len, &out, &out_len) == -1)
  {
    syserr("String conversion error (UTF-8 => ISO-8859-15)");
    return NULL;
  }
  *out = '\0';
  return conv_buffer;
}

char *conv_iso_8859_15_to_utf8(const char *s, unsigned s_len)
{
  char *out = conv_buffer;
  unsigned out_len = MAX_STR_LEN;
  iconv(cdesc_iso_to_utf8, NULL, NULL, NULL, NULL); /* reset state */
  if (iconv(cdesc_iso_to_utf8, (char **) &s, &s_len, &out, &out_len) == -1)
  {
    syserr("String conversion error (ISO-8859-15 => UTF-8)");
    return NULL;
  }
  *out = '\0';
  return conv_buffer;
}

char *conv_html_to_utf8(const char *s, int s_len)
{
  char str[MAX_STR_LEN + 1];
  unsigned u;
  int i, j, k, len;
  ENTRY e;
  ENTRY *pe;

  i = 0; j = 0;
  while (i < s_len && j < MAX_STR_LEN)
  {
    if (s[i] == '&')
    {
      ++i;
      if (s[i] == '#')
      {
        ++i;
        if (isdigit(s[i]))
        {
          u = 0;
          while (i < s_len && isdigit(s[i]))
          {
            u = u * 10 + (s[i] - '0');
            ++i;
          }
          if (i < s_len && s[i] == ';')
          {
            ++i;
          }
          ucs_to_utf8(u, str);
          len = strlen(str);
          k = 0;
          while (j < MAX_STR_LEN && k < len)
          {
            conv_buffer[j++] = str[k++];
          }
        }
        else
        {
          conv_buffer[j++] = '&';
          conv_buffer[j++] = '#';
        }
      }
      else if (isalpha(s[i]))
      {
        str[0] = '&';
        k = 1;
        while (i < s_len && isalpha(s[i]) && k < MAX_STR_LEN)
        {
          str[k++] = s[i++];
        }
        if (i < s_len && s[i] == ';')
        {
          ++i;
        }
        str[k] = '\0';
        e.key = str + 1;
        pe = hsearch(e, FIND);
        if (pe == NULL)
        {
          len = k;
        }
        else
        {
          ucs_to_utf8((int) pe->data, str);
          len = strlen(str);
        }
        k = 0;
        while (j < MAX_STR_LEN && k < len)
        {
          conv_buffer[j++] = str[k++];
        }
      }
      else
      {
        conv_buffer[j++] = '&';
      }
    }
    else
    {
      conv_buffer[j++] = s[i];
      ++i;
    }
  } /* end while */
  conv_buffer[j] = '\0';

  return conv_buffer;
}

/* String functions */

const char *trim_brackets(const char *s, int s_len, int *ss_len)
{
  const char *ss;
  int j, cnt;
  char c1, c2;

  ss = s;
  j = 0;
  while (j < s_len && isspace(s[j]))
  {
    ++j;
  }
  while (j < s_len && (s[j] == '[' || s[j] == '(' || s[j] == '{'))
  {
    switch (s[j]){
      case '[':
        c1 = '[';
        c2 = ']';
        break;
      case '(':
        c1 = '(';
        c2 = ')';
        break;
      case '{':
        c1 = '{';
        c2 = '}';
        break;
      default:
        fatal("Programming error - switch");
        break;
    };
    cnt = 1;
    ++j;
    while (j < s_len && cnt != 0)
    {
      if (s[j] == c1)
      {
        ++cnt;
      }
      else if (s[j] == c2)
      {
        --cnt;
      }
      ++j;
    }
    while (j < s_len && isspace(s[j]))
    {
      ++j;
    }
  } /* end while */
  ss = s + j;
  j = s_len - 1;
  while (j >= ss - s && isspace(s[j]))
  {
    --j;
  }
  while (j >= ss - s && (s[j] == ']' || s[j] == ')' || s[j] == '}'))
  {
    switch (s[j]){
      case ']':
        c1 = '[';
        c2 = ']';
        break;
      case ')':
        c1 = '(';
        c2 = ')';
        break;
      case '}':
        c1 = '{';
        c2 = '}';
        break;
      default:
        fatal("Programming error - switch");
        break;
    };
    cnt = 1;
    --j;
    while (j >= ss - s && cnt != 0)
    {
      if (s[j] == c1)
      {
        --cnt;
      }
      else if (s[j] == c2)
      {
        ++cnt;
      }
      --j;
    }
    while (j >= ss - s && isspace(s[j]))
    {
      --j;
    }
  }
  *ss_len = j - (ss - s) + 1;
  return ss;
}

const char *trim_spaces(const char *s, int s_len, int *ss_len)
{
  const char *ss;
  int j;

  ss = s;
  j = 0;
  while (j < s_len && isspace(s[j]))
  {
    ++j;
  }
  ss = s + j;
  j = s_len - 1;
  while (j >= ss - s && isspace(s[j]))
  {
    --j;
  }
  *ss_len = j - (ss - s) + 1;
  return ss;
}

int skip_ws(const char *s, int s_len, int i)
{
  while (i < s_len && isspace(s[i]))
  {
    ++i;
  }
  return i;
}

int read_word(const char *s, int s_len, int i, char *w, int w_len)
{
  int j = 0;
  while (i < s_len && !isspace(s[i]) && j < w_len - 1)
  {
    w[j++] = s[i++];
  }
  w[j] = '\0';
  return i;
}

char *xstrncpy(char *dest, const char *src, size_t size)
{
  char *dest_end = dest + size - 1;
  while (dest != dest_end && *src != '\0')
  {
    *dest = *src;
    ++dest;
    ++src;
  }
  *dest = '\0';
  return dest;
}

char *strrev(char *str)
{
  int j = strlen(str) - 1;
  int i = 0;
  char c;
  while (i < j)
  {
    c = str[i];
    str[i] = str[j];
    str[j] = c;
    ++i; --j;
  }
  return str;
}

int is_en_vowel(int c)
{
  if (isalpha(c))
  {
    // aeuioy
    if (c == 'a' || c == 'e' || c == 'u' || c == 'i' || c == 'o' ||
        c == 'y')
    {
      return 1;
    }
    else
    {
      return 0;
    }
  }
  else
  {
    return 0;
  }
}

int is_en_consonant(int c)
{
  return isalpha(c) && !is_en_vowel(c);
}
