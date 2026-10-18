#ifndef HALO_MAC_ENGINE_TAGS_H
#define HALO_MAC_ENGINE_TAGS_H
#include "native_tags.h"
/* Single-threaded map activation. Unbind before freeing the registry.
   Legacy diagnostic tag_get implementations must not be linked alongside this. */
int mac_engine_tags_bind(struct mac_native_tags *);
void mac_engine_tags_unbind(void);
int mac_engine_tags_checksum(uint32_t *out);
uint32_t mac_engine_tags_find_metadata(uint32_t group,const char *name);
const char *mac_engine_tags_name(uint32_t handle);
/* The bound registry (NULL if none). */
struct mac_native_tags *mac_engine_tags_active(void);
#endif
