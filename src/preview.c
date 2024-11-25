#include "raider.h"
#include "utils.h"

#include <grp.h>
#include <ncurses.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef __linux__
#include <sys/sysmacros.h>
#endif

const char FILE_TYPES[file_type_num][10] = {"unknown", "text", "document", "image", "video", "archive"};

const char W3MIMAGEDISPLAY_LOCATIONS[5][64] = {"/usr/lib/w3m/w3mimgdisplay", "/usr/libexec/w3m/w3mimgdisplay", "/usr/lib64/w3m/w3mimgdisplay", "/usr/libexec64/w3m/w3mimgdisplay", "/usr/local/libexec/w3m/w3mimgdisplay"};

bool PREVIEW_NEEDS_CLEARING = false;
char WAIT_CACHE[PATH_MAX] = "";


static
void display_not_found_msg(WINDOW* win) {
  wattron(win, COLOR_PAIR(PAIR_RED_BLACK) | A_BOLD);
  waddstr(win, "  [Not Found]");
  wattroff(win, COLOR_PAIR(PAIR_RED_BLACK) | A_BOLD);

  wrefresh(win);
}


static
int display_command_output(WINDOW* win, const char* cmd, bool raw) {
  int lines, cols, x, y;

  getbegyx(win, y, x);
  getmaxyx(win, lines, cols);

  FILE* p;
  char buf[4096] = "";
  if ((p = popen(cmd, "r"))) {
    int n = 0;
    while((fgetline(4096, buf, p) && n < lines)) {
      if (raw) {
        printf("%c[%i;%if", '\033', y+n+1, x+1); // move cursor
        printf("%s\n", buf);
        fflush(stdout);
      }
      else mvwaddnstr(win, n, 1, buf, cols-1);
      n++;
    }
    pclose(p);

    // reposition cursor to beginning
    if (raw) {
      printf("%c[%i;%if", '\033', y+1, x+1);
      fflush(stdout);
    }

    return 0;
  }
  return -1;
}


void preview_clear_x11(const void* preview, WINDOW* win) {
  werase(win);
  wrefresh(win);

  if (!PREVIEW_NEEDS_CLEARING) return;

  int x = ((const Preview*) preview)->x_width/2;
  int y = 0;
  int max_w = ((const Preview*) preview)->x_width/2;
  int max_h = ((const Preview*) preview)->x_height;

  char cmd[PATH_MAX+64];
  snprintf(cmd, sizeof(cmd),
           "echo -e '6;%i;%i;%i;%i;\n4;\n3;\n' | %s 2>&1 > /dev/null",
           x, y, max_w, max_h, ((const Preview*) preview)->w3mimgdisplay_path);

  if (system(cmd) == 0) PREVIEW_NEEDS_CLEARING = false;
}


void preview_clear_raw(const void* preview __attribute__((unused)), WINDOW* win) {
  werase(win);
  wrefresh(win);

  if (!PREVIEW_NEEDS_CLEARING) return;

  int lines, cols, x, y;

  getbegyx(WRGT, y, x);
  getmaxyx(WRGT, lines, cols);

  for (int i=0; i<lines; i++) {
    printf("%c[%i;%if", '\033', y+i+1, x+1); // position cursor
    for (int j=0; j<cols; j++)
      printf("%c", ' ');

    printf("\n");
  }

  // reposition cursor to beginning
  printf("%c[%i;%if", '\033', y+1, x+1);
  fflush(stdout);

  PREVIEW_NEEDS_CLEARING = false;
}


void preview_clear_none(const void* preview __attribute__((unused)), WINDOW* win) {
  werase(win);
  wrefresh(win);

  PREVIEW_NEEDS_CLEARING = false;
}


