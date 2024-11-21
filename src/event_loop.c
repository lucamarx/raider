#include "raider.h"
#include "utils.h"

#include <ncurses.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct {
  enum { no_input, key_down, key_cont, key_up } state;
  int key;
} KeyState;


static
KeyState update_key_state(KeyState ks, int ch) {
  // no_input
  if (ks.state == no_input && ch == ERR) return ks;
  if (ks.state == no_input && ch != ERR) {
    ks.state = key_down;
    ks.key = ch;
    return ks;
  }
  // key_down
  if (ks.state == key_down && ch == ERR) {
    ks.state = no_input;
    ks.key = -1;
    return ks;
  }
  if (ks.state == key_down && ch != ERR && ch == ks.key) {
    ks.state = key_cont;
    return ks;
  }
  if (ks.state == key_down && ch != ERR && ch != ks.key) {
    ks.state = key_down;
    ks.key = ch;
    return ks;
  }
  // key_cont
  if (ks.state == key_cont && ch == ERR) {
    ks.state = key_up;
    return ks;
  }
  if (ks.state == key_cont && ch != ERR && ch == ks.key) return ks;
  if (ks.state == key_cont && ch != ERR && ch != ks.key) {
    ks.state = key_down;
    ks.key = ch;
    return ks;
  }
  // key_up
  if (ks.state == key_up && ch == ERR) {
    ks.state = no_input;
    ks.key = -1;

    display_update_rgt(true);

    return ks;
  }
  if (ks.state == key_up && ch != ERR) {
    ks.state = key_down;
    ks.key = ch;
    return ks;
  }

  return ks;
}


void event_loop(void) {
  int ch;

  KeyState ks = {
    .state = no_input,
    .key = -1
  };

  while ((ch = getch())) {
    ks = update_key_state(ks, ch);

    if (ch == 'q')
      break;

    else if (ch == 'r')
      action_refresh();

    else if (ch == 'H')
      action_goto_home();

    else if (ch == KEY_UP || ch == 'k')
      action_up(ks.state == key_down || ks.state == key_up);

    else if (ch == KEY_DOWN || ch == 'j')
      action_down(ks.state == key_down || ks.state == key_up);

    else if (ch == KEY_PPAGE)
      action_page_up(ks.state == key_down || ks.state == key_up);

    else if (ch == KEY_NPAGE)
      action_page_down(ks.state == key_down || ks.state == key_up);

    else if (ks.state == key_down && ch == KEY_HOME)
      action_home();

    else if (ks.state == key_down && ch == KEY_END)
      action_end();


    else if (ks.state == key_down && (ch == KEY_RIGHT || ch == 'h'))
      action_forward();

    else if (ks.state == key_down && (ch == KEY_LEFT || ch == 'l'))
      action_backward();


    else if (ks.state == key_down && ch == ' ')
      action_select();


    else if (ks.state == key_down && ch == 'i')
      action_show_info();

    else if (ks.state == key_down && ch == 'p')
      display_update_rgt(true);


    else if (ks.state == key_down && (ch == 't' || ch == 'T' || ch == 'n' || ch == 'N' || ch == 'z' || ch == 'Z'))
      action_reorder(ch);

    else if (ks.state == key_down && ch == 's')
      action_open_shell();

    else if (ks.state == key_down && ch == 'e')
      action_open_editor();

    else if (ks.state == key_down && ch == '/')
      action_fzf_search();

    else if (ch == KEY_RESIZE)
      action_resize_window();

    events_consume(action_refresh);
  }
}
