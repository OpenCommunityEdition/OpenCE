#ifndef HALO_MAC_ENGINE_CAPACITY_H
#define HALO_MAC_ENGINE_CAPACITY_H
#include "../linux/include/halo_port_capacity.h"
/* Legacy Macs use the original resident cache budgets, as the constrained
   builds do. These are cache backing stores, not gameplay/network pool sizes.
   The shared desktop texture cache alone exceeds our 128 MiB address window. */
#undef HALO_PORT_TEXTURE_CACHE_PAGE_COUNT
#undef HALO_PORT_TEXTURE_CACHE_SIZE
#undef HALO_PORT_SOUND_CACHE_SIZE
#define HALO_PORT_TEXTURE_CACHE_PAGE_COUNT 0x580
#define HALO_PORT_TEXTURE_CACHE_SIZE (HALO_PORT_TEXTURE_CACHE_PAGE_COUNT*0x4000)
#define HALO_PORT_SOUND_CACHE_SIZE 0x400000
#endif
