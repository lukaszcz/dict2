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

#ifndef OPTIONS_H
#define OPTIONS_H

#include "list.h"

extern list_t *opt_autoload_list;
extern list_t *opt_active_list;
extern int opt_german_umlaut_conversion;
extern int opt_ignore_case;
extern int opt_search_forms;
extern int opt_search_inflections;
extern int opt_search_stem;
extern int opt_caching;
extern int opt_cache_min_file_size;

void options_set_defaults();
void options_read_from_file(const char *path);
void options_save_to_file(const char *path);
void options_cleanup();

void options_add_autoload_file(const char *path);
void options_add_autoload_file_last(const char *path);
void options_autoload_list_clear();

int options_check_active(const char *dict_name);
void options_add_active(const char *dict_name);
void options_remove_active(const char *dict_name);
void options_active_list_clear();

#endif
