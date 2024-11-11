#include "btree.h"
#include "raider.h"
#include "utils.h"

#include <grp.h>
#include <ncurses.h>
#include <pwd.h>
#include <string.h>
#include <sys/stat.h>


static
int get_entry_attrs(const Entry* entry) {
  if (btree_has_key(SELECTION, btree_cantor(entry->info.st_dev, entry->info.st_ino)))
    return COLOR_PAIR(PAIR_BLACK_YELLOW | A_BOLD);

  if (entry->is_link)
    return COLOR_PAIR(PAIR_GREEN_BLACK) | A_UNDERLINE;

  if (S_ISDIR(entry->info.st_mode))
    return COLOR_PAIR(PAIR_BLUE_BLACK);

  if (S_ISREG(entry->info.st_mode) && entry->info.st_mode & S_IXUSR)
    return COLOR_PAIR(PAIR_GREEN_BLACK) | A_BOLD;

  if (S_ISREG(entry->info.st_mode)) {
    switch (entry->type) {
    case file_type_num:
      return COLOR_PAIR(PAIR_DEFAULT);

    case unknown:
      return COLOR_PAIR(PAIR_DEFAULT);

    case text:
      return COLOR_PAIR(PAIR_DEFAULT);

    case document:
      return COLOR_PAIR(PAIR_CYAN_BLACK);

    case image:
      return COLOR_PAIR(PAIR_MAGENTA_BLACK);

    case video:
      return COLOR_PAIR(PAIR_MAGENTA_BLACK);

    case archive:
      return COLOR_PAIR(PAIR_RED_BLACK);
    }
  }

  return COLOR_PAIR(PAIR_DEFAULT);
}


void display_update_top(void) {
  wattron(WTOP, COLOR_PAIR(PAIR_GREEN_BLACK) | A_BOLD);
  mvwprintw(WTOP, 0, 0, "%s@%s: ", USER, HOST);
  wattroff(WTOP, COLOR_PAIR(PAIR_GREEN_BLACK) | A_BOLD);

  wattron(WTOP, COLOR_PAIR(PAIR_BLUE_BLACK) | A_BOLD);
  mvwaddstr(WTOP, 0, strlen(USER) + strlen(HOST) + 3, CURRENT_DIR);
  wattroff(WTOP, COLOR_PAIR(PAIR_BLUE_BLACK) | A_BOLD);

  wclrtoeol(WTOP);
  wrefresh(WTOP);
}


void display_update_bot(void) {
  char mode[11] = "----------";
  char size[32] = "";
  char ctime[32] = "";

  struct passwd* pws;
  struct group* grp;

  const Entry* current = &ENTRIES[STATE->pos];

  pws = getpwuid(current->info.st_uid);
  grp = getgrgid(current->info.st_gid);

  get_mode_line(mode, current);
  get_size_line(sizeof(size), size, current);
  get_time_line(sizeof(ctime), ctime, current->info.st_ctim.tv_sec);

  wattron(WBOT, COLOR_PAIR(PAIR_YELLOW_BLACK) | A_DIM);
  mvwaddstr(WBOT, 0, 0, mode);
  wattroff(WBOT, COLOR_PAIR(PAIR_YELLOW_BLACK) | A_DIM);

  wattron(WBOT, COLOR_PAIR(PAIR_DEFAULT) | A_DIM);
  mvwprintw(WBOT, 0, 11, "%s %s %s %s [%zu/%zu] (%c)", pws->pw_name, grp->gr_name, size, ctime, STATE->pos+1, STATE->files_n, STATE->order);
  wattroff(WBOT, COLOR_PAIR(PAIR_DEFAULT) | A_DIM);

  wclrtoeol(WBOT);
  wrefresh(WBOT);
}


void display_update_lft(void) {
  int lines, cols;

  getmaxyx(WLFT, lines, cols);

  werase(WLFT);

  for (size_t l = 0, i = STATE->start_pos; i <= STATE->end_pos && l < (size_t) lines; i++, l++) {

    if (l == STATE->pos - STATE->start_pos) {
      wattron(WLFT, COLOR_PAIR(PAIR_RED_BLACK) | A_BOLD);
      mvwaddch(WLFT, l, 0, '>');
      wattroff(WLFT, COLOR_PAIR(PAIR_RED_BLACK) | A_BOLD);
    }

    int attr = get_entry_attrs(&ENTRIES[i]);

    wattron(WLFT, attr);
    mvwaddnstr(WLFT, l, 2, ENTRIES[i].name, cols-3);
    wattroff(WLFT, attr);
  }

  wrefresh(WLFT);
}


void display_update_rgt(bool update_preview) {
  if (STATE->files_n == 0) return;

  Entry* current = &ENTRIES[STATE->pos];

  preview_clear(PREVIEW, WRGT);

  // preview directory
  if (update_preview && S_ISDIR(current->info.st_mode))
    preview_directory(PREVIEW, WRGT, current);

  else if (update_preview && S_ISREG(current->info.st_mode) && current->info.st_mode & S_IRUSR) {
    if (current->type == unknown) {
      // if type is unknown get it from mime type
      char mime[64] = "";
      get_mime_type(sizeof(mime), mime, current);

      if (starts_with(mime, "text/"))
        current->type = text;
    }

    preview_file(PREVIEW, WRGT, current);
  }
  else preview_file_info(WRGT, current);

  wrefresh(WRGT);
}