void preview_display_x11(const void* preview, WINDOW* win, const char* path) {
  int lines, cols __attribute__((unused));
  getmaxyx(win, lines, cols);

  int x = 100 + ((const Preview*) preview)->x_width/2;
  int y = ((const Preview*) preview)->x_height/lines;
  int max_w = ((const Preview*) preview)->x_width/2 - 200;
  int max_h = ((const Preview*) preview)->x_height - y;

  char esc_path[PATH_MAX];
  escape_quote(sizeof(esc_path), esc_path, path);

  char cmd[2*PATH_MAX+256];
  snprintf(cmd, sizeof(cmd), "echo -e '5;%s' | %s", esc_path, ((const Preview*) preview)->w3mimgdisplay_path);

  FILE* p;
  if ((p = popen(cmd, "r"))) {
    int sw, sh;
    int m = fscanf(p, "%i %i", &sw, &sh);
    pclose(p);

    if (m != 2) return;

    int w = sw;
    int h = sh;

    if (w > max_w) {
      h = h * max_w / w;
      w = max_w;
    }

    if (h > max_h) {
      w = w * max_h / h;
      h = max_h;
    }

    snprintf(cmd, sizeof(cmd),
             "echo -e '6;%i;%i;%i;%i;\n0;1;%i;%i;%i;%i;;;%i;%i;%s\n4;\n3;\n' | %s 2>&1 > /dev/null",
             x, y, max_w, max_h, x, y, w, h , sw, sh, esc_path, ((const Preview*) preview)->w3mimgdisplay_path);

    if (system(cmd) == 0) PREVIEW_NEEDS_CLEARING = true;
  }
}


void preview_display_chafa(const void* preview __attribute__((unused)), WINDOW* win, const char* path) {
  int lines, cols;
  getmaxyx(win, lines, cols);

  char esc_path[PATH_MAX];
  escape_quote(sizeof(esc_path), esc_path, path);

  char cmd[PATH_MAX+64];
  snprintf(cmd, sizeof(cmd), "2>/dev/null chafa --view-size %ix%i '%s'", cols/2, lines/2, esc_path);

  display_command_output(win, cmd, true);

  PREVIEW_NEEDS_CLEARING = true;
}


void preview_display_sixel(const void* preview __attribute__((unused)), WINDOW* win, const char* path) {
  int x, y;
  getbegyx(win, y, x);

  printf("%c[%i;%if", '\033', y+1, x+1); // move cursor

  FILE* f;
  char buf[256] = "";
  if ((f = fopen(path, "r"))) {
    while((fgets(buf, 256, f)))
      printf("%s", buf);

    fclose(f);
  }

  // reposition cursor to beginning
  printf("%c[%i;%if", '\033', y+1, x+1);
  fflush(stdout);

  PREVIEW_NEEDS_CLEARING = true;
}


void preview_text_file(const void* preview, WINDOW* win, const Entry* entry) {
  int lines, cols;
  getmaxyx(win, lines, cols);

  char path[PATH_MAX];
  int res = path_get_full(path, entry, false);

  if (res < 0) {
    ((const Preview*) preview)->preview_clear(preview, win);
    display_not_found_msg(win);
    return;
  }

  FILE* f;
  char buf[4096];
  if ((f = fopen(path, "r"))) {
    int n = 0;
    while((fgetline(sizeof(buf), buf, f) && n < lines)) {
      mvwaddnstr(win, n, 1, buf, cols-1);
      n++;
    }
    fclose(f);
  }
}


void previewer_info(const void* preview __attribute__((unused)), WINDOW* win, const Entry* entry) {
  preview_file_info(win, entry);
}


void previewer_image(const void* preview, WINDOW* win, const Entry* entry) {
  char path[PATH_MAX];
  int res = path_get_full(path, entry, false);

  ((Preview*) preview)->preview_clear(preview, win);

  if (res == 0)
    ((Preview*) preview)->preview_display(preview, win, path);
  else
    display_not_found_msg(win);
}


void previewer_video_text(const void* preview, WINDOW* win, const Entry* entry) {
  char path[PATH_MAX];
  int res = path_get_full(path, entry, true);

  if (res < 0) {
    preview_clear(preview, win);
    display_not_found_msg(win);
    return;
  }

  char opt[1024];
  snprintf(opt, sizeof(opt), "General;Title:     %%Movie%%\\nType:      %%ContentType%%\\nGenre:     %%Genre%%\\nPerformer: %%Performer%%\\n\\nFormat:    %%Format%%\\nSize:      %%FileSize/String%%\\nDuration:  %%Duration/String%%\\nBit Rate:  %%OverallBitRate/String%%\\n");

  char cmd[PATH_MAX+1024+64];
  snprintf(cmd, sizeof(cmd), "2>/dev/null mediainfo --Output='%s' '%s'", opt, path);

  if (display_command_output(win, cmd, false) != 0) preview_file_info(win, entry);
}


