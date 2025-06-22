#include "gif_handler.h"
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

// Get current time in milliseconds
unsigned long get_current_time_ms()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

// Convert GIF color to X11 pixel value
unsigned long gif_color_to_pixel(Display* display, GifColorType* color)
{
    int screen = DefaultScreen(display);
    XColor xcolor;
    xcolor.red = color->Red << 8;
    xcolor.green = color->Green << 8;
    xcolor.blue = color->Blue << 8;
    xcolor.flags = DoRed | DoGreen | DoBlue;
    
    if (XAllocColor(display, DefaultColormap(display, screen), &xcolor))
    {
        return xcolor.pixel;
    }
    return BlackPixel(display, screen);
}

AnimatedGif* load_animated_gif(Display* display, const char* filename)
{
    int error;
    GifFileType* gif_file = DGifOpenFileName(filename, &error);
    
    if (gif_file == NULL)
    {
        printf("Error opening GIF file: %s\n", GifErrorString(error));
        return NULL;
    }
    
    if (DGifSlurp(gif_file) == GIF_ERROR)
    {
        printf("Error reading GIF file: %s\n", GifErrorString(gif_file->Error));
        DGifCloseFile(gif_file, &error);
        return NULL;
    }
    
    AnimatedGif* gif = malloc(sizeof(AnimatedGif));
    if (!gif)
    {
        DGifCloseFile(gif_file, &error);
        return NULL;
    }
    
    gif->gif_file = gif_file;
    gif->frame_count = gif_file->ImageCount;
    gif->current_frame = 0;
    gif->width = gif_file->SWidth;
    gif->height = gif_file->SHeight;
    gif->last_frame_time = get_current_time_ms();
    
    // Allocate arrays for frames and delays
    gif->frames = malloc(sizeof(XImage*) * gif->frame_count);
    gif->delays = malloc(sizeof(int) * gif->frame_count);
    
    if (!gif->frames || !gif->delays)
    {
        free(gif->frames);
        free(gif->delays);
        free(gif);
        DGifCloseFile(gif_file, &error);
        return NULL;
    }
    
    int screen = DefaultScreen(display);
    Visual* visual = DefaultVisual(display, screen);
    int depth = DefaultDepth(display, screen);
    
    // Background color, the same as the window background
    unsigned char bg_red = 0x21;
    unsigned char bg_green = 0x25;
    unsigned char bg_blue = 0x29;
    
    // Process each frame
    for (int i = 0; i < gif->frame_count; i++)
    {
        SavedImage* frame = &gif_file->SavedImages[i];
        
        // Get frame delay from extension
        gif->delays[i] = 10; // Default 100ms
        int transparent_index = -1; // Track transparent color index
        
        for (int j = 0; j < frame->ExtensionBlockCount; j++)
        {
            ExtensionBlock* ext = &frame->ExtensionBlocks[j];
            if (ext->Function == GRAPHICS_EXT_FUNC_CODE && ext->ByteCount >= 4)
            {
                gif->delays[i] = (ext->Bytes[2] << 8) | ext->Bytes[1];
                if (gif->delays[i] == 0)
                {
                    gif->delays[i] = 10;
                }
                
                // Check for transparency
                if (ext->Bytes[0] & 0x01) // Transparency flag
                {
                    transparent_index = ext->Bytes[3];
                }
            }
        }
        
        // Create XImage for this frame
        char* data = malloc(gif->width * gif->height * 4); // 32-bit RGBA
        if (!data)
        {
            continue;
        }
        
        ColorMapObject* colormap = frame->ImageDesc.ColorMap ? 
            frame->ImageDesc.ColorMap : gif_file->SColorMap;
        
        // Convert GIF data to X11 format
        for (int y = 0; y < gif->height; y++)
        {
            for (int x = 0; x < gif->width; x++)
            {
                int gif_index = y * gif->width + x;
                int data_index = (y * gif->width + x) * 4;
                
                if (gif_index < frame->ImageDesc.Width * frame->ImageDesc.Height)
                {
                    unsigned char color_index = frame->RasterBits[gif_index];
                    
                    // Check if this pixel is transparent
                    if (transparent_index >= 0 && color_index == transparent_index)
                    {
                        // Use window background color for transparent pixels
                        data[data_index] = bg_blue;      // Blue
                        data[data_index + 1] = bg_green; // Green
                        data[data_index + 2] = bg_red;   // Red
                        data[data_index + 3] = (unsigned char)255;      // Alpha
                    }
                    else if (colormap && color_index < colormap->ColorCount)
                    {
                        GifColorType* color = &colormap->Colors[color_index];
                        data[data_index] = color->Blue;
                        data[data_index + 1] = color->Green;
                        data[data_index + 2] = color->Red;
                        data[data_index + 3] = (unsigned char)255;
                    }
                    else
                    {
                        // Invalid color index - use background color
                        data[data_index] = bg_blue;
                        data[data_index + 1] = bg_green;
                        data[data_index + 2] = bg_red;
                        data[data_index + 3] = (unsigned char)255;
                    }
                }
                else
                {
                    // Outside frame bounds - use background color
                    data[data_index] = bg_blue;
                    data[data_index + 1] = bg_green;
                    data[data_index + 2] = bg_red;
                    data[data_index + 3] = (unsigned char)255;
                }
            }
        }
        
        gif->frames[i] = XCreateImage(display, visual, depth, ZPixmap, 0,
                                     data, gif->width, gif->height, 32, 0);
    }
    
    return gif;
}

void update_gif_frame(AnimatedGif* gif)
{
    if (!gif || gif->frame_count <= 1)
    {
        return;
    }
    
    unsigned long current_time = get_current_time_ms();
    unsigned long elapsed = current_time - gif->last_frame_time;
    
    // Check if it's time to advance to next frame (delays are in centiseconds)
    if (elapsed >= gif->delays[gif->current_frame] * 10)
    {
        gif->current_frame = (gif->current_frame + 1) % gif->frame_count;
        gif->last_frame_time = current_time;
    }
}

void draw_animated_gif(Display* display, Window window, GC gc, AnimatedGif* gif, int x, int y)
{
    if (!gif || !gif->frames || gif->current_frame >= gif->frame_count)
    {
        return;
    }
    
    update_gif_frame(gif);
    
    XImage* current_image = gif->frames[gif->current_frame];
    if (current_image)
    {
        XPutImage(display, window, gc, current_image, 0, 0, x, y, 
                 gif->width, gif->height);
    }
}

void free_animated_gif(Display* display, AnimatedGif* gif)
{
    if (!gif)
    {
        return;
    }
    
    if (gif->frames)
    {
        for (int i = 0; i < gif->frame_count; i++)
        {
            if (gif->frames[i])
            {
                XDestroyImage(gif->frames[i]);
            }
        }
        free(gif->frames);
    }
    
    if (gif->delays)
    {
        free(gif->delays);
    }
    
    if (gif->gif_file)
    {
        int error;
        DGifCloseFile(gif->gif_file, &error);
    }
    
    free(gif);
}
