/*
GAME.C

symbols in this file:
00095260 0110:
	_game_initialize (0000)
00095370 00c0:
	_game_dispose (0000)
00095430 01f0:
	_game_precache_new_map (0000)
00095620 0020:
	_game_map_loading_in_progress (0000)
00095640 0080:
	_game_unload (0000)
000956C0 00c0:
	_game_dispose_from_old_map (0000)
00095780 00a0:
	_game_frame (0000)
00095820 00b0:
	_remove_quitting_players_from_game (0000)
000958D0 0110:
	_game_tick (0000)
000959E0 0030:
	_game_options_new (0000)
00095A10 0020:
	_game_options_verify (0000)
00095A30 0050:
	_game_initial_pulse (0000)
00095A80 0020:
	_game_set_players_are_double_speed (0000)
00095AA0 0010:
	_game_players_are_double_speed (0000)
00095AB0 0020:
	_game_difficulty_level_set (0000)
00095AD0 0010:
	_game_difficulty_level_get (0000)
00095AE0 0020:
	_game_difficulty_level_get_ignore_easy (0000)
00095B00 0030:
	_game_set_game_variant (0000)
00095B30 0030:
	_game_set_game_engine_index (0000)
00095B60 0040:
	_game_all_quiet (0000)
00095BA0 0140:
	_game_safe_to_save (0000)
00095CE0 0020:
	_game_safe_to_speak (0000)
00095D00 0010:
	_game_is_cooperative (0000)
00095D10 0010:
	_set_random_seed (0000)
00095D20 00d0:
	_game_load (0000)
00095DF0 0160:
	_game_initialize_for_new_map (0000)
00095F50 0060:
	_game_set_game_variant_from_name (0000)
0025AD18 000c:
	??_C@_0M@OGHJLDGG@game_update?$AA@ (0000)
0025AD24 000b:
	??_C@_0L@GOOMCKGH@impossible?$AA@ (0000)
0025AD30 0005:
	??_C@_04EFOBLGJF@hard?$AA@ (0000)
0025AD38 0007:
	??_C@_06IKLLLDHL@normal?$AA@ (0000)
0025AD40 0005:
	??_C@_04EDNPHEAP@easy?$AA@ (0000)
0025AD48 000d:
	??_C@_0N@FEKOENDC@game?5globals?$AA@ (0000)
0025AD58 002a:
	??_C@_0CK@PBEAEMDE@cache_files_precache_map_loaded?$CI@ (0000)
0025AD88 0054:
	??_C@_0FE@OLEMLKAO@cache_files_precache_in_progress@ (0000)
0025ADDC 0039:
	??_C@_0DJ@FJDPKGNI@read?5the?5last?5error?5message?5for?5@ (0000)
0025AE18 001b:
	??_C@_0BL@KNHJNBLO@c?3?2halo?2SOURCE?2game?2game?4c?$AA@ (0000)
0025AE34 002c:
	??_C@_0CM@DHJPIMML@shouldn?8t?5be?5here?4?4?4?5map?5?8?$CFs?8?5do@ (0000)
0025AE60 0015:
	??_C@_0BF@OMFFMPBC@game_globals?9?$DOactive?$AA@ (0000)
0025AE78 002a:
	??_C@_0CK@HKBOLAEF@player?5?$CFx?5failed?5to?5quit?0?5wanted@ (0000)
0025AEA8 0047:
	??_C@_0EH@OCEAOGL@?$CB?$CCthis?5is?5broken?5and?5should?5get?5@ (0000)
0025AEF0 0031:
	??_C@_0DB@DCHNCJKB@not?5safe?5to?5save?3?5vehicle_moving@ (0000)
0025AF24 0025:
	??_C@_0CF@PDKCNCHK@not?5safe?5to?5save?3?5any_player_is_@ (0000)
0025AF4C 002b:
	??_C@_0CL@CDOGDNMO@not?5safe?5to?5save?3?5any_player_is_@ (0000)
0025AF78 0028:
	??_C@_0CI@POCCMPOA@not?5safe?5to?5save?3?5any_unit_is_da@ (0000)
0025AFA0 0030:
	??_C@_0DA@BKJKAMHL@not?5safe?5to?5save?3?5dangerous_effe@ (0000)
0025AFD0 002e:
	??_C@_0CO@LFBOBOC@not?5safe?5to?5save?3?5dangerous_item@ (0000)
0025B000 0034:
	??_C@_0DE@DKEBJNA@not?5safe?5to?5save?3?5dangerous_proj@ (0000)
0025B034 002c:
	??_C@_0CM@IBBKLIBO@not?5safe?5to?5save?3?5ai_enemies_can@ (0000)
0025B060 001d:
	??_C@_0BN@FDEBMHJB@game_options_verify?$CIoptions?$CJ?$AA@ (0000)
0025B080 001a:
	??_C@_0BK@NHHNPJL@?$CBgame_globals?9?$DOmap_loaded?$AA@ (0000)
0025B09C 0016:
	??_C@_0BG@JKMPENHK@?$CBgame_globals?9?$DOactive?$AA@ (0000)
0025B0B4 0019:
	??_C@_0BJ@CDJMHJAA@game_globals?9?$DOmap_loaded?$AA@ (0000)
002DDDB0 0608:
	_global_game_difficulty_level_names (0000)
0043E48C 0004:
	_game_globals (0000)
*/