void previewer_document_text(const void* preview, WINDOW* win, const Entry* entry) {
  char path[PATH_MAX];
  int res = path_get_full(path, entry, true);

  if (res < 0) {
    preview_clear(preview, win);
    display_not_found_msg(win);
    return;
  }

  if (strcmp(entry->ext, "pdf") == 0 &&  ((const Preview*) preview)->has_pdftotext) {
    char cmd[PATH_MAX+64];
    snprintf(cmd, sizeof(cmd), "2>/dev/null pdftotext -f 0 -l 0  '%s' -", path);

    if (display_command_output(win, cmd, false) != 0) preview_file_info(win, entry);
  }
  else if (strcmp(entry->ext, "djvu") == 0 && ((const Preview*) preview)->has_djvutxt) {
    char cmd[PATH_MAX+64];
    snprintf(cmd, sizeof(cmd), "2>/dev/null djvutxt --page=0 '%s'", path);

    if (display_command_output(win, cmd, false) != 0) preview_file_info(win, entry);
  }
  else preview_file_info(win, entry);
}


void thumbnailer_video(const void* preview __attribute__((unused)), const char* path, const char* cache_path) {
  char esc_path[PATH_MAX];
  escape_quote(sizeof(esc_path), esc_path, path);

  char cmd[2*PATH_MAX+128];
  snprintf(cmd, sizeof(cmd),
           "(2>/dev/null 1>&2 ffmpegthumbnailer -i '%s' -s 0 -q 2 -o %s && kill -HUP %i) &",
           esc_path, cache_path, getpid());

  int r __attribute__((unused)) = system(cmd);
}


void thumbnailer_document(const void* preview __attribute__((unused)), const char* path, const char* cache_path) {
  char esc_path[PATH_MAX];
  escape_quote(sizeof(esc_path), esc_path, path);

  char cmd[2*PATH_MAX+128];
  snprintf(cmd, sizeof(cmd),
           "(2>/dev/null 1>&2 convert -density 120 '%s[0]' -quality 80 %s && kill -HUP %i) &",
           esc_path, cache_path, getpid());

  int r __attribute__((unused)) = system(cmd);
}


void thumbnailer_image_sixel(const void* preview __attribute__((unused)), const char* path, const char* cache_path) {
  char esc_path[PATH_MAX];
  escape_quote(sizeof(esc_path), esc_path, path);

  char cmd[2*PATH_MAX+128];
  snprintf(cmd, sizeof(cmd),
           "(2>/dev/null img2sixel '%s' -q low -w 400 -o %s && kill -HUP %i) &",
           esc_path, cache_path, getpid());

  int r __attribute__((unused)) = system(cmd);
}


void thumbnailer_video_sixel(const void* preview __attribute__((unused)), const char* path, const char* cache_path) {
  char esc_path[PATH_MAX];
  escape_quote(sizeof(esc_path), esc_path, path);

  char cmd[4*PATH_MAX+128];
  snprintf(cmd, sizeof(cmd),
           "(2>/dev/null 1>&2 ffmpegthumbnailer -i '%s' -s 0 -q 2 -o %s.jpg && 2>/dev/null img2sixel %s.jpg -q low -w 400 -o %s && kill -HUP %i) &",
           esc_path, cache_path, cache_path, cache_path, getpid());

  int r __attribute__((unused)) = system(cmd);
}


void thumbnailer_document_sixel(const void* preview __attribute__((unused)), const char* path, const char* cache_path) {
  char esc_path[PATH_MAX];
  escape_quote(sizeof(esc_path), esc_path, path);

  char cmd[4*PATH_MAX+128];
  snprintf(cmd, sizeof(cmd),
           "(2>/dev/null 1>&2 convert -density 120 '%s[0]' -quality 80 %s.jpg && 2>/dev/null img2sixel %s.jpg -q low -w 400 -o %s && kill -HUP %i) &",
           esc_path, cache_path, cache_path, cache_path, getpid());

  int r __attribute__((unused)) = system(cmd);
}


