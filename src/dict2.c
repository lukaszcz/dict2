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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "limits.h"
#include "utils.h"
#include "strutils.h"
#include "list.h"
#include "rbtree.h"
#include "wforms.h"
#include "options.h"
#include "gui.h"

/* Standard file paths */

char path_dict2_glade[MAX_STR_LEN];
char path_dict2_xpm[MAX_STR_LEN];
char path_honig_txt[MAX_STR_LEN];
char path_irregular_verbs_de_txt[MAX_STR_LEN];
char path_config_file[MAX_STR_LEN];
char path_readme[MAX_STR_LEN];
char path_cache_dir[MAX_STR_LEN];
char path_data_dir[MAX_STR_LEN];

int main(int argc, char *argv[])
{
#ifndef DEBUG
  int len;
  const char *home;
#endif
  DIR *dir;
  const char *s;
  char str[MAX_STR_LEN + 1];
  str[MAX_STR_LEN] = '\0';

#ifdef DEBUG
  strcpy(path_dict2_glade, "/home/lukasz/progs/dict2/dict2.glade");
  strcpy(path_dict2_xpm, "/home/lukasz/progs/dict2/pixmaps/dict2.xpm");
  strcpy(path_honig_txt, "/home/lukasz/progs/dict2/data/honig.txt");
  strcpy(path_irregular_verbs_de_txt, "/home/lukasz/progs/dict2/data/irregular_verbs_de.txt");
  strcpy(path_config_file, ".dict2.cfg");
  strcpy(path_readme, "/home/lukasz/progs/dict2/README");
  strcpy(path_cache_dir, ".dict2.cache/");
  strcpy(path_data_dir, "/home/lukasz/progs/dict2/data/");
#else
  len = strlen(INSTALL_PREFIX);

  strcpy(path_data_dir, INSTALL_PREFIX);
  strcpy(path_data_dir + len, "/share/dict2/");

  strcpy(path_dict2_glade, INSTALL_PREFIX);
  strcpy(path_dict2_glade + len, "/share/dict2/dict2.glade");

  strcpy(path_dict2_xpm, INSTALL_PREFIX);
  strcpy(path_dict2_xpm + len, "/share/dict2/dict2.xpm");

  strcpy(path_honig_txt, INSTALL_PREFIX);
  strcpy(path_honig_txt + len, "/share/dict2/honig.txt");

  strcpy(path_irregular_verbs_de_txt, INSTALL_PREFIX);
  strcpy(path_irregular_verbs_de_txt + len, "/share/dict2/irregular_verbs_de.txt");

  strcpy(path_readme, INSTALL_PREFIX);
  strcpy(path_readme + len, "/share/dict2/README");

  home = getenv("HOME");
  if (home != NULL)
  {
    strcpy(path_config_file, home);
    strcpy(path_cache_dir, home);
    len = strlen(home);
  }
  else
  {
    len = 0;
  }
  strcpy(path_config_file + len, "/.dict2.cfg");
  strcpy(path_cache_dir + len, "/.dict2.cache");
#endif

  dir = opendir(path_cache_dir);
  if (dir == NULL)
  {
    mkdir(path_cache_dir, 0777);
  }else
  {
    closedir(dir);
  }

  utils_init();
  strutils_init();
  list_init();
  options_set_defaults();

  options_read_from_file(path_config_file);

  while ((s = error_str()) != NULL)
  {
    fprintf(stderr, "Error: %s\n", s);
  }

  wforms_init();

  if (argc == 3 && strcmp(argv[1], "--test") == 0)
  {
    if (strcmp(argv[2], "wforms") == 0)
    {
      wforms_test();
    }
    else if (strcmp(argv[2], "utils") == 0)
    {
      utils_test();
    }
    else if (strcmp(argv[2], "rbtree") == 0)
    {
      rb_test();
    }
    else if (strcmp(argv[2], "all") == 0)
    {
      utils_test();
      rb_test();
      wforms_test();
    }
    else
    {
      fprintf(stderr, "Error: Unknown test requested.\n");
    }
  }
  else
  {
    if (!run_gui(argc, argv))
    {
      fprintf(stderr, "Error: Couldn't initialize graphical interface.\n");
    }
  }

  options_save_to_file(path_config_file);

  while ((s = error_str()) != NULL)
  {
    fprintf(stderr, "Error: %s\n", s);
  }

  wforms_cleanup();
  options_cleanup();
  list_cleanup();
  strutils_cleanup();
  utils_cleanup();
  return 0;
}
