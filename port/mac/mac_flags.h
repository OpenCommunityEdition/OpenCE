#ifndef HALO_MAC_FLAGS_H
#define HALO_MAC_FLAGS_H
#include <stdlib.h>
/* A HALO_MAC_* switch: "1" on, "0" off, anything else (or unset) its
   default. The renderer's measured wins default to on; "0" turns one off
   for comparisons. */
static __inline__ int mac_flag(const char *name, int fallback)
{
    const char *value = getenv(name);
    if (value && value[0] == '1' && !value[1]) return 1;
    if (value && value[0] == '0' && !value[1]) return 0;
    return fallback;
}
/* A HALO_MAC_PERF_* switch for a CPU-cost change that was timed on a G4
   (each is the same result by other means): on, "0" turns one off for a
   comparison, and HALO_MAC_PERF=0 turns every one off. */
static __inline__ int mac_perf_flag(const char *name)
{
    return mac_flag(name, mac_flag("HALO_MAC_PERF", 1));
}
#endif
