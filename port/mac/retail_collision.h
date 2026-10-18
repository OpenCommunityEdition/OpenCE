#ifndef HALO_MAC_RETAIL_COLLISION_H
#define HALO_MAC_RETAIL_COLLISION_H
#include "retail_cache.h"
struct mac_collision_node { uint32_t plane, child[2]; };
struct mac_collision_plane { float n[3], d; };
struct mac_collision_leaf { uint16_t flags, count; uint32_t first; };
struct mac_collision_reference { uint32_t plane, root; };
struct mac_collision_node2 { float plane[3]; uint32_t child[2]; };
struct mac_collision_surface { uint32_t plane, edge; uint8_t flags, breakable; int16_t material; };
struct mac_collision_edge { uint32_t vertex[2], edge[2], surface[2]; };
struct mac_collision_vertex { float p[3]; uint32_t edge; };
struct mac_retail_collision {
    uint32_t count[8];
    struct mac_collision_node *nodes;
    struct mac_collision_plane *planes;
    struct mac_collision_leaf *leaves;
    struct mac_collision_reference *references;
    struct mac_collision_node2 *nodes2;
    struct mac_collision_surface *surfaces;
    struct mac_collision_edge *edges;
    struct mac_collision_vertex *vertices;
};
/* Owned native arrays. Free after use; output must not already own allocations.
   Validates spans, finite floats, indices and surface rings before success. */
int mac_retail_collision_decode(const struct mac_retail_cache *, uint32_t bsp,
    struct mac_retail_collision *, const char **error);
void mac_retail_collision_free(struct mac_retail_collision *);
/* Engine bsp3d_test_point traversal: >=0 leaf, -1 solid, -2 bad query/cycle.
   Caller must retain unmodified decoded arrays. No object collision is tested. */
int32_t mac_retail_collision_leaf(const struct mac_retail_collision *, const float p[3]);
/* Diagnostic vertical point ray over static convex collision polygons. Returns
   nearest upward-facing surface in [bottom, top], or -1. Not engine capsule
   physics: no objects, moving/broken surfaces, step-up or wall response. */
int32_t mac_retail_collision_ground(const struct mac_retail_collision *,
    float x, float y, float top, float bottom, float *height);
#endif
