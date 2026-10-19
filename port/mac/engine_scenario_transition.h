#ifndef HALO_MAC_ENGINE_SCENARIO_TRANSITION_H
#define HALO_MAC_ENGINE_SCENARIO_TRANSITION_H

/* Native retail-v7 replacement for scenario_load/scenario_unload while an
   engine session scope owns the converted scenario and BSP globals. */
boolean mac_engine_scenario_load(const char *map_name);
void mac_engine_scenario_unload(void);
/* The session's world is going. What the unload held of it is let go
   unread: those pointers are into that world, and the scenario's index is
   an index into that session's tag table, which the next session builds
   again. A load in the next session must not read either. */
void mac_engine_scenario_forget(void);

#endif
