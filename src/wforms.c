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
  This module implements the Word Formation Algorithm. For the description
  of the algorithm see: docs/wfa.pdf (you probably need to invoke 'make pdf'
  first).
*/

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include "limits.h"
#include "paths.h"
#include "options.h"
#include "list.h"
#include "strutils.h"
#include "rbtree.h"
#include "wforms.h"

#define MAX_CCL 2
#define MAX_GROUPS 256
#define MAX_RULES 512

static const int max_phases[2] = { 6 /*de*/, 3 /*en*/ };

//-------------------------------------------------------------------
// Typedefs.
//-------------------------------------------------------------------

typedef struct{
  char *str; /* the word; dynamically allocated */
  int ccl; /* conversion chain length */
  unsigned char rules_mask[MAX_GROUPS / 8]; /* This is a bitvector. */
} word_data_t;

typedef struct Rule{
  int id;
  char *pattern; /* The pattern to be matched. Dynamically allocated. */
  char *arg; /* The rule argument - what is inserted into the set.
                May contain * and <n>, <-n>, where n is a digit.
                Dynamically allocated. */
  enum { RULE_PREFIX, RULE_SUFFIX, RULE_TOTAL } type;
} rule_t;

typedef enum { LANG_DE=0, LANG_EN } lang_t;
typedef enum { GROUP_INFLECT=0, GROUP_STEM, GROUP_FORMS } group_type_t;

#define MAX_LANGS 2
#define MAX_GROUP_TYPES 3

//-------------------------------------------------------------------
// Static global variables.
//-------------------------------------------------------------------

static rule_t rules[MAX_LANGS][MAX_GROUP_TYPES][MAX_RULES];
static int rules_num[MAX_LANGS][MAX_GROUP_TYPES];

static int group_id = 1; // the lowest unused rule group id

static rbtree_t *cwords1; // the set of words to which no rules apply in
  // this and subsequent phases
static rbtree_t *cwords2; // the set of words to which some rules may be
  // applied in the current phase
static rbtree_t *cwords3; // the words added in the current phase
static lang_t clang;
static group_type_t cgroup_type;

static word_data_t *word_data;

//-------------------------------------------------------------------
// Main helper functions.
//-------------------------------------------------------------------

/* Parses a file with rules. */
static void parse_rules_file(const char *path, lang_t lang,
                             group_type_t group_type);
/* Applies a rule r to word_str, unconditionally.
  star is the * part of word_str. cwords, cpref, csuf, word_data,
  word_str should be set to appropriate values before calling this function.
  */
static void apply_rule(const word_data_t *word_data, const char *star,
                       rule_t *r);
/* Converts a list of strings into a red-black tree representing
  a set of words. */
static rbtree_t *word_list_to_set(list_t *lst);
/* Converts a red-black tree containing words into a list of strings. */
static list_t *set_to_word_list(rbtree_t *set);
/* Frees a set containing words together with
   the data stored. */
static void word_set_free(rbtree_t *set);
/* Implements the Word Formation Algorithm. */
static list_t *wfa(list_t *lst, lang_t lang);


//-----------------------------------------------------------------
// Implementation of the main helper functions.
//-----------------------------------------------------------------

#define SET_RULE_MASK(wd, id) \
  (wd)->rules_mask[((id) >> 3)] |= (1 << ((id) & 7));

#define CHECK_RULE_MASK(wd, id) \
  ((((wd)->rules_mask[((id) >> 3)] >> ((id) & 7)) & 1) == 0)

static void free_word_data(void *data)
{
  word_data_t *wd = (word_data_t *) data;
  free(wd->str);
  free(wd);
}

static int cmp_word_data(void *x1, void *x2)
{
  return strcmp(((word_data_t *) x1)->str, ((word_data_t *) x2)->str);
}

