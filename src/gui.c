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

#include <glib.h>
#include <glib-object.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <sys/types.h>
#include <signal.h>

#include <stdio.h>
#include <string.h>

#include "paths.h"
#include "limits.h"
#include "utils.h"
#include "list.h"
#include "file.h"
#include "options.h"
#include "dictionary.h"
#include "cache.h"
#include "gui.h"

// the size of a dictionary above which to prompt whether to display or
// not
#define DICT_SIZE_PROMPT 1000

static dict_t *dicts[MAX_DICTS + MAX_DICTS_IN_FILE];
static int dict_active[MAX_DICTS + MAX_DICTS_IN_FILE];
static int dicts_num;

static int busy = 0; // nonzero if currently within a handler

#define STRBUF_SIZE 4096

static char strbuf[STRBUF_SIZE + 1];

static void gui_notifier(const char *event);
static void update_gui();
static void setup_progress_dialog(const char *text1, const char *text2);
static void set_cursor(GdkCursorType type);
static void reset_progressbar(const char *str);
static void load_dict(const char *filename);
static void autoload_dicts();
static void check_for_errors();
static GtkTreeView *get_current_results_view();
static void set_current_page_title(const char *text);
static void add_notebook_page();
static GtkListStore *init_tree_view_display(unsigned cols);
static void add_tree_view_row(GtkListStore *list_store, list_t *node);
static void display_results(GtkListStore *list_store, list_t *lst);
static void display_not_found(GtkListStore *list_store);
static void display_dictionary(dict_t *dict);
static void display_dicts_choice();
/* The returned list should be freed by the caller. */
static list_t *choose_dicts(const char *prompt);
static int view_progress();
static void error_box(const char* msg);
static int ask_yes_no(const char* question);
static void unload_dict(int dict_num);

void on_dict_toggled(GtkCellRendererToggle* toggle_renderer, gchar* path_str,
                     gpointer dummy);
void on_search_options_checkbutton_toggled(GObject *dummy1, gpointer dummy2);
void on_view_definition_button_clicked();

static GtkWindow *main_window;
static GtkEntry *text_entry;
static GtkNotebook *results_notebook;
static GtkTreeView *dicts_view;
static GtkTreeView *list_choose_view;
static GtkProgressBar *progress_dialog_progressbar;
static GtkDialog *progress_dialog;
static GtkLabel *progress_dialog_label1;
static GtkLabel *progress_dialog_label2;
static GtkDialog *list_choose_dialog;
static GtkDialog *about_dialog;
static GtkDialog *options_dialog;
static GtkDialog *readme_dialog;
static GtkDialog *definition_dialog;
static GtkLabel *list_choose_label;
static GtkStatusbar *statusbar;
static GtkTreeView *autoload_view;
static GtkCheckButton *umlaut_conversion_checkbutton;
static GtkCheckButton *ignore_case_checkbutton;
static GtkCheckButton *options_stem_checkbutton;
static GtkCheckButton *options_inflections_checkbutton;
static GtkCheckButton *options_forms_checkbutton;
static GtkCheckButton *options_caching_checkbutton;
static GtkCheckButton *stem_checkbutton;
static GtkCheckButton *inflections_checkbutton;
static GtkCheckButton *forms_checkbutton;
static GtkSpinButton *min_cached_file_size_spinbutton;
static GtkTextView *readme_textview;
static GtkTextView *lang1_textview;
static GtkTextView *lang2_textview;
static GtkLabel *lang1_label;
static GtkLabel *lang2_label;

static gboolean initialization_complete = FALSE;

static int progress_percent = 0;
int job_cancelled = 0;

