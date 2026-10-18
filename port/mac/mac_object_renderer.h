#ifndef HALO_MAC_OBJECT_RENDERER_H
#define HALO_MAC_OBJECT_RENDERER_H
#include <stdint.h>
#include "real_math.h"
#include "mac_bsp_renderer.h"
struct model;

struct mac_object_renderer_stats {
    unsigned int regions,parts,drawn_parts,textured_parts,unsupported_shaders,unsupported_shader_semantics,invalid_parts;
    unsigned long submitted_vertices;
    uint32_t first_unsupported_shader,first_unsupported_group;
    uint32_t first_semantic_shader,first_semantic_group;
};

/* Draw one already-created engine object using its actual region selections
   and current world node matrices. The caller supplies
   object_get_node_matrices(index), not model defaults. Geometry is the
   model's highest-detail permutation slot. This diagnostic supports opaque
   base-map draws only: Xbox shader animation, lighting, alpha and detail
   passes are not emulated. Unsupported base textures are visibly magenta and
   counted separately from shader families whose base map is drawn but whose
   composition is omitted. Requires the renderer's bitmap resource and current
   compatibility GL context. */
int mac_object_renderer_draw_instance(struct mac_bsp_renderer *,const struct model *,
    const byte *region_permutations,unsigned int permutation_count,
    const real_matrix4x3 *node_matrices,unsigned int matrix_count,
    struct mac_object_renderer_stats *);
real mac_object_renderer_decode_uv(short);

#endif
