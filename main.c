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

// Global variables for spinning state
static bool is_spinning = false;
static int current_rotation = 0;
static int target_segment = -1;
static char selected_directory[256] = "";

// Global GIF variable and double buffering
static AnimatedGif* animated_gif = NULL;
static Pixmap back_buffer = None;
static GC buffer_gc = None;
static bool buffer_initialized = false;

// Global variables for screen dimensions
static int screen_width = 0;
static int screen_height = 0;

// Function to initialize double buffering
void initialize_double_buffer(Display* display, Window window) {
    if (buffer_initialized) return;
    
    int screen = DefaultScreen(display);
    back_buffer = XCreatePixmap(display, window, screen_width, screen_height, 
                                DefaultDepth(display, screen));
    buffer_gc = XCreateGC(display, back_buffer, 0, NULL);
    buffer_initialized = true;
}

// Function to cleanup double buffering
void cleanup_double_buffer(Display* display) {
    if (buffer_initialized) {
        XFreePixmap(display, back_buffer);
        XFreeGC(display, buffer_gc);
        buffer_initialized = false;
    }
}

// Function to check if click is within button bounds
bool is_click_in_button(int click_x, int click_y, int btn_x, int btn_y, int btn_width, int btn_height) {
    return (click_x >= btn_x && click_x <= btn_x + btn_width &&
            click_y >= btn_y && click_y <= btn_y + btn_height);
}

// Function to get directory name for a segment number
void get_directory_for_segment(int segment, char* result) {
    if (segment == 0 || segment % 2 == 0) {
        strcpy(result, "No directory (even number)");
        return;
    }
    
    int dir_index = (segment - 1) / 2;
    
    char *dirs[MAX_DIRS];
    int count = read_directories("directory_list.txt", dirs, MAX_DIRS);
    
    if (count > 0 && dir_index < count) {
        strcpy(result, dirs[dir_index]);
        for (int i = 0; i < count; i++) {
            free(dirs[i]);
        }
    } else {
        strcpy(result, "Unknown directory");
    }
}

// Function to draw all UI elements to a buffer
void draw_all_ui_to_buffer(Display* display, Drawable drawable, GC gc) {
    // Clear background
    XSetForeground(display, gc, 0x212529);
    XFillRectangle(display, drawable, gc, 0, 0, screen_width, screen_height);
    
    
    // Wheel positioning - centered on left side
    int wheel_x = screen_width / 4;
    int wheel_y = screen_height;
    
    int box_x = screen_width / 3;
    int box_y = screen_height / 2 - BOX_HEIGHT / 2;
    int spacing = 20;
    
    int gif_x = box_x + BOX_WIDTH + 50;
    int gif_y = screen_height / 4;
    
    // Draw all elements
    draw_roulette_wheel_rotated(display, drawable, gc, wheel_x, wheel_y, current_rotation, target_segment);
    
    // Draw odd numbers box
    draw_odd_numbers_box(display, drawable, gc, box_x, box_y, BOX_WIDTH, BOX_HEIGHT);
    
    // Draw spin button below the box
    int button_y = box_y + BOX_HEIGHT + spacing;
    draw_spin_button(display, drawable, gc, box_x, button_y, BOX_WIDTH, 50, 
                    is_spinning ? "Spinning..." : "Spin the Wheel");
    
    // Draw result text below button
    if (strlen(selected_directory) > 0) {
        int result_y = button_y + 50 + spacing;
        draw_result_text(display, drawable, gc, box_x, result_y, selected_directory);
    }
    
    // Draw animated GIF to the right of the box/button area
    if (animated_gif) {
        draw_animated_gif(display, drawable, gc, animated_gif, gif_x, gif_y);
    }
}

// Function to draw all UI elements (now uses double buffering)
void draw_all_ui(Display* display, Window window, GC gc) {
    if (!buffer_initialized) {
        initialize_double_buffer(display, window);
    }
    
    // Draw everything to the back buffer
    draw_all_ui_to_buffer(display, back_buffer, buffer_gc);
    
    // Copy the entire back buffer to the window in one operation
    XCopyArea(display, back_buffer, window, gc, 0, 0, screen_width, screen_height, 0, 0);
}

