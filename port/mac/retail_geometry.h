#ifndef HALO_MAC_RETAIL_GEOMETRY_H
#define HALO_MAC_RETAIL_GEOMETRY_H
#include "retail_cache.h"
struct mac_retail_vertex { float position[3], normal[3], binormal[3], tangent[3], uv[2]; };
struct mac_retail_light_vertex { float normal[3], uv[2]; };
struct mac_retail_material {
    uint32_t shader_handle, vertex_count, light_vertex_count, triangle_count;
    int32_t lightmap_bitmap;
    const struct mac_retail_vertex *vertices;
    const struct mac_retail_light_vertex *light_vertices;
    const uint16_t *indices;
};
/* Callback data is native, read-only, valid only during callback. All disk
   geometry is validated before the first callback. Callback failure can
   follow previous callbacks; callers must discard partial uploads on failure. */
typedef int (*mac_retail_material_proc)(void *, const struct mac_retail_material *);
int mac_retail_bsp_geometry(const struct mac_retail_cache *, uint32_t bsp_index,
    mac_retail_material_proc, void *context, const char **error);
#endif
