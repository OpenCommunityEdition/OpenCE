#ifndef HALO_MAC_HOST_MEMORY_H
#define HALO_MAC_HOST_MEMORY_H
#include <stdint.h>
struct mac_host_memory { uint64_t physical, available; };
/* Available means free plus reclaimable inactive pages at this instant. */
int mac_host_memory_query(struct mac_host_memory *);
#endif
