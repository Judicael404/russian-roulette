#include "ui.h"
#include "constants.h"
#include "dirlist.h"
#include "wheel.h"

#include <X11/Xlib.h>
#include <stdio.h>
#include <string.h>

bool is_spinning = false;
int current_rotation = 0;
int target_segment = -1;
char selected_directory[256] = {0};

AnimatedGif* animated_gif = NULL;
Pixmap back_buffer = 0;
GC buffer_gc = 0;
bool buffer_initialized = false;

int screen_width = 800;
int screen_height = 600;

// spin button
void draw_spin_button(Display *dpy, Window win, GC gc, int x, int y, int width, int height, const char* label)
{
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

void draw_box_text(Display *dpy, Window win, GC gc, int x, int y, const char* text)
{
    XDrawString(dpy, win, gc, x, y, text, strlen(text));
}

void draw_result_text(Display *dpy, Window win, GC gc, int x, int y, const char* result)
{
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

void draw_odd_numbers_box(Display *dpy, Window win, GC gc, int x, int y, int width, int height)
{
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

    if (count < 0)
    {
        printf("error reading directories\n");
        return; // Error reading directories
    }

    // Add title
    draw_box_text(dpy, win, gc, tx, ty, "Odd Numbers:");
    ty += line_height + 5;

    int dir_idx = 0;
    for (int i = 1; i < NUM_SEGMENTS && dir_idx < count; i += 2, dir_idx++)
    {
        char buf[1024];
        snprintf(buf, sizeof(buf), "%d: %s", i, dirs[dir_idx]);
        draw_box_text(dpy, win, gc, tx, ty, buf);
        ty += line_height;
        if (ty > y + height - line_height)
        {
            break;
        }
    }
    
    // Free allocated memory
    free_directories(dirs, count);
}

// Function to initialize double buffering
void initialize_double_buffer(Display* display, Window window)
{
    if (buffer_initialized)
    {
        return;
    }
    
    int screen = DefaultScreen(display);
    back_buffer = XCreatePixmap(display, window, screen_width, screen_height, 
                               DefaultDepth(display, screen));
    buffer_gc = XCreateGC(display, back_buffer, 0, NULL);
    buffer_initialized = true;
}

// Function to cleanup double buffering
void cleanup_double_buffer(Display* display)
{
    if (buffer_initialized)
    {
        XFreePixmap(display, back_buffer);
        XFreeGC(display, buffer_gc);
        buffer_initialized = false;
    }
}

// Function to check if click is within button bounds
bool is_click_in_button(int click_x, int click_y, int btn_x, int btn_y, int btn_width, int btn_height)
{
    return (click_x >= btn_x && click_x <= btn_x + btn_width &&
            click_y >= btn_y && click_y <= btn_y + btn_height);
}

// Function to draw all UI elements to a buffer
void draw_all_ui_to_buffer(Display* display, Drawable drawable, GC gc)
{
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
    if (strlen(selected_directory) > 0)
    {
        int result_y = button_y + 50 + spacing;
        draw_result_text(display, drawable, gc, box_x, result_y, selected_directory);
    }
    
    // Draw animated GIF to the right of the box/button area
    if (animated_gif)
    {
        draw_animated_gif(display, drawable, gc, animated_gif, gif_x, gif_y);
    }
}

// Function to draw all UI elements (now uses double buffering)
void draw_all_ui(Display* display, Window window, GC gc)
{
    if (!buffer_initialized)
    {
        initialize_double_buffer(display, window);
    }
    
    // Draw everything to the back buffer
    draw_all_ui_to_buffer(display, back_buffer, buffer_gc);
    
    // Copy the entire back buffer to the window in one operation
    XCopyArea(display, back_buffer, window, gc, 0, 0, screen_width, screen_height, 0, 0);
}