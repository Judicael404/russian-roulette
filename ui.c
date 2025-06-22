#include "ui.h"
#include "constants.h"
#include "dirlist.h"

#include <X11/Xlib.h>
#include <stdio.h>
#include <string.h>


// spin button
void draw_spin_button(Display *dpy, Window win, GC gc, int x, int y, int width, int height, const char* label) {
    int screen = DefaultScreen(dpy);

    XSetForeground(dpy, gc, WhitePixel(dpy, screen));
    XFillRectangle(dpy, win, gc, x, y, width, height);

    XSetForeground(dpy, gc, BlackPixel(dpy, screen));
    XDrawRectangle(dpy, win, gc, x, y, width, height);

    // label
    int text_x = x + width / 6;
    int text_y = y + height / 2 + 6;
    XDrawString(dpy, win, gc, text_x, text_y, label, strlen(label));
}

void draw_box_text(Display *dpy, Window win, GC gc, int x, int y, const char* text) {
    XDrawString(dpy, win, gc, x, y, text, strlen(text));
}

void draw_result_text(Display *dpy, Window win, GC gc, int x, int y, const char* result) {
    int screen = DefaultScreen(dpy);
    
    // Draw background box for result
    XSetForeground(dpy, gc, WhitePixel(dpy, screen));
    XFillRectangle(dpy, win, gc, x, y, BOX_WIDTH, 80);
    
    XSetForeground(dpy, gc, BlackPixel(dpy, screen));
    XDrawRectangle(dpy, win, gc, x, y, BOX_WIDTH, 80);
    
    // Draw "Result:" label
    draw_box_text(dpy, win, gc, x + 5, y + 15, "Result:");
    
    // Draw the actual result
    draw_box_text(dpy, win, gc, x + 5, y + 35, result);
    
    // Draw execution status
    draw_box_text(dpy, win, gc, x + 5, y + 70, "in terminal");
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
    int count = read_directories("directory_list.txt", dirs, MAX_DIRS);

    if (count < 0) {
        printf("error reading directories\n");
        return; // Error reading directories
    }

    // Add title
    draw_box_text(dpy, win, gc, tx, ty, "Odd Numbers:");
    ty += line_height + 5;

    int dir_idx = 0;
    for (int i = 1; i < NUM_SEGMENTS && dir_idx < count; i += 2, dir_idx++) {
        char buf[1024];
        snprintf(buf, sizeof(buf), "%d: %s", i, dirs[dir_idx]);
        draw_box_text(dpy, win, gc, tx, ty, buf);
        ty += line_height;
        if (ty > y + height - line_height) break;
    }
    
    // Free allocated memory
    free_directories(dirs, count);
}