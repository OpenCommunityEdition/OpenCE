#ifndef HALO_MAC_ENGINE_SCENARIO_TRANSITION_H
#define HALO_MAC_ENGINE_SCENARIO_TRANSITION_H

/* Native retail-v7 replacement for scenario_load/scenario_unload while an
   engine session scope owns the converted scenario and BSP globals. */
boolean mac_engine_scenario_load(const char *map_name);
void mac_engine_scenario_unload(void);

#endif
