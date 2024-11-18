#include "btree.h"
#include "raider.h"
#include "utils.h"

#include <dirent.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


static
State* fix_directory_state(char* directory, State dflt) {
  State* tmp;
  struct stat dir_info;

  // get directory inode
  if (stat(directory, &dir_info) == 0) {
    tmp = (State*) btree_get(HISTORY, btree_cantor(dir_info.st_dev, dir_info.st_ino));

    if (tmp == NULL) {
      tmp = (State*) malloc(sizeof(State));

      tmp->start_pos = dflt.start_pos;
      tmp->pos       = dflt.pos;
      tmp->end_pos   = dflt.end_pos;

      tmp->order     = dflt.order;
      tmp->files_n   = dflt.files_n;

      btree_set(HISTORY, btree_cantor(dir_info.st_dev, dir_info.st_ino), (void*)tmp);
    }
    else if (tmp->files_n != dflt.files_n) {
      // file number has changed: it's necessary to fix the state
      tmp->start_pos = dflt.start_pos;
      tmp->pos       = dflt.pos;
      tmp->end_pos   = dflt.end_pos;

      tmp->files_n   = dflt.files_n;
    }
  }
  else {
    tmp = (State*) malloc(sizeof(State));

    tmp->start_pos = dflt.start_pos;
    tmp->pos       = dflt.pos;
    tmp->end_pos   = dflt.end_pos;

    tmp->order     = dflt.order;
    tmp->files_n   = dflt.files_n;
  }

  return tmp;
}


static
void move_pos_by(int delta) {
  if (delta < 0) {
    if ((int) STATE->pos < -delta) return;

    STATE->pos -= -delta;

    if ((int) STATE->start_pos < -delta) {
      STATE->end_pos -= STATE->start_pos;
      STATE->start_pos = 0;
    }
    else {
      STATE->start_pos -= -delta;
      STATE->end_pos -= -delta;
    }
  }
  else if (delta > 0) {
    if (STATE->pos + delta > STATE->files_n-1) return;

    STATE->pos += delta;

    if (STATE->end_pos + delta >= STATE->files_n-1) {
      STATE->start_pos += STATE->files_n - STATE->end_pos - 1;
      STATE->end_pos = STATE->files_n-1;
    }
    else {
      STATE->start_pos += delta;
      STATE->end_pos += delta;
    }
  }
}


static
void move_pos_to(size_t new_pos) {
  move_pos_by(STATE->pos <= new_pos ? (new_pos-STATE->pos) : -(STATE->pos-new_pos));
}


static
void suspend_exec_resume(const char* dir, const char* cmd, const char* err) {
  endwin();

  int r = chdir(dir);

  if (r == 0)
    r = system(cmd);

  init_curses();

  action_goto_path(CURRENT_DIR);

  if (r != 0) display_error(err);
}


void action_resize_window(void) {
  endwin();

  init_curses();

  preview_get_xwin_size(PREVIEW);

  display_update_top();
  display_update_bot();
  display_update_lft();
  display_update_rgt(true);
}


void action_goto(const char* dir_part, const char* file_part) {
  int N = list_dir(dir_part);

  State dflt;
  if (N < 0) {
    werase(WLFT);

    wattron(WLFT, COLOR_PAIR(PAIR_RED_BLACK) | A_BOLD);
    waddstr(WLFT, "  [Not Found]");
    wattroff(WLFT, COLOR_PAIR(PAIR_RED_BLACK) | A_BOLD);

    wrefresh(WLFT);

    werase(WRGT);
    wrefresh(WRGT);

    werase(WBOT);
    wrefresh(WBOT);
  }
  else if (N == 0) {
    strlcpy(CURRENT_DIR, dir_part, sizeof(CURRENT_DIR));

    dflt.pos       = 0;
    dflt.start_pos = 0;
    dflt.end_pos   = 0;
    dflt.files_n   = 0;
    dflt.order     = 'n';

    STATE = fix_directory_state(CURRENT_DIR, dflt);

    display_update_top();

    werase(WLFT);
    waddstr(WLFT, "  [Empty]");
    wrefresh(WLFT);

    werase(WRGT);
    wrefresh(WRGT);

    werase(WBOT);
    wrefresh(WBOT);

    update_titlebar();
  }
  else if (N > 0) {
    strlcpy(CURRENT_DIR, dir_part, sizeof(CURRENT_DIR));

    int l, c __attribute__((unused));
    getmaxyx(WLFT, l, c);

    dflt.pos       = 0;
    dflt.start_pos = 0;
    dflt.end_pos   = (N > l) ? (l-1) : (N-1);
    dflt.files_n   = N;
    dflt.order     = 'n';

    STATE = fix_directory_state(CURRENT_DIR, dflt);

    sort_dir(STATE->order);

    if (file_part[0] != '\0') {
      // goto file
      for (size_t i = 0; i < STATE->files_n; i++)
        if (strcmp(file_part, ENTRIES[i].name) == 0) {
          move_pos_to(i);
          break;
        }
    }

    display_update_top();
    display_update_lft();
    display_update_bot();
    display_update_rgt(true);

    update_titlebar();
  }
}


