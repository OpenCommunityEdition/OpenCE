#ifndef HALO_MAC_GPU_INFO_H
#define HALO_MAC_GPU_INFO_H
#include "mac_gpu_pick.h"

/* The detail a machine draws (the values the detail settings take when the
   user left them at their defaults) */
struct mac_detail_preset
{
    int specular, shadows, decals;
    /* 0 high, 1 low, 2 off */
    int particles;
    double scenery_pixels;
    /* objects other than scenery and units smaller on screen than this many pixels are
       left out (0: none), models are drawn at the level of detail of this fraction of
       their size (1: as the original), and whether the sun's lens flares are drawn */
    double object_pixels, lod_scale;
    int lens_flares;
    /* at low particles, the effects' particles no wider than this many pixels on screen
       (0: no limit) and how the wider are made less of (render_detail.c) */
    double particle_pixels;
    int particle_mode;
};

/* The video memory in megabytes, asked of the system (CGL) and else read
   from the renderer's name; 32 if neither says. */
int mac_gpu_info_vram_mb(void);
/* The renderer's name, or "" */
const char *mac_gpu_info_renderer(void);
int mac_gpu_info_is_ppc(void);
/* The map being played (its name, without .map): the heavy maps change what
   "auto" picks */
void mac_gpu_info_set_map(const char *name);
int mac_gpu_info_campaign(void);
/* Whether mac.detail (auto, high, low, lowest) picks a preset other than
   the stock detail; fills preset if so. */
int mac_gpu_info_detail(struct mac_detail_preset *preset);
/* Changes when the map or the machine's answer does, for the detail's users */
unsigned long mac_gpu_info_serial(void);
/* mac.texture_cache_mb and mac.texture_skip_levels, auto resolved: 0 if the
   setting is not available */
long mac_gpu_info_texture_cache_mb(void);
int mac_gpu_info_texture_skip_levels(void);

/* mac.adaptive_detail: the frame-time governor (mac_adaptive_detail.h). The loop reports each
   frame; the renderer reads the level (0: none of the ladder's rungs in force; render_detail.c
   applies them) and the serial, which changes when the level or the setting does. */
int mac_adaptive_detail_enabled(void);
int mac_adaptive_detail_level(void);
unsigned long mac_adaptive_detail_serial(void);
/* the rungs (MAC_ADAPT_*, as bits) the renderer cannot or need not apply */
void mac_adaptive_detail_set_skipped(unsigned mask);
/* whether the last rung is in force: the frame is drawn with 480 lines */
int mac_adaptive_detail_resolution_capped(void);
void mac_adaptive_detail_note_frame(double seconds, double wait_seconds);
void mac_adaptive_detail_new_map(void);
/* the run's summary into the log, if the governor was on */
void mac_adaptive_detail_summary(void);
#endif
