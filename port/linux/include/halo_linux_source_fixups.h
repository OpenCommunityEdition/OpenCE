/*
HALO_LINUX_SOURCE_FIXUPS.H

Game-only workarounds for source that MSVC accepts but clang rejects, where
editing the source itself would change the byte-matched MSVC output (see
port/linux/README.md for how each was checked).
*/

#ifndef __HALO_LINUX_SOURCE_FIXUPS_H
#define __HALO_LINUX_SOURCE_FIXUPS_H

/* rasterizer.h declares rasterizer_debug_drawing_begin(boolean opaque) while
rasterizer_xbox_debug.h declares a second `long zbias` parameter, and
rasterizer_debug.c includes both and passes two arguments. MSVC tolerates
the mismatch; the definition ignores zbias. Adding the parameter to
rasterizer.h perturbs MSVC's register allocation elsewhere, so instead every
declaration and call collapses to the one-parameter form here. */
#define rasterizer_debug_drawing_begin(opaque, ...) (rasterizer_debug_drawing_begin)(opaque)

#ifdef HALO_ANDROID
/* the screen at the device's aspect ratio (port/linux/src/d3d8_gl.c) */
long halo_android_screen_width(void);
/* while TRUE, drawing shifts right to center 640-column layouts */
void halo_android_ui_offset(unsigned char centered);
#endif

#endif
