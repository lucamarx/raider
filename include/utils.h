#ifndef UTILS_H
#define UTILS_H

#include "raider.h"
#include <stdbool.h>
#include <time.h>

#define PATH_DOES_NOT_EXISTS  -1
#define PATH_IS_NOT_ABSOLUTE  -2
#define PATH_IS_NOT_DIRECTORY -3
#define PATH_IS_NOT_FILE      -4
#define PATH_IS_SPECIAL       -5
#define PATH_IS_EMPTY         -6


// check if string starts with pattern
bool starts_with(const char* str, const char* pat);

// returns true if pat equals one of the element in the comma separated list strs
bool is_one_of(const char* pat, const char* strs);

// read whole line
char* fgetline(size_t size, char s[restrict size], FILE*restrict stream);

// split directory and file parts
int path_split_parts(char* dir_part, char* file_part, const char* path);

// get full path of entry (returns true if the path exists)
int path_get_full(char* path, const Entry* file_entry, bool escape);

// get mime type
void get_mime_type(size_t mimesz, char mime[mimesz], const Entry* file_entry);

// get file extension
void path_get_extension(size_t extsz, char ext[extsz], const char* file_name);

// check if path exists
bool path_exists(const char* path);

// escape single quotes (for shell command arguments)
void escape_quote(size_t escsz, char esc[escsz], const char* buf);

// get human readable size for entry (with units)
void get_size_line(size_t sizesz, char size[sizesz], const Entry* entry);

// get human readable mode line for entry
void get_mode_line(char* mode, const Entry* entry);

// get formatted time
void get_time_line(size_t timesz, char time[timesz], const time_t t);

// update window titlebar
void update_titlebar(void);

// save selected files
void selection_save(void);

// remove non-existing files from selection
void selection_purge(void);

// clear selection file
void selection_remove_file(void);

// subscribe to kernel events
void events_subscribe(const char* dir);

// consume kernel events
void events_consume(void (*on_dir_change)(void), void (*on_dir_unavailable)(void));

// unsubscribe from kernel events
void events_unsubscribe(void);
#endif
