#ifndef HALO_MAC_NATIVE_ANIMATION_TAGS_H
#define HALO_MAC_NATIVE_ANIMATION_TAGS_H
#include "retail_cache.h"
struct animation_graph;
/* Sound references carry salted 32-bit tag handles. The visitor receives
   their bit pattern unchanged; NONE (-1) references are skipped. */
typedef int (*mac_native_animation_sound_visitor)(void *,uint32_t group,uint32_t handle);
/* Complete owned native graph for supported uncompressed unit, weapon and
   vehicle families. Unsupported nonempty device families fail. */
struct mac_native_animation_tag {
    void *root,*owner;
    uint32_t handle,group;
    size_t root_bytes,bytes_owned;
};
int mac_native_animation_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_animation_tag *,const char **);
int mac_native_animation_visit_sounds(const struct animation_graph *,
    mac_native_animation_sound_visitor,void *context);
void mac_native_animation_owner_free(void *);
void mac_native_animation_tag_free(struct mac_native_animation_tag *);
#endif
