#ifndef UI_H
#define UI_H

#include <X11/Xlib.h>

void draw_odd_numbers_box(Display *dpy, Window win, GC gc, int x, int y, int width, int height);

#endif
