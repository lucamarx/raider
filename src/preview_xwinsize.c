#include "raider.h"

#ifdef HAS_X11
#include <stdlib.h>
#include "X11/Xatom.h"
#include "X11/Xlib.h"


void preview_get_xwin_size(Preview *preview) {
  Display* display;
  if ((display = XOpenDisplay(getenv("DISPLAY")))) {
    // https://en.wikibooks.org/wiki/X_Window_Programming

    // https://unix.stackexchange.com/questions/573121/get-current-screen-dimensions-via-xlib-using-c
    // https://stackoverflow.com/questions/3806872/window-position-in-xlib
    // https://stackoverflow.com/questions/73833171/how-to-get-the-active-window-using-x11-xlib-c-api

    Window x_root = DefaultRootWindow(display);
    Atom x_prop = XInternAtom(display, "_NET_ACTIVE_WINDOW", False);

    Atom x_return;
    int x_format_return;
    unsigned long x_nitems_return, x_bytes_left;
    unsigned char *x_data;

    int ret = XGetWindowProperty(display,
                                 x_root,
                                 x_prop,
                                 0,                //no offset
                                 1,                //one Window
                                 False,
                                 XA_WINDOW,
                                 &x_return,        //should be XA_WINDOW
                                 &x_format_return, //should be 32
                                 &x_nitems_return, //should be 1 (zero if there is no such window)
                                 &x_bytes_left,    //should be 0 (i'm not sure but should be atomic read)
                                 &x_data           //should be non-null
                                 );

    if (ret == Success && x_nitems_return) {
      Window x_window = ((Window*) x_data)[0];

      XFree(x_data);

      XWindowAttributes x_attr;
      XGetWindowAttributes(display, x_window, &x_attr);

      preview->has_x11 = true;
      preview->x_width = x_attr.width;
      preview->x_height = x_attr.height;
    }

    XCloseDisplay(display);
  }
}
#else
  void preview_get_xwin_size(Preview *preview __attribute__((unused))) {}
#endif
