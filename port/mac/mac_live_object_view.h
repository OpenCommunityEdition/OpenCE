#ifndef HALO_MAC_LIVE_OBJECT_VIEW_H
#define HALO_MAC_LIVE_OBJECT_VIEW_H
#include <stdint.h>
#define MAC_INPUT_NO_CARBON
#include "mac_runtime_view.h"
#include "mac_object_render_probe.h"

struct mac_live_object_view_result {
    uint32_t terrain_digest,overlay_digest;
    unsigned biped_parts,biped_vertices,biped_textured_parts,biped_unsupported_shaders,biped_unsupported_semantics;
    unsigned weapon_parts,weapon_vertices,weapon_textured_parts,weapon_unsupported_shaders,weapon_unsupported_semantics;
};

/* Render the current live biped and held weapon over a terrain frame from the
   caller's unchanged camera. Models use real node matrices and renderer-owned
   textures; unsupported shader composition remains counted. */
int mac_live_object_view_draw(struct mac_runtime_view *,long,const struct mac_bsp_camera *,
    struct mac_live_object_view_result *,const char **);
#endif
