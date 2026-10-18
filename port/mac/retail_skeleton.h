#ifndef HALO_MAC_RETAIL_SKELETON_H
#define HALO_MAC_RETAIL_SKELETON_H
#include "retail_cache.h"
#define MAC_RETAIL_MAX_MODEL_NODES 64
struct mac_retail_model_node {
    char name[32];
    int16_t sibling, child, parent;
    float translation[3], rotation[4], distance;
    /* Source real_matrix4x3 order: scale, forward[3], left[3], up[3], position[3]. */
    float inverse[13];
};
struct mac_retail_skeleton {
    uint32_t model_handle, checksum, count;
    struct mac_retail_model_node nodes[MAC_RETAIL_MAX_MODEL_NODES];
};
/* The cache must be a validated, immutable mac_retail_open view. Only mod2
   skeleton data is decoded, not its geometry or any animation graph. Output
   is zero on failure. Float bits are preserved; no normalization/recalculation. */
int mac_retail_skeleton_decode(const struct mac_retail_cache *, uint32_t model_handle,
    struct mac_retail_skeleton *, const char **error);
/* Resolve the single globals tag's multiplayer unit and its model. Salted
   handles and groups are checked at each step; no fixed map-specific indices. */
int mac_retail_multiplayer_model(const struct mac_retail_cache *,
    uint32_t *biped_handle, uint32_t *model_handle, const char **error);
#endif