static void parse_rules_file(const char *path, lang_t lang,
                             group_type_t group_type)
{
  FILE *f;
  char str[MAX_STR_LEN];
  const char *s;
  char w[MAX_STR_LEN];
  int s_len, i, size, j;
  rule_t *r;

  f = fopen(path, "r");
  if (f == NULL)
  {
    error("Cannot open word formation rules data file");
    rules_num[lang][group_type] = 0;
    return;
  }
  else
  {
    size = 0;
    while (fgets(str, MAX_STR_LEN, f) != NULL)
    {
      s = trim_spaces(str, strlen(str), &s_len);
      if (s_len > 0 && s[0] != '#')
      {
        ((char *)s)[s_len] = '\0';
        if (s[0] == '@')
        {
          if (s[1] == '@' && s_len == 2)
          {
            ++group_id;
            if (group_id > MAX_GROUPS)
            {
              fatal("Too many rule groups.");
            }
          }
          else
          {
            fatal("Expected '@@'.");
          }
        }
        else
        {
          if (size >= MAX_RULES)
          {
            fatal("Too many rules.");
          }
          r = &rules[lang][group_type][size];
          ++size;
          r->id = group_id;
          if (s[0] == '*')
          { // suffix or total
            i = 1;
            i = read_word(s, s_len, i, w, MAX_STR_LEN);
            if (s[i] != '\0' && !isspace(s[i]))
            {
              fatal("Expected whitespace.");
            }
            if (i == 1)
            { // total
              r->pattern = NULL;
              r->type = RULE_TOTAL;
            }
            else
            { // suffix
              r->pattern = xstrdup(w);
              r->type = RULE_SUFFIX;
            }
            i = skip_ws(s, s_len, i);
            i = read_word(s, s_len, i, w, MAX_STR_LEN);
            r->arg = xstrdup(w);
          }
          else
          { // prefix
            i = read_word(s, s_len, 0, w, MAX_STR_LEN);
            j = strlen(w) - 1;
            if (w[j] != '*')
            {
              fatal("Expected '*'.");
            }
            w[j] = '\0';
            r->pattern = xstrdup(w);
            i = skip_ws(s, s_len, i);
            i = read_word(s, s_len, i, w, MAX_STR_LEN);
            r->arg = xstrdup(w);
            r->type = RULE_PREFIX;
          }
        }
      }
    } // end while (fgets)
    fclose(f);
    rules_num[lang][group_type] = size;
  }
}

static void apply_rule(const word_data_t *word_data,
                       const char *star, rule_t *r)
{
  int i = 0, j = 0;
  char w[MAX_STR_LEN + 1];
  const char *s = r->arg;
  int s_len = strlen(s);
  word_data_t *wd;
  const char *word_str = word_data->str;
  int word_len = strlen(word_str);
  while (i < s_len && j < MAX_STR_LEN)
  {
    if (s[i] == '<')
    {
      int neg = 0;
      int val = 0;
      char c;
      ++i;
      if (s[i] == '-')
      {
        neg = 1;
        ++i;
      }
      while (isdigit(s[i]))
      {
        val = val * 10 + s[i] - '0';
        ++i;
      }
      if (s[i] != '>')
      {
        fatal("Expected '>'.");
      }
      ++i;
      if (val < 1 || val > s_len)
      {
        fatal("Wrong string position.");
      }
      if (neg)
      {
        c = word_str[word_len - val];
      }
      else
      {
        c = word_str[val - 1];
      }
      w[j++] = c;
    }
    else if (s[i] == '*')
    {
      ++i;
      xstrncpy(w + j, star, MAX_STR_LEN - j);
      j += strlen(star);
    }
    else
    {
      w[j++] = s[i++];
    }
  }
  if (j < MAX_STR_LEN)
  {
    w[j] = '\0';
  }
  else
  {
    w[MAX_STR_LEN] = '\0';
  }
  wd = (word_data_t *) xmalloc(sizeof(word_data_t));
  if (j > word_len)
  {
    wd->ccl = word_data->ccl + 1;
  }
  else
  {
    wd->ccl = word_data->ccl;
  }
  bzero(wd->rules_mask, sizeof(wd->rules_mask));
  SET_RULE_MASK(wd, r->id);
  wd->str = xstrdup(w);
  if (wd->ccl <= MAX_CCL && rb_search(cwords1, wd) == NULL &&
      rb_search(cwords2, wd) == NULL &&
      rb_search(cwords3, wd) == NULL)
  {
    rb_insert(cwords3, wd);
  }
  else
  {
    free_word_data(wd);
  }
}

