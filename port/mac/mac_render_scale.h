#ifndef HALO_MAC_RENDER_SCALE_H
#define HALO_MAC_RENDER_SCALE_H
/* mac.render_resolution: how many pixels the game draws a frame with, and
   what limits it. Pure functions, tested without a graphics card
   (tools/mac_render_scale_probe.c).

   The game's picture is 480 lines by a width that follows the window's
   shape (mac_window.c). The renderer draws it at a scale: 1 is the Xbox's
   pixels, 1.6667 is 800 lines. The picture is drawn into the lower left of
   the window's framebuffer, as large as the window has pixels for, and
   scaled to the window when it is presented (engine_device_state.c). */
#include <string.h>
#include <stdlib.h>
#include "mac_gpu_pick.h"

#define MAC_RENDER_LINES_AUTO (-1)
#define MAC_RENDER_LINES_NATIVE 0
#define MAC_RENDER_LINES_MINIMUM 480
#define MAC_RENDER_LINES_MAXIMUM 4320

/* The setting's words: "auto", "native", "original" (480 lines), a number of
   lines ("720", "720p") or a size whose height is used ("1280x720"). Anything
   else is "auto". */
static __inline__ long mac_render_parse(const char *text)
{
    const char *at;
    long lines;
    if (!text || !text[0] || !strcmp(text, "auto")) return MAC_RENDER_LINES_AUTO;
    if (!strcmp(text, "native")) return MAC_RENDER_LINES_NATIVE;
    if (!strcmp(text, "original")) return MAC_RENDER_LINES_MINIMUM;
    at = strchr(text, 'x');
    at = at ? at + 1 : text;
    lines = atol(at);
    if (lines <= 0) return MAC_RENDER_LINES_AUTO;
    if (lines < MAC_RENDER_LINES_MINIMUM) lines = MAC_RENDER_LINES_MINIMUM;
    if (lines > MAC_RENDER_LINES_MAXIMUM) lines = MAC_RENDER_LINES_MAXIMUM;
    return lines;
}

/* The pixels a frame may have for a card's memory: the framebuffer's colour
   and depth, and the copy the present reads, twelve bytes a pixel, in half
   of the card's memory. */
static __inline__ unsigned long mac_render_pixel_budget(int vram_mb)
{
    return vram_mb > 0 ? (unsigned long)vram_mb * 43690UL : 0UL;
}

static __inline__ float mac_render_sqrt(float x)
{
    float guess = x > 1.0f ? x * 0.5f : 1.0f;
    int step;
    if (!(x > 0.0f)) return 0.0f;
    for (step = 0; step < 24; ++step) guess = 0.5f * (guess + x / guess);
    return guess;
}

enum { MAC_RENDER_CLAMP_WINDOW = 1, MAC_RENDER_CLAMP_SIZE = 2, MAC_RENDER_CLAMP_MEMORY = 4 };

/* The scale for the lines wanted (MAC_RENDER_LINES_NATIVE: the largest the
   window holds) of a picture picture_width by 480 in a window of window_width
   by window_height pixels, with the card's largest texture or viewport
   (max_dimension, 0: unknown) and the pixels its memory allows (max_pixels,
   0: unknown). Never below 1. What cut the choice short is set in *clamped
   (MAC_RENDER_CLAMP_*) and the choice stays in the setting's list. */
static __inline__ float mac_render_scale_choose(long lines, unsigned picture_width, unsigned window_width,
    unsigned window_height, unsigned max_dimension, unsigned long max_pixels, int *clamped)
{
    float fit, scale, limit;
    int flags = 0;
    if (!picture_width || !window_width || !window_height) {
        if (clamped) *clamped = 0;
        return 1.0f;
    }
    fit = (float)window_height / 480.0f;
    limit = (float)window_width / (float)picture_width;
    if (limit < fit) fit = limit;
    scale = lines > 0 ? (float)lines / 480.0f : fit;
    if (scale > fit) { scale = fit; flags |= MAC_RENDER_CLAMP_WINDOW; }
    if (max_dimension) {
        limit = (float)max_dimension / (float)(picture_width > 480u ? picture_width : 480u);
        if (scale > limit) { scale = limit; flags |= MAC_RENDER_CLAMP_SIZE; }
    }
    if (max_pixels) {
        limit = mac_render_sqrt((float)max_pixels / ((float)picture_width * 480.0f));
        if (scale > limit) { scale = limit; flags |= MAC_RENDER_CLAMP_MEMORY; }
    }
    if (scale < 1.0f) scale = 1.0f;
    /* (a clamp by the window alone is no news: native is the window) */
    if (clamped) *clamped = lines > 0 ? flags : (flags & ~MAC_RENDER_CLAMP_WINDOW);
    return scale;
}

/* The picture in pixels at a scale */
static __inline__ unsigned mac_render_pixels(unsigned units, float scale)
{
    return (unsigned)((float)units * scale + 0.5f);
}
#endif
