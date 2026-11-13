/* ---------- headers */

#include "cseries/cseries.h"
#include "bungie_net/common/thread.h"
#include "cseries/errors.h"
#include "math/real_math.h"
#include "tag_files/files.h"
#include "text/unicode.h"
#include "game/game.h"
#include "main/main.h"
#include "networking/network_connection.h"
#include "interface/player_ui.h"
#include "saved games/saved_game_files.h"
#include "saved games/player_profile.h"

#include <xtl.h>

/* ---------- constants */

enum
{
	NUMBER_OF_AVAILABLE_PRIMARY_COLORS = 18,
	NUMBER_OF_GOOD_RANDOM_COLORS = 3,
	NUMBER_OF_RANDOM_COLORS = 17,
	DEFAULT_LOOK_SENSITIVITY = 3
};

enum
{
	_default_player_profile_standard = 0,
	_default_player_profile_inverted
};

/* ---------- macros */

/* ---------- structures */

typedef char verify_player_profile_size[
	sizeof(struct player_profile) == 0x30 ? 1 : -1];

struct player_profile_file_block
{
	struct player_profile profile;
	XCALCSIG_SIGNATURE checksum;
	byte padding[SAVED_GAME_FILE_BLOCK_SIZE -
		sizeof(struct player_profile) - sizeof(XCALCSIG_SIGNATURE)];
};

typedef char verify_player_profile_file_block_size[
	sizeof(struct player_profile_file_block) == SAVED_GAME_FILE_BLOCK_SIZE ? 1 : -1];

struct player_profile_write_request
{
	long player_profile_index;
	struct player_profile profile;
};

struct player_profile_runtime_globals
{
	struct player_profile default_profile;
	struct player_profile_write_request write_request;
	struct thread_reference *thread;
	boolean initialized;
};

typedef char verify_player_profile_thread_offset[
	offsetof(struct player_profile_runtime_globals, thread) == 0x64 ? 1 : -1];
typedef char verify_player_profile_initialized_offset[
	offsetof(struct player_profile_runtime_globals, initialized) == 0x68 ? 1 : -1];
typedef char verify_player_profile_globals_size[
	sizeof(struct player_profile_runtime_globals) == 0x6C ? 1 : -1];

/* ---------- prototypes */

static void build_default_profile(
	struct player_profile *profile,
	long i);
static boolean player_profile_read(
	long player_profile_index,
	struct player_profile *profile);
static unsigned long __stdcall player_profile_write_thread_proc(
	void *input);
static void player_profile_create_default_profiles_on_disk(
	void);
static void player_profile_write(
	long player_profile_index,
	struct player_profile *profile);

/* ---------- globals */

static long profile_color_table[NUMBER_OF_AVAILABLE_PRIMARY_COLORS] =
{
	0x00FFFFFF,
	0x00000000,
	0x00FE0000,
	0x000201E3,
	0x00707E71,
	0x00FFFF01,
	0x0000FF01,
	0x00FF56B9,
	0x00AB10F4,
	0x0001FFFF,
	0x006493ED,
	0x00FF7F00,
	0x001ECC91,
	0x00006401,
	0x00603814,
	0x00C69C6C,
	0x009D0B0E,
	0x00F5999E,
};

struct player_profile_runtime_globals player_profile_globals = { 0 };

/* ---------- public code */

void player_profiles_dispose(
	void)
{
	if (player_profile_globals.thread)
	{
		error(
			_error_silent,
			"waiting for asynchronous player profile writes to finish...");
		while (!thread_has_exited(player_profile_globals.thread))
		{
		}
		dispose_thread(player_profile_globals.thread);
		player_profile_globals.thread = NULL;
	}

	csmemset(
		&player_profile_globals,
		0,
		sizeof(player_profile_globals));

	return;
}

void player_profiles_enumerate_available_to_local_player_index(
	short local_player_index,
	word *number_of_profiles,
	long *player_profile_indices,
	boolean include_default_profiles)
{
	saved_game_files_enumerate_available_to_local_player_index(
		local_player_index,
		_saved_game_file_type_player_profile,
		number_of_profiles,
		player_profile_indices,
		include_default_profiles);

	return;
}

