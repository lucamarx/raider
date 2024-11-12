#include "utils.h"
#include "btree.h"
#include "raider.h"

#include <ctype.h>
#include <dirent.h>
#include <ncurses.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


bool starts_with(const char* str, const char* pat) {
  size_t l_str = strlen(str);
  size_t l_pat = strlen(pat);

  if (l_pat > l_str) return false;

  for (size_t i = 0; i < l_pat; i++)
    if (str[i] != pat[i]) return false;

  return true;
}


bool is_one_of(const char* pat, const char* strs) {
  char* r = strdup(strs);
  char* s = r;
  char* t;
  while ((t = strsep(&r, ","))) {
    if (strncmp(pat, t, strlen(pat)) == 0) {
      free(s);
      return true;
    }
  }
  free(s);
  return false;
}


char* fgetline(size_t size, char s[restrict size], FILE*restrict stream) {
  s[0] = 0;
  char* ret = fgets(s, size, stream);
  if (ret) {
    char* pos = strchr(s, '\n');
    if (pos) *pos = 0;
    else ret = 0;
  }
  return ret;
}


int path_split_parts(char* dir_part, char* file_part, const char* path) {
  // check that path is absolute
  if (strlen(path) == 0)
    return PATH_IS_EMPTY;

  if (path[0] != '/')
    return PATH_IS_NOT_ABSOLUTE;

  // check that it exists
  struct stat info;
  if (stat(path, &info) != 0)
    return PATH_DOES_NOT_EXISTS;

  // ad it is not a special file
  if (!S_ISDIR(info.st_mode) && !S_ISREG(info.st_mode))
    return PATH_IS_SPECIAL;

  if (S_ISDIR(info.st_mode)) {
    // path is a directory
    strlcpy(dir_part, path, FILENAME_MAX+1);

    // remove last /
    if (strlen(dir_part) > 1 && path[strlen(dir_part)-1] == '/')
      dir_part[strlen(dir_part)-1] = '\0';

    file_part[0] = '\0';
  }
  else if (S_ISREG(info.st_mode)) {
    // path is file
    strlcpy(dir_part, path, FILENAME_MAX+1);

    for (size_t i = strlen(dir_part); i > 0; i--) {
      if (dir_part[i-1] == '/') {
        strlcpy(file_part, dir_part+i, MAXNAMLEN+1);
        dir_part[i == 1 ? 1 : i-1] = '\0';
        break;
      }
    }
  }

  return 0;
}


int path_get_full(char* path, const Entry* file_entry, bool escape) {
  char buf[FILENAME_MAX+1];

  if (strlen(CURRENT_DIR) == 1)
    snprintf(buf, sizeof(buf), "/%s", file_entry->name);
  else
    snprintf(buf, sizeof(buf), "%s/%s", CURRENT_DIR, file_entry->name);

  bool exists = path_exists(buf);

  if (escape) escape_quote(FILENAME_MAX+1, path, buf);
  else strlcpy(path, buf, FILENAME_MAX+1);

  return exists ? 0 : PATH_DOES_NOT_EXISTS;
}


void get_mime_type(size_t mimesz, char mime[mimesz], const Entry* file_entry) {
  char path[FILENAME_MAX+1];
  int res = path_get_full(path, file_entry, true);

  mime[0] = '\0';

  if (res < 0) return;

  char cmd[FILENAME_MAX+64];
  snprintf(cmd, sizeof(cmd), "file -i -b '%s'", path);

  FILE* p;
  if ((p = popen(cmd, "r"))) {
    fgetline(mimesz, mime, p);
    pclose(p);
  }
}


void path_get_extension(size_t extsz, char ext[extsz], const char* file_name) {
  for (size_t i = strlen(file_name), j = 0; i > 0 && j <= extsz; i--, j++) {
    if (file_name[i-1] == '.') {
      strlcpy(ext, &file_name[i], extsz);
      ext[extsz+1] = '\0';

      for(int k = 0; ext[k]; k++)
        ext[k] = tolower(ext[k]);

      return;
    }
  }
  ext[0] = '\0';
}


bool path_exists(const char* path) {
  struct stat info;
  return stat(path, &info) == 0;
}


