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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "strutils.h"
#include "utils.h"

/* Initialization & cleanup */

void utils_init()
{
}

void utils_cleanup()
{
  const char *str;
  while ((str = error_str()) != NULL)
  {
    fprintf(stderr, "ERROR: %s\n", str);
  }
}

void utils_test()
{
}

/* Memory management */

void *xmalloc(size_t size)
{
  void *ptr;
  if ((ptr = malloc(size)) == 0)
  {
    fatal("Cannot allocate memory.");
  }
  return ptr;
}

void *xrealloc(void *ptr, size_t size)
{
  if ((ptr = realloc(ptr, size)) == 0)
  {
    fatal("Cannot reallocate memory.");
  }
  return ptr;
}

char *xstrdup(const char *str)
{
  char *str2 = strdup(str);
  if (str2 == NULL)
  {
    fatal("Cannot allocate memory.");
  }
  return str2;
}

/* error handling */

static int errors_num = 0;
static int fifo_start = 0;
static char *errormsg_fifo[MAX_ERRORS];
static char last_error_msg[MAX_STR_LEN + 1];

void fatal(const char *msg)
{
  fprintf(stderr, "Fatal error: %s\n", msg);
  exit(-1);
}

static void fatal_too_many_errors()
{
  int i;
  i = fifo_start;
  do{
    fprintf(stderr, "Error: %s", errormsg_fifo[i]);
    ++i;
    if (i == MAX_ERRORS)
    {
      i = 0;
    }
  }while(i != fifo_start);
  fatal("Too many errors.");
}

static void errormsg_fifo_push(char *msg)
{
  int i;

  assert (errors_num < MAX_ERRORS);

  i = fifo_start + errors_num;
  if (i >= MAX_ERRORS)
  {
    i -= MAX_ERRORS;
  }
  errormsg_fifo[i] = msg;
  ++errors_num;
}

void error(const char *msg)
{
  if (errors_num < MAX_ERRORS)
  {
    errormsg_fifo_push(xstrdup(msg));
  }
  else
  {
    fatal_too_many_errors();
  }
}

void syserr(const char *msg)
{
  char *str;
  const char *str2;
  int len1;
  if (errors_num < MAX_ERRORS)
  {
    str2 = strerror(errno);
    len1 = strlen(msg);
    str = (char*) xmalloc(sizeof(char) * (len1 + strlen(str2) + 4));
    strcpy(str, msg);
    str[len1] = ' ';
    str[len1 + 1] = '-';
    str[len1 + 2] = ' ';
    strcpy(str + len1 + 3, str2);
    errormsg_fifo_push(str);
  }
  else
  {
    fatal_too_many_errors();
  }
}

const char *error_str()
{
  if (errors_num == 0)
  {
    return NULL;
  }
  else
  {
    xstrncpy(last_error_msg, errormsg_fifo[fifo_start], MAX_STR_LEN);
    last_error_msg[MAX_STR_LEN] = '\0';
    free(errormsg_fifo[fifo_start]);
    ++fifo_start;
    if (fifo_start == MAX_ERRORS)
    {
      fifo_start = 0;
    }
    --errors_num;
    return last_error_msg;
  }
}

/* Notification */

progress_notifier_t progress_notifier;
int progress_max;

notifier_t notifier;

/* Misc */

unsigned long file_size(const char *path)
{
  struct stat buf;
  if (stat(path, &buf) == -1)
  {
    return 0;
  }
  else
  {
    return buf.st_size;
  }
}

time_t file_mtime(const char *path)
{
  struct stat buf;
  if (stat(path, &buf) == -1)
  {
    return 0;
  }
  else
  {
    return buf.st_mtime;
  }
}
