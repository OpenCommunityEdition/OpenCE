#ifndef HALO_MAC_ENGINE_SERVICES_H
#define HALO_MAC_ENGINE_SERVICES_H

/* Optional ownership query for debug_free. The arena registers its predicate
   only while its allocation is live, so standalone service probes need no
   dependency on the arena implementation. */
void mac_engine_services_set_arena_ownership(int (*owns_pointer)(const void *pointer));

#endif
