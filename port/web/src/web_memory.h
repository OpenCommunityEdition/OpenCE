/* Browser memory ownership. Fixed map windows never belong to the C heap. */
#ifndef HALO_WEB_MEMORY_H
#define HALO_WEB_MEMORY_H

#include <stddef.h>
#include "../../linux/game/cache_file_formats.h"

#define WEB_HEAP_LIMIT CUSTOM_EDITION_TAG_CACHE_ADDRESS

size_t web_memory_bytes(void);
void web_memory_initialize(void);
void *web_custom_edition_tag_cache(void);

#endif
