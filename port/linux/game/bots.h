/* bots.h: host-controlled computer players (port/linux/game/bots.c)

A bot is a player whose controls come from
code instead of a controller: it is in the lobby, the scores and the game's
rules as any player is, spawns, respawns and scores. Each bot is the one
player of a machine of its own at the top of the machine indices
(BOTS_FIRST_MACHINE up). Local games have only their own real machine;
opted-in network hosts reserve this range. Ownership is tracked separately. The bots have no view and no controller: nothing is
drawn for them. */
#ifndef HALO_BOTS_H
#define HALO_BOTS_H

struct player_action;
struct network_player;
struct network_game;

/* Optional host bots: 63 plus one human, within the existing 128 slots. */
#define BOTS_MAXIMUM 63
/* the first bot's machine index (the last machine indices of the session) */
#define BOTS_FIRST_MACHINE (HALO_PORT_MAXIMUM_NETWORK_MACHINES - BOTS_MAXIMUM)

enum
{
	/* bots.teams: in a team game, the bots on both teams (evening them
	with the players), or all on the team the players are not on */
	_bots_teams_even = 0,
	_bots_teams_against,
};

/* the settings (bots.count, bots.skill, bots.teams) */
long bots_wanted_count(void);
short bots_skill(void);
short bots_teams(void);
/* The inherited Slayer bots, plus CTF/King when enhanced bots are enabled. */
int bots_gametype_allowed(long engine);
int bots_gametype_active(long engine);
/* Host controls use existing player joins/quits; team is NONE, red 0 or blue 1. */
int bots_manage(int add, int team, char *status, long size);
void bots_management_reset(void);
/* -1 for an unrelated command, otherwise the host action's success. */
int bots_console_command(char const *expression);
/* Host policy is read at creation and may be applied from the pregame menu. */
void bots_network_session_begin(void);
void bots_network_session_end(void);
int bots_network_reserved(void);
void bots_network_lobby_enable(int enabled);
int bots_lobby_settings_apply(char *status, long size);
int bots_session_allowed(void);
void bots_player_ownership(long machine_index, int present);
int bots_player_owned(long machine_index);
/* One human machine with owned bots can start the ordinary lobby countdown. */
int bots_lobby_can_start(struct network_game *game);
void bots_network_record_actions(struct player_action *actions);

/* Whether a machine ID falls in the reserved range; not an ownership check. */
int bots_machine_is_bot(long machine_index);
/* the network player for bot number `bot_index` (0 up): its machine,
controller, name and no team or colour yet */
void bots_network_player(long bot_index, struct network_player *player);

/* the game tick (players_update_before_game): every bot's controls this
tick, into the actions of the players by their index; host only */
void bots_update_actions(struct player_action *actions);

/* a map's start and end (players.c) */
void bots_initialize_for_new_map(void);
void bots_dispose_from_old_map(void);

#endif
