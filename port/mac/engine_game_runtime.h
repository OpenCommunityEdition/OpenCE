#ifndef HALO_MAC_ENGINE_GAME_RUNTIME_H
#define HALO_MAC_ENGINE_GAME_RUNTIME_H
/* Own only genuine core game state inside the already-owned game arena.
   No services, map load, active game or full tick is established. */
int mac_game_core_start(void);
int mac_game_core_stop(void);
/* Construct the retail source-private options layout and enter game_load. */
int mac_game_load_native_map(const char *map_name);
#endif