void player_profile_delete(
	long player_profile_index)
{
	if (player_profile_index != NONE &&
		!delete_enumerated_saved_game_file(player_profile_index))
	{
		error(
			_error_silent,
			"player_profile_delete() failed (profile index= #0x%lX)",
			player_profile_index);
	}

	return;
}

boolean player_profile_get_from_path(
	char *full_path,
	struct player_profile *profile)
{
	boolean succeeded = FALSE;
	byte block[SAVED_GAME_FILE_BLOCK_SIZE];
	struct file_reference file;
	XCALCSIG_SIGNATURE checksum;

	match_assert("c:\\halo\\SOURCE\\saved games\\player_profile.c", 0xD8, full_path && profile);

	if (file_reference_create_from_path(&file, full_path, FALSE) &&
		file_open(&file, FLAG(_permission_read_bit)))
	{
		if (file_read(&file, SAVED_GAME_FILE_BLOCK_SIZE, block))
		{
			saved_game_file_generate_checksum(block, sizeof(struct player_profile), &checksum);

			if (!csmemcmp(&checksum, block+sizeof(struct player_profile), sizeof(checksum)))
			{
				csmemcpy(profile, block, sizeof(struct player_profile));
				succeeded = TRUE;
			}
			else
			{
				error(_error_silent, "checksum failed on player profile file");
			}
		}
		else
		{
			error(_error_silent, "failed to read player profile");
		}

		file_close(&file);
	}
	else
	{
		error(_error_silent, "failed to open player profile file");
	}

	return succeeded;
}

word player_profile_number_of_available_primary_colors(
	void)
{
	return NUMBER_OF_AVAILABLE_PRIMARY_COLORS;
}

real_rgb_color player_profile_get_rgb_color(
	long color_index)
{
	long color;
	real_rgb_color rgb_color;

	color_index = color_index < NUMBER_OF_AVAILABLE_PRIMARY_COLORS - 1 ?
		color_index : NUMBER_OF_AVAILABLE_PRIMARY_COLORS - 1;
	color_index = color_index < 0 ? 0 : color_index;
	color = profile_color_table[color_index];

	rgb_color.red = ((color >> 16) & 0xFF) / 255.f;
	rgb_color.green = ((color >> 8) & 0xFF) / 255.f;
	rgb_color.blue = (color & 0xFF) / 255.f;
	return rgb_color;
}

void player_profile_get_highest_completed_solo_level(
	struct player_profile *profile,
	short *level,
	short *difficulty)
{
	long i;

	match_assert("c:\\halo\\SOURCE\\saved games\\player_profile.c", 0x1B8, profile && level && difficulty);

	*level = NONE;
	*difficulty = _game_difficulty_level_normal;

	for (i = 0; i < NUMBER_OF_SINGLE_PLAYER_LEVELS; i++)
	{
		if (profile->single_player_map_flags[i])
		{
			if (TEST_FLAG(profile->single_player_map_flags[i], _game_difficulty_level_impossible))
			{
				*level = (short)i;
				*difficulty = _game_difficulty_level_impossible;
			}
			else if (TEST_FLAG(profile->single_player_map_flags[i], _game_difficulty_level_hard))
			{
				*level = (short)i;
				*difficulty = _game_difficulty_level_hard;
			}
			else if (TEST_FLAG(profile->single_player_map_flags[i], _game_difficulty_level_normal))
			{
				*level = (short)i;
				*difficulty = _game_difficulty_level_normal;
			}
			else if (TEST_FLAG(profile->single_player_map_flags[i], _game_difficulty_level_easy))
			{
				*level = (short)i;
				*difficulty = _game_difficulty_level_easy;
			}
		}
	}

	return;
}

boolean player_profile_get_enclosing_directory_path(
	long profile,
	char *full_path)
{
	return saved_game_file_get_path_to_enclosing_directory(profile, full_path);
}

/* port: port_config.c's (game.unlock_all_levels) */
int config_boolean(char const *name);

