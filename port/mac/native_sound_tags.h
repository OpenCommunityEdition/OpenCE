#ifndef HALO_MAC_NATIVE_SOUND_TAGS_H
#define HALO_MAC_NATIVE_SOUND_TAGS_H
#include "native_object_tags.h"
/* Owned root, ranges, permutations and unmodified encoded sample bytes.
   Optional external sounds.map storage is copied before return.
   Sample offsets are normalized to engine_cache_files native file spaces.
   This does not create a streaming cache, decode samples, initialize audio or play sound. */
int mac_native_sound_decode(const struct mac_retail_cache *,uint32_t,
    const void *,size_t,struct mac_native_object_tag *,const char **);
/* Owned listener environment; all scalar reverb parameters must be finite.
   Applying them to the audio device remains the sound manager's responsibility. */
int mac_native_sound_environment_decode(const struct mac_retail_cache *,uint32_t,
    struct mac_native_object_tag *,const char **);
#endif
