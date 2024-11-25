/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2024, Luca Marx
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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
