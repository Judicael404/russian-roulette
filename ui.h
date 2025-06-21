#ifndef UI_H
#define UI_H

#include <X11/Xlib.h>

void draw_odd_numbers_box(Display *dpy, Window win, GC gc, int x, int y, int width, int height);
void draw_spin_button(Display *dpy, Window win, GC gc, int x, int y, int width, int height, const char* label);
void draw_result_text(Display *dpy, Window win, GC gc, int x, int y, const char* result);

#endif