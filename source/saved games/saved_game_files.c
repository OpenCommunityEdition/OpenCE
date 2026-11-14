/* ---------- headers */

#include "cseries/cseries.h"
#include "cseries/errors.h"
#include "bungie_net/common/thread.h"
#include "saved games/saved_game_files.h"
#include "input/input.h"
#include "memory/crc.h"
#include "tag_files/files.h"
#include "text/unicode.h"
#include "interface/ui_widget.h"
#include "interface/ui_widget_event_handler_functions.h"
#include "saved games/game_state.h"
#include "saved games/player_profile.h"
#include "saved games/playlist_profile.h"
#include "interface/player_ui.h"
#include "text/text_group.h"
#include "game_modes.h" /* port: port/linux/game/game_modes.c */
#include "mcc_progress.h" /* port: port/linux/game/mcc_progress.c */
#include "tag_files/tag_groups.h"
/* the saved game file checksum is an XDK content signature, and enumerated
   saved game metadata is created/deleted through the XDK save game API. */
#include <xtl.h>

/* ---------- constants */

enum
{
	SAVED_GAME_FILES_MUTEX_TIMEOUT = MILLISECONDS_PER_SECOND*SECONDS_PER_MINUTE*MINUTES_PER_HOUR
};

enum
{
	MAXIMUM_ENUMERATED_SAVED_GAME_FILES_ANY_TYPE_PER_MEMORY_UNIT = 100
};

enum
{
	MAXIMUM_SAVED_GAME_NAME_LENGTH = MAX_GAMENAME
};

enum
{
	NUMBER_OF_SUPPORTED_MEMORY_UNITS = 1
};

enum
{
	MEMORY_UNIT_ROOT_PATH_SIZE = 8
};

enum
{
	MAXIMUM_UNTITLED_SAVED_GAMES = 999
};

/* index of the "untitled saved game" name format inside
   ui\\saved_game_file_strings */
enum
{
	_saved_game_file_string_untitled_name_format = 2
};

enum
{
	MINIMUM_FREE_DISK_SPACE = 40*1024*1024
};

enum
{
	_saved_game_file_system_ok = 0,
	_saved_game_file_system_out_of_disk_space,
	_saved_game_file_system_too_many_saved_games,
	NUMBER_OF_SAVED_GAME_FILE_SYSTEM_CHECK_RESULTS
};

/* a saved game file is verified by checksumming the leading bytes of its profile
   block and comparing that against the signature stored immediately after them */
enum
{
	PLAYER_PROFILE_CHECKSUM_DATA_SIZE = 48,
	PLAYLIST_PROFILE_CHECKSUM_DATA_SIZE = 104
};



/* ---------- macros */

/* an enumerated saved game file is identified by a packed profile index:
   [31] and [30] are flags, [27..16] the file index within the memory unit,
   [15..8] the memory unit and [3..0] the saved game file type. */
#define SAVED_GAME_FILE_INDEX_TYPE(profile_index) ((profile_index)&0xF)
#define SAVED_GAME_FILE_INDEX_MEMORY_UNIT(profile_index) (((profile_index)>>8)&0xFF)
#define SAVED_GAME_FILE_INDEX_FILE_INDEX(profile_index) (((profile_index)>>16)&0xFFF)
#define SAVED_GAME_FILE_INDEX_BUILD(n, memory_unit_index, type) \
	(((((n)&0xFFF)<<8)|((memory_unit_index)&0xFF))<<8|((type)&0xF))

/* ---------- structures */

/* one record of a memory unit's mapfile; 0x206 bytes, written and read whole */
struct enumerated_saved_game_file
{
	char path[MAXIMUM_FILENAME_LENGTH+1];
	wchar_t display_name[MAX_GAMENAME];
	short type;
	short index;
	boolean read_only;
	boolean valid;
};

typedef char verify_enumerated_saved_game_file_size[
	sizeof(struct enumerated_saved_game_file) == 0x206 ? 1 : -1];

struct saved_game_files_globals
{
	struct file_reference memory_unit_mapfile;
	struct mutex_reference *general_mutex;
	struct mutex_reference *mapfile_mutex;
	short next_enumerated_profile_index;
	boolean initialized;
	boolean memory_units_dirty;
	boolean enumeration_in_progress;
};

typedef char verify_saved_game_files_globals_memory_units_dirty_offset[
	offsetof(struct saved_game_files_globals, memory_units_dirty) == 0x117 ? 1 : -1];
typedef char verify_saved_game_files_globals_size[
	sizeof(struct saved_game_files_globals) == 0x11C ? 1 : -1];

/* ---------- prototypes */

static boolean find_and_create_directory_if_necessary(
	char const *path);
static boolean enumerate_saved_game_file(
	struct enumerated_saved_game_file *file);
static boolean enumerate_saved_game_files_start(
	word memory_unit);
static boolean enumerate_mapfile_start(
	word memory_unit_index);
static boolean enumerate_mapfile_end(
	word memory_unit_index);
static boolean enumerate_saved_game_file_from_mapfile(
	struct enumerated_saved_game_file *file);
static long count_enumerated_profiles_in_mapfile(
	word memory_unit_index);
static long build_saved_game_file_index(
	long type,
	long memory_unit_index,
	long n,
	boolean read_only,
	boolean valid);
static boolean enumerate_saved_game_files_end(
	word memory_unit);
static short enumerate_default_playlist_profiles(
	void);
static short enumerate_default_player_profiles(
	void);
static boolean get_nth_entry_in_mapfile(
	word memory_unit_index,
	word n,
	struct enumerated_saved_game_file *file);
static boolean set_nth_entry_in_mapfile(
	word memory_unit_index,
	word n,
	struct enumerated_saved_game_file *file);
static boolean remove_nth_entry_in_mapfile(
	word memory_unit_index,
	word n);
static boolean append_entry_to_mapfile(
	word memory_unit_index,
	struct enumerated_saved_game_file *file,
	long *profile_index);
static void enumerate_memory_units(
	void);
static boolean saved_game_files_take_mapfile_mutex(
	void);
static void saved_game_files_release_mapfile_mutex(
	void);
static short enumerate_default_profiles(
	void);

/* ---------- globals */

/* only the hard drive is supported by this build */
static wchar_t *memory_unit_root_path[NUMBER_OF_SUPPORTED_MEMORY_UNITS] =
{
	L"u:\\"
};

static char *memory_unit_mapfile_path[NUMBER_OF_SUPPORTED_MEMORY_UNITS] =
{
	"z:\\saved\\hdmu.map"
};

static wchar_t saved_game_file_display_name[MAX_GAMENAME];

struct saved_game_files_globals saved_game_files_globals = {0};

/* ---------- public code */

void saved_game_files_initialize(
	void)
{
	if (!find_and_create_directory_if_necessary("z:\\saved"))
	{
		error(_error_silent, "failed to find/create '%s' directory", "z:\\saved");
	}

	if (!find_and_create_directory_if_necessary("z:\\saved\\player_profiles"))
	{
		error(_error_silent, "failed to find/create '%s' directory", "z:\\saved\\player_profiles");
	}

	if (!find_and_create_directory_if_necessary("z:\\saved\\player_profiles\\default_profile"))
	{
		error(_error_silent, "failed to find/create '%s' directory", "z:\\saved\\player_profiles\\default_profile");
	}

	if (!find_and_create_directory_if_necessary("z:\\saved\\playlists"))
	{
		error(_error_silent, "failed to find/create '%s' directory", "z:\\saved\\playlists");
	}

	if (!find_and_create_directory_if_necessary("z:\\saved\\playlists\\default_playlist"))
	{
		error(_error_silent, "failed to find/create '%s' directory", "z:\\saved\\playlists\\default_playlist");
	}

	if (!find_and_create_directory_if_necessary("z:\\saved\\recordings"))
	{
		error(_error_silent, "failed to find/create '%s' directory", "z:\\saved\\recordings");
	}

	if (!find_and_create_directory_if_necessary("z:\\saved\\recordings\\last_recording"))
	{
		error(_error_silent, "failed to find/create '%s' directory", "z:\\saved\\recordings\\last_recording");
	}

	csmemset(&saved_game_files_globals, 0, sizeof(struct saved_game_files_globals));
	saved_game_files_globals.memory_units_dirty = TRUE;
	saved_game_files_globals.general_mutex = NULL;
	saved_game_files_globals.mapfile_mutex = NULL;

	saved_game_files_globals.initialized =
		create_mutex(&saved_game_files_globals.general_mutex) &&
		create_mutex(&saved_game_files_globals.mapfile_mutex);

	match_vassert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		191,
		saved_game_files_globals.initialized,
		"failed to initialize saved game files");

	player_profiles_initialize();
	playlist_profiles_initialize();

	return;
}