// star must be at least strlen(str) + 1 bytes long.
static int rule_matches(rule_t *r, const char *str, char *star)
{
  int i, j, s_len, len, neg, f;
  const char *s;
  assert (r != NULL);
  assert (str != NULL);
  assert (star != NULL);

  switch(r->type){
    case RULE_PREFIX:
      j = 0; i = 0;
      s = r->pattern;
      s_len = strlen(s);
      len = strlen(str);
      while (i < s_len && j < len)
      {
        switch(s[i]){
          case '<':
            if (s_len - i < 3)
            {
              fatal("Error in wforms file.");
            }
            ++i;
            if (s[i] == 'c')
            {
              if (!is_en_consonant(str[j]))
              {
                return 0;
              }
              ++j;
            }
            else if (s[i] == 'v')
            {
              if (!is_en_vowel(str[j]))
              {
                return 0;
              }
              ++j;
            }
            else
            {
              fatal("Error in wforms file");
            }
            ++i;
            if (s[i] != '>')
            {
              fatal("Error in wforms file.");
            }
            ++i;
            break;
          case '[':
            if (!isalnum(str[j]))
            { // to avoid corrupting UTF-8 characters
              return 0;
            }
            f = 0;
            ++i;
            if (s[i] == '^')
            {
              neg = 1;
              ++i;
            }
            else
            {
              neg = 0;
            }
            while (i < s_len && s[i] != ']')
            {
              f = f || (s[i] == str[j]);
              ++i;
            }
            if (i == s_len)
            {
              fatal("Error in wforms.");
            }
            ++i;
            if ((!neg && !f) || (neg && f))
            {
              return 0;
            }
            ++j;
            break;
          default:
            if (s[i] != str[j])
            {
              return 0;
            }
            ++i; ++j;
            break;
        };
      }
      if (j < len)
      {
        strcpy(star, str + j);
        return 1;
      }
      else
      {
        return 0;
      }

    case RULE_SUFFIX:
      j = strlen(str) - 1;
      s = r->pattern;
      i = strlen(s) - 1;
      while (i >= 0 && j >= 0)
      {
        switch(s[i]){
          case '>':
            if (i < 2)
            {
              fatal("Error in wforms file.");
            }
            --i;
            if (s[i] == 'c')
            {
              if (!is_en_consonant(str[j]))
              {
                return 0;
              }
              --j;
            }
            else if (s[i] == 'v')
            {
              if (!is_en_vowel(str[j]))
              {
                return 0;
              }
              --j;
            }
            else
            {
              fatal("Error in wforms file");
            }
            --i;
            if (s[i] != '<')
            {
              fatal("Error in wforms file.");
            }
            --i;
            break;
          case ']':
            if (!isalnum(str[j]))
            { // to avoid corrupting UTF-8 characters
              return 0;
            }
            f = 0;
            --i;
            while (i >= 0 && s[i] != '[')
            {
              if (i > 0 && s[i] == '^' && s[i - 1] == '[')
              {
                f = !f;
              }
              else
              {
                f = f || (s[i] == str[j]);
              }
              --i;
            }
            if (i < 0)
            {
              fatal("Error in wforms.");
            }
            --i;
            if (!f)
            {
              return 0;
            }
            --j;
            break;
          default:
            if (s[i] != str[j])
            {
              return 0;
            }
            --i; --j;
            break;
        };
      }
      if (j >= 0)
      {
        strncpy(star, str, j + 1);
        star[j + 1] = '\0';
        return 1;
      }
      else
      {
        return 0;
      }

    case RULE_TOTAL:
      strcpy(star, str);
      return 1;
  }
  return 0;
}