long player_profile_new(
	short local_player_index,
	wchar_t *name)
{
	struct file_reference profile_file;
	long player_profile_index = create_enumerated_saved_game_file(
		_saved_game_file_type_player_profile,
		local_player_index,
		name);

	if (player_profile_index != NONE)
	{
		if (saved_game_file_open(&profile_file, player_profile_index))
		{
			struct player_profile_file_block block = {0};
			struct player_profile *profile = &block.profile;
			long level;
			boolean succeeded;

			csmemset(profile, 0, sizeof(struct player_profile));
			profile->primary_color_index = NONE;
			profile->controller_settings.look_sensitivity = DEFAULT_LOOK_SENSITIVITY;
			profile->controller_settings.invert_look = FALSE;
			profile->controller_settings.flight_stick_aircraft_controls = FALSE;
			profile->controller_settings.ingame_help_disabled = FALSE;
			profile->controller_settings.vibration_disabled = FALSE;
			profile->last_single_player_map_played = 0;
			profile->controller_settings.button_preset = _button_preset_standard;
			profile->controller_settings.joystick_preset = _joystick_preset_standard;
			profile->flags = 0;
			ustrncpy(profile->player_name, name, MAXIMUM_PLAYER_PROFILE_NAME_LENGTH-1);
			profile->player_name[MAXIMUM_PLAYER_PROFILE_NAME_LENGTH-1] = 0;

			/* port: a new profile starts with no level finished, as on the
			Xbox (retail has no "### DEBUG unlocking all solo levels for newly
			created profile", which this debug build did for every new
			profile); config.toml's game.unlock_all_levels still does it */
			if (config_boolean("game.unlock_all_levels"))
			{
				for (level = 0; level < NUMBER_OF_SINGLE_PLAYER_LEVELS; level++)
				{
					long difficulty = 0;

					do
					{
						profile->single_player_map_flags[level] |= FLAG(difficulty);
						difficulty++;
					}
					while (difficulty < NUMBER_OF_GAME_DIFFICULTY_LEVELS);
				}
			}

			saved_game_file_generate_checksum(&block.profile, sizeof(block.profile),
				&block.checksum);

			succeeded = file_set_position(&profile_file, 0) &&
				file_write(&profile_file, sizeof(block), &block);
			saved_game_file_close(&profile_file, player_profile_index);

			if (!succeeded)
			{
				error(_error_silent, "failed to initialize newly created player profile");
				delete_enumerated_saved_game_file(player_profile_index);
				player_profile_index = NONE;
			}
		}
		else
		{
			error(_error_silent, "failed to open newly created player profile");
			delete_enumerated_saved_game_file(player_profile_index);
			player_profile_index = NONE;
		}
	}
	else
	{
		error(_error_silent, "failed to create new player profile");
	}

	return player_profile_index;
}

boolean player_profile_get(
	long player_profile_index,
	struct player_profile *profile)
{
	boolean succeeded = FALSE;

	match_vassert("c:\\halo\\SOURCE\\saved games\\player_profile.c", 0xC2, profile, "profile");

	if (player_profile_index == NONE)
	{
		csmemcpy(
			profile,
			&player_profile_globals.default_profile,
			sizeof(struct player_profile));
	}
	else
	{
		succeeded = player_profile_read(player_profile_index, profile);
	}

	return succeeded;
}

real_argb_color player_profile_get_argb_color(
	long color_index)
{
	real_argb_color argb_color;
	real_rgb_color rgb_color;

	rgb_color = player_profile_get_rgb_color(color_index);
	argb_color.alpha = 1.f;
	argb_color.rgb = rgb_color;

	return argb_color;
}

long player_profile_get_random_good_color(
	void)
{
	return seed_random_range(
		get_global_local_random_seed_address(),
		0,
		NUMBER_OF_GOOD_RANDOM_COLORS);
}

long player_profile_get_random_color(
	void)
{
	return seed_random_range(
		get_global_local_random_seed_address(),
		0,
		NUMBER_OF_RANDOM_COLORS);
}

void player_profiles_initialize(
	void)
{
	csmemset(
		&player_profile_globals,
		0,
		sizeof(player_profile_globals));
	player_profile_globals.initialized = TRUE;
	player_profile_create_default_profiles_on_disk();

	return;
}

void player_profile_save(
	long player_profile_index,
	struct player_profile *profile)
{
	match_vassert("c:\\halo\\SOURCE\\saved games\\player_profile.c", 0x100, profile, "profile");