struct game_options;

/* ---------- headers */

#include "cseries/cseries.h"
#define set_random_seed set_random_seed_inline
#include "game/game.h"
#undef set_random_seed
#include "game/game_engine.h"
#include "memory/data.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct game_options
{
	unsigned long flags;
	short code_version;
	short difficulty;
	unsigned long random_seed;
	char map_name[256];
};

struct game_options_prefix
{
	unsigned long flags;
	short code_version;
	short difficulty;
};

struct game_runtime_globals_prefix
{
	boolean map_loaded;
	boolean active;
	boolean players_are_double_speed;
	boolean map_load_in_progress;
	real loading_progress;
	struct game_options_prefix options;
};

typedef char verify_game_options_size[
	sizeof(struct game_options) == 0x10C ? 1 : -1];
typedef char verify_game_options_code_version_offset[
	offsetof(struct game_options, code_version) == 0x4 ? 1 : -1];
typedef char verify_game_options_difficulty_offset[
	offsetof(struct game_options, difficulty) == 0x6 ? 1 : -1];
typedef char verify_game_options_random_seed_offset[
	offsetof(struct game_options, random_seed) == 0x8 ? 1 : -1];
typedef char verify_game_options_map_name_offset[
	offsetof(struct game_options, map_name) == 0xC ? 1 : -1];
typedef char verify_game_runtime_globals_players_are_double_speed_offset[
	offsetof(struct game_runtime_globals_prefix, players_are_double_speed) == 0x2 ? 1 : -1];
typedef char verify_game_runtime_globals_difficulty_offset[
	offsetof(struct game_runtime_globals_prefix, options) +
		offsetof(struct game_options_prefix, difficulty) == 0xE ? 1 : -1];

/* ---------- prototypes */

void game_engine_game_starting(
	void);

void game_engine_player_added(
	long player_index);

void recorded_animations_dispose(
	void);
void cinematic_dispose(
	void);
void hs_dispose(
	void);
void cheats_dispose(
	void);
void ui_widgets_dispose(
	void);
void editor_dispose(
	void);
void ai_dispose(
	void);
void player_effect_dispose(
	void);
void rumble_dispose(
	void);
void game_sound_dispose(
	void);
void sound_classes_dispose(
	void);
void game_engine_dispose(
	void);
void particle_systems_dispose(
	void);
void weather_particle_systems_dispose(
	void);
void effects_dispose(
	void);
void particles_dispose(
	void);
void contrails_dispose(
	void);
void players_dispose(
	void);
void decals_dispose(
	void);
void breakable_surfaces_dispose(
	void);
void structures_dispose(
	void);
