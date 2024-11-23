#include "raider.h"
#include "utils.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>


int list_dir(const char* path) {
  DIR* dir = opendir(path);

  if (dir == NULL) return PATH_DOES_NOT_EXISTS;

  int N = 0;
  struct dirent* dir_entry;
  while ((dir_entry = readdir(dir)) != NULL) {
    if (dir_entry->d_name[0] == '.') continue;
    N++;
  }

  if (N == 0) {
    closedir(dir);
    return N;
  }

  if (ENTRIES != NULL) free(ENTRIES);

  ENTRIES = calloc(N, sizeof(Entry));

  rewinddir(dir);

  int n = 0;
  char file_name[FILENAME_MAX+1];
  while ((dir_entry = readdir(dir)) != NULL) {
    if (dir_entry->d_name[0] == '.') continue;

    strlcpy(ENTRIES[n].name, dir_entry->d_name, sizeof(ENTRIES[n].name));

    path_get_extension(sizeof(ENTRIES[n].ext), ENTRIES[n].ext, dir_entry->d_name);

    snprintf(file_name, sizeof(file_name), "%s/%s", path, dir_entry->d_name);

    struct stat info;
    if (stat(file_name, &info) == 0) {
      ENTRIES[n].info = info;
      ENTRIES[n].type = unknown;

      if (is_one_of(ENTRIES[n].ext, "txt,org"))
        ENTRIES[n].type = text;

      else if (is_one_of(ENTRIES[n].ext, "pdf,djvu"))
        ENTRIES[n].type = document;

      else if (is_one_of(ENTRIES[n].ext, "png,jpg,jpeg"))
        ENTRIES[n].type = image;

      else if (is_one_of(ENTRIES[n].ext, "avi,mkv,mov,mp4,mpg,wmv,mpeg,webm"))
        ENTRIES[n].type = video;

      else if (is_one_of(ENTRIES[n].ext, "tar,tgz,zip,rar"))
        ENTRIES[n].type = archive;
    }

    ENTRIES[n].is_link = lstat(file_name, &info) == 0 && S_ISLNK(info.st_mode);
    n++;
  }

  closedir(dir);

  return N;
}


int by_name_asc(const void* a, const void* b) {
  const char* a_name = ((const Entry*) a)->name;
  const char* b_name = ((const Entry*) b)->name;

  return strcmp(a_name, b_name);
}

int by_name_dsc(const void* a, const void* b) {
  const char* a_name = ((const Entry*) a)->name;
  const char* b_name = ((const Entry*) b)->name;

  return -strcmp(a_name, b_name);
}

int by_size_asc(const void* a, const void* b) {
  int a_size = ((const Entry*) a)->info.st_size;
  int b_size = ((const Entry*) b)->info.st_size;

  return a_size - b_size;
}

int by_size_dsc(const void* a, const void* b) {
  int a_size = ((const Entry*) a)->info.st_size;
  int b_size = ((const Entry*) b)->info.st_size;

  return b_size - a_size;
}

int by_ctime_asc(const void* a, const void* b) {
  int a_ctime = ((const Entry*) a)->info.st_ctim.tv_sec;
  int b_ctime = ((const Entry*) b)->info.st_ctim.tv_sec;

  return a_ctime - b_ctime;
}

int by_ctime_dsc(const void* a, const void* b) {
  int a_ctime = ((const Entry*) a)->info.st_ctim.tv_sec;
  int b_ctime = ((const Entry*) b)->info.st_ctim.tv_sec;

  return b_ctime - a_ctime;
}


void sort_dir(char order) {
  switch (order) {
  case 'n':
    qsort(ENTRIES, STATE->files_n, sizeof(ENTRIES[0]), by_name_asc);
    break;
  case 'N':
    qsort(ENTRIES, STATE->files_n, sizeof(ENTRIES[0]), by_name_dsc);
    break;
  case 'z':
    qsort(ENTRIES, STATE->files_n, sizeof(ENTRIES[0]), by_size_asc);
    break;
  case 'Z':
    qsort(ENTRIES, STATE->files_n, sizeof(ENTRIES[0]), by_size_dsc);
    break;
  case 't':
    qsort(ENTRIES, STATE->files_n, sizeof(ENTRIES[0]), by_ctime_asc);
    break;
  case 'T':
    qsort(ENTRIES, STATE->files_n, sizeof(ENTRIES[0]), by_ctime_dsc);
    break;
  }
}
