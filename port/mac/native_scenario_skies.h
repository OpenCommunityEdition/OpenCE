#ifndef HALO_MAC_NATIVE_SCENARIO_SKIES_H
#define HALO_MAC_NATIVE_SCENARIO_SKIES_H
#include "retail_cache.h"
#include <stdint.h>
struct scenario;

/* Owned scenario sky references. This converts reference records and names,
   not the referenced sky tag payload or its render resources. */
struct mac_native_scenario_skies {
    void *references;
    uint32_t count;
    void *owner;
};

int mac_native_scenario_skies_decode(const struct mac_retail_cache *,
    struct mac_native_scenario_skies *,const char **);
void mac_native_scenario_skies_free(struct mac_native_scenario_skies *);
int mac_native_scenario_skies_attach(const struct mac_native_scenario_skies *,
    struct scenario *);
#endif
