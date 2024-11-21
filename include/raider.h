#ifndef RAIDER_H
#define RAIDER_H

#include "btree.h"

#include <dirent.h>
#include <ncurses.h>
#include <sys/stat.h>

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__APPLE__)
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#define BSD_KQUEUE
#endif

#define RAIDER_VERSION "0.4.1"

// color pairs
#define PAIR_DEFAULT       1
#define PAIR_RED_BLACK     2
#define PAIR_BLUE_BLACK    3
#define PAIR_CYAN_BLACK    4
#define PAIR_GREEN_BLACK   5
#define PAIR_YELLOW_BLACK  6
#define PAIR_BLACK_YELLOW  7
#define PAIR_MAGENTA_BLACK 8

// config
typedef struct {
  bool    has_fd;
  bool    has_fzf;
  bool    has_vim;
  bool    has_find;
  bool    has_emacsclient;
} Config;

// file types
typedef enum { unknown, text, document, image, video, archive, file_type_num } FileType;

// file description
typedef struct {
  char        name[MAXNAMLEN+1]; // file name
  char        ext[10];           // extension
  FileType    type;              // content type (guessed from extension)
  struct stat info;              // file info
  bool        is_link;           // if it is a symbolic link
} Entry;

// the current state
typedef struct {
  size_t start_pos;
  size_t pos;
  size_t end_pos;
  size_t files_n;
  char   order;
} State;

// specific file previewer
typedef void (*Previewer)(const void*, WINDOW*, const Entry*);
typedef void (*Thumbnailer)(const void*, const char* path, const char* cache_path);

// preview types
typedef enum { x11, chafa, sixel, none, preview_type_num } PreviewMode;

// preview config
typedef struct {
  bool    has_x11;
  bool    has_chafa;
  bool    has_convert;
  bool    has_djvutxt;
  bool    has_img2sixel;
  bool    has_pdftotext;
  bool    has_mediainfo;
  bool    has_thumbnailer;
  bool    has_w3mimgdisplay;
  char    w3mimgdisplay_path[64];

  size_t  x_width;
  size_t  x_height;

  PreviewMode mode;

  void (*preview_clear)(const void*, WINDOW*);
  void (*preview_display)(const void*, WINDOW*, const char*);

  Previewer   previewer[file_type_num];
  Thumbnailer thumbnailer[file_type_num];

} Preview;


// initializes curses
void init_curses(void);

// read directory
int list_dir(const char* path);

// sort directory
void sort_dir(char order);

// resize window callback
void action_resize_window(void);

// go to the specified directory and point to file
void action_goto(const char* dir_part, const char* file_part);

// go to the specified path
void action_goto_path(const char* path);

// go to home directory
void action_goto_home(void);

// refresh
void action_refresh(void);

// move the pointer up by 1
void action_up(bool update_preview);

// move the pointer down by 1
void action_down(bool update_preview);

// move the pointer up by a page (scrolling if necessary)
void action_page_up(bool update_preview);

// move the pointer down by a page (scrolling if necessary)
void action_page_down(bool update_preview);

// move the pointer to the top of the file list
void action_home(void);

// move the pointer to the bottom of the file list
void action_end(void);

// change directory or open file
void action_forward(void);

// go to parent directory
void action_backward(void);

// order files
void action_reorder(const char order);

// select item
void action_select(void);

// show file/directory information
void action_show_info(void);

// drop into a shell in the current directory
void action_open_shell(void);

// open text file or directory into editor
void action_open_editor(void);

// FZF search
void action_fzf_search(void);

// the main event loop
void event_loop(void);

// update panes
void display_update_top(void);
void display_update_bot(void);
void display_update_lft(void);
void display_update_rgt(bool update_preview);
void display_error(const char* error);

// initialize preview based on available stuff
void preview_init(Preview* preview);

// get avaoiable preview modes
void preview_get_modes(const Preview* preview, size_t modesz, char modes[modesz]);

// set preview mode
int preview_set_mode(Preview* preview, const char* mode);

// get/update X windows configuration
void preview_get_xwin_size(Preview* preview);

// clear preview (generic)
void preview_clear(const Preview* preview, WINDOW* win);

// refresh preview (generic)
void preview_refresh(const Preview* preview, WINDOW* win);

// display preview for file (generic)
void preview_file(const Preview* preview, WINDOW* win, const Entry* entry);

// preview directory (generic)
void preview_directory(const Preview *preview, WINDOW* win, const Entry* dir_entry);

// preview file info
void preview_file_info(WINDOW* win, const Entry* dir_entry);

// externs
extern WINDOW*  WTOP;
extern WINDOW*  WBOT;
extern WINDOW*  WLFT;
extern WINDOW*  WRGT;

extern State*   STATE;
extern Entry*   ENTRIES;
extern Config*  CONFIG;
extern Preview* PREVIEW;
extern BTNode*  HISTORY;
extern BTNode*  SELECTION;

extern char     USER[256];
extern char     HOST[256];
extern char     CURRENT_DIR[FILENAME_MAX+1];

#ifdef BSD_KQUEUE
  extern int           KQ;
  extern int           KQ_FD;
  extern struct kevent KQ_CHANGE;
#endif

#endif