void saved_game_files_dispose(
	void)
{
	if (saved_game_files_globals.general_mutex)
	{
		dispose_mutex(saved_game_files_globals.general_mutex);
		saved_game_files_globals.general_mutex = NULL;
	}

	if (saved_game_files_globals.mapfile_mutex)
	{
		dispose_mutex(saved_game_files_globals.mapfile_mutex);
		saved_game_files_globals.mapfile_mutex = NULL;
	}

	player_profiles_dispose();
	playlist_profiles_dispose();
	saved_game_files_globals.initialized = FALSE;

	return;
}

boolean saved_game_file_close(
	struct file_reference *saved_game_file,
	long profile_index)
{
	long type = SAVED_GAME_FILE_INDEX_TYPE(profile_index);
	long memory_unit = SAVED_GAME_FILE_INDEX_MEMORY_UNIT(profile_index);
	long n = SAVED_GAME_FILE_INDEX_FILE_INDEX(profile_index);
	boolean success;

	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		603,
		memory_unit==_memory_unit_hard_drive);

	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		606,
		saved_game_file);
	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		607,
		(type >= 0) && (type < NUMBER_OF_SAVED_GAME_FILE_TYPES));
	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		608,
		(memory_unit >= 0) && (memory_unit < NUMBER_OF_MEMORY_UNITS));
	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		609,
		(n >= 0) && (n < MAXIMUM_ENUMERATED_SAVED_GAME_FILES_ANY_TYPE_PER_MEMORY_UNIT));

	success = file_close(saved_game_file) && (memory_unit==_memory_unit_hard_drive);

	return success;
}

word saved_game_file_get_type(
	long profile_index)
{
	return SAVED_GAME_FILE_INDEX_TYPE(profile_index);
}

void saved_game_files_notify_memory_units_changed(
	void)
{
	saved_game_files_globals.memory_units_dirty = TRUE;

	return;
}

boolean saved_game_files_take_mutex(
	void)
{
	return take_mutex(saved_game_files_globals.general_mutex, SAVED_GAME_FILES_MUTEX_TIMEOUT);
}

void saved_game_files_release_mutex(
	void)
{
	release_mutex(saved_game_files_globals.general_mutex);

	return;
}

short saved_game_perform_file_system_checks(
	void)
{
	ULARGE_INTEGER free_bytes_available;
	char root_path[MEMORY_UNIT_ROOT_PATH_SIZE];
	ULARGE_INTEGER total_number_of_free_bytes;
	ULARGE_INTEGER total_number_of_bytes;
	XGAME_FIND_DATA find_data;
	short result = _saved_game_file_system_ok;

	if (GetDiskFreeSpaceEx(
			wide_to_ascii(memory_unit_root_path[_memory_unit_hard_drive], root_path, sizeof(root_path)),
			&free_bytes_available,
			&total_number_of_bytes,
			&total_number_of_free_bytes) &&
		free_bytes_available.QuadPart < MINIMUM_FREE_DISK_SPACE)
	{
		result = _saved_game_file_system_out_of_disk_space;
	}
	else
	{
		HANDLE find_handle = XFindFirstSaveGame(
			wide_to_ascii(memory_unit_root_path[_memory_unit_hard_drive], root_path, sizeof(root_path)),
			&find_data);
		unsigned long number_of_saved_games = 1;

		if (find_handle != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (number_of_saved_games >= MAXIMUM_ENUMERATED_SAVED_GAME_FILES_ANY_TYPE_PER_MEMORY_UNIT)
				{
					break;
				}

				number_of_saved_games++;
			}
			while ((boolean)XFindNextSaveGame(find_handle, &find_data) == TRUE);

			if (!XFindClose(find_handle))
			{
				error(_error_silent, "XFindClose() failed");
			}

			if (number_of_saved_games >= MAXIMUM_ENUMERATED_SAVED_GAME_FILES_ANY_TYPE_PER_MEMORY_UNIT)
			{
				result = _saved_game_file_system_too_many_saved_games;
			}
		}
	}

	return result;
}

boolean saved_game_file_name_unique(
	wchar_t const *name)
{
	char save_game_directory[MAXIMUM_FILENAME_LENGTH+1];
	char root_path[MEMORY_UNIT_ROOT_PATH_SIZE];
	boolean unique = FALSE;

	if (name && name[0] &&
		XCreateSaveGame(
			wide_to_ascii(memory_unit_root_path[_memory_unit_hard_drive], root_path, sizeof(root_path)),
			name,
			OPEN_EXISTING,
			0,
			save_game_directory,
			sizeof(save_game_directory)))
	{
		unique = TRUE;
	}

	return unique;
}

void saved_game_file_remember_player1_last_used_profile_directory(
	char const *directory_path)
{
	struct file_reference file;

	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		1050,
		directory_path);

	if (file_reference_create_from_path(&file, "z:\\lastprof.txt", FALSE) &&
		file_create(&file) &&
		file_open(&file, FLAG(_permission_write_bit)))
	{
		if (!file_write(&file, MAXIMUM_FILENAME_LENGTH+1, directory_path))
		{
			error(_error_silent, "failed to write to '%s'", "z:\\lastprof.txt");
		}

		file_close(&file);
	}
	else
	{
		error(_error_silent, "failed to open '%s'", "z:\\lastprof.txt");
	}

	return;
}

boolean saved_game_file_retrieve_player1_last_used_profile_directory(
	char *directory_path)
{
	struct file_reference file;
	boolean success = FALSE;

	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		1076,
		directory_path);

	if (file_reference_create_from_path(&file, "z:\\lastprof.txt", FALSE) &&
		file_open(&file, FLAG(_permission_read_bit)))
	{
		success = file_read(&file, MAXIMUM_FILENAME_LENGTH+1, directory_path);

		if (!success)
		{
			error(_error_silent, "failed to read from '%s'", "z:\\lastprof.txt");
		}

		file_close(&file);
	}
	else
	{
		error(_error_silent, "failed to open '%s'", "z:\\lastprof.txt");
	}

	directory_path[MAXIMUM_FILENAME_LENGTH] = 0;

	return success;
}

void saved_game_file_remember_last_used_multiplayer_variant_directory(
	char const *directory_path)
{
	struct file_reference file;

	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		1102,
		directory_path);

	if (file_reference_create_from_path(&file, "z:\\lastmpvr.txt", FALSE) &&
		file_create(&file) &&
		file_open(&file, FLAG(_permission_write_bit)))
	{
		if (!file_write(&file, MAXIMUM_FILENAME_LENGTH+1, directory_path))
		{
			error(_error_silent, "failed to write to '%s'", "z:\\lastmpvr.txt");
		}

		file_close(&file);
	}
	else
	{
		error(_error_silent, "failed to open '%s'", "z:\\lastmpvr.txt");
	}

	return;
}

boolean saved_game_file_retrieve_last_used_multiplayer_variant_directory(
	char *directory_path)
{
	struct file_reference file;
	boolean success = FALSE;

	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		1128,
		directory_path);

	if (file_reference_create_from_path(&file, "z:\\lastmpvr.txt", FALSE) &&
		file_open(&file, FLAG(_permission_read_bit)))
	{
		success = file_read(&file, MAXIMUM_FILENAME_LENGTH+1, directory_path);

		if (!success)
		{
			error(_error_silent, "failed to read from '%s'", "z:\\lastmpvr.txt");
		}

		file_close(&file);
	}
	else
	{
		error(_error_silent, "failed to open '%s'", "z:\\lastmpvr.txt");
	}

	directory_path[MAXIMUM_FILENAME_LENGTH] = 0;

	return success;
}

void saved_game_file_remember_last_used_multiplayer_map(
	char const *map_name)
{
	struct file_reference file;
	/* port: the name in a buffer of the size written (callers' may be
	smaller: the menus' map list's are 64 bytes), the rest zeros */
	char name[MAXIMUM_FILENAME_LENGTH+1];

	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		1207,
		map_name);
	csmemset(name, 0, sizeof(name));
	csstrncpy(name, map_name, sizeof(name) - 1);

	if (file_reference_create_from_path(&file, "z:\\lastmpmp.txt", FALSE) &&
		file_create(&file) &&
		file_open(&file, FLAG(_permission_write_bit)))
	{
		if (!file_write(&file, MAXIMUM_FILENAME_LENGTH+1, name))
		{
			error(_error_silent, "failed to write to '%s'", "z:\\lastmpmp.txt");
		}

		file_close(&file);
	}
	else
	{
		error(_error_silent, "failed to open '%s'", "z:\\lastmpmp.txt");
	}

	return;
}

