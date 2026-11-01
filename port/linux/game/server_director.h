/*
SERVER_DIRECTOR.H

A dedicated server's director (server_director.c), its commands
(server_commands.c) and its status (server_status.c): what the game's
sources call of them, and what they share. halo_server_mode() says whether
this run is a dedicated server (port/linux/src/server_mode.c).
*/

#ifndef __SERVER_DIRECTOR_H
#define __SERVER_DIRECTOR_H

#include "../src/server_mode.h" /* port: port/linux/src/server_mode.c */

struct server_json;
struct server_entry;
struct server_rotation;

/* the director's states (server_status.c names them) */
enum
{
	_server_state_waiting,
	_server_state_hosting,
	_server_state_lobby,
	_server_state_loading,
	_server_state_playing,
	_server_state_postgame,
	_server_state_stopping,
	NUMBER_OF_SERVER_STATES
};

/* why a game ended (game_ended's reason) */
enum
{
	_server_end_score,
	_server_end_time,
	_server_end_idle,
	_server_end_empty,
	_server_end_command,
	_server_end_next,
	/* a new network version's update that would wait no longer
	(server.update_game_minutes), and the players' vote (!endgame) */
	_server_end_update,
	_server_end_vote,
	NUMBER_OF_SERVER_ENDS
};

/* when a stop asked for comes (sv_stop, sv_restart: drain's after) */
enum
{
	_server_drain_now,
	/* once the game being played is over (at once with none) */
	_server_drain_game,
	/* once nobody plays on the server */
	_server_drain_empty,
	/* (no stop: the one waiting cancelled) */
	_server_drain_cancel,
	NUMBER_OF_SERVER_DRAINS
};

/* ---------- the director (server_director.c) */

/* each frame, from the main loop (main.c), beside network_test_update */
void server_update(boolean main_menu_loaded, real seconds);
/* (network_server_manager.c) whether the lobby may count down: its entry
set, enough players, and teams as server.team_start says */
boolean server_director_ok_to_countdown(long player_count, boolean needs_more_teams);
/* the game's fatal error (main.c's halt_and_catch_fire, main_loop_of_death) */
void server_fatal(char const *text);

/* the director's state, and the seconds in it */
short server_director_state(void);
/* the rotation, the entry being played (its index; NONE for one sv_map
interposed, or before the first is set), and the next */
struct server_rotation const *server_director_rotation(void);
long server_director_index(void);
struct server_entry const *server_director_current(void);
struct server_entry const *server_director_next(void);
/* games hosted since the start, and this game's seconds */
long server_director_games_hosted(void);
real server_director_game_seconds(void);

/* the commands' (server_commands.c) */
/* the game ended now: with its scores (end_game), or without (skip) */
boolean server_director_end_game(short reason, boolean skip_postgame);
/* whether a game is being played that server_director_end_game would end */
boolean server_director_playing(void);
/* the entry played next (sv_map; now: the lobby's set again, or the game
ended at once if one runs; once the lobby's start was sent, after the game
it starts): TRUE and when in reason, else FALSE and why */
boolean server_director_interpose(char const *text, char *reason, long reason_size);
/* the rotation replaced (sv_rotation_set, PUT /api/rotation): FALSE with
the rejected entries said, and nothing changed */
boolean server_director_set_rotation(char const *text, boolean write, struct server_json *rejected);
/* the rotation's next game is the entry at index (sv_rotation_begin) */
boolean server_director_rotation_begin(long index);
/* the settings read again (sv_reload, SIGHUP) */
void server_director_reload(void);
/* the server stops: its game ended, every machine told, the listings
withdrawn; then the process ends with the code */
void server_director_quit(int exit_code, char const *why);
/* the settings the game takes from server.* applied now (the name, the
most players, the listing) */
void server_director_apply_settings(void);
/* a command dropped the machine's players: their leaving is said so
(player_left's reason: "kicked", "banned"; a static text) */
void server_director_player_removed(long machine_index, char const *reason);
/* the server stops (exit code 0, or 5 to be restarted) now, or once the
game is over or the server is empty (a drain): until then its listings are
withdrawn and new players refused. The notice was said already */
void server_director_drain(short after, boolean restart, char const *notice);
/* the drain waiting cancelled: listed and open again; FALSE if none was */
boolean server_director_drain_cancel(char const *notice);
/* (network_server_manager.c) whether new machines are refused: a drain */
boolean server_director_refuses_joins(void);
/* the status's "drain": null, or {after, restart, since} */
void server_director_drain_json(struct server_json *json);

/* ---------- the commands (server_commands.c) */

/* (hs.c) a command of the server's, run if the line is one, with the role
the line came with (server_commands_run's; the owner's otherwise): TRUE
with its result in result */
boolean server_command_try(char const *expression, boolean *result);
/* a line run with a role (standard input's, the web's): the console's
output it made, lines separated by newlines (free it), and whether it
succeeded */
char *server_commands_run(char const *line, int role, int origin, boolean *succeeded);
/* the role of the command being run (the owner's when none runs) */
int server_commands_role(void);

/* ---------- the game's events (server_status.c; nothing but on a dedicated
server) */

/* a player killed (network_distributed_player_killed's, on the host) */
void server_event_player_killed(long killing_player_index, long killing_object_index, long dead_player_index,
	boolean friendly_fire);
/* a line of chat the host sends on (network_chat.c), by its sender's
absolute player index */
void server_event_chat(short player_index, short channel, char const *text);
/* a machine found running its game faster than the host's (action "kicked"
or "unverified") */
void server_event_speed_hack(long machine_index, char const *names, real rate, long ahead_ticks,
	char const *action);
/* a machine's download of the game's map (map_share.c): state "asked",
"downloading", "downloaded" or "failed" */
void server_event_map_share(long machine_index, char const *map, char const *state, unsigned long done,
	unsigned long total);

/* ---------- the status (server_status.c) */

/* the status (docs/dedicated_server.md, "The status"), as the role sees it */
void server_status_write(struct server_json *json, int role);
/* the server's identity, for the status and server_started: its process,
start, instance, region, ports and publisher key's id */
void server_status_identity(struct server_json *json);
/* the players, as the role sees them */
void server_status_players(struct server_json *json, int role);
/* one player, as the role sees him (a players[] element) */
void server_status_player(struct server_json *json, long slot, int role);
/* the maps there are */
void server_status_maps(struct server_json *json);
/* the gametypes there are, and their rules' schema */
void server_status_gametypes(struct server_json *json);
/* the state's name ("lobby"...) */
char const *server_status_state_name(short state);
/* the players' scores changed since this was last asked (score events) */
boolean server_status_scores_changed(void);
/* the network game's player at slot (its name in ASCII into name): TRUE */
boolean server_status_player_name(long slot, char *name, long size);
/* the team scores of a team game (two), else FALSE */
boolean server_status_team_scores(long *red, long *blue);

#endif
