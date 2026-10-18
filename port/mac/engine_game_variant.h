#ifndef MAC_ENGINE_GAME_VARIANT_H
#define MAC_ENGINE_GAME_VARIANT_H

struct game_variant;

/* Call after game_initialize; select stock Slayer before loading a map. */
void mac_engine_game_variant_report(void);
int mac_engine_game_variant_select_slayer(const char **error);
int mac_engine_game_variant_select_multiplayer(const char **error);

/* Call after game_initialize; the campaign runs with no game engine. */
int mac_engine_game_variant_select_campaign(const char **error);

/* Initialize/update the selected engine only with the real loaded scenario. */
int mac_engine_game_variant_initialize_map(const char **error);
int mac_engine_game_variant_update(const char **error);

#endif