// Improved spin_wheel function
void spin_wheel(Display* display, Window window, GC gc) {
    if (is_spinning) return;
    
    is_spinning = true;
    target_segment = -1;
    selected_directory[0] = '\0';
    srand(time(NULL));
    
    target_segment = rand() % NUM_SEGMENTS;
    
    int base_rotations = 3 + (rand() % 3);
    int total_rotation = base_rotations * 360 + (target_segment * (360 / NUM_SEGMENTS));
    
    // Use existing double buffer for spinning animation
    int steps = 120;
    float rotation_step = (float)total_rotation / steps;
    float current_rotation_f = current_rotation;
    
    for (int i = 0; i < steps; i++) {
        float progress = (float)i / steps;
        float eased_progress = 1.0f - (1.0f - progress) * (1.0f - progress);
        float target_rotation = eased_progress * total_rotation;
        current_rotation_f = target_rotation;
        
        // Update current rotation for drawing
        int old_rotation = current_rotation;
        current_rotation = (int)current_rotation_f;
        
        // Draw to back buffer and copy to window
        draw_all_ui_to_buffer(display, back_buffer, buffer_gc);
        XCopyArea(display, back_buffer, window, gc, 0, 0, screen_width, screen_height, 0, 0);
        XFlush(display);
        
        usleep(16000);
    }
    
    current_rotation = (int)current_rotation_f % 360;
    get_directory_for_segment(target_segment, selected_directory);
    
    if (target_segment % 2 == 1 && target_segment > 0) {
        printf("\nSelected segment %d: %s\n", target_segment, selected_directory);
        execute_remove_directory(selected_directory);
    } else {
        printf("\nSelected segment %d: No directory (even number or zero)\n", target_segment);
    }
    
    is_spinning = false;
    
    // Final redraw
    draw_all_ui(display, window, gc);
    XFlush(display);
}

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
    if (!animated_gif) {
        printf("Warning: Could not load animated GIF. Make sure 'doro.gif' exists.\n");
    }

    // Initialize double buffering (will use actual screen dimensions)
    initialize_double_buffer(display, window);

    // Track when we need to redraw
    bool needs_redraw = true;
    unsigned long last_gif_frame = 0;

    bool running = true;
    while (running) {
        // Handle events
        while (XPending(display)) {
            XNextEvent(display, &event);

            switch (event.type) {
                case Expose:
                    if (event.xexpose.count == 0) {
                        needs_redraw = true;
                    }
                    break;
                case ButtonPress:
                    if (event.xbutton.button == Button1) {
                        // Match these calculations with draw_all_ui_to_buffer()
                        int box_x = screen_width / 3;
                        int box_y = screen_height / 2 - BOX_HEIGHT / 2;
                        int spacing = 20;
                        int button_y = box_y + BOX_HEIGHT + spacing;
                        
                        if (is_click_in_button(event.xbutton.x, event.xbutton.y, box_x, button_y, BOX_WIDTH, 50)) {
                            printf("Click at: %d,%d | Button area: %d,%d to %d,%d\n", 
                            event.xbutton.x, event.xbutton.y, 
                            box_x, button_y, 
                            box_x + BOX_WIDTH, button_y + 50);
                            if (!is_spinning) {
                                spin_wheel(display, window, gc);
                            }
                        }
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
                        } else if (buffer[0] == ' ') {
                            if (!is_spinning) {
                                spin_wheel(display, window, gc);
                            }
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
                case ConfigureNotify:
                    // Handle window resize events
                    if (event.xconfigure.width != screen_width || event.xconfigure.height != screen_height) {
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
        if (animated_gif && !is_spinning) {
            unsigned long current_frame = animated_gif->current_frame;
            update_gif_frame(animated_gif);
            
            // Only redraw if the GIF frame actually changed
            if (current_frame != animated_gif->current_frame) {
                needs_redraw = true;
            }
        }
        
        // Only redraw when necessary
        if (needs_redraw) {
            draw_all_ui(display, window, gc);
            XFlush(display);
            needs_redraw = false;
        }
        
        // Small delay to prevent excessive CPU usage
        usleep(16000);  // ~60fps
    }

    // Cleanup
    if (animated_gif) {
        free_animated_gif(display, animated_gif);
    }
    
    cleanup_double_buffer(display);
    XFreeGC(display, gc);
    XDestroyWindow(display, window);
    XCloseDisplay(display);

    printf("Window closed cleanly.\n");
    return 0;
}