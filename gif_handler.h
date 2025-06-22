#ifndef GIF_HANDLER_H
#define GIF_HANDLER_H

#include <X11/Xlib.h>
#include <gif_lib.h>

typedef struct {
    GifFileType* gif_file;
    int frame_count;
    int current_frame;
    XImage** frames;
    int* delays;  // Delay for each frame in centiseconds
    int width;
    int height;
    unsigned long last_frame_time;
} AnimatedGif;

// Function declarations
AnimatedGif* load_animated_gif(Display* display, const char* filename);
void draw_animated_gif(Display* display, Window window, GC gc, AnimatedGif* gif, int x, int y);
void update_gif_frame(AnimatedGif* gif);
void free_animated_gif(Display* display, AnimatedGif* gif);

#endif