void action_goto_path(const char* path) {
  char dir_part[FILENAME_MAX+1];
  char file_part[MAXNAMLEN+1];

  int res = path_split_parts(dir_part, file_part, path);

  if (res < 0) {
    werase(WLFT);

    wattron(WLFT, COLOR_PAIR(PAIR_RED_BLACK) | A_BOLD);
    waddstr(WLFT, "  [Invalid Path]");
    wattroff(WLFT, COLOR_PAIR(PAIR_RED_BLACK) | A_BOLD);

    wrefresh(WLFT);

    werase(WRGT);
    wrefresh(WRGT);

    werase(WBOT);
    wrefresh(WBOT);

    return;
  }

  action_goto(dir_part, file_part);
}


void action_up(bool update_preview) {
  if (STATE->pos == 0) return;

  if (STATE->start_pos > 0 && STATE->pos <= STATE->start_pos+4) {
    STATE->start_pos--;
    STATE->end_pos--;
  }
  STATE->pos--;

  display_update_lft();
  display_update_bot();
  display_update_rgt(update_preview);
}


void action_down(bool update_preview) {
  if (STATE->pos == STATE->files_n-1) return;

  if (STATE->end_pos < STATE->files_n-1 && STATE->pos >= STATE->end_pos-4) {
    STATE->start_pos++;
    STATE->end_pos++;
  }
  STATE->pos++;

  display_update_lft();
  display_update_bot();
  display_update_rgt(update_preview);
}


void action_page_up(bool update_preview) {
  int l, c __attribute__((unused));

  getmaxyx(WLFT, l, c);
  move_pos_by(-(l/2));

  display_update_lft();
  display_update_bot();
  display_update_rgt(update_preview);
}


void action_page_down(bool update_preview) {
  int l, c __attribute__((unused));

  getmaxyx(WLFT, l, c);
  move_pos_by(l/2);

  display_update_lft();
  display_update_bot();
  display_update_rgt(update_preview);
}


void action_home(void) {
  move_pos_to(0);

  display_update_lft();
  display_update_bot();
  display_update_rgt(true);
}


void action_end(void) {
  move_pos_to(STATE->files_n-1);

  display_update_lft();
  display_update_bot();
  display_update_rgt(true);
}


void action_forward(void) {
  const Entry* current = &ENTRIES[STATE->pos];
  char path[FILENAME_MAX+1];

  if (S_ISDIR(current->info.st_mode)) {
    // change directory
    if (strlen(CURRENT_DIR) == 1)
      snprintf(path, sizeof(path), "%s%s", CURRENT_DIR, current->name);
    else
      snprintf(path, sizeof(path), "%s/%s", CURRENT_DIR, current->name);

    action_goto_path(path);
  }
  else if (S_ISREG(current->info.st_mode) && current->info.st_mode & S_IRUSR) {
    // open file
    int res = path_get_full(path, current, true);

    if (res == 0) {
      char cmd[FILENAME_MAX+64];
      snprintf(cmd, sizeof(cmd), "2>/dev/null 1>&2 xdg-open '%s' &", path);

      if (system(cmd) != 0) display_error("cannot open file");
    }
  }
}


void action_backward(void) {
  char path[FILENAME_MAX+1];
  char* rest;
  strlcpy(path, CURRENT_DIR, sizeof(path));

  for (size_t i = strlen(path); i > 0; i--) {
    if (path[i-1] == '/') {
      path[i == 1 ? 1 : i-1] = '\0';
      rest = &path[i == 1 ? 2 : i];
      action_goto(path, rest);
      break;
    }
  }
}


