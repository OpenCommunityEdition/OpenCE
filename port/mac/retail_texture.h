#ifndef HALO_MAC_RETAIL_TEXTURE_H
#define HALO_MAC_RETAIL_TEXTURE_H
#include "retail_cache.h"
struct mac_retail_bitmap {
    uint32_t width, height, format, external, file_offset, stored_bytes, level_bytes;
};
/* 2D base level only. Swizzled/paletted/volume/cube inputs are rejected. */
int mac_retail_bitmap_get(const struct mac_retail_cache *, uint32_t handle,
    uint32_t index, struct mac_retail_bitmap *out);
/* Diagnostic base-map selection: senv or a bounded schi first-map subset.
   Does not implement complete shader lighting/blending/animation semantics. */
int mac_retail_base_bitmap(const struct mac_retail_cache *, uint32_t shader, uint32_t *bitmap);
/* schi's source-defined first layer with only zero offset/rotation and finite
   scales. Later maps and Chicago composition remain unsupported. */
int mac_retail_chicago_base_bitmap(const struct mac_retail_cache *, uint32_t shader,
    uint32_t *bitmap, float *u_scale, float *v_scale);
/* Model shader base-map subset. Reads the checked soso model map, validates
   its bitmap reference and finite UV scales; this is not full shader support. */
int mac_retail_model_base_bitmap(const struct mac_retail_cache *, uint32_t shader,
    uint32_t *bitmap, float *u_scale, float *v_scale);
/* Exact base-level input and RGBA8 output sizes; no native packed pixels.
   Input and output buffers must not overlap. */
int mac_retail_texture_decode(uint32_t format, uint32_t width, uint32_t height,
    const unsigned char *source, size_t source_bytes, unsigned char *rgba, size_t rgba_bytes);
#endif
