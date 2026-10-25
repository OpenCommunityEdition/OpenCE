/* game_modes.h: OpenCE's gametypes (port/linux/game/game_modes.c)

SWAT, Infection and Gun Game are modes of Slayer, and Hardcore is a set of
rules over any game engine's: a gametype's PC options (game_engine.h's struct
game_variant_options, from mode on) say which, so a saved gametype, the
network game and the game all carry them. The built-in ones are written with
the Xbox's default gametypes (playlist_profile.c), after them, and the
Select Gametype screen lists them in a bank of their own, OPENCE. */
#ifndef HALO_GAME_MODES_H
#define HALO_GAME_MODES_H

struct game_engine;
struct game_variant;
struct game_variant_options;

/* the built-in gametypes' place among the default ones (the Xbox's 26 come
first: playlist_profile.c checks it) */
#define GAME_MODES_FIRST_DEFAULT_PLAYLIST_PROFILE 26

/* ---- the built-in gametypes */

short game_modes_gametype_count(void);
/* its name in the gametype lists */
wchar_t const *game_modes_gametype_name(short index);
/* its variant and options */
void game_modes_gametype(short index, struct game_variant *variant, struct game_variant_options *options);
/* the one of that name (debug.network_test's: "swat", "infection",
"gun_game", "hardcore", "hardcore_ctf" ...), FALSE if none is */
boolean game_modes_gametype_by_name(char const *name, struct game_variant *variant,
	struct game_variant_options *options);

/* ---- a gametype's options */

/* OpenCE's options at their defaults, with no mode (a gametype's that has
none of its own: game_variant_options_default, and one saved before them) */
void game_modes_options_default(struct game_variant_options *options);
/* each of OpenCE's options in range (a file's or another machine's), its
mode's own set to their defaults where none were given */
void game_modes_options_sanitize(struct game_variant_options *options);
/* the mode a gametype plays: its options' if its game engine and teams
allow it (SWAT and Infection on teams, Gun Game free for all, all of them
Slayer), else none */
short game_modes_variant_mode(struct game_variant const *variant, struct game_variant_options const *options);
/* the name the menus show for its kind of game ("INFECTION", "HARDCORE
CTF"), NULL for a game engine's own ("SLAYER") */
wchar_t const *game_modes_variant_kind_name(struct game_variant const *variant,
	struct game_variant_options const *options);
/* a game's kind in a byte, as its LAN advertisement and internet listing
carry it to the server browser: the mode it plays (game_modes_variant_mode),
and GAME_MODES_KIND_HARDCORE */
#define GAME_MODES_KIND_HARDCORE 0x80
byte game_modes_kind(struct game_variant const *variant, struct game_variant_options const *options);
/* the name of a game engine's game of that kind, in full or abbreviated
("HC CTF": the server browser's column), NULL for the game engine's own
(another machine's kind: a mode of any game engine but Slayer, or one this
build does not know, is none) */
wchar_t const *game_modes_kind_name(long game_engine_index, byte kind, boolean abbreviated);
/* whether a gametype is one of OpenCE's (a mode, or Hardcore) */
boolean game_modes_variant_is_opence(struct game_variant const *variant, struct game_variant_options const *options);

/* ---- the game being played */

/* its mode (_game_mode_none with no game engine) */
short game_modes_mode(void);
/* whether it plays by Hardcore's rules */
boolean game_modes_hardcore(void);
/* the game engine a mode plays with: Infection's and Gun Game's are
Slayer's with their own rules; SWAT's variant has no shields
(game_engine_initialize) */
struct game_engine *game_modes_engine(struct game_engine *engine);
/* a player's spawn, after its starting equipment (game_engine_postspawn_player_update) */
void game_modes_postspawn(long player_index);
/* a player killed (game_engine_player_killed, after the game engine's):
Hardcore's restore of the killer */
void game_modes_player_killed(long killing_player_index, long dead_player_index, boolean friendly_fire);
/* the kill being recorded is a melee's (damage.c, around
game_statistics_record_kill) */
void game_modes_kill_by_melee(boolean melee);
boolean game_modes_killed_by_melee(void);
/* a shot's hit on a player's head (a bullet's, a plasma bolt's, a needle's,
not a melee's or an explosion's) kills at once (SWAT) */
boolean game_modes_headshot_kills(long object_index, boolean shot);
/* the motion sensor is not drawn (Hardcore) */
boolean game_modes_motion_sensor_hidden(void);
/* no aim assist: neither autoaim nor magnetism (Hardcore) */
boolean game_modes_aim_assist_off(void);

#endif
