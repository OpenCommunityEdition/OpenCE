#ifndef HALO_MAC_RUNTIME_VIEW_H
#define HALO_MAC_RUNTIME_VIEW_H
#include "mac_bsp_renderer.h"
#include "mac_input.h"
#include "retail_cache.h"

enum mac_runtime_view_mode {
    MAC_RUNTIME_VIEW_HIDDEN=0,
    MAC_RUNTIME_VIEW_VISIBLE=1
};
struct mac_runtime_view { void *state; };
struct mac_runtime_view_frame {
    struct mac_input_frame input;
    unsigned scene_pixels,frame_number;
    uint32_t pixel_digest;
};
typedef int (*mac_runtime_view_overlay_proc)(void *,struct mac_bsp_renderer *);

/* Diagnostic bridge only: the caller supplies engine camera state each frame;
   the bridge does not run movement/gameplay or Xbox rasterizer callbacks.
   Hidden mode is enum value zero; visible mode must be explicitly selected.
   Zero-initialize the wrapper before open. Keep the cache/file alive until close. */
int mac_runtime_view_open(struct mac_runtime_view *,const struct mac_retail_cache *,
    uint32_t bsp_index,const char *bitmap_path,unsigned width,unsigned height,
    enum mac_runtime_view_mode mode,const char *title,const char **error);
int mac_runtime_view_draw(struct mac_runtime_view *,const struct mac_bsp_camera *,
    struct mac_runtime_view_frame *,const char **error);
/* Optional diagnostic overlay; invoked after the BSP with its camera matrices
   and current compatibility GL context, before readback. A NULL callback
   clears it. The context must remain alive until cleared or the view closes. */
int mac_runtime_view_set_overlay(struct mac_runtime_view *,mac_runtime_view_overlay_proc,void *);
/* Writes the last completed frame as a bottom-up-corrected PPM screenshot. */
int mac_runtime_view_write_ppm(const struct mac_runtime_view *,const char *path);
int mac_runtime_view_is_visible(const struct mac_runtime_view *);
int mac_runtime_view_is_accelerated(const struct mac_runtime_view *);
const char *mac_runtime_view_renderer(const struct mac_runtime_view *);
void mac_runtime_view_close(struct mac_runtime_view *);

#endif