	if (player_profile_index != NONE)
	{
		player_profile_write(player_profile_index, profile);
	}

	return;
}

void player_profile_save_last_level_played(
	short local_player_index)
{
	struct player_profile profile;
	short level = main_get_current_solo_level();
	long player_profile_index;

	match_assert("c:\\halo\\SOURCE\\saved games\\player_profile.c", 0x17A, (local_player_index>=0) && (local_player_index<MAXIMUM_NUMBER_OF_LOCAL_PLAYERS));

	if (level != NONE)
	{
		match_assert("c:\\halo\\SOURCE\\saved games\\player_profile.c", 0x17D, (level>=0) && (level<NUMBER_OF_SINGLE_PLAYER_LEVELS));

		player_profile_index = player_ui_get_active_player_profile_index(local_player_index);

		if (player_profile_index != NONE)
		{
			player_ui_get_active_player_profile(local_player_index, &profile);

			if (profile.last_single_player_map_played != level)
			{
				profile.last_single_player_map_played = level;
				player_profile_save(player_profile_index, &profile);
			}

			player_ui_set_active_player_profile(
				local_player_index,
				player_profile_index,
				&profile);
		}
	}

	return;
}

void player_profile_save_level_completed(
	short local_player_index)
{
	struct player_profile profile;
	short level;
	short difficulty;
	long player_profile_index;

	match_assert("c:\\halo\\SOURCE\\saved games\\player_profile.c", 0x197, (local_player_index>=0) && (local_player_index<MAXIMUM_NUMBER_OF_LOCAL_PLAYERS));

	level = main_get_current_solo_level();
	difficulty = game_difficulty_level_get();
	/* port: a level not in the campaign (a Custom Edition map's, played
	alone or as network co-op) is not the profile's to record */
	if (level == NONE)
		return;

	match_assert("c:\\halo\\SOURCE\\saved games\\player_profile.c", 0x19D, (level>=0) && (level<NUMBER_OF_SINGLE_PLAYER_LEVELS) && (difficulty >= 0) && (difficulty < NUMBER_OF_GAME_DIFFICULTY_LEVELS));

	player_profile_index = player_ui_get_active_player_profile_index(local_player_index);

	if (player_profile_index != NONE)
	{
		player_ui_get_active_player_profile(local_player_index, &profile);

		profile.single_player_map_flags[level] |= FLAG(difficulty);

		player_profile_write(player_profile_index, &profile);

		player_ui_set_active_player_profile(
			local_player_index,
			player_profile_index,
			&profile);
	}
	else
	{
		error(
			_error_silent,
			"failed to save player's current level as being completed because the player profile was not found");
	}

	return;
}

/* ---------- private code */

static void build_default_profile(
	struct player_profile *profile,
	long i)
{
	match_assert("c:\\halo\\SOURCE\\saved games\\player_profile.c", 0x237, (profile != NULL) && (i>=0) && (i<NUMBER_OF_DEFAULT_PROFILES));

	csmemset(profile, 0, sizeof(struct player_profile));

	profile->primary_color_index = NONE;
	profile->controller_settings.look_sensitivity = DEFAULT_LOOK_SENSITIVITY;
	profile->controller_settings.invert_look = FALSE;
	profile->controller_settings.flight_stick_aircraft_controls = FALSE;
	profile->controller_settings.ingame_help_disabled = FALSE;
	profile->flags |= (word)(FLAG(_player_profile_default_profile_bit) | (i<<8));
	profile->controller_settings.vibration_disabled = FALSE;
	profile->last_single_player_map_played = 0;

	switch (i)
	{
		case _default_player_profile_standard:
			profile->controller_settings.button_preset = _button_preset_standard;
			profile->controller_settings.joystick_preset = _joystick_preset_standard;
			break;

		case _default_player_profile_inverted:
			profile->controller_settings.invert_look = TRUE;
			profile->controller_settings.button_preset = _button_preset_standard;
			profile->controller_settings.joystick_preset = _joystick_preset_standard;
			break;

		default:
			match_vassert("c:\\halo\\SOURCE\\saved games\\player_profile.c", 0x252, FALSE, "unknown default profile configuration requested");
			break;
	}

	return;
}

