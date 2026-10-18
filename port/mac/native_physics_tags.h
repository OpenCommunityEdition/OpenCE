#ifndef HALO_MAC_NATIVE_PHYSICS_TAGS_H
#define HALO_MAC_NATIVE_PHYSICS_TAGS_H
#include "retail_cache.h"
struct mac_native_physics_tag {
    void *root,*owner;
    uint32_t handle;
    size_t root_bytes,bytes_owned;
};
/* Endian-correct, owned physics_definition with its inertial, powered mass,
   and mass-point blocks. Checks bounds, finite scalars, friction enums, and
   powered-mass-point indices. Object/model-node compatibility is checked by
   the vehicle asset loader. */
int mac_native_physics_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_physics_tag *,const char **error);
void mac_native_physics_owner_free(void *);
void mac_native_physics_tag_free(struct mac_native_physics_tag *);
#endif
