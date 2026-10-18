#ifndef MAC_VEHICLE_RUNTIME_PROBE_H
#define MAC_VEHICLE_RUNTIME_PROBE_H

#include <stdint.h>

/* Place the map's actual palette-zero vehicle through the stock scenario
   constructor. The caller owns normal object cleanup. */
int mac_vehicle_runtime_place(uint32_t expected_definition,long *object_index);
/* Check the same real vehicle datum after the caller's normal objects_update
   loop; this function never advances time or updates the object itself. */
int mac_vehicle_runtime_after_update(long object_index,uint32_t expected_definition);

#endif
