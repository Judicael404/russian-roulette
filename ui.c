#include "ui.h"
#include "constants.h"
#include "dirlist.h"

#include <X11/Xlib.h>
#include <stdio.h>
#include <string.h>


void draw_box_text(Display *dpy, Window win, GC gc, int x, int y, const char* text) {
    XDrawString(dpy, win, gc, x, y, text, strlen(text));
}

void draw_odd_numbers_box(Display *dpy, Window win, GC gc, int x, int y, int width, int height) {
    int screen = DefaultScreen(dpy);

    // Draw white-filled rectangle
    XSetForeground(dpy, gc, WhitePixel(dpy, screen));
    XFillRectangle(dpy, win, gc, x, y, width, height);

    // Draw black border
    XSetForeground(dpy, gc, BlackPixel(dpy, screen));
    XDrawRectangle(dpy, win, gc, x, y, width, height);

    // Draw the odd numbers inside
    int line_height = 15;
    int tx = x + 10;
    int ty = y + line_height;

    char *dirs[MAX_DIRS];
    int count = read_directories("directories.txt", dirs, MAX_DIRS);

    if (count < 0) {
        return; // Error reading directories
    }

    for (int i = 1; i < NUM_SEGMENTS; i += 2) {
        char buf[1024];
        snprintf(buf, sizeof(buf), "%d:", i);
        draw_box_text(dpy, win, gc, tx, ty, buf);
        ty += line_height;
        if (ty > y + height - line_height) break;
    }
}