static void apply_rules_to_word_data(void *data)
{
  int i, size;
  rule_t *r;
  char star[MAX_STR_LEN];
  size = rules_num[clang][cgroup_type];
  word_data = (word_data_t *) data;
  for (i = 0; i < size; ++i)
  {
    r = &rules[clang][cgroup_type][i];
    if (CHECK_RULE_MASK(word_data, r->id) &&
        rule_matches(r, word_data->str, star))
    { // rule not yet applied to this word
      SET_RULE_MASK(word_data, r->id);
      if (strlen(star) > 2)
      {
        apply_rule(word_data, star, r);
      }
    }
  }
}

static rbtree_t *word_list_to_set(list_t *lst)
{
  rbtree_t *set;
  char w[MAX_STR_LEN];
  char *s;
  int s_len;
  set = rb_new(cmp_word_data);
  while (lst != NULL)
  {
    word_data_t *wd = (word_data_t *) xmalloc(sizeof(word_data_t));
    xstrncpy(w, lst->u.str, MAX_STR_LEN);
    s = (char *) trim_spaces(w, strlen(w), &s_len);
    s[s_len] = '\0';
    wd->str = xstrdup(s);
    wd->ccl = 0;
    bzero(wd->rules_mask, sizeof(wd->rules_mask));
    if (rb_search(set, wd) == NULL)
    {
      rb_insert(set, wd);
    }
    else
    {
      free(wd);
    }
    lst = lst->next;
  }
  return set;
}

static list_t *word_list;

static void add_to_word_list(void *data)
{
  list_t *node = strlist_node_new(((word_data_t *) data)->str);
  node->next = word_list;
  word_list = node;
}

static list_t *set_to_word_list(rbtree_t *set)
{
  word_list = NULL;
  rb_for_all(set, add_to_word_list);
  return word_list;
}

static void word_set_free(rbtree_t *set)
{
  rb_for_all(set, free_word_data);
  rb_free(set);
}

static void add_to_cwords1(void *data)
{
  assert (rb_search(cwords1, data) == 0);
  rb_insert(cwords1, data);
}

static list_t *wfa(list_t *lst, lang_t lang)
{
  list_t *lst2;
  int phase = 0;
  clang = lang;
  cwords1 = rb_new(cmp_word_data);
  cwords2 = word_list_to_set(lst);
  strlist_free(lst);
  while (!rb_empty(cwords2) && phase < max_phases[lang])
  {
    cwords3 = rb_new(cmp_word_data);
    if (opt_search_inflections)
    {
      cgroup_type = GROUP_INFLECT;
      rb_for_all(cwords2, apply_rules_to_word_data);
    }
    if (opt_search_stem)
    {
      cgroup_type = GROUP_STEM;
      rb_for_all(cwords2, apply_rules_to_word_data);
    }
    if (opt_search_forms)
    {
      cgroup_type = GROUP_FORMS;
      rb_for_all(cwords2, apply_rules_to_word_data);
    }
    rb_for_all(cwords2, add_to_cwords1);
    rb_free(cwords2);
    cwords2 = cwords3;
    ++phase;
  }
  rb_for_all(cwords2, add_to_cwords1);
  rb_free(cwords2);
  lst2 = set_to_word_list(cwords1);
  word_set_free(cwords1);
#ifdef DEBUG
  fprintf(stderr, "\nWord Formation Algorithm\n");
  fprintf(stderr, "phases: %d\n", phase);
  fprintf(stderr, "words: %d\n", list_length(lst2));
#endif
  return lst2;
}


//-----------------------------------------------------------------
// Public functions.
//-----------------------------------------------------------------

