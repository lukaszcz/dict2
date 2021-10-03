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

#ifndef UTILS_H
#define UTILS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <assert.h>
#include <stdlib.h>

#include "limits.h"

/* Initialization & cleanup */

void utils_init();
void utils_cleanup();
void utils_test();

/* Memory management */

void *xmalloc(size_t size);
void *xrealloc(void *ptr, size_t size);
char *xstrdup(const char *str);

/* Errors */

/*
 * Errors are indicated by setting errors_num to the number of unhandled
 * errors. Such errors may (and should) be handled by the interface module
 * using the error_str() function, which fetches unhandled error messages
 * in FIFO order. The interface module should check for errors every time
 * it calls some function, even if the function doesn't return any error
 * indication - it might just set errors_num and return void.
 */

/* Prints the error message and aborts. */
void fatal(const char *msg);
/* Adds the error message to the unhandled errors FIFO.
  Increases errors_num. */
void error(const char *msg);
/* The same, but adds a message returned by strerror(errno). */
void syserr(const char *msg);
/* Fetches the first error message in the queue. Returns NULL
  if there are currently no error messages in the queue. Removes
  the returned message from the queue. The returned string points
  to static storage area, which will be overwritten by the next
  call to error_str. */
const char *error_str();

/* Notifiers */

// Should return non-zero if the job is expected to continue,
// zero if it should be canceled.
typedef int (*progress_notifier_t)();
typedef void (*notifier_t)(const char *event);

extern progress_notifier_t progress_notifier;
extern int progress_max;
/* notifier should be set by the interface. The program logic uses
  this function to notify various events (what's being cached,
  searched, etc.) */
extern notifier_t notifier;

/* Misc */

/* Returns 0 if cannot stat. */
unsigned long file_size(const char *path);
/* Returns modification time or 0 if cannot stat. */
time_t file_mtime(const char *path);

#endif
