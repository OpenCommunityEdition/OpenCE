#ifndef HALO_MAC_BSP_RENDERER_H
#define HALO_MAC_BSP_RENDERER_H
#include "retail_geometry.h"

struct mac_bsp_renderer;
struct mac_bsp_camera {
    double position[3];
    double yaw,pitch;
};
struct mac_bsp_renderer_stats {
    unsigned long vertices;
    uint32_t texture_digest;
    unsigned int texture_count,unsupported_materials;
};

/* Requires a current compatibility GL context for drawing and destruction.
   The cache and its source bytes must outlive this renderer. The optional
   bitmap file supplies real external texture bytes. */
struct mac_bsp_renderer *mac_bsp_renderer_open(const struct mac_retail_cache *,
    uint32_t bsp_index,const char *bitmap_path,const char **error);
int mac_bsp_renderer_draw_topdown(struct mac_bsp_renderer *,unsigned width,unsigned height);
int mac_bsp_renderer_draw_camera(struct mac_bsp_renderer *,unsigned width,unsigned height,
    const struct mac_bsp_camera *);
/* Resolve a model shader's real retail base bitmap through this renderer's
   existing decoded texture cache. Requires its bitmap resource file and a
   current GL context. Returns zero when the shader/bitmap family is not
   supported; on success binds texture unit zero and returns nonzero. */
int mac_bsp_renderer_bind_model_texture(struct mac_bsp_renderer *,uint32_t shader_handle,
    float *u_scale,float *v_scale);
void mac_bsp_renderer_get_stats(const struct mac_bsp_renderer *,struct mac_bsp_renderer_stats *);
void mac_bsp_renderer_close(struct mac_bsp_renderer *);

#endif
