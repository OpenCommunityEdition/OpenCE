#ifndef HALO_MAC_ENGINE_NETWORK_TEST_H
#define HALO_MAC_ENGINE_NETWORK_TEST_H
/* Hosting or joining a system link game in the Mac session, which has no
   menu and no lobby. Called once a frame from the session's loop
   (tools/mac_session_interactive.c), before the tick. */
void mac_engine_network_test_update(real seconds);
/* Whether the test's game has ended (the title map it would go back to is
   not in the session): the loop stops */
int mac_engine_network_test_game_over(void);
/* The menus' network games: the netcode started when the menus are up (a
   frame's call), a session opened on a map file, the begin held for another
   session (and made again there), and the game that cannot begin */
void mac_engine_network_menu_update(void);
void mac_engine_network_session_open(const char *map_path);
int mac_engine_network_begin_deferred(const char *map_name);
void mac_engine_network_begin_resume(void);
/* every frame: an invite link copied onto the clipboard before switching to
   the game (network.join_from_clipboard) */
void mac_engine_network_clipboard_update(void);
void mac_engine_network_begin_failed(void);
#endif
