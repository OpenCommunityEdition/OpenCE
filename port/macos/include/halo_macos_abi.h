/*
HALO_MACOS_ABI.H

What the macOS port's guest and host agree on beyond the Android contract
(port/android/include/halo_android_abi.h, which both ports share): where
the guest's 4 GB lies in the host, and the structures of the desktop
services the macOS host adds (port/macos/guest/guest_host_macos.h). As
there, only fixed-width members cross the boundary.

This header is included by both halves.
*/

#ifndef __HALO_MACOS_ABI_H
#define __HALO_MACOS_ABI_H

#include <stdint.h>

/* The guest's 4 GB: guest address A is host address base + A, where base
is a multiple of 4 GB (port/macos/host/host_memory.c). Guest code keeps
base in x28 and uses x27 as scratch (tools/guest_asm_rebase.py). */
#define HALO_MACOS_GUEST_SPAN 0x100000000ull

/* a display mode (SDL_DisplayMode without its pointer) */
struct halo_macos_display_mode
{
	uint32_t display;
	uint32_t format;
	int32_t width;
	int32_t height;
	float pixel_density;
	float refresh_rate;
	int32_t refresh_numerator;
	int32_t refresh_denominator;
};

/* the most modes host_sdl_fullscreen_display_modes reports */
#define HALO_MACOS_DISPLAY_MODE_MAXIMUM 64

/* a message box's button (SDL_MessageBoxButtonData, its text a guest
pointer) */
struct halo_macos_message_box_button
{
	uint32_t flags;
	int32_t id;
	uint32_t text;
};

#endif