void escape_quote(size_t escsz, char esc[escsz], const char* buf) {
  // single quote escape: abc'def -> abc'\''def
  for (size_t i = 0, j = 0; i < strlen(buf); i++) {
    if (buf[i] == '\'') {
      if (j+3 >= escsz) break;
      esc[j] = '\'';
      j++;
      esc[j] = '\\';
      j++;
      esc[j] = '\'';
      j++;
      esc[j] = '\'';
    }
    else
      esc[j] = buf[i];

    j++;
    esc[j] = '\0';
  }
}


void get_size_line(size_t sizesz, char size[sizesz], const Entry* entry) {
  if (entry->info.st_size < 1e3) {
    snprintf(size, sizesz, "%lliB", (unsigned long long int) entry->info.st_size);
  }
  else if (entry->info.st_size < 1e6) {
    double sz = entry->info.st_size / 1000.0;
    snprintf(size, sizesz, "%.1fKB", sz);
  }
  else if (entry->info.st_size < 1e9) {
    double sz = entry->info.st_size / 1000000.0;
    snprintf(size, sizesz, "%.1fMB", sz);
  }
  else {
    double sz = entry->info.st_size / 1000000000.0;
    snprintf(size, sizesz, "%.1fGB", sz);
  }
}


void get_mode_line(char* mode, const Entry* entry) {
  if (entry->is_link)                     mode[0] = 'l';
  else if (S_ISREG(entry->info.st_mode))  mode[0] = '-';
  else if (S_ISDIR(entry->info.st_mode))  mode[0] = 'd';
  else if (S_ISCHR(entry->info.st_mode))  mode[0] = 'c';
  else if (S_ISBLK(entry->info.st_mode))  mode[0] = 'b';
  else if (S_ISFIFO(entry->info.st_mode)) mode[0] = 'f';

  if (entry->info.st_mode & S_IRUSR) mode[1] = 'r';
  if (entry->info.st_mode & S_IWUSR) mode[2] = 'w';
  if (entry->info.st_mode & S_IXUSR) mode[3] = 'x';

  if (entry->info.st_mode & S_IRGRP) mode[4] = 'r';
  if (entry->info.st_mode & S_IWGRP) mode[5] = 'w';
  if (entry->info.st_mode & S_IXGRP) mode[6] = 'x';

  if (entry->info.st_mode & S_IROTH) mode[7] = 'r';
  if (entry->info.st_mode & S_IWOTH) mode[9] = 'w';
  if (entry->info.st_mode & S_IXOTH) mode[9] = 'x';
}


void get_time_line(size_t timesz, char time[timesz], const time_t t) {
  struct tm* tt = gmtime(&t);
  strftime(time, timesz, "%Y-%m-%d %H:%M:%S", tt);
}


void update_titlebar() {
  char dir[FILENAME_MAX+1];
  char cmd[FILENAME_MAX+64];

  escape_quote(sizeof(dir), dir, CURRENT_DIR);

  snprintf(cmd, sizeof(cmd), "echo -n '%c]0;raider: %s [%i]%c'", '\033', dir, getpid(), '\007');

  system(cmd);
}


static
void* write_selected(BTKey k __attribute__((unused)), void* val, void* ctx) {
  fprintf((FILE*) ctx, "%s\n", (char*) val);
  return val;
}


void selection_save() {
  char path[FILENAME_MAX+1];
  snprintf(path, sizeof(path), "%s/.raider-sel-%i", getenv("HOME"), getpid());

  FILE* f = fopen(path, "w");
  if (f == NULL)  return;

  btree_traverse(SELECTION, write_selected, f);

  fclose(f);
}


static
void* remove_dangling(BTKey k __attribute__((unused)), void* val, void* ctx __attribute__((unused))) {
  char* path = (char*) val;
  if (!path_exists(path)) {
    free(path);
    return NULL;
  }
  return val;
}


void selection_purge(void) {
  btree_traverse(SELECTION, remove_dangling, NULL);
  selection_save();
}

void selection_remove_file() {
  char path[FILENAME_MAX+1];
  snprintf(path, sizeof(path), "%s/.raider-sel-%i", getenv("HOME"), getpid());

  if (path_exists(path)) unlink(path);
}
