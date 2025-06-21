#ifndef WHEEL_H
#define WHEEL_H

#include <X11/Xlib.h>


void draw_roulette_wheel(Display *dpy, Window win, GC gc, int width, int height);

#endif
#define GREEN 0x00FF00
#define RED 0xFF0000
#define BLACK 0x000000
#define WHITE 0xFFFFFF