int run_gui(int argc, char** argv)
{
  GladeXML *xml;
  GtkCellRenderer *toggle_renderer;
  GtkTreeViewColumn *column1;
  GtkTreeViewColumn *column2;
  GtkAccelGroup *accel_group;
  GtkWidget *widget;
  FILE *f;
  int strbuf_len, i;

  dicts_num = 0;
  strbuf[STRBUF_SIZE] = '\0';
  notifier = gui_notifier;

  if (gtk_init_check(&argc, &argv) == FALSE)
  {
    return 0;
  }

  xml = glade_xml_new(path_dict2_glade, NULL, NULL);
  if (xml == NULL)
  {
    return 0;
  }

  initialization_complete = FALSE;

  glade_xml_signal_autoconnect(xml);
  main_window = GTK_WINDOW (glade_xml_get_widget(xml, "main_window"));
  text_entry = GTK_ENTRY (glade_xml_get_widget(xml, "text_entry"));
  min_cached_file_size_spinbutton =
      GTK_SPIN_BUTTON (glade_xml_get_widget(xml,
                      "min_cached_file_size_spinbutton"));
  dicts_view = GTK_TREE_VIEW (glade_xml_get_widget(xml, "dicts_view"));
  progress_dialog =
      GTK_DIALOG (glade_xml_get_widget(xml, "progress_dialog"));
  progress_dialog_progressbar =
      GTK_PROGRESS_BAR (glade_xml_get_widget(xml,
                                             "progress_dialog_progressbar"));
  progress_dialog_label1 =
      GTK_LABEL (glade_xml_get_widget(xml, "progress_dialog_label1"));
  progress_dialog_label2 =
      GTK_LABEL (glade_xml_get_widget(xml, "progress_dialog_label2"));
  list_choose_dialog =
      GTK_DIALOG (glade_xml_get_widget(xml, "list_choose_dialog"));
  list_choose_view =
      GTK_TREE_VIEW (glade_xml_get_widget(xml, "list_choose_view"));
  list_choose_label =
      GTK_LABEL (glade_xml_get_widget(xml, "list_choose_label"));
  about_dialog =
      GTK_DIALOG (glade_xml_get_widget(xml, "about_dialog"));
  options_dialog = GTK_DIALOG (glade_xml_get_widget(xml, "options_dialog"));
  readme_dialog = GTK_DIALOG (glade_xml_get_widget(xml, "readme_dialog"));
  definition_dialog = GTK_DIALOG (glade_xml_get_widget(xml, "definition_dialog"));
  statusbar = GTK_STATUSBAR (glade_xml_get_widget(xml, "statusbar"));
  autoload_view = GTK_TREE_VIEW (glade_xml_get_widget(xml, "autoload_view"));
  umlaut_conversion_checkbutton =
      GTK_CHECK_BUTTON (glade_xml_get_widget(xml, "umlaut_conversion_checkbutton"));
  ignore_case_checkbutton =
      GTK_CHECK_BUTTON (glade_xml_get_widget(xml, "ignore_case_checkbutton"));
  options_stem_checkbutton =
      GTK_CHECK_BUTTON (glade_xml_get_widget(xml, "options_stem_checkbutton"));
  options_inflections_checkbutton =
      GTK_CHECK_BUTTON (glade_xml_get_widget(xml, "options_inflections_checkbutton"));
  options_forms_checkbutton =
      GTK_CHECK_BUTTON (glade_xml_get_widget(xml, "options_forms_checkbutton"));
  options_caching_checkbutton =
      GTK_CHECK_BUTTON (glade_xml_get_widget(xml, "options_caching_checkbutton"));
  stem_checkbutton =
      GTK_CHECK_BUTTON (glade_xml_get_widget(xml, "stem_checkbutton"));
  inflections_checkbutton =
      GTK_CHECK_BUTTON (glade_xml_get_widget(xml, "inflections_checkbutton"));
  forms_checkbutton =
      GTK_CHECK_BUTTON (glade_xml_get_widget(xml, "forms_checkbutton"));
  readme_textview =
      GTK_TEXT_VIEW (glade_xml_get_widget(xml, "readme_textview"));
  lang1_textview =
      GTK_TEXT_VIEW (glade_xml_get_widget(xml, "lang1_textview"));
  lang2_textview =
      GTK_TEXT_VIEW (glade_xml_get_widget(xml, "lang2_textview"));
  lang1_label =
      GTK_LABEL (glade_xml_get_widget(xml, "lang1_label"));
  lang2_label =
      GTK_LABEL (glade_xml_get_widget(xml, "lang2_label"));
  results_notebook =
      GTK_NOTEBOOK (glade_xml_get_widget(xml, "results_notebook"));
  gtk_notebook_remove_page(results_notebook, 0);
  add_notebook_page();

  // accelerators

  widget = glade_xml_get_widget(xml, "menuitem_new_page");
  accel_group = gtk_accel_group_new();
  gtk_window_add_accel_group(GTK_WINDOW(main_window), accel_group);
  gtk_widget_add_accelerator(widget, "activate", accel_group, GDK_t, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator(widget, "activate", accel_group, GDK_n, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  widget = glade_xml_get_widget(xml, "menuitem_close_page");
  accel_group = gtk_accel_group_new();
  gtk_window_add_accel_group(GTK_WINDOW(main_window), accel_group);
  gtk_widget_add_accelerator(widget, "activate", accel_group, GDK_w, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  widget = glade_xml_get_widget(xml, "menuitem_next_page");
  accel_group = gtk_accel_group_new();
  gtk_window_add_accel_group(GTK_WINDOW(main_window), accel_group);
  gtk_widget_add_accelerator(widget, "activate", accel_group, GDK_Page_Down, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator(widget, "activate", accel_group, GDK_Right, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);

  widget = glade_xml_get_widget(xml, "menuitem_prev_page");
  accel_group = gtk_accel_group_new();
  gtk_window_add_accel_group(GTK_WINDOW(main_window), accel_group);
  gtk_widget_add_accelerator(widget, "activate", accel_group, GDK_Page_Up, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator(widget, "activate", accel_group, GDK_Left, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);

  widget = glade_xml_get_widget(xml, "menuitem_load");
  accel_group = gtk_accel_group_new();
  gtk_window_add_accel_group(GTK_WINDOW(main_window), accel_group);
  gtk_widget_add_accelerator(widget, "activate", accel_group, GDK_l, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  widget = glade_xml_get_widget(xml, "menuitem_unload");
  accel_group = gtk_accel_group_new();
  gtk_window_add_accel_group(GTK_WINDOW(main_window), accel_group);
  gtk_widget_add_accelerator(widget, "activate", accel_group, GDK_u, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  widget = glade_xml_get_widget(xml, "menuitem_quit");
  accel_group = gtk_accel_group_new();
  gtk_window_add_accel_group(GTK_WINDOW(main_window), accel_group);
  gtk_widget_add_accelerator(widget, "activate", accel_group, GDK_q, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  widget = glade_xml_get_widget(xml, "menuitem_view_dictionary");
  accel_group = gtk_accel_group_new();
  gtk_window_add_accel_group(GTK_WINDOW(main_window), accel_group);
  gtk_widget_add_accelerator(widget, "activate", accel_group, GDK_d, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  widget = glade_xml_get_widget(xml, "menuitem_options");
  accel_group = gtk_accel_group_new();
  gtk_window_add_accel_group(GTK_WINDOW(main_window), accel_group);
  gtk_widget_add_accelerator(widget, "activate", accel_group, GDK_o, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  // initialize the main window

  gtk_window_set_icon_from_file(main_window, path_dict2_xpm, NULL);

  // initialize dicts_view

  toggle_renderer = gtk_cell_renderer_toggle_new();
  g_signal_connect(toggle_renderer, "toggled", G_CALLBACK(on_dict_toggled), NULL);
  column1 = gtk_tree_view_column_new_with_attributes ("Toggle",
                                                      toggle_renderer,
                                                      "active", 0, NULL);
  column2 = gtk_tree_view_column_new_with_attributes ("Name",
                                                      gtk_cell_renderer_text_new(),
                                                      "text", 1, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (dicts_view), column1);
  gtk_tree_view_append_column (GTK_TREE_VIEW (dicts_view), column2);

  // initialize list_choose_view

  column1 = gtk_tree_view_column_new_with_attributes ("Name",
                                                      gtk_cell_renderer_text_new(),
                                                      "text", 0, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list_choose_view), column1);

  // initialize autoload_view

  column1 = gtk_tree_view_column_new_with_attributes ("Name",
                                                      gtk_cell_renderer_text_new(),
                                                      "text", 0, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (autoload_view), column1);

  // initialize readme_textview

  f = fopen(path_readme, "r");
  if (f != NULL)
  {
    GtkTextBuffer* buf = gtk_text_view_get_buffer(readme_textview);
    strbuf_len = fread(strbuf, 1, STRBUF_SIZE, f);
    if (g_utf8_validate(strbuf, strbuf_len, 0))
    {
      gtk_text_buffer_set_text(buf, strbuf, strbuf_len);
    }
    else
    {
      fprintf(stderr, "Invalid UTF-8 in the README file.");
    }
    fclose(f);
  }

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(stem_checkbutton),
                               opt_search_stem);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(inflections_checkbutton),
                               opt_search_inflections);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(forms_checkbutton),
                               opt_search_forms);

  initialization_complete = TRUE;

  if (opt_autoload_list != NULL)
  {
    autoload_dicts();
  }

  gtk_main();

  for (i = 0; i < dicts_num; ++i)
  {
    dict_free(dicts[i]);
  }
  check_for_errors();

  return 1;
}

/* auxiliary functions */

static void gui_notifier(const char *event)
{
  if (strcmp(event, "cache_start") == 0)
  {
    gtk_label_set_text(progress_dialog_label1, "Caching ");
    gtk_widget_show_all(GTK_WIDGET(progress_dialog));
    update_gui();
  }
  else if (strcmp(event, "cache_finish") == 0)
  {
    gtk_label_set_text(progress_dialog_label1, "Loading ");
    gtk_widget_show_all(GTK_WIDGET(progress_dialog));
    update_gui();
  }
}

static void update_gui()
{
  while (gtk_events_pending())
  {
    gtk_main_iteration();
  }
}

static void set_cursor(GdkCursorType type)
{
  GdkCursor *cursor = gdk_cursor_new(type);
  gdk_window_set_cursor(GTK_WIDGET(main_window)->window, cursor);
//  gdk_window_set_cursor(GTK_WIDGET(progress_dialog)->window, cursor);
  gdk_cursor_unref(cursor);
  update_gui();
}

static void reset_progressbar(const char *str)
{
  progress_percent = 0;
  gtk_label_set_text(progress_dialog_label2, str);
  gtk_progress_bar_set_fraction(progress_dialog_progressbar, 0);
  gtk_widget_show_all(GTK_WIDGET(progress_dialog));
  update_gui();
  gtk_widget_show_now(GTK_WIDGET(progress_dialog));
}

static void setup_progress_dialog(const char *text1, const char *text2)
{
  progress_percent = 0;
  job_cancelled = 0;
  gtk_widget_show_all(GTK_WIDGET(progress_dialog));
  update_gui();
  gtk_window_set_title(GTK_WINDOW(progress_dialog), text1);
  gtk_label_set_text(progress_dialog_label1, text2);
  gtk_progress_bar_set_fraction(progress_dialog_progressbar, 0);
  update_gui();
  gtk_widget_show_all(GTK_WIDGET(progress_dialog));
  update_gui();
}

static void load_dict(const char *filename)
{
  file_t *file;
  int i, n;
  dict_t *dict;

  if (dicts_num >= MAX_DICTS)
  {
    error_box("Too many dictionaries loaded.");
    return;
  }
  file = file_load(filename);
  if (file == NULL)
  {
    check_for_errors();
    return;
  }
  file_read_header(file);
  n = file_header.dicts_num;
  progress_notifier = view_progress;
  progress_max = 100 / n;
  for (i = 0; i < n; ++i)
  {
    dict = dict_create(file, i);
    check_for_errors();
    if (dict == NULL)
    {
      return;
    }
    dicts[dicts_num] = dict;
    dict_active[dicts_num] = 1;
    ++dicts_num;
  }
}

static void autoload_dicts()
{
  list_t *lst;
  int n, i;

  setup_progress_dialog("Loading...", "Loading ");
  gtk_widget_hide_all(GTK_WIDGET(main_window));
  update_gui();

  n = list_length(opt_autoload_list);
  lst = opt_autoload_list;
  progress_max = 100 / n;
  progress_notifier = view_progress;
  progress_percent = 0;
  while (lst != NULL)
  {
    reset_progressbar(lst->u.str);
    load_dict(lst->u.str);
    lst = lst->next;
  }
  for (i = 0; i < dicts_num; ++i)
  {
    dict_active[i] = options_check_active(dicts[i]->name);
  }
  check_for_errors();

  gtk_widget_hide_all(GTK_WIDGET(progress_dialog));
  gtk_widget_show_all(GTK_WIDGET(main_window));
  display_dicts_choice();
}

static void check_for_errors()
{
  const char *str;
  while ((str = error_str()) != NULL)
  {
    error_box(str);
  }
}

static GtkTreeView *get_current_results_view()
{
  return GTK_TREE_VIEW(gtk_bin_get_child(GTK_BIN(gtk_notebook_get_nth_page(results_notebook, gtk_notebook_get_current_page(results_notebook)))));
}

static void set_current_page_title(const char *text)
{
  if (gtk_notebook_get_n_pages(results_notebook) == 0)
  {
    add_notebook_page();
  }

  gtk_notebook_set_tab_label_text(results_notebook, gtk_notebook_get_nth_page(results_notebook, gtk_notebook_get_current_page(results_notebook)), text);
}

static void add_notebook_page()
{
  GtkTreeView *results_view;
  GtkScrolledWindow *wnd;
  gint page_num;

  // create a new GtkTreeView for the results

  results_view = GTK_TREE_VIEW(gtk_tree_view_new());
  gtk_tree_view_set_headers_visible(results_view, FALSE);
  g_signal_connect(results_view, "row-activated",
                   G_CALLBACK(on_view_definition_button_clicked), NULL);

  // create a scrolled window

  wnd = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new(NULL, NULL));
  gtk_container_add(GTK_CONTAINER(wnd), GTK_WIDGET(results_view));

  // create a new notebook page

  page_num = gtk_notebook_append_page(results_notebook, GTK_WIDGET(wnd), NULL);
  gtk_widget_show_all(GTK_WIDGET(results_notebook));
  gtk_notebook_set_current_page(results_notebook, page_num);
}

static GtkListStore *init_tree_view_display(unsigned cols)
{
  GtkTreeView *results_view;
  GtkTreeModel *model;
  GtkTreeViewColumn *column;
  gint width;
  unsigned i;
  GType *types;
  GtkListStore *list_store;

  if (gtk_notebook_get_n_pages(results_notebook) == 0)
  {
    add_notebook_page();
  }

  results_view = get_current_results_view();

  // destroy the current model

  model = gtk_tree_view_get_model(results_view);
  gtk_tree_view_set_model(results_view, NULL);
  if (model != NULL)
  {
    gtk_list_store_clear(GTK_LIST_STORE (model));
      //g_free(model);
  }

  // remove all columns

  while ((column = gtk_tree_view_get_column(results_view, 0)) != NULL)
  {
    gtk_tree_view_remove_column(results_view, column);
  }

  // add new columns

  gdk_drawable_get_size(GDK_DRAWABLE(gtk_tree_view_get_bin_window(results_view)), &width, NULL);
  width -= (width / 50);
  width /= cols;

  for (i = 0; i != cols; ++i)
  {
    column = gtk_tree_view_column_new_with_attributes ("title",
                                  gtk_cell_renderer_text_new(),
                                  "text", i,
                                  NULL);
    gtk_tree_view_column_set_min_width(column, width);
    gtk_tree_view_column_set_max_width(column, width);
    gtk_tree_view_append_column (GTK_TREE_VIEW (results_view), column);
  }

  // create a list store

  types = (GType*) xmalloc(sizeof(GType) * cols);
  for (i = 0; i != cols; ++i)
  {
    types[i] = G_TYPE_STRING;
  }
  list_store = gtk_list_store_newv (cols, types);
  free(types);

  return list_store;
}

static void add_tree_view_row(GtkListStore *list_store, list_t *lst)
{
  GtkTreeIter iter;
  unsigned j;

  assert (lst != NULL);

  gtk_list_store_append (list_store, &iter);
  j = 0;
  while (lst != NULL)
  {
    gtk_list_store_set (list_store, &iter,
                        j, lst->u.str,
                        -1);
    lst = lst->next;
    ++j;
  }
}

static void display_results(GtkListStore *list_store, list_t *lst)
{
  list_t *l = lst;

  while (l != NULL)
  {
    add_tree_view_row(list_store, l->u.lst);
    l = l->next;
  }
}

static void display_not_found(GtkListStore *list_store)
{
  GtkTreeIter iter;
  gtk_list_store_append (list_store, &iter);
  gtk_list_store_set (list_store, &iter,
                      0, "Not found",
                      -1);
}

static void display_dictionary(dict_t *dict)
{
  unsigned cols;
  int i;
  GtkListStore *list_store;
  list_t *lst;

  assert (dict != NULL);

  if (dict->size > DICT_SIZE_PROMPT)
  {
    snprintf(strbuf, STRBUF_SIZE,
             "This will display the whole dictionary (%d entries)."
             "Are you sure you want to continue?", dict->size);
    if (!ask_yes_no(strbuf))
    {
      return;
    }
  }

  set_current_page_title(dict->name);
  cols = dict->entries_num;

  list_store = init_tree_view_display(cols);
  i = file_read_header(dict->file);
  while (i < dict->file->length)
  {
    lst = line_idx_to_entry_list(dict, i);
    if (strlist_utf8_validate(lst))
    {
      add_tree_view_row(list_store, lst);
    }
    strlist_free(lst);
    i = file_skip_line(dict->file, i);
  }

  if (dict->size == 0)
  {
    GtkTreeIter iter;
    gtk_list_store_append (list_store, &iter);
    gtk_list_store_set (list_store, &iter,
                        0, "The dictionary is empty",
                        -1);
  }

  gtk_tree_view_set_model(get_current_results_view(),
                          GTK_TREE_MODEL (list_store));
}

static void display_dicts_choice()
{
  int i;
  GtkTreeIter iter;
  GtkListStore* list_store = gtk_list_store_new (2,
                                                 G_TYPE_BOOLEAN,
                                                 G_TYPE_STRING);

  for (i = 0; i != dicts_num; ++i)
  {
    gtk_list_store_append (list_store, &iter);
    gtk_list_store_set (list_store, &iter,
                        0, dict_active[i],
                        1, dicts[i]->name,
                        -1);
  }

  gtk_tree_view_set_model(dicts_view, GTK_TREE_MODEL(list_store));
}

static void tree_path_free(gpointer path, gpointer dummy)
{
  gtk_tree_path_free((GtkTreePath*)(path));
}

static list_t *choose_dicts(const char *prompt)
{
  list_t *dict_list = NULL;
  int i;

  gtk_label_set_text(list_choose_label, prompt);
  GtkListStore* list_store = gtk_list_store_new(1, G_TYPE_STRING);
  for (i = 0; i != dicts_num; ++i)
  {
    GtkTreeIter iter;
    gtk_list_store_append(list_store, &iter);
    gtk_list_store_set(list_store, &iter,
                       0, dicts[i]->name,
                       -1);
  }
  gtk_tree_view_set_model(list_choose_view, GTK_TREE_MODEL(list_store));

  gtk_widget_show_all(GTK_WIDGET(list_choose_dialog));
  gint response = gtk_dialog_run(list_choose_dialog);
  gtk_widget_hide_all(GTK_WIDGET(list_choose_dialog));
  if (response == GTK_RESPONSE_OK)
  {
    GList* list = gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(list_choose_view),
        NULL);
    GList* node = list;
    while (node != NULL)
    {
      gchar *str = gtk_tree_path_to_string((GtkTreePath*)(node->data));
      int dict_num = atoi(str);
      list_t *ln = list_node_new();
      ln->u.value = dict_num;
      ln->next = dict_list;
      dict_list = ln;
      node = node->next;
      g_free(str);
    }
    g_list_foreach (list, tree_path_free, NULL);
    g_list_free (list);
  }
  return dict_list;
}

static void search_dicts(const char *text, search_t search_type)
{
  int i, cols, active_dicts_num, not_found;
  keyword_handle_t de_handle = NULL;
  keyword_handle_t en_handle = NULL;
  list_t *lst;
  list_t *lst2;
  GtkListStore *list_store;
  guint context_id;

  context_id = gtk_statusbar_get_context_id(statusbar, "default context");
  snprintf(strbuf, STRBUF_SIZE, "Searching %s...", text);
  gtk_statusbar_push(statusbar, context_id, strbuf);
  set_current_page_title(text);
  update_gui();
  set_cursor(GDK_WATCH);

  if (dicts_num == 0)
  {
    list_store = init_tree_view_display(1);
    display_not_found(list_store);
    gtk_tree_view_set_model(get_current_results_view(),
                            GTK_TREE_MODEL(list_store));
    gtk_statusbar_pop(statusbar, context_id);
    set_cursor(GDK_LEFT_PTR);
    update_gui();
    return;
  }

  active_dicts_num = 0;
  for (i = 0; i < dicts_num; ++i)
  {
    if (dict_active[i])
    {
      ++active_dicts_num;
      if (search_type == SEARCH_KEYWORD)
      {
        if (strcmp(dicts[i]->langs[0], "de") == 0)
        {
          if (de_handle == NULL)
          {
            de_handle = dict_keyword_handle_new(text, "de");
          }
        }
        else
        {
          assert (strcmp(dicts[i]->langs[0], "en") == 0);
          if (en_handle == NULL)
          {
            en_handle = dict_keyword_handle_new(text, "en");
          }
        }
      } // end if (search_type == SEARCH_KEYWORD)
    } // end if (dict_active[i])
  } // end for
  lst = NULL;
  not_found = 1;
  if (active_dicts_num != 0)
  {
    progress_max = 100 / active_dicts_num;
    progress_notifier = view_progress;
    cols = 1;
    for (i = 0; i < dicts_num; ++i)
    {
      if (dict_active[i])
      {
        gtk_label_set_text(progress_dialog_label2, dicts[i]->name);
        update_gui();
        if (search_type == SEARCH_KEYWORD)
        {
          if (strcmp(dicts[i]->langs[0], "de") == 0)
          {
            lst2 = dict_search_keyword(dicts[i], de_handle);
          }
          else
          {
            assert (strcmp(dicts[i]->langs[0], "en") == 0);
            lst2 = dict_search_keyword(dicts[i], en_handle);
          }
        }
        else
        {
          lst2 = dict_search(dicts[i], text, search_type);
        }
        if (lst2 != NULL && dicts[i]->entries_num > cols)
        {
          cols = dicts[i]->entries_num;
        }
        lst = list_append(lst2, lst);
        check_for_errors();
      }
    }
    list_store = init_tree_view_display(cols);
    if (lst != NULL)
    {
      not_found = 0;
    }
    lst = sort_search_results(lst);
    display_results(list_store, lst);
    list_free_2(lst, node_strlist_free);
  }
  else
  {
    list_store = init_tree_view_display(1);
  }

  if (not_found)
  {
    display_not_found(list_store);
  }

  if (search_type == SEARCH_KEYWORD)
  {
    dict_keyword_handle_free(de_handle);
    dict_keyword_handle_free(en_handle);
  }

  gtk_tree_view_set_model(get_current_results_view(),
                          GTK_TREE_MODEL(list_store));
  gtk_statusbar_pop(statusbar, context_id);
  set_cursor(GDK_LEFT_PTR);
  update_gui();
}

static void load_dicts_from_file(const char *filename)
{
  guint context_id;
  setup_progress_dialog("Loading...", "Loading ");
  reset_progressbar(filename);
  context_id = gtk_statusbar_get_context_id(statusbar, "default context");
  snprintf(strbuf, STRBUF_SIZE, "Loading dictionaries from %s...", filename);
  gtk_statusbar_push(statusbar, context_id, strbuf);
  update_gui();
  set_cursor(GDK_WATCH);

  load_dict(filename);
  check_for_errors();

  gtk_widget_hide_all(GTK_WIDGET(progress_dialog));
  display_dicts_choice();
  set_cursor(GDK_LEFT_PTR);
  gtk_statusbar_pop(statusbar, context_id);
}

static int view_progress()
{
  if (job_cancelled)
  {
    gtk_widget_hide_all(GTK_WIDGET(progress_dialog));
    update_gui();
    return 0;
  }

  ++progress_percent;
  double fraction = (double) progress_percent / 100;
  if (fraction > 1.0)
  {
    fraction = 1.0;
  }

  gtk_progress_bar_set_fraction(progress_dialog_progressbar, fraction);
  gtk_widget_show_all(GTK_WIDGET(progress_dialog));
  update_gui();
  gtk_widget_show_now(GTK_WIDGET(progress_dialog));

  return 1;
}

static void error_box(const char *msg)
{
  GtkWidget* error_dialog = gtk_message_dialog_new(main_window, GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                   "%s", msg);
  gtk_dialog_run (GTK_DIALOG (error_dialog));
  gtk_widget_destroy(error_dialog);
}

static int ask_yes_no(const char *question)
{
  int result;
  GtkWidget* dialog = gtk_message_dialog_new(main_window, GTK_DIALOG_MODAL,
                                             GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
                                             "%s", question);
  result = (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES);
  gtk_widget_destroy(dialog);
  return result;
}

static void unload_dict(int dict_num)
{
  dict_t *dict;
  int i;
  guint context_id = gtk_statusbar_get_context_id(statusbar, "default context");
  snprintf(strbuf, STRBUF_SIZE, "Unloading %s...", dicts[dict_num]->name);
  gtk_statusbar_push(statusbar, context_id, strbuf);
  set_cursor(GDK_WATCH);
  update_gui();

  /* destroy the dictionary */
  dict = dicts[dict_num];
  --dicts_num;
  for (i = dict_num; i < dicts_num; ++i)
  {
    dicts[i] = dicts[i + 1];
    dict_active[i] = dict_active[i + 1];
  }
  dict_free(dict);

  gtk_statusbar_pop(statusbar, context_id);
  set_cursor(GDK_LEFT_PTR);
  update_gui();
}

void on_dict_toggled(GtkCellRendererToggle *toggle_renderer, gchar *path_str,
                     gpointer dummy)
{
  GtkTreeModel *model;
  GtkTreePath *path;
  GtkTreeIter iter;
  if (busy)
  {
    return;
  }
  busy = 1;

  model = gtk_tree_view_get_model(dicts_view);
  path = gtk_tree_path_new_from_string(path_str);
  gtk_tree_model_get_iter(model, &iter, path);
  gtk_tree_path_free(path);

  int dict_num = atoi(path_str);
  dict_active[dict_num] = !dict_active[dict_num];
  if (dict_active[dict_num])
  {
    options_add_active(dicts[dict_num]->name);
  } else
  {
    options_remove_active(dicts[dict_num]->name);
  }
  gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                     0, dict_active[dict_num],
                     -1);

  busy = 0;
}

void on_load1_activate(GtkWidget *dummy1, gpointer dummy2)
{
  GtkWidget *dialog;
  if (busy)
  {
    return;
  }
  busy = 1;

  dialog = gtk_file_chooser_dialog_new ("Load a dictionary",
                                        main_window,
                                        GTK_FILE_CHOOSER_ACTION_OPEN,
                                        GTK_STOCK_CANCEL,
                                        GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_OPEN,
                                        GTK_RESPONSE_ACCEPT,
                                        NULL);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
  {
    char *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    gtk_widget_destroy (dialog);

    load_dicts_from_file(filename);
    g_free(filename);
  }
  else
  {
    gtk_widget_destroy(dialog);
  }

  busy = 0;
}

void on_unload1_activate(GObject *dummy1, gpointer dummy2)
{
  list_t *node;
  list_t *dict_list;
  if (busy)
  {
    return;
  }
  busy = 1;

  dict_list = choose_dicts("Choose the dictionary to unload.");
  node = dict_list;
  while (node != NULL)
  {
    unload_dict(node->u.value);
    node = node->next;
  }
  list_free(dict_list);
  display_dicts_choice();

  busy = 0;
}

void on_copy1_activate(GObject *dummy1, gpointer dummy2)
{
  const gchar *text = gtk_entry_get_text(text_entry);
  gtk_clipboard_set_text (gtk_clipboard_get (gdk_atom_intern("CLIPBOARD", FALSE)), text, -1);
}

void on_cut1_activate(GObject *dummy1, gpointer dummy2)
{
  on_copy1_activate(dummy1, dummy2);
  gtk_entry_set_text (text_entry, "");
}

void clipboard_text_receiver(GtkClipboard *dummy1, const gchar *text, gpointer dummy2)
{
  if (text != NULL)
  {
    gtk_entry_set_text (text_entry, text);
  }
}

void on_paste1_activate(GObject *dummy1, gpointer dummy2)
{
  GtkClipboard *clipboard = gtk_clipboard_get (gdk_atom_intern("CLIPBOARD", FALSE));
  gtk_clipboard_request_text (clipboard, clipboard_text_receiver, NULL);
}

void on_add_autoload_button_clicked(GObject *dummy1, gpointer dummy2)
{
  GtkWidget* dialog;
/*  if (busy)
  {
    return;
  }
  busy = 1;*/

  dialog = gtk_file_chooser_dialog_new ("Choose a file",
                                        main_window,
                                        GTK_FILE_CHOOSER_ACTION_OPEN,
                                        GTK_STOCK_CANCEL,
                                        GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                        NULL);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
  {
    GtkListStore *list_store;
    GtkTreeIter iter;
    char *filename;
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    gtk_widget_destroy (dialog);

    list_store = GTK_LIST_STORE(gtk_tree_view_get_model(autoload_view));
    gtk_list_store_append(list_store, &iter);
    gtk_list_store_set(list_store, &iter,
                       0, filename,
                       -1);
    g_free(filename);
  }
  else
  {
    gtk_widget_destroy(dialog);
  }

  //busy = 0;
}

static void remove_from_list_store(GtkTreeModel *model, GtkTreePath *path,
                                   GtkTreeIter *iter, gpointer data)
{
  gtk_list_store_remove(GTK_LIST_STORE(model), iter);
}

void on_remove_autoload_button_clicked(GObject *dummy1, gpointer dummy2)
{
/*  if (busy)
  {
    return;
  }
  busy = 1;*/
  gtk_tree_selection_selected_foreach(gtk_tree_view_get_selection(autoload_view),
                                      remove_from_list_store, NULL);
  //busy = 0;
}

static gboolean add_autoload_func(GtkTreeModel *model, GtkTreePath *tree_path,
                                  GtkTreeIter *tree_iter,
                                  gpointer dummy)
{
  char *str;
  gtk_tree_model_get(model, tree_iter,
                     0, &str,
                     -1);
  options_add_autoload_file_last(str);
  g_free(str);
  return FALSE;
}

void on_options1_activate(GObject *dummy1, gpointer dummy2)
{
  GtkTreeIter iter;
  GtkListStore *list_store;
  if (busy)
  {
    return;
  }
  busy = 1;

  list_store = gtk_list_store_new (1, G_TYPE_STRING);
  list_t *lst;

  lst = opt_autoload_list;
  while (lst != NULL)
  {
    gtk_list_store_append (list_store, &iter);
    gtk_list_store_set (list_store, &iter,
                        0, lst->u.str,
                        -1);
    lst = lst->next;
  }

  gtk_tree_view_set_model(autoload_view, GTK_TREE_MODEL(list_store));

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(umlaut_conversion_checkbutton),
                               opt_german_umlaut_conversion);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ignore_case_checkbutton),
                               opt_ignore_case);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(options_stem_checkbutton),
                               opt_search_stem);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(options_inflections_checkbutton),
                               opt_search_inflections);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(options_forms_checkbutton),
                               opt_search_forms);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(options_caching_checkbutton),
                               opt_caching);
  gtk_spin_button_set_value(min_cached_file_size_spinbutton,
                            opt_cache_min_file_size / 1024);

  gtk_widget_show_all(GTK_WIDGET(options_dialog));
  if (gtk_dialog_run(options_dialog) == GTK_RESPONSE_OK)
  {
    opt_german_umlaut_conversion = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(umlaut_conversion_checkbutton));
    opt_ignore_case = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ignore_case_checkbutton));
    opt_search_stem = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(options_stem_checkbutton));
    opt_search_inflections = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(options_inflections_checkbutton));
    opt_search_forms = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(options_forms_checkbutton));
    opt_caching = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(options_caching_checkbutton));
    opt_cache_min_file_size = gtk_spin_button_get_value_as_int(min_cached_file_size_spinbutton);
    opt_cache_min_file_size *= 1024;

    options_autoload_list_clear();
    gtk_tree_model_foreach(gtk_tree_view_get_model(autoload_view), add_autoload_func, NULL);
  }
  gtk_widget_hide_all(GTK_WIDGET(options_dialog));

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(stem_checkbutton),
                               opt_search_stem);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(inflections_checkbutton),
                               opt_search_inflections);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(forms_checkbutton),
                               opt_search_forms);

  busy = 0;
}

