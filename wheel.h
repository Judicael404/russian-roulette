#ifndef WHEEL_H
#define WHEEL_H

#include <X11/Xlib.h>

#define GREEN 0x00FF00
#define RED 0xFF0000
#define BLACK 0x000000
#define WHITE 0xFFFFFF

void draw_roulette_wheel(Display *dpy, Window win, GC gc, int width, int height);

#endif