boolean saved_game_file_retrieve_last_used_multiplayer_map(
	char *map_name)
{
	struct file_reference file;
	boolean success = FALSE;

	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		1233,
		map_name);

	if (file_reference_create_from_path(&file, "z:\\lastmpmp.txt", FALSE) &&
		file_open(&file, FLAG(_permission_read_bit)))
	{
		success = file_read(&file, MAXIMUM_FILENAME_LENGTH+1, map_name);

		if (!success)
		{
			error(_error_silent, "failed to read from '%s'", "z:\\lastmpmp.txt");
		}

		file_close(&file);
	}
	else
	{
		error(_error_silent, "failed to open '%s'", "z:\\lastmpmp.txt");
	}

	map_name[MAXIMUM_FILENAME_LENGTH] = 0;

	return success;
}

void saved_game_file_generate_checksum(
	void const *buffer,
	word buffer_size,
	XCALCSIG_SIGNATURE *checksum)
{
	HANDLE signature;

	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		1259,
		buffer);

	signature = XCalculateSignatureBegin(0);

	if (signature != INVALID_HANDLE_VALUE)
	{
		if (XCalculateSignatureUpdate(signature, buffer, buffer_size))
		{
			error(_error_silent, "XCalculateSignatureUpdate() failed");
		}

		if (XCalculateSignatureEnd(signature, checksum))
		{
			error(_error_silent, "XCalculateSignatureEnd() failed");
		}
	}
	else
	{
		error(_error_silent, "XCalculateSignatureBegin() failed");
	}

	return;
}

static boolean saved_game_files_take_mapfile_mutex(
	void)
{
	return take_mutex(saved_game_files_globals.mapfile_mutex, SAVED_GAME_FILES_MUTEX_TIMEOUT);
}

static void saved_game_files_release_mapfile_mutex(
	void)
{
	release_mutex(saved_game_files_globals.mapfile_mutex);

	return;
}

wchar_t *saved_game_file_get_display_name(
	long profile_index)
{
	struct enumerated_saved_game_file file;
	long memory_unit = SAVED_GAME_FILE_INDEX_MEMORY_UNIT(profile_index);
	long n = SAVED_GAME_FILE_INDEX_FILE_INDEX(profile_index);

	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		299,
		memory_unit==_memory_unit_hard_drive);

	saved_game_file_display_name[0] = 0;

	if ((memory_unit >= 0) && (memory_unit < NUMBER_OF_MEMORY_UNITS) &&
		(n >= 0) && (n < MAXIMUM_ENUMERATED_SAVED_GAME_FILES_ANY_TYPE_PER_MEMORY_UNIT))
	{
		if (get_nth_entry_in_mapfile(memory_unit, n, &file))
		{
			ustrncpy(saved_game_file_display_name, file.display_name, MAX_GAMENAME-1);
			saved_game_file_display_name[MAX_GAMENAME-1] = 0;
		}
	}
	else
	{
		error(_error_silent, "invalid saved game file index");
	}

	return saved_game_file_display_name;
}

boolean saved_game_file_open(
	struct file_reference *saved_game_file,
	long profile_index)
{
	struct enumerated_saved_game_file file;
	long type = SAVED_GAME_FILE_INDEX_TYPE(profile_index);
	long memory_unit = SAVED_GAME_FILE_INDEX_MEMORY_UNIT(profile_index);
	long n = SAVED_GAME_FILE_INDEX_FILE_INDEX(profile_index);
	boolean success;

	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		577,
		memory_unit==_memory_unit_hard_drive);

	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		580,
		saved_game_file);
	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		581,
		(type >= 0) && (type < NUMBER_OF_SAVED_GAME_FILE_TYPES));
	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		582,
		(memory_unit >= 0) && (memory_unit < NUMBER_OF_MEMORY_UNITS));
	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		583,
		(n >= 0) && (n < MAXIMUM_ENUMERATED_SAVED_GAME_FILES_ANY_TYPE_PER_MEMORY_UNIT));

	success = get_nth_entry_in_mapfile(memory_unit, n, &file) &&
		file_reference_create_from_path(saved_game_file, file.path, FALSE) &&
		(memory_unit==_memory_unit_hard_drive) &&
		file_open(saved_game_file, FLAG(_permission_read_bit)|FLAG(_permission_write_bit));

	return success;
}

boolean synchronize_metadata_display_name_with_profile_name(
	long profile_index,
	wchar_t *game_display_name)
{
	struct enumerated_saved_game_file file;
	boolean success = TRUE;
	long type = SAVED_GAME_FILE_INDEX_TYPE(profile_index);
	long memory_unit = SAVED_GAME_FILE_INDEX_MEMORY_UNIT(profile_index);
	long n = SAVED_GAME_FILE_INDEX_FILE_INDEX(profile_index);

	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		774,
		memory_unit==_memory_unit_hard_drive);

	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		777,
		(type >= 0) && (type < NUMBER_OF_SAVED_GAME_FILE_TYPES));
	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		778,
		(memory_unit >= 0) && (memory_unit < NUMBER_OF_MEMORY_UNITS));
	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		779,
		(n >= 0) && (n < MAXIMUM_ENUMERATED_SAVED_GAME_FILES_ANY_TYPE_PER_MEMORY_UNIT));

	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		781,
		ustrlen(game_display_name)<MAXIMUM_SAVED_GAME_NAME_LENGTH);

	if (get_nth_entry_in_mapfile(memory_unit, n, &file))
	{
		if ((game_display_name != NULL) && (game_display_name[0] != 0) &&
			ustrcmp(file.display_name, game_display_name))
		{
			if (memory_unit == _memory_unit_hard_drive)
			{
				char root_path[MEMORY_UNIT_ROOT_PATH_SIZE] = {0};
				char save_game_directory[MAXIMUM_FILENAME_LENGTH+1] = {0};

				if (!XCreateSaveGame(
						wide_to_ascii(memory_unit_root_path[memory_unit], root_path, sizeof(root_path)),
						game_display_name,
						CREATE_NEW,
						0,
						save_game_directory,
						sizeof(save_game_directory)))
				{
					char new_path[MAXIMUM_FILENAME_LENGTH+1];

					switch (file.type)
					{
						case _saved_game_file_type_player_profile:
						{
							char old_persistent_storage_path[MAXIMUM_FILENAME_LENGTH+1];
							char new_persistent_storage_path[MAXIMUM_FILENAME_LENGTH+1];
							char *filename;

							_snprintf(new_path, MAXIMUM_FILENAME_LENGTH, "%s%s", save_game_directory, "blam.sav");
							new_path[MAXIMUM_FILENAME_LENGTH] = 0;
							success = (boolean)CopyFileA(file.path, new_path, TRUE);
							/* port: preserve the MCC-only companion before the old directory is deleted. */
							if (success) success = mcc_progress_copy(file.path, save_game_directory);

							if (success == TRUE)
							{
								csstrncpy(old_persistent_storage_path, file.path, MAXIMUM_FILENAME_LENGTH);
								old_persistent_storage_path[MAXIMUM_FILENAME_LENGTH] = 0;
								filename = strstr(old_persistent_storage_path, "blam.sav");

								if (filename)
								{
									*filename = 0;
									csstrncat(old_persistent_storage_path,
										game_state_get_persistent_storage_filename(), MAXIMUM_FILENAME_LENGTH);
									old_persistent_storage_path[MAXIMUM_FILENAME_LENGTH] = 0;
									_snprintf(new_persistent_storage_path, MAXIMUM_FILENAME_LENGTH, "%s%s",
										save_game_directory, game_state_get_persistent_storage_filename());
									success = (boolean)CopyFileA(
										old_persistent_storage_path, new_persistent_storage_path, TRUE);
								}
								else
								{
									success = FALSE;
								}
							}
							break;
						}

						case _saved_game_file_type_game_variant:
							_snprintf(new_path, MAXIMUM_FILENAME_LENGTH, "%s%s", save_game_directory, "blam.lst");
							new_path[MAXIMUM_FILENAME_LENGTH] = 0;
							success = (boolean)CopyFileA(file.path, new_path, TRUE);
							break;

						default:
							match_assert(
								"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
								835,
								!"unknown enumerated file type");
							success = FALSE;
							break;
					}

					if (success == TRUE)
					{
						if (XDeleteSaveGame(root_path, file.display_name))
						{
							error(_error_silent, "XDeleteSaveGame() failed to delete old saved game metadata in rename_metadata_display_name()");
						}

						csstrncpy(file.path, new_path, MAXIMUM_FILENAME_LENGTH);
						file.path[MAXIMUM_FILENAME_LENGTH] = 0;
						ustrncpy(file.display_name, game_display_name, MAXIMUM_SAVED_GAME_NAME_LENGTH-1);
						file.display_name[MAXIMUM_SAVED_GAME_NAME_LENGTH-1] = 0;

						if (!set_nth_entry_in_mapfile(memory_unit, n, &file))
						{
							error(_error_silent, "failed to update memory unit mapfile after renaming saved game metadata");
						}
					}
					else if (!success)
					{
						if (XDeleteSaveGame(root_path, game_display_name))
						{
							error(_error_silent, "XDeleteSaveGame() failed to delete empty saved game metadata in rename_metadata_display_name()");
						}
					}
				}
				else
				{
					error(_error_silent, "XCreateSaveGame() failed in synchronize_metadata_display_name_with_profile_name() (maybe the name is already in use?)");
				}
			}
			else
			{
				error(_error_silent, "failed to mount memory unit #%d", memory_unit);
			}
		}
	}
	else
	{
		error(_error_silent, "get_nth_entry_in_mapfile() failed");
	}

	return success;
}

