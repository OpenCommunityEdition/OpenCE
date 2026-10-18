#ifndef HALO_MAC_NATIVE_COLLISION_TAGS_H
#define HALO_MAC_NATIVE_COLLISION_TAGS_H
#include "retail_cache.h"
struct mac_native_collision_tag {
    void *root,*owner;
    uint32_t handle;
    size_t root_bytes,bytes_owned;
};
/* Produces an actual engine collision_model with all semantic resistance,
   material, region/permutation, sphere, node and collision BSP fields native.
   All pointers are owned allocations; no portable structure is published.
   Unused/padding fields and block definition pointers are zero. Effects retain
   salted handles with owned canonical tag names; effect payloads remain external.
   Nonempty obsolete modifiers are rejected by the portable decoder. Dependency
   closure and matching the collision node count to an object's model remain
   loading responsibilities. Free before reuse. Output empty on failure. */
int mac_native_collision_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_collision_tag *,const char **error);
void mac_native_collision_owner_free(void *);
void mac_native_collision_tag_free(struct mac_native_collision_tag *);
#endif
