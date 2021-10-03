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

#ifndef CACHE_H
#define CACHE_H

#include "dictionary.h"

/* For a description of the cache file format see: docs/cache.pdf
   (you will probably need to run 'make pdf' first) */

/* All cache files are located in the cache directory -
  see paths.h */


/* cache_load uses progress_* variables from utils.h,
   so they should be set to sensible values before calling this
   function */

/* Loads a cached dictionary. Returns NULL if the dictionary
  is not currently cached. */
int cache_load(dict_t *dict, int dict_num);
/* Saves a dictionary in a special cache file. The previous content
  of the cache file (if any) is lost. Sends a notification event "cache_start"
  (ie. calls notifier("start_start"), see utils.h) upon starting the caching,
  and "cache_finish" upon ending. */
void cache_save(dict_t *dict, int dict_num);
/* Removes all files in the cache directory. */
void cache_clear();

#endif
