#ifndef HALO_MAC_ENGINE_PHYSICAL_MEMORY_H
#define HALO_MAC_ENGINE_PHYSICAL_MEMORY_H

#include "xtl.h"

/* CPU virtual backing for the Xbox physical-memory API. The returned address
   is in the game's reserved address window; it is not a GPU bus address. */
int mac_engine_physical_memory_start(void);
void mac_engine_physical_memory_stop(void);
unsigned long mac_engine_physical_memory_allocation_count(void);
/* True only when the complete virtual range belongs to a live XPhysicalAlloc. */
int mac_engine_physical_memory_contains(const void *address, unsigned long bytes);

/* port: the whole XPhysicalAlloc allocation that holds [address, address+bytes):
   its start and size (the engine's tag cache, texture cache, sound cache and
   game state are such allocations; the GPU path reads Xbox maps' vertices,
   indices and textures from them). 0 if the range is not in one. */
int mac_engine_physical_memory_span(const void *address, unsigned long bytes,
    unsigned long *start, unsigned long *size);

#endif
