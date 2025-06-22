#ifndef UI_H
#define UI_H

#include "gif_handler.h"
#include <X11/Xlib.h>

// Global variables for spinning state
extern bool is_spinning;
extern int current_rotation;
extern int target_segment;
extern char selected_directory[256];

// Global GIF variable and double buffering
extern AnimatedGif* animated_gif;
extern Pixmap back_buffer;
extern GC buffer_gc;
extern bool buffer_initialized;

// Global variables for screen dimensions
extern int screen_width;
extern int screen_height;

void draw_odd_numbers_box(Display *dpy, Window win, GC gc, int x, int y, int width, int height);
void draw_spin_button(Display *dpy, Window win, GC gc, int x, int y, int width, int height, const char* label);
void draw_result_text(Display *dpy, Window win, GC gc, int x, int y, const char* result);
void initialize_double_buffer(Display* display, Window window);
void cleanup_double_buffer(Display* display);
bool is_click_in_button(int click_x, int click_y, int btn_x, int btn_y, int btn_width, int btn_height);
// Function to draw all UI elements to a buffer
void draw_all_ui_to_buffer(Display* display, Drawable drawable, GC gc);
// Function to draw all UI elements (uses double buffering)
void draw_all_ui(Display* display, Window window, GC gc);
#endif