boolean saved_game_file_get_path_to_enclosing_directory(
	long profile_index,
	char *full_path)
{
	struct enumerated_saved_game_file file;
	boolean success = FALSE;

	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		909,
		full_path);

	full_path[0] = 0;

	if (profile_index != NONE)
	{
		long memory_unit = SAVED_GAME_FILE_INDEX_MEMORY_UNIT(profile_index);
		long n = SAVED_GAME_FILE_INDEX_FILE_INDEX(profile_index);

		match_assert(
			"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
			919,
			memory_unit==_memory_unit_hard_drive);

		if ((memory_unit >= 0) && (memory_unit < NUMBER_OF_MEMORY_UNITS) &&
			(n >= 0) && (n < MAXIMUM_ENUMERATED_SAVED_GAME_FILES_ANY_TYPE_PER_MEMORY_UNIT))
		{
			if (get_nth_entry_in_mapfile(memory_unit, n, &file))
			{
				char const *filename = NULL;
				char *directory_end;

				if (file.type == _saved_game_file_type_player_profile)
				{
					filename = "blam.sav";
				}
				else if (file.type == _saved_game_file_type_game_variant)
				{
					filename = "blam.lst";
				}
				else
				{
					error(_error_silent, "unknown saved game file type");
				}

				if (filename)
				{
					csstrncpy(full_path, file.path, MAXIMUM_FILENAME_LENGTH);
					full_path[MAXIMUM_FILENAME_LENGTH] = 0;

					directory_end = strstr(full_path, filename);

					if (directory_end)
					{
						*directory_end = 0;
						success = TRUE;
					}
					else
					{
						error(_error_silent, "player profile pathname doesn't appear to be valid");
						full_path[0] = 0;
					}
				}
			}
			else
			{
				error(_error_silent, "unable to locate the specified player profile file in memory unit mapfile");
			}
		}
		else
		{
			error(_error_silent, "invalid saved game file index");
		}
	}

	return success;
}

static boolean set_nth_entry_in_mapfile(
	word memory_unit_index,
	word n,
	struct enumerated_saved_game_file *file)
{
	boolean success = FALSE;

	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		2018,
		memory_unit_index==_memory_unit_hard_drive);

	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		2020,
		!saved_game_files_globals.enumeration_in_progress);
	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		2021,
		(memory_unit_index < NUMBER_OF_MEMORY_UNITS) && (file != NULL));

	if (saved_game_files_take_mapfile_mutex())
	{
		if (file_reference_create_from_path(&saved_game_files_globals.memory_unit_mapfile,
				memory_unit_mapfile_path[memory_unit_index], FALSE) &&
			file_open(&saved_game_files_globals.memory_unit_mapfile, FLAG(_permission_write_bit)))
		{
			unsigned long mapfile_size = file_get_eof(&saved_game_files_globals.memory_unit_mapfile);
			unsigned long entry_offset = n*sizeof(struct enumerated_saved_game_file);

			if (mapfile_size%sizeof(struct enumerated_saved_game_file))
			{
				error(_error_silent, "memory unit mapfile for memory unit #%d is possibly corrupt", memory_unit_index);
			}

			if (entry_offset+sizeof(struct enumerated_saved_game_file) <= mapfile_size)
			{
				if (file_set_position(&saved_game_files_globals.memory_unit_mapfile, entry_offset) &&
					file_write(&saved_game_files_globals.memory_unit_mapfile, sizeof(struct enumerated_saved_game_file), file))
				{
					success = TRUE;
				}
				else
				{
					success = FALSE;
					error(_error_silent, "failed to update entry #%d from memory unit mapfile (#%d)", n, memory_unit_index);
				}
			}
			else
			{
				error(_error_silent, "invalid profile index (#%d) into memory unit #%d specified", n, memory_unit_index);
			}

			if (!file_close(&saved_game_files_globals.memory_unit_mapfile))
			{
				error(_error_silent, "failed to close memory unit mapfile for memory unit #%d", memory_unit_index);
				success = FALSE;
			}
		}
		else
		{
			error(_error_silent, "failed to open memory unit mapfile for memory unit #%d", memory_unit_index);
		}

		saved_game_files_release_mapfile_mutex();
	}
	else
	{
		error(_error_silent, "failed to take mapfile mutex");
	}

	return success;
}

static short enumerate_default_profiles(
	void)
{
	short number_of_playlist_files = enumerate_default_playlist_profiles();
	short number_of_player_profile_files = enumerate_default_player_profiles();

	return number_of_playlist_files+number_of_player_profile_files;
}

long create_enumerated_saved_game_file(
	word saved_game_file_type,
	short local_player_index,
	wchar_t *display_name)
{
	byte block[SAVED_GAME_FILE_BLOCK_SIZE];
	struct file_reference saved_game_file;
	long new_profile_index = NONE;
	short file_system_check;

	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		332,
		(saved_game_file_type<NUMBER_OF_SAVED_GAME_FILE_TYPES) && ((local_player_index==NONE) || (local_player_index<MAXIMUM_GAMEPADS)) && (display_name != NULL));

	if (saved_game_files_globals.memory_units_dirty)
	{
		enumerate_memory_units();
	}

	file_system_check = saved_game_perform_file_system_checks();

	switch (file_system_check)
	{
		case _saved_game_file_system_out_of_disk_space:
			display_error_abort_to_dashboard_deferred(33, TRUE);
			break;

		case _saved_game_file_system_too_many_saved_games:
			display_error_abort_to_dashboard_deferred(34, TRUE);
			break;
	}

	if (file_system_check == _saved_game_file_system_ok)
	{
		long number_of_entries = count_enumerated_profiles_in_mapfile(_memory_unit_hard_drive);

		if (number_of_entries < MAXIMUM_ENUMERATED_SAVED_GAME_FILES_ANY_TYPE_PER_MEMORY_UNIT)
		{
			char root_path[MEMORY_UNIT_ROOT_PATH_SIZE] = {0};
			char save_game_directory[MAXIMUM_FILENAME_LENGTH+1] = {0};

			if (!XCreateSaveGame(
					wide_to_ascii(memory_unit_root_path[_memory_unit_hard_drive], root_path, sizeof(root_path)),
					display_name,
					CREATE_NEW,
					0,
					save_game_directory,
					sizeof(save_game_directory)))
			{
				struct enumerated_saved_game_file file = {0};
				long profile_index;
				word checksum_data_size;

				ustrncpy(file.display_name, display_name, MAX_GAMENAME-1);
				file.type = saved_game_file_type;
				file.display_name[MAX_GAMENAME-1] = 0;
				file.index = number_of_entries;
				file.read_only = FALSE;
				file.valid = FALSE;

				switch (saved_game_file_type)
				{
					case _saved_game_file_type_player_profile:
						_snprintf(file.path, MAXIMUM_FILENAME_LENGTH, "%s%s", save_game_directory, "blam.sav");
						checksum_data_size = PLAYER_PROFILE_CHECKSUM_DATA_SIZE;
						game_state_create_persistent_storage(save_game_directory);
						break;

					case _saved_game_file_type_game_variant:
						_snprintf(file.path, MAXIMUM_FILENAME_LENGTH, "%s%s", save_game_directory, "blam.lst");
						checksum_data_size = PLAYLIST_PROFILE_CHECKSUM_DATA_SIZE;
						break;

					default:
						file.type = NONE;
						break;
				}

				if (file.type != NONE)
				{
					if (file_reference_create_from_path(&saved_game_file, file.path, FALSE) &&
						file_create(&saved_game_file))
					{
						if (file_open(&saved_game_file, FLAG(_permission_write_bit)))
						{
							csmemset(block, 0, sizeof(block));
							saved_game_file_generate_checksum(block, checksum_data_size,
								(XCALCSIG_SIGNATURE *)(block+checksum_data_size));

							if (file_write(&saved_game_file, sizeof(block), block))
							{
								file.valid = TRUE;
							}

							if (!file_close(&saved_game_file))
							{
								error(_error_silent, "file_close() failed in create_enumerated_saved_game_file()");
							}
						}
						else
						{
							error(_error_silent, "failed to write blank saved game file block to disk");
						}

						if (append_entry_to_mapfile(_memory_unit_hard_drive, &file, &profile_index))
						{
							match_assert(
								"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
								434,
								profile_index == file.index);

							new_profile_index = build_saved_game_file_index(saved_game_file_type,
								_memory_unit_hard_drive, file.index, file.read_only, file.valid);

							goto done;
						}
						else
						{
							error(_error_silent, "append_entry_to_mapfile() failed; deleting newly created meta data");
						}
					}
					else
					{
						error(_error_silent, "failed to create empty saved game file '%s'", file.path);
					}
				}

				if (XDeleteSaveGame(
						wide_to_ascii(memory_unit_root_path[_memory_unit_hard_drive], root_path, sizeof(root_path)),
						display_name))
				{
					error(_error_silent, "XDeleteSaveGame() failed... ghost meta data likely");
				}

				goto done;
			}
			else
			{
				error(_error_silent, "XCreateSaveGame() failed to create meta data for a new saved game file");
			}
		}
		else
		{
			error(_error_silent, "failed to create new saved game file because there are already the maximum number of game files on the hard drive");
			display_error_deferred(36, NONE, TRUE, FALSE);
		}
	}