void preview_init(Preview* preview) {
  // create cache directory
  char buf[PATH_MAX];
  snprintf(buf, sizeof(buf), "%s/.cache/raider", getenv("HOME"));
  mkdir(buf, S_IRWXU|S_IXUSR);

  // get X11
  preview->has_x11 = false;
  preview->x_width = preview->x_height = 0;

  preview_get_xwin_size(preview);

  // check installed sw
  preview->has_chafa = system("which chafa 1>/dev/null 2>/dev/null") == 0;
  preview->has_convert = system("which convert 1>/dev/null 2>/dev/null") == 0;
  preview->has_djvutxt = system("which djvutxt 1>/dev/null 2>/dev/null") == 0;
  preview->has_img2sixel = system("which img2sixel 1>/dev/null 2>/dev/null") == 0;
  preview->has_pdftotext = system("which pdftotext 1>/dev/null 2>/dev/null") == 0;
  preview->has_mediainfo = system("which mediainfo 1>/dev/null 2>/dev/null") == 0;
  preview->has_thumbnailer = system("which ffmpegthumbnailer 1>/dev/null 2>/dev/null") == 0;

  preview->has_w3mimgdisplay = false;

  for (int i=0; i<5; i++)
    if (path_exists(W3MIMAGEDISPLAY_LOCATIONS[i])) {
      preview->has_w3mimgdisplay = true;
      strlcpy(preview->w3mimgdisplay_path, W3MIMAGEDISPLAY_LOCATIONS[i], sizeof(preview->w3mimgdisplay_path));
    }

  // initialize default previewers and thumbnailers
  for (size_t i = 0; i < file_type_num; i++) {
    preview->previewer[i] = NULL;
    preview->thumbnailer[i] = NULL;
  }

  preview->previewer[text] = preview_text_file;

  if (preview->has_pdftotext || preview->has_djvutxt)
    preview->previewer[document] = previewer_document_text;

  if (preview->has_mediainfo)
    preview->previewer[video] = previewer_video_text;
}


void preview_get_modes(const Preview* preview, size_t modesz, char modes[modesz]) {
  modes[0] = '\0';

  if (preview->has_x11 && preview->has_w3mimgdisplay)
    strlcat(modes, " x11", modesz);

  if (preview->has_img2sixel)
    strlcat(modes, " sixel", modesz);

  if (preview->has_chafa)
    strlcat(modes, " chafa", modesz);

  strlcat(modes, " none", modesz);
}


int preview_set_mode(Preview* preview, const char* mode) {
  if (strcmp(mode, "x11") == 0) {
    if (!preview->has_x11 || !preview->has_w3mimgdisplay) {
      fprintf(stderr, "cannot use x11 preview unless x11 is available and w3mimgdisplay is installed\n");
      return -1;
    }

    preview->preview_clear = preview_clear_x11;
    preview->preview_display = preview_display_x11;

    preview->previewer[image] = previewer_image;

    if (preview->has_convert)     preview->thumbnailer[document] = thumbnailer_document;
    if (preview->has_thumbnailer) preview->thumbnailer[video]    = thumbnailer_video;

    preview->mode = x11;
  }
  else if (strcmp(mode, "sixel") == 0) {
    if (!preview->has_img2sixel) {
      fprintf(stderr, "cannot use sixel preview unless img2sixel is installed\n");
      return -1;
    }

    preview->preview_clear = preview_clear_raw;
    preview->preview_display = preview_display_sixel;

    preview->thumbnailer[image] = thumbnailer_image_sixel;

    if (preview->has_convert)     preview->thumbnailer[document] = thumbnailer_document_sixel;
    if (preview->has_thumbnailer) preview->thumbnailer[video]    = thumbnailer_video_sixel;

    preview->mode = sixel;
  }
  else if (strcmp(mode, "chafa") == 0) {
    if (!preview->has_chafa) {
      fprintf(stderr, "cannot use chafa preview unless chafa is installed\n");
      return -1;
    }

    preview->preview_clear = preview_clear_raw;
    preview->preview_display = preview_display_chafa;

    preview->previewer[image] = previewer_image;

    if (preview->has_convert)     preview->thumbnailer[document] = thumbnailer_document;
    if (preview->has_thumbnailer) preview->thumbnailer[video]    = thumbnailer_video;

    preview->mode = chafa;
  }
  else if (strcmp(mode, "none") == 0) {
    preview->preview_clear = preview_clear_none;

    preview->mode = none;
  }
  else {
    fprintf(stderr, "invalid preview mode: %s\n", mode);
    return -1;
  }
  return 0;
}


void preview_clear(const Preview* preview, WINDOW* win) {
  preview->preview_clear(preview, win);
}


void preview_refresh(const Preview* preview, WINDOW* win) {
  if (WAIT_CACHE[0] != '\0' && path_exists(WAIT_CACHE)) {
    preview->preview_clear(preview, win);

    preview->preview_display(preview, win, WAIT_CACHE);
  }
}


