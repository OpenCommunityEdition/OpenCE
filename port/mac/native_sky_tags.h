#ifndef HALO_MAC_NATIVE_SKY_TAGS_H
#define HALO_MAC_NATIVE_SKY_TAGS_H
#include "retail_cache.h"
#include "native_tags.h"
#include <stddef.h>
#include <stdint.h>

/* Owned native `sky ` root and its populated light records. Referenced model,
   lens-flare and fog-screen tags remain salted handles and must be loaded by
   their own providers before rendering consumes them. */
struct mac_native_sky_tag {
    void *root,*owner;
    size_t root_bytes,bytes_owned;
    uint32_t handle;
};
int mac_native_sky_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_sky_tag *,const char **);
void mac_native_sky_tag_free(struct mac_native_sky_tag *);
/* Publish transfers ownership only on success. This does not load the model,
   animation graph, fog screen or lens flares. */
int mac_native_sky_publish(const struct mac_retail_cache *,struct mac_native_tags *,
    uint32_t,const char **);
#endif