done:
	return new_profile_index;
}

boolean delete_enumerated_saved_game_file(
	long profile_index)
{
	struct enumerated_saved_game_file file;
	boolean success = FALSE;

	if (saved_game_files_globals.memory_units_dirty)
	{
		error(_error_silent, "failed to delete saved game file because memory units have been inserted/removed");
	}
	else
	{
		long type = SAVED_GAME_FILE_INDEX_TYPE(profile_index);
		long memory_unit = SAVED_GAME_FILE_INDEX_MEMORY_UNIT(profile_index);
		long n = SAVED_GAME_FILE_INDEX_FILE_INDEX(profile_index);

		match_assert(
			"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
			495,
			memory_unit==_memory_unit_hard_drive);

		if ((type >= 0) && (type < NUMBER_OF_SAVED_GAME_FILE_TYPES) &&
			(memory_unit >= 0) && (memory_unit < NUMBER_OF_MEMORY_UNITS) &&
			(n >= 0) && (n < MAXIMUM_ENUMERATED_SAVED_GAME_FILES_ANY_TYPE_PER_MEMORY_UNIT))
		{
			if (get_nth_entry_in_mapfile(memory_unit, n, &file))
			{
				char root_path[MEMORY_UNIT_ROOT_PATH_SIZE] = {0};

				/* only the hard drive is ever mounted in this build */
				success = (memory_unit == _memory_unit_hard_drive);

				if (success)
				{
					if (!TEST_FLAG(profile_index, _saved_game_file_index_read_only_bit))
					{
						if (XDeleteSaveGame(
								wide_to_ascii(memory_unit_root_path[memory_unit], root_path, sizeof(root_path)),
								file.display_name))
						{
							error(_error_silent, "XDeleteSaveGame() failed... ghost meta data likely");
							success = FALSE;
						}
					}

					if (!remove_nth_entry_in_mapfile(memory_unit, n))
					{
						error(_error_silent, "remove_nth_entry_in_mapfile() failed");
						success = FALSE;
					}
					else
					{
						/* port: the files after it moved down the list; the
						indices the players hold follow them */
						player_ui_saved_game_file_removed(profile_index);
					}

					if (memory_unit != _memory_unit_hard_drive)
					{
						error(_error_silent, "failed to unmount memory unit");
					}
				}
				else
				{
					error(_error_silent, "failed to mount memory unit #%d", memory_unit);
				}
			}
			else
			{
				error(_error_silent, "get_nth_entry_in_mapfile() failed in delete_enumerated_saved_game_file()");
			}
		}
		else
		{
			error(_error_silent, "delete_enumerated_saved_game_file() failed because the game file index was invalid");
		}
	}

	reset_last_player1_profile_index();

	return success;
}

/* port: a file's index once the file of removed_index has left its memory
unit's list (delete_enumerated_saved_game_file): the files after it move
down one, so their indices do; NONE for the file removed */
long saved_game_file_index_after_removal(
	long profile_index,
	long removed_index)
{
	long n;
	long removed_n;

	if (profile_index == NONE || removed_index == NONE ||
		SAVED_GAME_FILE_INDEX_MEMORY_UNIT(profile_index) != SAVED_GAME_FILE_INDEX_MEMORY_UNIT(removed_index))
	{
		return profile_index;
	}
	n = SAVED_GAME_FILE_INDEX_FILE_INDEX(profile_index);
	removed_n = SAVED_GAME_FILE_INDEX_FILE_INDEX(removed_index);
	if (n == removed_n)
		return NONE;
	if (n < removed_n)
		return profile_index;
	return (long)(((unsigned long)profile_index & ~(0xFFFUL << 16)) | ((unsigned long)(n - 1) << 16));
}

void saved_game_file_get_useable_untitled_profile_name(
	wchar_t *display_name)
{
	long string_list_index;

	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		705,
		display_name);

	display_name[0] = 0;

	string_list_index = tag_loaded(UNICODE_STRING_LIST_TAG, "ui\\saved_game_file_strings");

	if (string_list_index != NONE)
	{
		char root_path[MEMORY_UNIT_ROOT_PATH_SIZE] = {0};
		char save_game_directory[MAXIMUM_FILENAME_LENGTH+1] = {0};
		long index;

		wide_to_ascii(memory_unit_root_path[_memory_unit_hard_drive], root_path, sizeof(root_path));

		for (index = 0; index < MAXIMUM_UNTITLED_SAVED_GAMES; index++)
		{
			usnprintf(display_name, MAX_GAMENAME-1,
				ustring_format_checked(unicode_string_list_get_string(string_list_index, _saved_game_file_string_untitled_name_format), "d"),
				index+1);
			display_name[MAX_GAMENAME-1] = 0;

			if (XCreateSaveGame(root_path, display_name, OPEN_EXISTING, 0, save_game_directory, sizeof(save_game_directory)))
			{
				break;
			}
		}

		if (index == MAXIMUM_UNTITLED_SAVED_GAMES)
		{
			error(_error_silent, "%d untitled saved games! clean up your hard drive!!", MAXIMUM_UNTITLED_SAVED_GAMES);
			display_name[0] = 0;
		}
	}
	else
	{
		error(_error_silent, "unicode string lis tag '%s' not loaded", "ui\\saved_game_file_strings");
	}

	return;
}

void saved_game_files_enumerate_available_to_local_player_index(
	short player_index,
	word saved_game_file_type,
	word *number_of_profiles,
	long *player_profile_indices,
	boolean include_default_profiles)
{
	struct enumerated_saved_game_file file;
	long number_of_available_profiles = 0;
	word memory_unit_index = _memory_unit_hard_drive;

	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		236,
		((player_index==NONE) || ((player_index>=0) && (player_index<MAXIMUM_GAMEPADS))) && (saved_game_file_type<NUMBER_OF_SAVED_GAME_FILE_TYPES) && (number_of_profiles != NULL) && (player_profile_indices != NULL));

	if (take_mutex(saved_game_files_globals.general_mutex, SAVED_GAME_FILES_MUTEX_TIMEOUT))
	{
		long number_of_entries;

		if (saved_game_files_globals.memory_units_dirty)
		{
			enumerate_memory_units();
		}

		number_of_entries = count_enumerated_profiles_in_mapfile(memory_unit_index);
		if (saved_game_files_take_mapfile_mutex())
		{
			if (enumerate_mapfile_start(memory_unit_index))
			{
				long entry_index;

				for (entry_index = 0;
					number_of_available_profiles < *number_of_profiles && entry_index < number_of_entries;
					entry_index++)
				{
					if (!enumerate_saved_game_file_from_mapfile(&file))
					{
						break;
					}

					if (file.type == saved_game_file_type &&
						((include_default_profiles == TRUE) || !file.read_only))
					{
						player_profile_indices[number_of_available_profiles] =
							build_saved_game_file_index(saved_game_file_type, memory_unit_index,
								entry_index, file.read_only, file.valid);
						number_of_available_profiles++;
					}
				}

				enumerate_mapfile_end(memory_unit_index);
			}

			saved_game_files_release_mapfile_mutex();
		}
		else
		{
			error(_error_silent, "failed to take mapfile mutex");
		}

		release_mutex(saved_game_files_globals.general_mutex);
	}
	else
	{
		error(_error_silent, "failed to take saved game files mutex");
	}

	*number_of_profiles = number_of_available_profiles;

	return;
}

