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
    draw_roulette_wheel_rotated(dpy, win, gc, width, height, 0, -1);
}

void draw_roulette_wheel_rotated(Display *dpy, Window win, GC gc, int width, int height, int rotation, int selected_segment) {
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
        int start_angle = (i * angle + rotation) % 360;
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

        // If this is the selected segment, draw a highlight border
        if (i == selected_segment) {
            // Allocate gold color
            XColor gold_color;
            gold_color.red = ((GOLD >> 16) & 0xFF) * 257;
            gold_color.green = ((GOLD >> 8) & 0xFF) * 257;
            gold_color.blue = (GOLD & 0xFF) * 257;
            gold_color.flags = DoRed | DoGreen | DoBlue;
            XAllocColor(dpy, DefaultColormap(dpy, screen), &gold_color);
            
            XSetForeground(dpy, gc, gold_color.pixel);
            XSetLineAttributes(dpy, gc, 5, LineSolid, CapRound, JoinRound);  // Thick line
            
            // Draw outer highlight border
            XDrawArc(dpy, win, gc, cx - radius - 3, cy - radius - 3, 
                    2 * (radius + 3), 2 * (radius + 3), start_angle * 64, angle * 64);
            
            // Draw inner highlight border
            XDrawArc(dpy, win, gc, cx - (radius - 15), cy - (radius - 15), 
                    2 * (radius - 15), 2 * (radius - 15), start_angle * 64, angle * 64);
            
            // Draw radial lines to emphasize the segment
            double start_theta = (start_angle * M_PI) / 180.0;
            double end_theta = ((start_angle + angle) * M_PI) / 180.0;
            
            // Start radial line
            int x1 = cx + (radius - 15) * cos(start_theta);
            int y1 = cy - (radius - 15) * sin(start_theta);
            int x2 = cx + (radius + 3) * cos(start_theta);
            int y2 = cy - (radius + 3) * sin(start_theta);
            XDrawLine(dpy, win, gc, x1, y1, x2, y2);
            
            // End radial line
            x1 = cx + (radius - 15) * cos(end_theta);
            y1 = cy - (radius - 15) * sin(end_theta);
            x2 = cx + (radius + 3) * cos(end_theta);
            y2 = cy - (radius + 3) * sin(end_theta);
            XDrawLine(dpy, win, gc, x1, y1, x2, y2);
            
            // Reset line width
            XSetLineAttributes(dpy, gc, 0, LineSolid, CapRound, JoinRound);
        }

        // Position text in the middle of each segment (with rotation)
        double theta = ((start_angle + angle / 2.0) * M_PI) / 180.0;
        int tx = cx + (radius - 40) * cos(theta);
        int ty = cy - (radius - 40) * sin(theta);
        char buf[4];
        snprintf(buf, sizeof(buf), "%d", numbers[i]);
        XSetForeground(dpy, gc, WhitePixel(dpy, screen));
        draw_text(dpy, win, gc, tx - 10, ty + 5, buf);
    }
    
    // Draw a pointer/indicator at the top
    XSetForeground(dpy, gc, WhitePixel(dpy, screen));
    // Draw triangle pointer
    XPoint triangle[3];
    triangle[0].x = cx;
    triangle[0].y = cy - radius - 10;
    triangle[1].x = cx - 10;
    triangle[1].y = cy - radius + 10;
    triangle[2].x = cx + 10;
    triangle[2].y = cy - radius + 10;
    XFillPolygon(dpy, win, gc, triangle, 3, Convex, CoordModeOrigin);
}