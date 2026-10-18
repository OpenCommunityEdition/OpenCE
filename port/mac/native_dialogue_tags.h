#ifndef HALO_MAC_NATIVE_DIALOGUE_TAGS_H
#define HALO_MAC_NATIVE_DIALOGUE_TAGS_H
#include "native_object_tags.h"
/* Converts the full 0x1010 dialogue definition. Retail zero-filled enum-gap
   references are normalized to NONE; live vocalization dependencies remain
   sound-tag handles for a separate owner to resolve. */
int mac_native_dialogue_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **error);
#endif
