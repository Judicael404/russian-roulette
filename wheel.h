#ifndef WHEEL_H
#define WHEEL_H

#include <X11/Xlib.h>

void draw_roulette_wheel(Display *dpy, Window win, GC gc, int width, int height);
void draw_roulette_wheel_rotated(Display *dpy, Window win, GC gc, int width, int height, int rotation, int selected_segment);
void get_directory_for_segment(int segment, char* result);
void spin_wheel(Display* display, Window window, GC gc);
#endif