static boolean player_profile_read(
	long player_profile_index,
	struct player_profile *profile)
{
	boolean succeeded = FALSE;
	byte block[SAVED_GAME_FILE_BLOCK_SIZE];
	struct file_reference file;
	XCALCSIG_SIGNATURE checksum;
	struct player_profile sanitized_profile;

	match_vassert("c:\\halo\\SOURCE\\saved games\\player_profile.c", 0x261, profile, "profile");

	if (player_profile_globals.thread)
	{
		error(
			_error_silent,
			"waiting for asynchronous player profile io to finish...");
		while (!thread_has_exited(player_profile_globals.thread))
		{
		}
		dispose_thread(player_profile_globals.thread);
		player_profile_globals.thread = NULL;
	}

	if (TEST_FLAG(player_profile_index, _saved_game_file_index_valid_bit))
	{
		if (saved_game_files_take_mutex())
		{
			if (saved_game_file_open(&file, player_profile_index))
			{
				if (file_read(&file, SAVED_GAME_FILE_BLOCK_SIZE, block))
				{
					saved_game_file_generate_checksum(block, sizeof(struct player_profile), &checksum);

					if (!csmemcmp(&checksum, block+sizeof(struct player_profile), sizeof(checksum)))
					{
						csmemcpy(profile, block, sizeof(struct player_profile));
					}
					else
					{
						error(
							_error_silent,
							"checksum failed on player profile file, sanitizing memory resident version...");

						csmemset(&sanitized_profile, 0, sizeof(sanitized_profile));
						sanitized_profile.primary_color_index = NONE;
						sanitized_profile.controller_settings.look_sensitivity = DEFAULT_LOOK_SENSITIVITY;
						sanitized_profile.controller_settings.invert_look = FALSE;
						sanitized_profile.controller_settings.flight_stick_aircraft_controls = FALSE;
						sanitized_profile.controller_settings.ingame_help_disabled = FALSE;
						sanitized_profile.controller_settings.vibration_disabled = FALSE;
						sanitized_profile.last_single_player_map_played = 0;
						sanitized_profile.controller_settings.button_preset = _button_preset_standard;
						sanitized_profile.controller_settings.joystick_preset = _joystick_preset_standard;
						sanitized_profile.flags = 0;
						ustrncpy(
							sanitized_profile.player_name,
							saved_game_file_get_display_name(player_profile_index),
							MAXIMUM_PLAYER_PROFILE_NAME_LENGTH-1);
						sanitized_profile.player_name[MAXIMUM_PLAYER_PROFILE_NAME_LENGTH-1] = 0;
						csmemcpy(profile, &sanitized_profile, sizeof(struct player_profile));
					}

					/* a sanitized profile still counts as a successful get */
					succeeded = TRUE;
				}
				else
				{
					error(_error_silent, "failed to read player profile from file");
				}

				saved_game_file_close(&file, player_profile_index);
			}
			else
			{
				error(_error_silent, "failed to open player profile file");
			}

			saved_game_files_release_mutex();
		}
		else
		{
			error(
				_error_silent,
				"failed to get saved game files mutex; perhaps another operation is in progress?");
		}
	}
	else
	{
		error(
			_error_silent,
			"checksum failed on player profile file, sanitizing memory resident version...");

		csmemset(&sanitized_profile, 0, sizeof(sanitized_profile));
		sanitized_profile.primary_color_index = NONE;
		sanitized_profile.controller_settings.look_sensitivity = DEFAULT_LOOK_SENSITIVITY;
		sanitized_profile.controller_settings.invert_look = FALSE;
		sanitized_profile.controller_settings.flight_stick_aircraft_controls = FALSE;
		sanitized_profile.controller_settings.ingame_help_disabled = FALSE;
		sanitized_profile.controller_settings.vibration_disabled = FALSE;
		sanitized_profile.last_single_player_map_played = 0;
		sanitized_profile.controller_settings.button_preset = _button_preset_standard;
		sanitized_profile.controller_settings.joystick_preset = _joystick_preset_standard;
		sanitized_profile.flags = 0;
		ustrncpy(
			sanitized_profile.player_name,
			saved_game_file_get_display_name(player_profile_index),
			MAXIMUM_PLAYER_PROFILE_NAME_LENGTH-1);
		sanitized_profile.player_name[MAXIMUM_PLAYER_PROFILE_NAME_LENGTH-1] = 0;
		csmemcpy(profile, &sanitized_profile, sizeof(struct player_profile));

		succeeded = TRUE;
	}

