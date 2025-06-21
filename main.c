#include "wheel.h"
#include "ui.h"
#include "constants.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

int main(int argc, char* argv[]) {
    Display* display;
    Window window;
    GC gc;
    XEvent event;
    int screen;

    display = XOpenDisplay(NULL);
    if (display == NULL) {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }

    screen = DefaultScreen(display);

    window = XCreateSimpleWindow(display, RootWindow(display, screen),
                             100, 100, SCREEN_WIDTH, SCREEN_HEIGHT, 1,
                             BlackPixel(display, screen), 0x212529);

    XStoreName(display, window, "X11 Simple Window");

    Atom wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &wm_delete_window, 1);

    XSelectInput(display, window, ExposureMask | KeyPressMask | StructureNotifyMask | ButtonPressMask);

    XMapWindow(display, window);

    gc = XCreateGC(display, window, 0, NULL);
    XSetForeground(display, gc, BlackPixel(display, screen));

    bool running = true;
    while (running) {
        XNextEvent(display, &event);

        switch (event.type) {
            case Expose:
                if (event.xexpose.count == 0) {
                    draw_roulette_wheel(display, window, gc, 500, 500);
                    draw_odd_numbers_box(display, window, gc,  SCREEN_WIDTH - 100, SCREEN_HEIGHT / 3, BOX_WIDTH, BOX_HEIGHT);
                    XFlush(display);
                }
                break;
            case KeyPress: {
                char buffer[1];
                KeySym keysym;
                XComposeStatus compose_status;
                int len = XLookupString(&event.xkey, buffer, sizeof(buffer), &keysym, &compose_status);

                if (len == 1) {
                    if (buffer[0] == 'q') {
                        running = false;
                    }
                }
                if (keysym == XK_Escape) {
                    running = false;
                }
                break;
            }
            case ClientMessage:
                if (event.xclient.data.l[0] == wm_delete_window) {
                    running = false;
                }
                break;
            default:
                break;
        }
    }

    XFreeGC(display, gc);
    XDestroyWindow(display, window);
    XCloseDisplay(display);

    printf("Window closed cleanly.\n");
    return 0;
}