long saved_game_file_find_profile_index_for_directory_path(
	char const *directory_path,
	short type)
{
	struct enumerated_saved_game_file file;
	long profile_index = NONE;
	unsigned long directory_path_length;
	word memory_unit_index = _memory_unit_hard_drive;

	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		1158,
		directory_path);

	directory_path_length = csstrlen(directory_path);

	if (take_mutex(saved_game_files_globals.general_mutex, SAVED_GAME_FILES_MUTEX_TIMEOUT))
	{
		if (saved_game_files_take_mapfile_mutex())
		{
			long number_of_entries = count_enumerated_profiles_in_mapfile(memory_unit_index);

			if (enumerate_mapfile_start(memory_unit_index))
			{
				long entry_index;

				for (entry_index = 0; entry_index < number_of_entries; entry_index++)
				{
					if (!enumerate_saved_game_file_from_mapfile(&file))
					{
						break;
					}

					if (file.type == type &&
						!_strnicmp(directory_path, file.path, directory_path_length))
					{
						profile_index = build_saved_game_file_index(type,
							memory_unit_index, entry_index, file.read_only, file.valid);
						break;
					}
				}

				enumerate_mapfile_end(memory_unit_index);
			}

			saved_game_files_release_mapfile_mutex();
		}
		else
		{
			error(_error_silent, "failed to take mapfile mutex");
		}

		release_mutex(saved_game_files_globals.general_mutex);
	}
	else
	{
		error(_error_silent, "failed to take save game files mutex");
	}

	return profile_index;
}

void saved_game_files_delete_all_custom_profiles(
	void)
{
	XGAME_FIND_DATA find_data;
	long number_of_enumerated_files = 0;
	unsigned long memory_unit_index;

	memory_unit_index = _memory_unit_hard_drive;

	while (memory_unit_index <= _memory_unit_hard_drive)
	{
		if (enumerate_saved_game_files_start(memory_unit_index))
		{
			if (memory_unit_index == _memory_unit_hard_drive)
			{
				char root_path[MEMORY_UNIT_ROOT_PATH_SIZE] = {0};
				HANDLE find_handle = XFindFirstSaveGame(
					wide_to_ascii(memory_unit_root_path[memory_unit_index], root_path, sizeof(root_path)),
					&find_data);

				if (find_handle != INVALID_HANDLE_VALUE)
				{
					do
					{
						if (number_of_enumerated_files >= MAXIMUM_ENUMERATED_SAVED_GAME_FILES_ANY_TYPE_PER_MEMORY_UNIT)
						{
							break;
						}

						if (XDeleteSaveGame(root_path, find_data.szSaveGameName))
						{
							error(_error_silent, "XDeleteSaveGame() failed to delete profile");
						}
					}
					while ((boolean)XFindNextSaveGame(find_handle, &find_data));

					if (!XFindClose(find_handle))
					{
						error(_error_silent, "XFindClose() failed");
					}
				}

				number_of_enumerated_files += enumerate_default_profiles();
			}

			enumerate_saved_game_files_end(memory_unit_index);
		}

		memory_unit_index++;
	}

	return;
}

static void enumerate_memory_units(
	void)
{
	wchar_t message[MAXIMUM_FILENAME_LENGTH+1];
	XGAME_FIND_DATA find_data;
	byte block[SAVED_GAME_FILE_BLOCK_SIZE];
	struct file_reference saved_game_file;
	XCALCSIG_SIGNATURE checksum;
	HANDLE find_handle;
	unsigned long old_style_crc;
	unsigned long memory_unit_index = _memory_unit_hard_drive;
	long number_of_enumerated_files = 0;

	while (memory_unit_index <= _memory_unit_hard_drive)
	{
		if (take_mutex(saved_game_files_globals.general_mutex, SAVED_GAME_FILES_MUTEX_TIMEOUT))
		{
			if (enumerate_saved_game_files_start(memory_unit_index))
			{
				if (memory_unit_index == _memory_unit_hard_drive)
				{
					char root_path[MEMORY_UNIT_ROOT_PATH_SIZE] = {0};
					find_handle = XFindFirstSaveGame(
						wide_to_ascii(memory_unit_root_path[memory_unit_index], root_path, sizeof(root_path)),
						&find_data);

					if (find_handle != INVALID_HANDLE_VALUE)
					{
						do
						{
							if (number_of_enumerated_files >= MAXIMUM_ENUMERATED_SAVED_GAME_FILES_ANY_TYPE_PER_MEMORY_UNIT)
							{
								break;
							}

							{
								struct enumerated_saved_game_file file = {0};
								word checksum_data_size;

								if (_snprintf(
										file.path,
										MAXIMUM_FILENAME_LENGTH,
										"%s%s",
										find_data.szSaveGameDirectory,
										"blam.sav") > 0 &&
									file_reference_create_from_path(&saved_game_file, file.path, FALSE) &&
									file_exists(&saved_game_file))
								{
									file.type = _saved_game_file_type_player_profile;
									checksum_data_size = PLAYER_PROFILE_CHECKSUM_DATA_SIZE;
								}
								else if (_snprintf(
										file.path,
										MAXIMUM_FILENAME_LENGTH,
										"%s%s",
										find_data.szSaveGameDirectory,
										"blam.lst") > 0 &&
									file_reference_create_from_path(&saved_game_file, file.path, FALSE) &&
									file_exists(&saved_game_file))
								{
									file.type = _saved_game_file_type_game_variant;
									checksum_data_size = PLAYLIST_PROFILE_CHECKSUM_DATA_SIZE;
								}
								else
								{
									usnprintf(
										message,
										MAXIMUM_FILENAME_LENGTH,
										L"random crap found by XFindNextSaveGame(): display name= '%s' path= '%hs'",
										find_data.szSaveGameName,
										find_data.wfd.cFileName);
									message[MAXIMUM_FILENAME_LENGTH] = 0;
									error(
										_error_silent,
										"%s",
										wide_to_ascii(message, (char *)message, sizeof(message)));
									file.type = NONE;
								}

								if (file.type != NONE)
								{
									ustrncpy(file.display_name, find_data.szSaveGameName, MAX_GAMENAME-1);
									file.display_name[MAX_GAMENAME-1] = 0;

									if (file_open(
											&saved_game_file,
											FLAG(_permission_read_bit)|FLAG(_permission_write_bit)))
									{
										if (file_read(&saved_game_file, sizeof(block), block))
										{
											saved_game_file_generate_checksum(block, checksum_data_size, &checksum);
											if (!csmemcmp(&checksum, block+checksum_data_size, sizeof(checksum)))
											{
												file.valid = TRUE;
											}
											else
											{
												error(_error_silent, "checksum validation failed for '%s'", file.path);
												crc_new(&old_style_crc);
												crc_checksum_buffer(&old_style_crc, block, checksum_data_size);
												if (!csmemcmp(&old_style_crc, block+checksum_data_size, sizeof(old_style_crc)))
												{
													error(_error_silent, "checksum validation matched old-style crc; updating saved game file '%s'", file.path);
													csmemcpy(block+checksum_data_size, &checksum, sizeof(checksum));
													if (file_set_position(&saved_game_file, 0) &&
														file_write(&saved_game_file, sizeof(block), block))
													{
														file.valid = TRUE;
													}
												}
											}
										}
										else
										{
											error(_error_silent, "failed to read saved game file to verify checksum");
										}

										if (!file_close(&saved_game_file))
										{
											error(_error_silent, "failed to close saved game file after verifying checksum");
										}
									}
									else
									{
										error(_error_silent, "failed to open saved game file to verify checksum");
									}

									if (!enumerate_saved_game_file(&file))
									{
										break;
									}
									number_of_enumerated_files++;
								}
							}
						}
						while ((boolean)XFindNextSaveGame(find_handle, &find_data));

						if (!XFindClose(find_handle))
						{
							error(_error_silent, "XFindClose() failed");
						}
					}

					number_of_enumerated_files += enumerate_default_profiles();
				}

				enumerate_saved_game_files_end(memory_unit_index);
			}

			release_mutex(saved_game_files_globals.general_mutex);
		}

		memory_unit_index++;
	}

	saved_game_files_globals.memory_units_dirty = FALSE;
	return;
}

void enumerate_memory_units_test(
	void)
{
	enumerate_memory_units();

	return;
}

/* ---------- private code */

static boolean find_and_create_directory_if_necessary(
	char const *path)
{
	struct file_reference directory;
	boolean success = TRUE;

	if (!file_reference_create_from_path(&directory, path, TRUE) ||
		(!file_exists(&directory) && !file_create(&directory)))
	{
		success = FALSE;
	}

	return success;
}

static boolean enumerate_saved_game_files_start(
	word memory_unit)
{
	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		1643,
		memory_unit==_memory_unit_hard_drive);

	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		1647,
		!saved_game_files_globals.enumeration_in_progress);

	if (file_reference_create_from_path(&saved_game_files_globals.memory_unit_mapfile,
			memory_unit_mapfile_path[memory_unit], FALSE) &&
		file_create(&saved_game_files_globals.memory_unit_mapfile) &&
		file_open(&saved_game_files_globals.memory_unit_mapfile, FLAG(_permission_write_bit)))
	{
		saved_game_files_globals.next_enumerated_profile_index = 0;
		saved_game_files_globals.enumeration_in_progress = TRUE;
	}
	else
	{
		error(_error_silent, "failed to create/open memory unit mapfile for memory unit #%d", memory_unit);
		saved_game_files_globals.next_enumerated_profile_index = NONE;
	}

	return saved_game_files_globals.enumeration_in_progress;
}