void on_about1_activate(GObject *dummy1, gpointer dummy2)
{
  if (busy)
  {
    return;
  }
  busy = 1;
  gtk_widget_show_all(GTK_WIDGET(about_dialog));
  gtk_dialog_run(about_dialog);
  gtk_widget_hide_all(GTK_WIDGET(about_dialog));
  busy = 0;
}

void on_readme1_activate(GObject *dummy1, gpointer dummy2)
{
  if (busy)
  {
    return;
  }
  busy = 1;
  gtk_widget_show_all(GTK_WIDGET(readme_dialog));
  gtk_dialog_run(readme_dialog);
  gtk_widget_hide_all(GTK_WIDGET(readme_dialog));
  busy = 0;
}

void on_find1_clicked(GObject *dummy1, gpointer dummy2)
{
  const gchar *text;
  if (busy)
  {
    return;
  }
  busy = 1;

  text = gtk_entry_get_text(text_entry);
  if (!(text == NULL || strcmp(text, "") == 0))
  {
    search_dicts(text, SEARCH_KEYWORD);
  }

  busy = 0;
}

void on_find_regex1_clicked(GObject *dummy1, gpointer dummy2)
{
  const gchar *text;
  if (busy)
  {
    return;
  }
  busy = 1;

  text = gtk_entry_get_text(text_entry);
  if (!(text == NULL || strcmp(text, "") == 0))
  {
    setup_progress_dialog("Searching...", "Searching in ");
    search_dicts(text, SEARCH_REGEX);
    gtk_widget_hide_all(GTK_WIDGET(progress_dialog));
  }

  busy = 0;
}