void preview_file(const Preview* preview, WINDOW* win, const Entry* entry) {
  if (preview->thumbnailer[entry->type] != NULL) {
    preview_file_info(win, entry);

    char cache_path[PATH_MAX];
    snprintf(cache_path,
             sizeof(cache_path),
             "%s/.cache/raider/%i-%llu.%s",
             getenv("HOME"),
             (int) entry->info.st_dev,
             (unsigned long long int) entry->info.st_ino,
             preview->mode == sixel ? "six" : "jpg");

    if (path_exists(cache_path)) {
      preview->preview_clear(preview, win);

      preview->preview_display(preview, win, cache_path);
    }
    else {
      strlcpy(WAIT_CACHE, cache_path, sizeof(WAIT_CACHE));

      char path[PATH_MAX];
      int res = path_get_full(path, entry, false);

      if (res == 0)
        preview->thumbnailer[entry->type](preview, path, cache_path);
      else {
        preview_clear(preview, win);
        display_not_found_msg(win);
      }
    }
  }
  else if (preview->previewer[entry->type] != NULL)
    preview->previewer[entry->type](preview, win, entry);
  else
    preview_file_info(win, entry);
}


void preview_directory(const Preview* preview, WINDOW* win, const Entry* dir_entry) {
  char dir_path[PATH_MAX];
  int res = path_get_full(dir_path, dir_entry, false);

  if (res < 0) {
    preview_clear(preview, win);
    display_not_found_msg(win);
    return;
  }

  DIR* dir = opendir(dir_path);

  if (dir == NULL) {
    preview_clear(preview, win);
    display_not_found_msg(win);
    return;
  }

  int lines, cols;
  getmaxyx(win, lines, cols);

  int n = 0;
  struct dirent* entry;
  while ((entry = readdir(dir)) != NULL && n < lines) {
    if (entry->d_name[0] == '.') continue;
    mvwaddnstr(WRGT, n, 1, entry->d_name, cols-1);
    n++;
  }

  closedir(dir);
}


void preview_file_info(WINDOW* win, const Entry* entry) {
  char mode[11] = "----------";
  char size[64] = "";

  char ctime[64] = "";
  char mtime[64] = "";
  char atime[64] = "";

  struct passwd* pws;
  struct group* grp;

  pws = getpwuid(entry->info.st_uid);
  grp = getgrgid(entry->info.st_gid);

  get_mode_line(mode, entry);
  get_size_line(sizeof(size), size, entry);
  get_time_line(sizeof(ctime), ctime, entry->info.st_ctim.tv_sec);
  get_time_line(sizeof(mtime), mtime, entry->info.st_mtim.tv_sec);
  get_time_line(sizeof(atime), atime, entry->info.st_atim.tv_sec);

  if (entry->is_link) {
    mvwprintw(win, 0, 1, "  Link: %s", entry->name);

    char file_path[PATH_MAX];
    char link_path[PATH_MAX];
    if (path_get_full(file_path, entry, false) == 0) {
      size_t l = readlink(file_path, link_path, sizeof(link_path));
      link_path[l] = '\0';
      mvwprintw(win, 1, 1, "        -> %s", link_path);
    }
  }
  else if (S_ISDIR(entry->info.st_mode))
    mvwprintw(win, 0, 1, "   Dir: %s", entry->name);
  else if (S_ISREG(entry->info.st_mode))
    mvwprintw(win, 0, 1, "  File: %s", entry->name);

  mvwprintw(win, 2, 1,  "  Size: %s", size);
  mvwprintw(win, 2, 22, "FileType: %s (%s)", FILE_TYPES[entry->type], entry->ext);

  mvwprintw(win, 3, 1,  "  Mode: (%s)", mode);
  mvwprintw(win, 3, 22, "Uid: (%i/%s) Gid: (%i/%s)", entry->info.st_uid, pws->pw_name, entry->info.st_gid, grp->gr_name);

  mvwprintw(win, 4, 1,  "Device: %i,%i", major(entry->info.st_dev), minor(entry->info.st_dev));
  mvwprintw(win, 4, 22, "Inode: %lli", (unsigned long long int) entry->info.st_ino);

  mvwprintw(win, 5, 1,  "Access: %s", atime);

  mvwprintw(win, 6, 1,  "Modify: %s", mtime);

  mvwprintw(win, 7, 1,  "Change: %s", ctime);
}