list_t *wforms_add(list_t *lst, const char *lang)
{
  if (strcmp(lang, "de") == 0)
  {
    return wfa(lst, LANG_DE);
  }
  else if (strcmp(lang, "en") == 0)
  {
    return wfa(lst, LANG_EN);
  }
  else
  {
    error("Unknown language.");
    return lst;
  }
}

void wforms_init()
{
  char path[MAX_STR_LEN];
  int l1 = strlen(path_data_dir);
  strcpy(path, path_data_dir);

  strcpy(path + l1, "/de.inflect");
  parse_rules_file(path, LANG_DE, GROUP_INFLECT);

  strcpy(path + l1, "/de.stem");
  parse_rules_file(path, LANG_DE, GROUP_STEM);

  strcpy(path + l1, "/de.forms");
  parse_rules_file(path, LANG_DE, GROUP_FORMS);

  strcpy(path + l1, "/en.inflect");
  parse_rules_file(path, LANG_EN, GROUP_INFLECT);

  strcpy(path + l1, "/en.stem");
  parse_rules_file(path, LANG_EN, GROUP_STEM);

  strcpy(path + l1, "/en.forms");
  parse_rules_file(path, LANG_EN, GROUP_FORMS);
}

void wforms_cleanup()
{
  int i, j, k, size;
  for (i = 0; i < MAX_LANGS; ++i)
  {
    for (j = 0; j < MAX_GROUP_TYPES; ++j)
    {
      size = rules_num[i][j];
      for (k = 0; k < size; ++k)
      {
        free(rules[i][j][k].arg);
        if (rules[i][j][k].pattern != NULL)
        {
          free(rules[i][j][k].pattern);
        }
      }
    }
  }
}

void wforms_test()
{
  char w[MAX_STR_LEN];
  int len;

  fprintf(stderr, "\ninflect? (y/n) ");
  fgets(w, MAX_STR_LEN, stdin);
  if (w[0] == 'y')
  {
    opt_search_inflections = 1;
  }
  else
  {
    opt_search_inflections = 0;
  }

  fprintf(stderr, "stem? (y/n) ");
  fgets(w, MAX_STR_LEN, stdin);
  if (w[0] == 'y')
  {
    opt_search_stem = 1;
  }
  else
  {
    opt_search_stem = 0;
  }

  fprintf(stderr, "forms? (y/n) ");
  fgets(w, MAX_STR_LEN, stdin);
  if (w[0] == 'y')
  {
    opt_search_forms = 1;
  }
  else
  {
    opt_search_forms = 0;
  }

  fprintf(stderr, "\nWord formation algorithm test (de).\n");
  for (;;)
  {
    fprintf(stderr, "Enter word (q to quit): ");
    fgets(w, MAX_STR_LEN, stdin);
    if (strcmp(w, "q\n") == 0)
    {
      break;
    }
    else
    {
      list_t *lst = list_node_new();
      lst->u.str = xstrdup(w);
      lst->next = NULL;
      lst = wforms_add(lst, "de");
      len = 0;
      printf("\n");
      while (lst != NULL)
      {
        printf("%s\n", lst->u.str);
        lst = lst->next;
        ++len;
      }
      printf("\n%d\n\n", len);
    }
  }

  fprintf(stderr, "\nWord formation algorithm test (en).\n");
  for (;;)
  {
    fprintf(stderr, "Enter word (q to quit): ");
    fgets(w, MAX_STR_LEN, stdin);
    if (strcmp(w, "q\n") == 0)
    {
      break;
    }
    else
    {
      list_t *lst = list_node_new();
      lst->u.str = xstrdup(w);
      lst->next = NULL;
      lst = wforms_add(lst, "en");
      len = 0;
      printf("\n");
      while (lst != NULL)
      {
        printf("%s\n", lst->u.str);
        lst = lst->next;
        ++len;
      }
      printf("\n%d\n\n", len);
    }
  }
}