void on_find_exact_clicked(GObject *dummy1, gpointer dummy2)
{
  const gchar *text;
  if (busy)
  {
    return;
  }
  busy = 1;

  text = gtk_entry_get_text(text_entry);
  if (!(text == NULL || strcmp(text, "") == 0))
  {
    search_dicts(text, SEARCH_EXACT);
  }

  busy = 0;
}

void on_clear_cache_button_clicked(GObject *dummy1, gpointer dummy2)
{
/*  if (busy)
  {
    return;
  }
  busy = 1;*/
  cache_clear();
//  busy = 0;
}

void on_progress_dialog_cancel_button_clicked(GObject *dummy1, gpointer dummy2)
{
  job_cancelled = 1;
}

void on_page_new_activate(GObject *dummy1, gpointer dummy2)
{
  if (busy)
  {
    return;
  }
  busy = 1;
  add_notebook_page();
  busy = 0;
}

void on_page_close_activate(GObject *dummy1, gpointer dummy2)
{
  if (busy)
  {
    return;
  }
  busy = 1;
  gtk_notebook_remove_page(results_notebook,
                           gtk_notebook_get_current_page(results_notebook));
  busy = 0;
}

void on_page_next_activate(GObject *dummy1, gpointer dummy2)
{
  if (busy)
  {
    return;
  }
  busy = 1;
  gtk_notebook_next_page(results_notebook);
  busy = 0;
}