void render_dispose(
	void);
void objects_dispose(
	void);
void director_dispose(
	void);
void interface_dispose(
	void);
void game_allegiance_dispose(
	void);
void saved_game_files_dispose(
	void);
void event_manager_dispose(
	void);
void input_abstraction_dispose(
	void);
void player_ui_dispose(
	void);
void game_state_dispose(
	void);
void telnet_console_dispose(
	void);
void transport_dispose(
	void);
void bink_playback_dispose(
	void);
void progress_bar_dispose(
	void);

void rasterizer_dispose_from_old_map(
	void);
void game_state_dispose_from_old_map(
	void);
void cheats_dispose_from_old_map(
	void);
void recorded_animations_dispose_from_old_map(
	void);
void hs_dispose_from_old_map(
	void);
void cinematic_dispose_from_old_map(
	void);
void editor_dispose_from_old_map(
	void);
void ai_dispose_from_old_map(
	void);
void player_effect_dispose_from_old_map(
	void);
void rumble_dispose_from_old_map(
	void);
void point_physics_dispose_from_old_map(
	void);
void particle_systems_dispose_from_old_map(
	void);
void weather_particle_systems_dispose_from_old_map(
	void);
void decals_dispose_from_old_map(
	void);
void breakable_surfaces_dispose_from_old_map(
	void);
void structures_dispose_from_old_map(
	void);
void render_dispose_from_old_map(
	void);
void objects_dispose_from_old_map(
	void);
void director_dispose_from_old_map(
	void);
void observer_dispose_from_old_map(
	void);
void interface_dispose_from_old_map(
	void);
void players_dispose_from_old_map(
	void);
void contrails_dispose_from_old_map(
	void);
void particles_dispose_from_old_map(
	void);
void effects_dispose_from_old_map(
	void);
void game_sound_dispose_from_old_map(
	void);
void sound_classes_dispose_from_old_map(
	void);
void sound_dispose_from_old_map(
	void);
void game_allegiance_dispose_from_old_map(
	void);
void update_server_delete(
	void);
void game_engine_dispose_from_old_map(
	void);
void scenario_dispose_from_old_map(
	void);
void ui_widgets_close_all(
	void);
void ui_widgets_safe_to_load(
	boolean safe);

void collision_log_begin_period(
	short period);
void collision_log_end_period(
	void);
void particles_update(
	real dt);
void contrails_update(
	real dt);
void particle_systems_update(
	real dt);
void widgets_update(
	real dt);
void game_sound_update(
	real dt);
void scenario_frame_update(
	real dt);
void rasterizer_frame_update(
	real dt);
void numeric_countdown_timer_update(
	void);

/* ---------- globals */

extern struct game_runtime_globals_prefix *game_globals;
extern struct game_variant game_variant_global;
extern struct data_array *player_data;
extern short player_spawn_count;

/* ---------- public code */

void game_options_new(
	struct game_options *options)
{
	csmemset(options, 0, sizeof(*options));
	options->code_version = 0;
	options->difficulty = _game_difficulty_level_normal;
	options->random_seed = 0xDEADBEEF;
	return;
}

void game_initial_pulse(
	void)
{
	struct data_iterator iterator;

	data_iterator_new(&iterator, player_data);
	while (data_iterator_next(&iterator))
		game_engine_player_added(iterator.datum_index);
	game_engine_game_starting();

	return;
}

void game_set_players_are_double_speed(
	boolean players_are_double_speed)
{
	game_globals->players_are_double_speed = players_are_double_speed;

	return;
}

boolean game_players_are_double_speed(
	void)
{
	return game_globals->players_are_double_speed;
}

void game_difficulty_level_set(
	short difficulty)
{
	game_globals->options.difficulty = difficulty;

	return;
}

short game_difficulty_level_get(
	void)
{
	return game_globals->options.difficulty;
}

short game_difficulty_level_get_ignore_easy(
	void)
{
	short difficulty = game_globals->options.difficulty;

	return difficulty > _game_difficulty_level_normal ? difficulty : _game_difficulty_level_normal;
}

