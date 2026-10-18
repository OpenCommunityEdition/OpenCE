#ifndef HALO_MAC_NATIVE_LOOPING_SOUND_TAGS_H
#define HALO_MAC_NATIVE_LOOPING_SOUND_TAGS_H
#include "native_object_tags.h"
/* Owned native looping-sound root/tracks/details; all referenced sound payloads
   require separate loading. Does not initialize sound services or play audio. */
int mac_native_looping_sound_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **);
#endif