	return succeeded;
}

static unsigned long __stdcall player_profile_write_thread_proc(
	void *input)
{
	struct player_profile_write_request *request = input;
	struct player_profile_file_block block = {0};
	struct file_reference file;

	match_vassert("c:\\halo\\SOURCE\\saved games\\player_profile.c", 0x2D7, input, "input");

	error(_error_silent, "begin player profile write");

	if (saved_game_files_take_mutex())
	{
		long player_profile_index = request->player_profile_index;
		boolean failed = FALSE;
		struct player_profile *profile = &request->profile;

		if (saved_game_file_open(&file, player_profile_index))
		{
			csmemcpy(&block.profile, profile, sizeof(struct player_profile));
			saved_game_file_generate_checksum(&block.profile, sizeof(block.profile),
				&block.checksum);

			if (!file_set_position(&file, 0) ||
				!file_write(&file, sizeof(block), &block))
			{
				error(_error_silent, "failed to write player profile to file");
				failed = TRUE;
			}

			if (saved_game_file_close(&file, player_profile_index) &&
				!synchronize_metadata_display_name_with_profile_name(player_profile_index, profile->player_name))
			{
				error(_error_silent, "metadata name may not match game display name");
			}
		}
		else
		{
			error(_error_silent, "failed to open player profile file");
		}

		if (failed)
		{
			delete_enumerated_saved_game_file(player_profile_index);
		}

		saved_game_files_release_mutex();
	}
	else
	{
		error(
			_error_silent,
			"failed to get saved game files mutex; perhaps another operation is in progress?");
	}

	error(_error_silent, "end player profile write");

	return 0;
}

static void player_profile_create_default_profiles_on_disk(
	void)
{
	long i;

	for (i = 0; i < NUMBER_OF_DEFAULT_PROFILES; i++)
	{
		struct player_profile_file_block block = {0};
		char full_path[MAXIMUM_FILENAME_LENGTH+1];
		struct file_reference file;
		boolean succeeded = FALSE;

		build_default_profile(&block.profile, i);

		_snprintf(full_path, MAXIMUM_FILENAME_LENGTH, "z:\\saved\\player_profiles\\default_profile\\%02d.sav", i);

		if (file_reference_create_from_path(&file, full_path, FALSE))
		{
			saved_game_file_generate_checksum(&block.profile, sizeof(block.profile),
				&block.checksum);

			if (file_create(&file) &&
				file_open(&file, FLAG(_permission_write_bit)))
			{
				succeeded = file_set_position(&file, 0) &&
					file_write(&file, sizeof(block), &block);

				if (!file_close(&file))
				{
					error(
						_error_silent,
						"failed to close default player profile file '%s'",
						full_path);
				}
			}
		}

		if (!succeeded)
		{
			error(
				_error_silent,
				"failed to create/update default player profile file '%s' on disk",
				full_path);
		}
	}

	return;
}

static void player_profile_write(
	long player_profile_index,
	struct player_profile *profile)
{
	match_vassert("c:\\halo\\SOURCE\\saved games\\player_profile.c", 0x2C0, profile, "profile");

	if (player_profile_globals.thread)
	{
		error(
			_error_silent,
			"waiting for asynchronous player profile io to finish...");
		while (!thread_has_exited(player_profile_globals.thread))
		{
		}
		dispose_thread(player_profile_globals.thread);
		player_profile_globals.thread = NULL;
	}

	player_profile_globals.write_request.player_profile_index = player_profile_index;
	csmemcpy(
		&player_profile_globals.write_request.profile,
		profile,
		sizeof(struct player_profile));

	create_thread(
		0,
		player_profile_write_thread_proc,
		&player_profile_globals.write_request,
		&player_profile_globals.thread);

	return;
}
