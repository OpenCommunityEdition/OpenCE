#ifndef HALO_MAC_DISPLAY_MODES_H
#define HALO_MAC_DISPLAY_MODES_H
/* Which of a display's modes Video Setup offers for fullscreen (the
   resolution list, engine_display.c), pure so that it is checked without a
   display (tools/mac_display_probe.c). */

/* The game's least picture is 640x480. An interlaced mode flickers, and one
   the system does not call safe for the hardware (an unsupported timing)
   can leave the panel blank; a mode of fewer than 24 bits a pixel has no
   32-bit color for the frame buffers. */
static __inline__ int mac_display_mode_usable(long width, long height, long bits_per_pixel, int safe_for_hardware,
    int interlaced)
{
    if (width < 640 || height < 480) return 0;
    if (width > 16384 || height > 16384) return 0;
    if (!safe_for_hardware || interlaced) return 0;
    if (bits_per_pixel > 0 && bits_per_pixel < 24) return 0;
    return 1;
}

/* Widescreen modes are those wider than 3:2 (16:10, 16:9 and beyond), the
   others 4:3 and 5:4: the Video Setup list of one or the other follows
   Widescreen On or Off */
static __inline__ int mac_display_mode_is_wide(long width, long height)
{
    return height > 0 && width * 2 > height * 3;
}
#endif
