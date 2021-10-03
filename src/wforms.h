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

/*
 * The wforms unit takes care of creating alternate word forms from words
 * given, i.e. it practically implements the 'inflect', 'stem', and 'forms'
 * options. All conversions are performed by one general algorithm described
 * in wforms.c. Additional data files with conversion rules must be present
 * (they have the names: lang.opt, where lang is 'de' or 'en', opt is
 * 'inflect', 'stem', or 'forms'). wforms comes from 'word forms'.
 */

 /* For a description of the Word Formation Algorithm see: docs/wfa.pdf
   (you will probably need to run 'make pdf' first) */

#ifndef WFORMS_H
#define WFORMS_H

#include "list.h"

/* Adds alternate forms of words (taking into account opt_inflect, opt_stem,
   and opt_forms) in the given list to the list and returns the resulting
   list. Assumes that lang is the language of all the words in the
   list (currently either 'de' or 'en'). */
list_t *wforms_add(list_t *lst, const char *lang);

/* Initializes the wforms module by loading and parsing conversion rules
   data files. */
void wforms_init();
void wforms_cleanup();
void wforms_test();

#endif
