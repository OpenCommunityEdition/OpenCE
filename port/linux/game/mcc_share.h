/* MCC package sharing in a network game (mcc_share.c): the game side of
 * mcc_share_session.c. A joiner whose host plays an MCC package it does not have
 * asks the host for it (messages 103 and 104), the files are staged and checked,
 * and the package is installed only when it is the host's content. The Custom
 * Edition map sharing (map_share.c) is separate and unchanged.
 *
 * The host's network.map_share setting enables automatic package offers.
 * MCC rules and planning retain namespace, file and resource boundaries. */
#ifndef MCC_SHARE_H
#define MCC_SHARE_H

/* every frame (map_share_update calls it): the host's hashing and serving, the
 * joiner's download */
void mcc_share_game_update(void);
/* the download's progress, drawn over the screen (map_share_render calls it) */
void mcc_share_game_render(void);

/* Whether this machine is fetching the package of that MCC level (cache_files.c takes
 * it as present and waits to precache, as for a Custom Edition download). */
boolean mcc_share_game_downloading(char const *level_name);
/* whether this machine's players wait to join while it fetches (map_share_holds_players's) */
boolean mcc_share_game_holds_players(void);
/* Host timeout protection while downloading or briefly admitting the player
 * after verified successful completion (map_share_machine_downloading). */
boolean mcc_share_game_machine_active(long machine_index);
/* Actual content still pending; post-install admission grace must not hold players. */
boolean mcc_share_game_machine_needs_map(long machine_index);

/* The distributed messages 103 (a joiner's request, to the host) and 104 (the host's
 * response), from the sender's connection alone (from_stream). */
boolean mcc_share_game_handles_message(word const *message, word size);
void mcc_share_game_handle_message(long machine_index, word const *message, word size, boolean from_stream);

struct network_game_server_client_machine;
void mcc_share_game_disconnect(void);
void mcc_share_game_peer_left(struct network_game_server_client_machine *machine);
int mcc_share_game_can_download(void);
int mcc_share_game_has_workshop(void);
void mcc_share_game_prompt_text(wchar_t text[512]);
void mcc_share_game_prompt_closed(void);
void mcc_share_game_prompt_action(int action);
#endif
