#include "dirlist.h"
#include "wheel.h"
#include "ui.h"
#include "constants.h"
#include "shell_executor.h"
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
static int target_segment = -1;  // -1 means no selection
static char selected_directory[256] = "";

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
    
    // For odd segments, map to directories
    int dir_index = (segment - 1) / 2;  // 1->0, 3->1, 5->2, etc.
    
    char *dirs[MAX_DIRS];
    int count = read_directories("directory_list.txt", dirs, MAX_DIRS);
    
    if (count > 0 && dir_index < count) {
        strcpy(result, dirs[dir_index]);
        // Free the allocated memory
        for (int i = 0; i < count; i++) {
            free(dirs[i]);
        }
    } else {
        strcpy(result, "Unknown directory");
    }
}

// Function to perform spinning animation
void spin_wheel(Display* display, Window window, GC gc) {
    if (is_spinning) return;  // Prevent multiple spins
    
    is_spinning = true;
    target_segment = -1;  // Reset selection
    selected_directory[0] = '\0';  // Clear previous result
    srand(time(NULL));
    
    // Generate random target segment
    target_segment = rand() % NUM_SEGMENTS;
    
    // Calculate total rotation (multiple full rotations + target)
    int base_rotations = 3 + (rand() % 3);  // 3-5 full rotations
    int total_rotation = base_rotations * 360 + (target_segment * (360 / NUM_SEGMENTS));
    
    // Create a pixmap for double buffering
    int screen = DefaultScreen(display);
    Pixmap buffer = XCreatePixmap(display, window, SCREEN_WIDTH, SCREEN_HEIGHT, 
                                  DefaultDepth(display, screen));
    GC buffer_gc = XCreateGC(display, buffer, 0, NULL);
    
    // Animate the spinning with smoother parameters
    int steps = 120;  // More steps for smoother animation
    float rotation_step = (float)total_rotation / steps;
    float current_rotation_f = current_rotation;
    
    for (int i = 0; i < steps; i++) {
        // Use easing function for more natural deceleration
        float progress = (float)i / steps;
        float eased_progress = 1.0f - (1.0f - progress) * (1.0f - progress);  // Ease-out quadratic
        float target_rotation = eased_progress * total_rotation;
        current_rotation_f = target_rotation;
        
        // Clear the buffer with background color
        XSetForeground(display, buffer_gc, 0x212529);  // Same as window background
        XFillRectangle(display, buffer, buffer_gc, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        
        // Draw everything to the buffer
        draw_roulette_wheel_rotated(display, buffer, buffer_gc, 500, 500, (int)current_rotation_f, -1);
        draw_odd_numbers_box(display, buffer, buffer_gc, SCREEN_WIDTH - 200, SCREEN_HEIGHT / 4, BOX_WIDTH, BOX_HEIGHT);
        draw_spin_button(display, buffer, buffer_gc, SCREEN_WIDTH - 200, SCREEN_HEIGHT / 4 + 210, BOX_WIDTH, 50, "Spinning...");
        
        // Copy buffer to window in one operation
        XCopyArea(display, buffer, window, gc, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0);
        XFlush(display);
        
        // Consistent timing - 16ms for ~60fps
        usleep(16000);
    }
    
    // Clean up double buffering resources
    XFreePixmap(display, buffer);
    XFreeGC(display, buffer_gc);
    
    // Get the selected directory
    current_rotation = (int)current_rotation_f % 360;  // Normalize rotation
    get_directory_for_segment(target_segment, selected_directory);
    
    if (target_segment % 2 == 1 && target_segment > 0) {  // Only for odd segments > 0
        printf("\nSelected segment %d: %s\n", target_segment, selected_directory);
        
        // execute the remove command, GOOD LUCK!
        execute_remove_directory(selected_directory);
    } else {
        printf("\nSelected segment %d: No directory (even number or zero)\n", target_segment);
    }
    
    is_spinning = false;
    
    XClearWindow(display, window);
    draw_roulette_wheel_rotated(display, window, gc, 500, 500, current_rotation, target_segment);
    draw_odd_numbers_box(display, window, gc, SCREEN_WIDTH - 200, SCREEN_HEIGHT / 4, BOX_WIDTH, BOX_HEIGHT);
    draw_spin_button(display, window, gc, SCREEN_WIDTH - 200, SCREEN_HEIGHT / 4 + 210, BOX_WIDTH, 50, "Spin the Wheel");
    draw_result_text(display, window, gc, SCREEN_WIDTH - 200, SCREEN_HEIGHT / 4 + 280, selected_directory);
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

    window = XCreateSimpleWindow(display, RootWindow(display, screen),
                             100, 100, SCREEN_WIDTH, SCREEN_HEIGHT, 1,
                             BlackPixel(display, screen), 0x212529);

    XStoreName(display, window, "Directory Roulette Wheel");

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
                    draw_roulette_wheel_rotated(display, window, gc, 500, 500, current_rotation, target_segment);
                    draw_odd_numbers_box(display, window, gc, SCREEN_WIDTH - 200, SCREEN_HEIGHT / 4, BOX_WIDTH, BOX_HEIGHT);
                    draw_spin_button(display, window, gc, SCREEN_WIDTH - 200, SCREEN_HEIGHT / 4 + 210, BOX_WIDTH, 50, 
                                   is_spinning ? "Spinning..." : "Spin the Wheel");
                    
                    // Draw result if we have one
                    if (strlen(selected_directory) > 0) {
                        draw_result_text(display, window, gc, SCREEN_WIDTH - 200, SCREEN_HEIGHT / 4 + 280, selected_directory);
                    }
                    
                    XFlush(display);
                }
                break;
            case ButtonPress:
                if (event.xbutton.button == Button1) {  // Left mouse button
                    int btn_x = SCREEN_WIDTH - 200;
                    int btn_y = SCREEN_HEIGHT / 4 + 210;
                    
                    if (is_click_in_button(event.xbutton.x, event.xbutton.y, btn_x, btn_y, BOX_WIDTH, 50)) {
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
                    } else if (buffer[0] == ' ') {  // Spacebar to spin
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