static boolean enumerate_saved_game_files_end(
	word memory_unit)
{
	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		1669,
		memory_unit==_memory_unit_hard_drive);

	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		1671,
		saved_game_files_globals.enumeration_in_progress);

	if (!file_close(&saved_game_files_globals.memory_unit_mapfile))
	{
		error(_error_silent, "failed to close memory unit mapfile for memory unit #%d", memory_unit);
	}

	saved_game_files_globals.next_enumerated_profile_index = NONE;
	saved_game_files_globals.enumeration_in_progress = FALSE;

	return TRUE;
}

static boolean enumerate_saved_game_file(
	struct enumerated_saved_game_file *file)
{
	boolean success;

	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		1857,
		(saved_game_files_globals.enumeration_in_progress) && (saved_game_files_globals.next_enumerated_profile_index >= 0));

	if (saved_game_files_globals.next_enumerated_profile_index < MAXIMUM_ENUMERATED_SAVED_GAME_FILES_ANY_TYPE_PER_MEMORY_UNIT)
	{
		file->index = saved_game_files_globals.next_enumerated_profile_index++;
		success = file_write(&saved_game_files_globals.memory_unit_mapfile, sizeof(struct enumerated_saved_game_file), file);
	}
	else
	{
		error(_error_silent, "the maximum number of game files have already been enumerated (time to clean up your hard drive and/or memory cards)");
		success = FALSE;
	}

	return success;
}

static boolean enumerate_mapfile_start(
	word memory_unit_index)
{
	boolean success;

	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		1883,
		memory_unit_index==_memory_unit_hard_drive);

	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		1887,
		!saved_game_files_globals.enumeration_in_progress);

	if (file_reference_create_from_path(&saved_game_files_globals.memory_unit_mapfile,
			memory_unit_mapfile_path[memory_unit_index], FALSE) &&
		file_open(&saved_game_files_globals.memory_unit_mapfile, FLAG(_permission_read_bit)))
	{
		unsigned long mapfile_size = file_get_eof(&saved_game_files_globals.memory_unit_mapfile);

		if (mapfile_size%sizeof(struct enumerated_saved_game_file))
		{
			error(_error_silent, "memory unit mapfile for memory unit #%d is possibly corrupt", memory_unit_index);
		}

		success = TRUE;
	}
	else
	{
		error(_error_silent, "failed to open memory unit mapfile for memory unit #%d", memory_unit_index);
		success = FALSE;
	}

	return success;
}

static boolean enumerate_mapfile_end(
	word memory_unit_index)
{
	boolean success;

	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		1916,
		memory_unit_index==_memory_unit_hard_drive);

	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		1918,
		!saved_game_files_globals.enumeration_in_progress);
	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		1919,
		memory_unit_index < NUMBER_OF_MEMORY_UNITS);

	success = file_close(&saved_game_files_globals.memory_unit_mapfile);

	if (!success)
	{
		error(_error_silent, "enumerate_mapfile_end FAILED on memory unit #%d", memory_unit_index);
	}

	return success;
}

static boolean enumerate_saved_game_file_from_mapfile(
	struct enumerated_saved_game_file *file)
{
	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		1939,
		!saved_game_files_globals.enumeration_in_progress);
	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		1940,
		file);

	return file_read(&saved_game_files_globals.memory_unit_mapfile, sizeof(struct enumerated_saved_game_file), file);
}

static long count_enumerated_profiles_in_mapfile(
	word memory_unit_index)
{
	unsigned long mapfile_size;
	long number_of_entries;

	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		2144,
		memory_unit_index==_memory_unit_hard_drive);

	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		2146,
		!saved_game_files_globals.enumeration_in_progress);
	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		2147,
		memory_unit_index < NUMBER_OF_MEMORY_UNITS);

	if (file_reference_create_from_path(&saved_game_files_globals.memory_unit_mapfile,
			memory_unit_mapfile_path[memory_unit_index], FALSE) &&
		file_get_size(&saved_game_files_globals.memory_unit_mapfile, &mapfile_size))
	{
		number_of_entries = mapfile_size/sizeof(struct enumerated_saved_game_file);
	}
	else
	{
		number_of_entries = 0;
	}

	return number_of_entries;
}

static long build_saved_game_file_index(
	long type,
	long memory_unit_index,
	long n,
	boolean read_only,
	boolean valid)
{
	long profile_index = SAVED_GAME_FILE_INDEX_BUILD(n, memory_unit_index, type);

	if (read_only == TRUE)
	{
		profile_index |= FLAG(_saved_game_file_index_read_only_bit);
	}

	if (valid == TRUE)
	{
		profile_index |= FLAG(_saved_game_file_index_valid_bit);
	}

	return profile_index;
}

static short enumerate_default_playlist_profiles(
	void)
{
	byte block[SAVED_GAME_FILE_BLOCK_SIZE];
	char path[MAXIMUM_FILENAME_LENGTH+1];
	struct file_reference file;
	XCALCSIG_SIGNATURE checksum;
	short number_of_profiles = playlist_profile_number_of_default_profiles_on_disk();
	long string_list_index = tag_loaded(UNICODE_STRING_LIST_TAG, "ui\\default_multiplayer_game_setting_names");
	short profile_index = 0;

	if (string_list_index != NONE)
	{
		while (profile_index < number_of_profiles)
		{
			/* port: past the Xbox's, OpenCE's gametypes (game_modes.c) */
			wchar_t const *display_name = profile_index < GAME_MODES_FIRST_DEFAULT_PLAYLIST_PROFILE ?
				unicode_string_list_get_string(string_list_index, profile_index) :
				game_modes_gametype_name((short)(profile_index - GAME_MODES_FIRST_DEFAULT_PLAYLIST_PROFILE));

			_snprintf(path, MAXIMUM_FILENAME_LENGTH, "z:\\saved\\playlists\\default_playlist\\%02d\\blam.lst", profile_index);

			if (file_reference_create_from_path(&file, path, FALSE) && file_exists(&file))
			{
				struct enumerated_saved_game_file entry = {0};

				csstrncpy(entry.path, path, MAXIMUM_FILENAME_LENGTH);
				entry.path[MAXIMUM_FILENAME_LENGTH] = 0;
				ustrncpy(entry.display_name, display_name, MAX_GAMENAME-1);
				entry.display_name[MAX_GAMENAME-1] = 0;
				entry.type = _saved_game_file_type_game_variant;
				entry.read_only = TRUE;

				if (file_open(&file, FLAG(_permission_read_bit)))
				{
					if (file_read(&file, sizeof(block), block))
					{
						saved_game_file_generate_checksum(block, PLAYLIST_PROFILE_CHECKSUM_DATA_SIZE, &checksum);

						if (!csmemcmp(&checksum, block+PLAYLIST_PROFILE_CHECKSUM_DATA_SIZE, sizeof(checksum)))
						{
							entry.valid = TRUE;
						}
						else
						{
							error(_error_silent, "checksum validation failed for '%s'", entry.path);
						}
					}
					else
					{
						error(_error_silent, "failed to read saved game variant file to verify checksum");
					}

					if (!file_close(&file))
					{
						error(_error_silent, "failed to close saved game variant file after verifying checksum");
					}
				}
				else
				{
					error(_error_silent, "failed to open saved game variant file to verify checksum");
				}

				if (!enumerate_saved_game_file(&entry))
				{
					error(_error_silent, "failed to enumerate default playlist file '%s'", path);
					break;
				}
			}

			profile_index++;
		}
	}
	else
	{
		error(_error_silent, "failed to enumerate default playlist files because their name string list tag was not loaded");
	}

	return profile_index;
}