void game_set_game_variant(
	struct game_variant *variant)
{
	if (!variant)
	{
		csmemset(&game_variant_global, 0, sizeof(game_variant_global));
	}
	else
	{
		game_variant_global = *variant;
	}

	return;
}

void game_set_game_engine_index(
	short index)
{
	match_assert(
		"c:\\halo\\SOURCE\\game\\game.c",
		0x311,
		!"this is broken and should get updated for the variants, ask michael");

	return;
}

boolean game_is_cooperative(
	void)
{
	return player_spawn_count > 1;
}

void set_random_seed(
	unsigned long seed)
{
	*get_global_random_seed_address() = seed;

	return;
}

void game_dispose(
	void)
{
	recorded_animations_dispose();
	cinematic_dispose();
	hs_dispose();
	cheats_dispose();
	ui_widgets_dispose();
	editor_dispose();
	ai_dispose();
	player_effect_dispose();
	rumble_dispose();
	game_sound_dispose();
	sound_classes_dispose();
	game_engine_dispose();
	particle_systems_dispose();
	weather_particle_systems_dispose();
	effects_dispose();
	particles_dispose();
	contrails_dispose();
	players_dispose();
	decals_dispose();
	breakable_surfaces_dispose();
	structures_dispose();
	render_dispose();
	objects_dispose();
	director_dispose();
	interface_dispose();
	game_allegiance_dispose();
	game_time_dispose();
	saved_game_files_dispose();
	event_manager_dispose();
	input_abstraction_dispose();
	player_ui_dispose();
	game_state_dispose();
	telnet_console_dispose();
	transport_dispose();
	bink_playback_dispose();
	progress_bar_dispose();

	return;
}

void game_dispose_from_old_map(
	void)
{
	rasterizer_dispose_from_old_map();
	game_state_dispose_from_old_map();
	cheats_dispose_from_old_map();
	recorded_animations_dispose_from_old_map();
	hs_dispose_from_old_map();
	cinematic_dispose_from_old_map();
	editor_dispose_from_old_map();
	ai_dispose_from_old_map();
	player_effect_dispose_from_old_map();
	rumble_dispose_from_old_map();
	point_physics_dispose_from_old_map();
	particle_systems_dispose_from_old_map();
	weather_particle_systems_dispose_from_old_map();
	decals_dispose_from_old_map();
	breakable_surfaces_dispose_from_old_map();
	structures_dispose_from_old_map();
	render_dispose_from_old_map();
	objects_dispose_from_old_map();
	director_dispose_from_old_map();
	observer_dispose_from_old_map();
	interface_dispose_from_old_map();
	players_dispose_from_old_map();
	contrails_dispose_from_old_map();
	particles_dispose_from_old_map();
	effects_dispose_from_old_map();
	game_sound_dispose_from_old_map();
	sound_classes_dispose_from_old_map();
	sound_dispose_from_old_map();
	game_allegiance_dispose_from_old_map();
	update_server_delete();
	game_engine_dispose_from_old_map();
	scenario_dispose_from_old_map();
	game_time_dispose_from_old_map();
	ui_widgets_close_all();
	ui_widgets_safe_to_load(FALSE);
	game_globals->active = FALSE;

	return;
}

void game_frame(
	real dt)
{
	real frame_dt;

	if (game_globals->players_are_double_speed)
		frame_dt = dt * 0.5f;
	else
		frame_dt = dt;

	match_assert(
		"c:\\halo\\SOURCE\\game\\game.c",
		585,
		game_globals->active);

	collision_log_begin_period(1);
	particles_update(frame_dt);
	contrails_update(frame_dt);
	particle_systems_update(frame_dt);
	widgets_update(frame_dt);
	game_sound_update(frame_dt);
	scenario_frame_update(frame_dt);
	rasterizer_frame_update(frame_dt);
	numeric_countdown_timer_update();
	collision_log_end_period();

	return;
}

/* ---------- private code */