void on_page_previous_activate(GObject *dummy1, gpointer dummy2)
{
  if (busy)
  {
    return;
  }
  busy = 1;
  gtk_notebook_prev_page(results_notebook);
  busy = 0;
}

void on_view_dictionary_activate(GObject *dummy1, gpointer dummy2)
{
  list_t *list;
  list_t *lst;
  if (busy)
  {
    return;
  }
  busy = 1;

  lst = choose_dicts("Choose the dictionary to display");
  list = lst;
  while (lst != NULL)
  {
    display_dictionary(dicts[lst->u.value]);
    lst = lst->next;
  }
  list_free(list);

  busy = 0;
}

static void text_entry_insert_text(const char *text)
{
  gint pos;
  pos = gtk_editable_get_position(GTK_EDITABLE(text_entry));
  gtk_editable_insert_text(GTK_EDITABLE(text_entry), text, strlen(text),
                           &pos);
  gtk_editable_set_position(GTK_EDITABLE(text_entry), pos);
  gtk_widget_grab_focus(GTK_WIDGET(text_entry));
  gtk_editable_select_region(GTK_EDITABLE(text_entry), pos, pos);
}

void on_ss_button_clicked(GObject *dummy1, gpointer dummy2)
{
  text_entry_insert_text("ß");
}