static short enumerate_default_player_profiles(
	void)
{
	byte block[SAVED_GAME_FILE_BLOCK_SIZE];
	char path[MAXIMUM_FILENAME_LENGTH+1];
	struct file_reference file;
	XCALCSIG_SIGNATURE checksum;
	long string_list_index = tag_loaded(UNICODE_STRING_LIST_TAG, "ui\\shell\\strings\\default_player_profile_names");
	short profile_index = 0;

	if (string_list_index != NONE)
	{
		while (profile_index < NUMBER_OF_DEFAULT_PROFILES)
		{
			wchar_t *display_name = unicode_string_list_get_string(string_list_index, profile_index);

			_snprintf(path, MAXIMUM_FILENAME_LENGTH, "z:\\saved\\player_profiles\\default_profile\\%02d.sav", profile_index);

			if (file_reference_create_from_path(&file, path, FALSE) && file_exists(&file))
			{
				struct enumerated_saved_game_file entry = {0};

				csstrncpy(entry.path, path, MAXIMUM_FILENAME_LENGTH);
				entry.path[MAXIMUM_FILENAME_LENGTH] = 0;
				ustrncpy(entry.display_name, display_name, MAX_GAMENAME-1);
				entry.display_name[MAX_GAMENAME-1] = 0;
				entry.type = _saved_game_file_type_player_profile;
				entry.read_only = TRUE;

				if (file_open(&file, FLAG(_permission_read_bit)))
				{
					if (file_read(&file, sizeof(block), block))
					{
						saved_game_file_generate_checksum(block, PLAYER_PROFILE_CHECKSUM_DATA_SIZE, &checksum);

						if (!csmemcmp(&checksum, block+PLAYER_PROFILE_CHECKSUM_DATA_SIZE, sizeof(checksum)))
						{
							entry.valid = TRUE;
						}
						else
						{
							error(_error_silent, "checksum validation failed for '%s'", entry.path);
						}
					}
					else
					{
						error(_error_silent, "failed to read saved game player profile file to verify checksum");
					}

					if (!file_close(&file))
					{
						error(_error_silent, "failed to close saved game player profile file after verifying checksum");
					}
				}
				else
				{
					error(_error_silent, "failed to open saved game player profile file to verify checksum");
				}

				if (!enumerate_saved_game_file(&entry))
				{
					error(_error_silent, "failed to enumerate default player profile file '%s'", path);
					break;
				}
			}

			profile_index++;
		}
	}
	else
	{
		error(_error_silent, "failed to enumerate default player profile files because their name string list tag was not loaded");
	}

	return profile_index;
}

static boolean append_entry_to_mapfile(
	word memory_unit_index,
	struct enumerated_saved_game_file *file,
	long *profile_index)
{
	boolean success = FALSE;

	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		2081,
		memory_unit_index==_memory_unit_hard_drive);

	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		2083,
		!saved_game_files_globals.enumeration_in_progress);
	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		2084,
		(memory_unit_index < NUMBER_OF_MEMORY_UNITS) && (file != NULL) && (profile_index != NULL));

	if (saved_game_files_take_mapfile_mutex())
	{
		if (file_reference_create_from_path(&saved_game_files_globals.memory_unit_mapfile,
				memory_unit_mapfile_path[memory_unit_index], FALSE) &&
			file_open(&saved_game_files_globals.memory_unit_mapfile, FLAG(_permission_write_bit)))
		{
			unsigned long mapfile_size = file_get_eof(&saved_game_files_globals.memory_unit_mapfile);
			long number_of_entries = mapfile_size/sizeof(struct enumerated_saved_game_file);
			unsigned long entry_offset = number_of_entries*sizeof(struct enumerated_saved_game_file);

			if (number_of_entries < MAXIMUM_ENUMERATED_SAVED_GAME_FILES_ANY_TYPE_PER_MEMORY_UNIT)
			{
				if (mapfile_size%sizeof(struct enumerated_saved_game_file))
				{
					error(_error_silent, "memory unit mapfile for memory unit #%d is possibly corrupt", memory_unit_index);
				}

				if (file_set_position(&saved_game_files_globals.memory_unit_mapfile, entry_offset) &&
					file_write(&saved_game_files_globals.memory_unit_mapfile, sizeof(struct enumerated_saved_game_file), file))
				{
					success = TRUE;
					*profile_index = number_of_entries;
				}
				else
				{
					success = FALSE;
					error(_error_silent, "failed to append entry to memory unit mapfile (#%d)", memory_unit_index);
				}
			}
			else
			{
				error(_error_silent, "can't add new entry to memory unit mapfile because the maximum number of profiles have already been added");
			}

			if (!file_close(&saved_game_files_globals.memory_unit_mapfile))
			{
				error(_error_silent, "failed to close memory unit mapfile for memory unit #%d", memory_unit_index);
				success = FALSE;
			}
		}
		else
		{
			error(_error_silent, "failed to open memory unit mapfile for memory unit #%d", memory_unit_index);
		}

		saved_game_files_release_mapfile_mutex();
	}
	else
	{
		error(_error_silent, "failed to take mapfile mutex");
	}

	return success;
}

static boolean remove_nth_entry_in_mapfile(
	word memory_unit_index,
	word n)
{
	struct enumerated_saved_game_file file;
	unsigned long mapfile_size;
	unsigned long entry_offset = n*sizeof(struct enumerated_saved_game_file);
	unsigned long next_entry_offset;
	boolean success = FALSE;

	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		2170,
		memory_unit_index==_memory_unit_hard_drive);

	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		2172,
		!saved_game_files_globals.enumeration_in_progress);
	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		2173,
		memory_unit_index < NUMBER_OF_MEMORY_UNITS);

	if (saved_game_files_take_mapfile_mutex())
	{
		if (file_reference_create_from_path(&saved_game_files_globals.memory_unit_mapfile,
				memory_unit_mapfile_path[memory_unit_index], FALSE) &&
			file_get_size(&saved_game_files_globals.memory_unit_mapfile, &mapfile_size) &&
			mapfile_size >= entry_offset+sizeof(struct enumerated_saved_game_file) &&
			file_open(&saved_game_files_globals.memory_unit_mapfile, FLAG(_permission_read_bit)|FLAG(_permission_write_bit)))
		{
			success = file_set_position(&saved_game_files_globals.memory_unit_mapfile, entry_offset);

			next_entry_offset = entry_offset+sizeof(struct enumerated_saved_game_file);

			if (success == TRUE)
			{
				while (next_entry_offset < mapfile_size)
				{
					if (!file_read_from_position(&saved_game_files_globals.memory_unit_mapfile,
							next_entry_offset, sizeof(struct enumerated_saved_game_file), &file) ||
						!file_write_to_position(&saved_game_files_globals.memory_unit_mapfile,
							entry_offset, sizeof(struct enumerated_saved_game_file), &file))
					{
						error(_error_silent, "failed to update memory unit mapfile after removing an enumerated file");
						success = FALSE;
						break;
					}

					next_entry_offset += sizeof(struct enumerated_saved_game_file);
					entry_offset += sizeof(struct enumerated_saved_game_file);
				}
			}

			if (success)
			{
				success = file_set_eof(&saved_game_files_globals.memory_unit_mapfile,
					mapfile_size-sizeof(struct enumerated_saved_game_file));
			}

			if (!file_close(&saved_game_files_globals.memory_unit_mapfile))
			{
				error(_error_silent, "failed to close memory unit map file");
				success = FALSE;
			}
		}

		saved_game_files_release_mapfile_mutex();
	}
	else
	{
		error(_error_silent, "failed to take mapfile mutex");
	}

	return success;
}

static boolean get_nth_entry_in_mapfile(
	word memory_unit_index,
	word n,
	struct enumerated_saved_game_file *file)
{
	boolean success = FALSE;

	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		1959,
		memory_unit_index==_memory_unit_hard_drive);

	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		1961,
		!saved_game_files_globals.enumeration_in_progress);
	match_assert(
		"c:\\halo\\SOURCE\\saved games\\saved_game_files.c",
		1962,
		(memory_unit_index < NUMBER_OF_MEMORY_UNITS) && (file != NULL));

	if (saved_game_files_take_mapfile_mutex())
	{
		if (file_reference_create_from_path(&saved_game_files_globals.memory_unit_mapfile,
				memory_unit_mapfile_path[memory_unit_index], FALSE) &&
			file_open(&saved_game_files_globals.memory_unit_mapfile, FLAG(_permission_read_bit)))
		{
			unsigned long mapfile_size = file_get_eof(&saved_game_files_globals.memory_unit_mapfile);
			unsigned long entry_offset = n*sizeof(struct enumerated_saved_game_file);

			if (mapfile_size%sizeof(struct enumerated_saved_game_file))
			{
				error(_error_silent, "memory unit mapfile for memory unit #%d is possibly corrupt", memory_unit_index);
			}

			if (entry_offset+sizeof(struct enumerated_saved_game_file) <= mapfile_size)
			{
				if (file_set_position(&saved_game_files_globals.memory_unit_mapfile, entry_offset) &&
					file_read(&saved_game_files_globals.memory_unit_mapfile, sizeof(struct enumerated_saved_game_file), file))
				{
					success = TRUE;
				}
				else
				{
					success = FALSE;
					error(_error_silent, "failed to retrieve entry #%d from memory unit mapfile (#%d)", n, memory_unit_index);
				}
			}
			else
			{
				error(_error_silent, "invalid profile index (#%d) into memory unit #%d specified", n, memory_unit_index);
			}

			if (!file_close(&saved_game_files_globals.memory_unit_mapfile))
			{
				error(_error_silent, "failed to close memory unit mapfile for memory unit #%d", memory_unit_index);
				success = FALSE;
			}
		}
		else
		{
			error(_error_silent, "failed to open memory unit mapfile for memory unit #%d", memory_unit_index);
		}

		saved_game_files_release_mapfile_mutex();
	}
	else
	{
		error(_error_silent, "failed to take mapfile mutex");
	}

	return success;
}
