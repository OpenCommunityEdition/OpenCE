#ifndef HALO_MAC_NATIVE_ANTENNA_TAGS_H
#define HALO_MAC_NATIVE_ANTENNA_TAGS_H
#include "retail_cache.h"
#include "objects/widgets/antenna.h"
struct point_physics_definition;
struct mac_native_antenna_tag {
    struct antenna_definition *root;
    void *owner;
    uint32_t handle,group;
    size_t root_bytes,bytes_owned;
};
struct mac_native_point_physics_tag {
    struct point_physics_definition *root;
    void *owner;
    uint32_t handle,group;
    size_t root_bytes,bytes_owned;
};
/* Source layouts are witnessed by antenna.h: root 0xd0, vertex 0x80;
   point_physics.h: root 0x40. Referenced bitmap/physics records are decoded
   separately and must be published before callers dereference them. */
int mac_native_antenna_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_antenna_tag *,const char **);
int mac_native_point_physics_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_point_physics_tag *,const char **);
void mac_native_antenna_owner_free(void *);
void mac_native_antenna_tag_free(struct mac_native_antenna_tag *);
void mac_native_point_physics_owner_free(void *);
void mac_native_point_physics_tag_free(struct mac_native_point_physics_tag *);
#endif