void action_reorder(const char order) {
  char current_file_name[MAXNAMLEN+1];

  if (order == STATE->order) return;

  strlcpy(current_file_name, ENTRIES[STATE->pos].name, sizeof(current_file_name));

  sort_dir(order);

  STATE->order = order;

  for (size_t i = 0; i < STATE->files_n; i++)
    if (strcmp(current_file_name, ENTRIES[i].name) == 0) {
      move_pos_to(i);
      break;
    }

  display_update_lft();
  display_update_bot();
  display_update_rgt(true);
}


void action_select(void){
  if (STATE->files_n == 0) return;

  const Entry* current = &ENTRIES[STATE->pos];
  BTKey k = btree_cantor(current->info.st_dev, current->info.st_ino);

  if (!btree_has_key(SELECTION, k)) {
    // select
    char* path = (char*) malloc(sizeof(char)*(FILENAME_MAX+1));
    int res = path_get_full(path, current, false);

    if (res == 0)
      btree_set(SELECTION, k, path);
    else
      free(path);
  }
  else {
    // deselect
    char* path = (char*) btree_get(SELECTION, k);
    btree_set(SELECTION, k, NULL);
    free(path);
  }

  action_down(true);
  display_update_lft();
}


void action_show_info(void) {
  if (STATE->files_n == 0) return;

  preview_clear(PREVIEW, WRGT);
  preview_file_info(WRGT, &ENTRIES[STATE->pos]);
  wrefresh(WRGT);
}


void action_open_shell(void) {
  selection_save();

  char cmd[128] = "";
  snprintf(cmd, sizeof(cmd), "echo 'selected files are at ~/.raider-sel-%i'; %s", getpid(), getenv("SHELL"));

  suspend_exec_resume(CURRENT_DIR, cmd, "cannot open shell here");

  selection_purge();
}


void action_open_editor(void) {
  if (!CONFIG->has_emacsclient && !CONFIG->has_vim) return;

  const Entry* current = &ENTRIES[STATE->pos];

  if (S_ISDIR(current->info.st_mode)) {
    if (CONFIG->has_emacsclient)
      suspend_exec_resume(CURRENT_DIR, "emacsclient -nw .", "cannot open editor here");
    else if (CONFIG->has_vim)
      suspend_exec_resume(CURRENT_DIR, "vim .", "cannot open editor here");
  }
  else if (S_ISREG(current->info.st_mode) && current->info.st_mode & S_IRUSR && current->type == text) {
    char path[MAXNAMLEN+1];
    escape_quote(sizeof(path), path, current->name);

    char cmd[MAXNAMLEN+128] = "";

    if (CONFIG->has_emacsclient)
      snprintf(cmd, sizeof(cmd), "emacsclient -nw '%s'", path);
    else if (CONFIG->has_vim)
      snprintf(cmd, sizeof(cmd), "vim '%s'", path);

    suspend_exec_resume(CURRENT_DIR, cmd, "cannot open editor here");
  }
}


void action_fzf_search(void) {
  if (!CONFIG->has_fzf || (!CONFIG->has_fd && !CONFIG->has_find)) return;

  char cmd[32];

  if (CONFIG->has_fd) strlcpy(cmd, "fd . | fzf", sizeof(cmd));
  else strlcpy(cmd, "find . | fzf", sizeof(cmd));

  endwin();

  FILE* p;
  char path[FILENAME_MAX+1];
  char full_path[FILENAME_MAX+1];
  if (chdir(CURRENT_DIR) == 0 && (p = popen(cmd, "r")) != NULL) {
    fgetline(sizeof(path), path, p);
    pclose(p);

#pragma GCC diagnostic push
#ifndef __clang__
#pragma GCC diagnostic ignored "-Wformat-truncation"
#endif
    if (strlen(CURRENT_DIR) == 1)
      snprintf(full_path, sizeof(full_path), "%s%s", CURRENT_DIR, path);
    else
      snprintf(full_path, sizeof(full_path), "%s/%s", CURRENT_DIR, path);
#pragma GCC diagnostic pop

    init_curses();

    action_goto_path(full_path);
  }
  else init_curses();
}
