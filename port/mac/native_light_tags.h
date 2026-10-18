#ifndef HALO_MAC_NATIVE_LIGHT_TAGS_H
#define HALO_MAC_NATIVE_LIGHT_TAGS_H
#include "retail_cache.h"
struct mac_native_light_tag {
    void *root,*owner;
    uint32_t handle,group,field_digest;
    size_t root_bytes,bytes_owned;
};
/* Complete 0x160 native light storage; referenced bitmap/lens tags must be
   published separately before consumers dereference them. */
int mac_native_light_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_light_tag *,const char **);
void mac_native_light_owner_free(void *);
void mac_native_light_tag_free(struct mac_native_light_tag *);
int mac_native_light_visit_references(const struct mac_native_light_tag *,
    int (*visit)(void *,uint32_t,uint32_t),void *context);
#endif
