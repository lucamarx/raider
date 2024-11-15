#include "raider.h"
#include "utils.h"

#include <locale.h>
#include <ncurses.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

WINDOW*  WTOP = NULL;
WINDOW*  WBOT = NULL;
WINDOW*  WLFT = NULL;
WINDOW*  WRGT = NULL;

State*   STATE     = NULL;
Entry*   ENTRIES   = NULL;
Config*  CONFIG    = NULL;
Preview* PREVIEW   = NULL;
BTNode*  HISTORY   = NULL;
BTNode*  SELECTION = NULL;

char     HOST[256] = "";
char     USER[256] = "";
char     CURRENT_DIR[FILENAME_MAX+1] = "";


void init_curses(void) {
  initscr();
  curs_set(0);

  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  nodelay(stdscr, TRUE);
  timeout(100);

  start_color();

  init_pair(PAIR_DEFAULT,       COLOR_WHITE,   COLOR_BLACK);
  init_pair(PAIR_RED_BLACK,     COLOR_RED,     COLOR_BLACK);
  init_pair(PAIR_BLUE_BLACK,    COLOR_BLUE,    COLOR_BLACK);
  init_pair(PAIR_CYAN_BLACK,    COLOR_CYAN,    COLOR_BLACK);
  init_pair(PAIR_GREEN_BLACK,   COLOR_GREEN,   COLOR_BLACK);
  init_pair(PAIR_YELLOW_BLACK,  COLOR_YELLOW,  COLOR_BLACK);
  init_pair(PAIR_BLACK_YELLOW,  COLOR_BLACK,   COLOR_YELLOW);
  init_pair(PAIR_MAGENTA_BLACK, COLOR_MAGENTA, COLOR_BLACK);

  getmaxyx(stdscr, LINES, COLS); // is it necessary?

  refresh(); // this is necessary to prevent getch to clear the screen

  // setup windows
  WTOP = newwin(1, COLS, 0, 0);
  WBOT = newwin(1, COLS, LINES-1, 0);

  WLFT = newwin(LINES-2, COLS/2, 1, 0);
  WRGT = newwin(LINES-2, COLS/2, 1, COLS/2);
}


void config_init(Config* config) {
  config->has_fd = system("which fd 1>/dev/null 2>/dev/null") == 0;
  config->has_fzf = system("which fzf 1>/dev/null 2>/dev/null") == 0;
  config->has_vim = system("which vim 1>/dev/null 2>/dev/null") == 0;
  config->has_find = system("which find 1>/dev/null 2>/dev/null") == 0;
  config->has_emacsclient = system("which emacsclient 1>/dev/null 2>/dev/null") == 0;
}


void help() {
  char modes[32];
  preview_get_modes(PREVIEW, sizeof(modes), modes);

  printf("usage: raider [-h] [-v] [-p preview mode] [-s file]\n");
  printf("       where preview mode is one of:%s\n", modes);
}

void done(void) {
  endwin();

  selection_remove_file();

  if (SELECTION != NULL) btree_free(SELECTION);
  if (HISTORY != NULL) btree_free(HISTORY);
  if (ENTRIES != NULL) free(ENTRIES);
  if (PREVIEW != NULL) free(PREVIEW);
  if (CONFIG != NULL) free(CONFIG);
}


void sigint_handler(int signum __attribute__((unused))) {
  done();
  exit(EXIT_SUCCESS);
}


void sighup_handler(int signum __attribute__((unused))) {
  preview_refresh(PREVIEW, WRGT);
}


int main(int argc, char* argv[]) {
  // https://stackoverflow.com/questions/4703168/adding-unicode-utf8-chars-to-a-ncurses-display-in-c
  setlocale(LC_ALL, "");

  char preview_mode[8] = "none";
  char start_path[FILENAME_MAX+1] = "";

  PREVIEW = (Preview*) malloc(sizeof(Preview));
  preview_init(PREVIEW);

  int opt;
  while ((opt = getopt(argc, argv, "hvp:s:")) != -1) {
    if (opt == 'h') {
      help();
      exit(EXIT_SUCCESS);
    }
    else if (opt == 'v') {
      printf("raider version %s\n", RAIDER_VERSION);
      exit(EXIT_SUCCESS);
    }
    else if (opt == 'p') strlcpy(preview_mode, optarg, sizeof(preview_mode));
    else if (opt == 's') strlcpy(start_path, optarg, sizeof(start_path));
    else {
      help();
      exit(EXIT_FAILURE);
    }
  }

  if (preview_set_mode(PREVIEW, preview_mode) < 0) {
    free(PREVIEW);
    exit(EXIT_FAILURE);
  }

  CONFIG = (Config*) malloc(sizeof(Config));
  config_init(CONFIG);

  HISTORY = btree_new(0);
  SELECTION = btree_new(0);

  strlcpy(USER, getenv("USER"), sizeof(USER));
  gethostname(HOST, sizeof(HOST));

  char path[FILENAME_MAX+1];

  if (start_path[0] != '\0') {
    if (start_path[0] == '/')
      strlcpy(path, start_path, sizeof(path));
    else {
      char cwd[FILENAME_MAX];
      snprintf(path, sizeof(path), "%s/%s", getcwd(cwd, FILENAME_MAX+1), start_path);
    }
  }
  else getcwd(path, FILENAME_MAX);

  signal(SIGINT, sigint_handler);
  signal(SIGHUP, sighup_handler);

  init_curses();

  action_goto_path(path);

  event_loop();

  done();

  return EXIT_SUCCESS;
}
