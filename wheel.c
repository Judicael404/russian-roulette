#include "wheel.h"
#include "constants.h"

#include <math.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <string.h>

void draw_segment(Display *dpy, Window win, GC gc, int cx, int cy, int r, int start_angle, int arc_angle, unsigned long color) {
    XSetForeground(dpy, gc, color);
    XFillArc(dpy, win, gc, cx - r, cy - r, 2 * r, 2 * r, start_angle * 64, arc_angle * 64);
}

void draw_text(Display *dpy, Window win, GC gc, int x, int y, const char* text) {
    XDrawString(dpy, win, gc, x, y, text, strlen(text));
}

void draw_roulette_wheel(Display *dpy, Window win, GC gc, int width, int height) {
    int screen = DefaultScreen(dpy);
    int cx = width / 2;
    int cy = height / 2;
    int radius = (width < height ? width : height) / 2 - 20;

    int numbers[NUM_SEGMENTS] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
        12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23
    };
    
    for (int i = 0; i < NUM_SEGMENTS; ++i) {
        int angle = 360 / NUM_SEGMENTS;  // 15 degrees per segment
        int start_angle = i * angle;
        unsigned long color;

        // Green for 0, red/black alternating for others
        if (i == 0) {
            color = GREEN;
        } else if (i % 2 == 0) {
            color = RED;
        } else {
            color = BLACK;
        }

        XColor xcol;
        xcol.red = ((color >> 16) & 0xFF) * 257;
        xcol.green = ((color >> 8) & 0xFF) * 257;
        xcol.blue = (color & 0xFF) * 257;
        xcol.flags = DoRed | DoGreen | DoBlue;
        XAllocColor(dpy, DefaultColormap(dpy, screen), &xcol);

        draw_segment(dpy, win, gc, cx, cy, radius, start_angle, angle, xcol.pixel);

        // Position text in the middle of each segment
        double theta = ((start_angle + angle / 2.0) * M_PI) / 180.0;
        int tx = cx + (radius - 40) * cos(theta);
        int ty = cy - (radius - 40) * sin(theta);
        char buf[4];
        snprintf(buf, sizeof(buf), "%d", numbers[i]);
        XSetForeground(dpy, gc, WhitePixel(dpy, screen));
        draw_text(dpy, win, gc, tx - 10, ty + 5, buf);
    }
}