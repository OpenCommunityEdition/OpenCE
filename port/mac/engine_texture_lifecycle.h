#ifndef HALO_MAC_ENGINE_TEXTURE_LIFECYCLE_H
#define HALO_MAC_ENGINE_TEXTURE_LIFECYCLE_H

/* Session lease for the native CPU-resident bitmap set. This is not an Xbox
   texture-cache allocator or a GPU-resource cache. */
int mac_engine_texture_lifecycle_bind(void);
int mac_engine_texture_lifecycle_unbind(void);
int mac_engine_texture_lifecycle_ready(void);
int mac_engine_texture_lifecycle_is_bound(void);
/* Read-only assertion that the native session has already installed its
   actual owned bitmap roots. */
int mac_native_bitmap_cache_is_bound(void);

#endif