void on_ae_button_clicked(GObject *dummy1, gpointer dummy2)
{
  text_entry_insert_text("ä");
}

void on_oe_button_clicked(GObject *dummy1, gpointer dummy2)
{
  text_entry_insert_text("ö");
}

void on_ue_button_clicked(GObject *dummy1, gpointer dummy2)
{
  text_entry_insert_text("ü");
}

void on_big_ae_button_clicked(GObject *dummy1, gpointer dummy2)
{
  text_entry_insert_text("Ä");
}

void on_big_oe_button_clicked(GObject *dummy1, gpointer dummy2)
{
  text_entry_insert_text("Ö");
}

void on_big_ue_button_clicked(GObject *dummy1, gpointer dummy2)
{
  text_entry_insert_text("Ü");
}

void on_search_options_checkbutton_toggled(GObject *dummy1, gpointer dummy2)
{
  if (initialization_complete)
  {
    opt_search_stem = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(stem_checkbutton));
    opt_search_inflections = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(inflections_checkbutton));
    opt_search_forms = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(forms_checkbutton));
  }
}

gboolean on_progress_dialog_delete()
{
  job_cancelled = 1;
  return TRUE;
}

static void view_definition(GtkTreeModel *model, GtkTreePath *path,
                            GtkTreeIter *iter, gpointer data)
{
  GValue value1;
  GValue value2;
  memset(&value1, 0, sizeof(GValue));
  memset(&value2, 0, sizeof(GValue));
  gtk_tree_model_get_value(model, iter, 0, &value1);
  gtk_tree_model_get_value(model, iter, 1, &value2);
  gtk_text_buffer_set_text(gtk_text_view_get_buffer(lang1_textview),
                           g_value_get_string(&value1), -1);
  gtk_text_buffer_set_text(gtk_text_view_get_buffer(lang2_textview),
                           g_value_get_string(&value2), -1);
  g_value_unset(&value1);
  g_value_unset(&value2);
  gtk_widget_show_all(GTK_WIDGET (definition_dialog));
}

void on_view_definition_button_clicked()
{
  GtkTreeSelection *tree_selection;
  if (busy)
  {
    return;
  }
  busy = 1;
  tree_selection = gtk_tree_view_get_selection(get_current_results_view());
  gtk_tree_selection_selected_foreach(tree_selection,
                                      view_definition, NULL);
  busy = 0;
}

void definition_dialog_close()
{
  gtk_widget_hide_all(GTK_WIDGET (definition_dialog));
}

void on_quit()
{
  int i;
  if (busy)
  {
    return;
  }
  busy = 1;
  set_cursor(GDK_WATCH);
  for (i = 0; i < dicts_num; ++i)
  {
    guint context_id = gtk_statusbar_get_context_id(statusbar, "default context");
    snprintf(strbuf, STRBUF_SIZE, "Unloading %s...", dicts[i]->name);
    gtk_statusbar_push(statusbar, context_id, strbuf);
    update_gui();
    dict_free(dicts[i]);
    gtk_statusbar_pop(statusbar, context_id);
    update_gui();
  }
  set_cursor(GDK_LEFT_PTR);
  check_for_errors();
  dicts_num = 0;
  gtk_main_quit();
  busy = 0;
}
