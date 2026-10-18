#ifndef HALO_MAC_ENGINE_MENU_H
#define HALO_MAC_ENGINE_MENU_H
/* The PC version's menus (port/assets/menus, built into widget tags by
   port/linux/game/menu_tags.c) over the loaded game map. Opt-in: the
   diagnostic HALO_MAC_MENU=1. */
int mac_engine_menu_requested(void);
/* Builds the menus' tags into the bound tag registry and, for main_menu,
   opens the main menu (else the pause menu is ready for Escape); 0 if that
   failed (the log says why). */
int mac_engine_menu_start(const char *map_name, int main_menu);
int mac_engine_menu_active(void);
/* One game tick of the menus' widgets. */
void mac_engine_menu_tick(void);
void mac_engine_menu_stop(void);
/* The menus' art as the texture binder's borrowed resolver wants it:
   1 (a GL name and scale), 0 (not the menus'), -1 (cannot be drawn). */
int mac_engine_menu_art_resolve(const void *resource, unsigned char sampler,
    unsigned int *name, float scale[4]);
/* Menu mode (HALO_MAC_MENU=1): the menus start the games. */
int mac_engine_menu_enabled(void);
/* The keyboard as the first game pad for the widgets: arrows, Return (A),
   Backspace (B), Escape (B with a widget up, Start in a game: the pause
   menu). */
struct mac_input_frame;
void mac_engine_menu_input(const struct mac_input_frame *frame);
/* What the widgets asked for since the last call. */
enum { MAC_MENU_REQUEST_NONE, MAC_MENU_REQUEST_START, MAC_MENU_REQUEST_MENU };
int mac_engine_menu_poll(void);
/* The scenario path of the last START request, as the menus name it
   (levels\test\bloodgulch\bloodgulch). */
const char *mac_engine_menu_pending_map(void);
/* A network game begun (engine_network_test.c): the next poll is a START of that map. */
void mac_engine_menu_request_game(const char *map_name);
/* the game's local players, one for each controller (split screen); the first's player index */
long mac_engine_menu_create_local_players(void);
#endif
