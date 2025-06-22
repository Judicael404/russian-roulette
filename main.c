#include "dirlist.h"
#include "wheel.h"
#include "ui.h"
#include "constants.h"
#include "shell_executor.h"
#include "gif_handler.h"

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
    Display* display;
    Window window;
    GC gc;
    XEvent event;
    int screen;

    display = XOpenDisplay(NULL);
    if (display == NULL)
    {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }

    screen = DefaultScreen(display);
    
    // Get actual screen dimensions and account for window decorations
    int display_width = DisplayWidth(display, screen);
    int display_height = DisplayHeight(display, screen);
    
    // Set window size to near-maximum (leave some space for window decorations)
    screen_width = display_width - 100;  // Leave margin for window manager
    screen_height = display_height - 100;
    
    printf("Creating maximized window: %dx%d\n", screen_width, screen_height);

    // Create large windowed window
    window = XCreateSimpleWindow(display, RootWindow(display, screen),
                             50, 50, screen_width, screen_height, 1,
                             BlackPixel(display, screen), 0x212529);

    XStoreName(display, window, "Directory Roulette Wheel - Maximized");

    // Set window to be maximized (but not fullscreen)
    Atom wm_state = XInternAtom(display, "_NET_WM_STATE", False);
    Atom wm_maximized_vert = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", False);
    Atom wm_maximized_horz = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
    
    Atom maximized_atoms[2] = {wm_maximized_vert, wm_maximized_horz};
    XChangeProperty(display, window, wm_state, XA_ATOM, 32,
                   PropModeReplace, (unsigned char*)maximized_atoms, 2);

    Atom wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &wm_delete_window, 1);

    XSelectInput(display, window, ExposureMask | KeyPressMask | StructureNotifyMask | ButtonPressMask);

    XMapWindow(display, window);

    gc = XCreateGC(display, window, 0, NULL);
    XSetForeground(display, gc, BlackPixel(display, screen));

    // Load GIF
    animated_gif = load_animated_gif(display, "doro.gif");
    if (!animated_gif)
    {
        printf("Warning: Could not load animated GIF. Make sure 'doro.gif' exists.\n");
    }

    // Initialize double buffering (will use actual screen dimensions)
    initialize_double_buffer(display, window);

    // Track when we need to redraw
    bool needs_redraw = true;
    unsigned long last_gif_frame = 0;

    bool running = true;
    while (running)
    {
        // Handle events
        while (XPending(display))
        {
            XNextEvent(display, &event);

            switch (event.type)
            {
                case Expose:
                    if (event.xexpose.count == 0)
                    {
                        needs_redraw = true;
                    }
                    break;
                    
                case ButtonPress:
                    if (event.xbutton.button == Button1)
                    {
                        // Match these calculations with draw_all_ui_to_buffer()
                        int box_x = screen_width / 3;
                        int box_y = screen_height / 2 - BOX_HEIGHT / 2;
                        int spacing = 20;
                        int button_y = box_y + BOX_HEIGHT + spacing;
                        
                        if (is_click_in_button(event.xbutton.x, event.xbutton.y, box_x, button_y, BOX_WIDTH, 50))
                        {
                            printf("Click at: %d,%d | Button area: %d,%d to %d,%d\n", 
                                  event.xbutton.x, event.xbutton.y, 
                                  box_x, button_y, 
                                  box_x + BOX_WIDTH, button_y + 50);
                            if (!is_spinning)
                            {
                                spin_wheel(display, window, gc);
                            }
                        }
                    }
                    break;
                    
                case KeyPress:
                {
                    char buffer[1];
                    KeySym keysym;
                    XComposeStatus compose_status;
                    int len = XLookupString(&event.xkey, buffer, sizeof(buffer), &keysym, &compose_status);

                    if (len == 1)
                    {
                        if (buffer[0] == 'q')
                        {
                            running = false;
                        }
                        else if (buffer[0] == ' ')
                        {
                            if (!is_spinning)
                            {
                                spin_wheel(display, window, gc);
                            }
                        }
                    }
                    if (keysym == XK_Escape)
                    {
                        running = false;
                    }
                    break;
                }
                
                case ClientMessage:
                    if (event.xclient.data.l[0] == wm_delete_window)
                    {
                        running = false;
                    }
                    break;
                    
                case ConfigureNotify:
                    // Handle window resize events
                    if (event.xconfigure.width != screen_width || event.xconfigure.height != screen_height)
                    {
                        screen_width = event.xconfigure.width;
                        screen_height = event.xconfigure.height;
                        
                        // Recreate double buffer with new dimensions
                        cleanup_double_buffer(display);
                        initialize_double_buffer(display, window);
                        needs_redraw = true;
                        
                        printf("Window resized to: %dx%d\n", screen_width, screen_height);
                    }
                    break;
                    
                default:
                    break;
            }
        }
        
        // Update GIF animation and check if frame changed
        if (animated_gif && !is_spinning)
        {
            unsigned long current_frame = animated_gif->current_frame;
            update_gif_frame(animated_gif);
            
            // Only redraw if the GIF frame actually changed
            if (current_frame != animated_gif->current_frame)
            {
                needs_redraw = true;
            }
        }
        
        // Only redraw when necessary
        if (needs_redraw)
        {
            draw_all_ui(display, window, gc);
            XFlush(display);
            needs_redraw = false;
        }
        
        // Small delay to prevent excessive CPU usage
        usleep(16000);  // ~60fps
    }

    // Cleanup
    if (animated_gif)
    {
        free_animated_gif(display, animated_gif);
    }
    
    cleanup_double_buffer(display);
    XFreeGC(display, gc);
    XDestroyWindow(display, window);
    XCloseDisplay(display);

    printf("Window closed cleanly.\n");
    return 0;
}