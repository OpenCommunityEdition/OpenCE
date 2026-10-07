/*
CUSTOM_EDITION_MAPS.C

Lists Custom Edition maps for the menus (custom_edition_maps.h).

Whenever the level list opens, the maps folder and then the Halo Custom
Edition install's maps folder are scanned for CE caches (.map; OpenSauce
caches are refused by the loader, so not listed). Multiplayer maps join the
level list; campaign maps (solo scenarios) are kept apart for co-op. The
stock campaign levels get display indices here too, so the menus can show
them the same way.

A map's level name is levels\test\<name>\<name>, like the Xbox levels;
the cache loader finds a map by the last part. The game engine keeps 63
characters of a level name, so a name over 25 characters becomes
levels\<name>, and one over 56 is skipped. A map named like an Xbox level
is skipped too, since that level is already listed.

Each map can have, beside it in its folder:
- <name>.bmp, its picture (bmp_files.c). The middle is cropped to the
  menus' picture shape. It is read the first time it is drawn, and freed
  when the maps are rescanned. Without one, the unknown level's picture
  is shown.
- <name>.txt, its description, line by line (the Xbox descriptions use
  lines of about 20 characters).
*/

/* ---------- headers */

#include "cseries.h"
#include "errors.h"
#include "tag_files/files.h"
#include "tag_files/tag_files.h"
#include "bitmaps/bitmaps.h"
#include "bitmaps/bitmap_group.h"
#include "cache/cache_files.h"
#include "main/main.h"
#include "saved games/player_profile.h"
#include "text/unicode.h"
#include "bmp_files.h"
#include "custom_edition_cache.h"
#include "custom_edition_maps.h"

#include <stdio.h>
#include <stdlib.h>

/* ---------- constants */

/* Custom Edition campaign maps, for co-op */
#define MAXIMUM_CUSTOM_EDITION_CAMPAIGNS 1024
#define FIRST_CUSTOM_CAMPAIGN_DISPLAY_INDEX 0x6000
/* room for the level list's Xbox levels (ui_widget_event_handler_functions.c has 13) */
#define MAXIMUM_XBOX_LEVELS 16

/* Level names. The game engine keeps 63 characters of one (game_engine.c,
struct game_engine_stage), so long map names use the short form. */
#define LEVEL_NAME_FORMAT "levels\\test\\%s\\%s"
#define LONG_LEVEL_NAME_FORMAT "levels\\%s"
#define LONGEST_LEVEL_NAME_FORMAT_NAME 25
#define MAXIMUM_MAP_NAME_LENGTH 56

/* Display indices for CE multiplayer maps, clear of every string and frame
index in the menus' tags (15 level names, 14 level pictures), and below the
custom campaigns' and ui_widget.c's spinner descriptions' (0x7000). */
#define FIRST_DISPLAY_INDEX 0x4000

/* The stock campaign levels, for co-op: their display indices, names, and
pictures (the campaign menu's, one frame per level in order). */
#define FIRST_CAMPAIGN_DISPLAY_INDEX 0x3000
#define CAMPAIGN_LEVEL_PICTURES_TAG_NAME "ui\\shell\\bitmaps\\sp_levels"
static wchar_t campaign_level_names[NUMBER_OF_SINGLE_PLAYER_LEVELS][32] =
{
	L"The Pillar of Autumn", L"Halo", L"The Truth and Reconciliation", L"The Silent Cartographer",
	L"Assault on the Control Room", L"343 Guilty Spark", L"The Library", L"Two Betrayals", L"Keyes", L"The Maw",
};
static wchar_t campaign_level_description[] = L"A campaign level, played co-op";

/* The level pictures tag, the picture shape (140x114 in the menus' 640x480;
the lobby's is 139x113), and the unknown level's frame
(ui_widget_game_data_input_functions.c). */
#define LEVEL_PICTURES_TAG_NAME "ui\\shell\\bitmaps\\mp_map_grafix"
#define LEVEL_PICTURE_SHAPE_WIDTH 140
#define LEVEL_PICTURE_SHAPE_HEIGHT 114
#define UNKNOWN_LEVEL_FRAME 13

#define PICTURE_EXTENSION ".bmp"
/* a 4K screenshot is 25 MB as a bmp file */
#define MAXIMUM_PICTURE_FILE_BYTES 0x04000000L
/* picture textures are square, about twice the menus' picture size */
#define PICTURE_TEXTURE_SIZE 256

#define DESCRIPTION_EXTENSION ".txt"
/* descriptions are a few short lines */
#define MAXIMUM_DESCRIPTION_FILE_BYTES 1024
#define MAXIMUM_DESCRIPTION_LENGTH 127

/* bitmaps.c's format of 32-bit color with alpha */
enum
{
	_bitmap_format_a8r8g8b8 = 11,
};

/* (the multiplayer maps', custom campaigns' and spinner descriptions'
display indices do not meet) */
typedef char verify_multiplayer_display_indices[
	FIRST_DISPLAY_INDEX + CUSTOM_EDITION_MAPS_MAXIMUM <= FIRST_CUSTOM_CAMPAIGN_DISPLAY_INDEX ? 1 : -1];
typedef char verify_campaign_display_indices[
	FIRST_CUSTOM_CAMPAIGN_DISPLAY_INDEX + MAXIMUM_CUSTOM_EDITION_CAMPAIGNS <= 0x7000 ? 1 : -1];

/* ---------- structures */

struct custom_edition_map
{
	/* file name without extension, and the folder it was found in */
	char name[MAXIMUM_MAP_NAME_LENGTH + 1];
	char folder[32];
	/* saved_game_file_remember_last_used_multiplayer_map writes
	MAXIMUM_FILENAME_LENGTH+1 characters of a level name */
	char level_name[MAXIMUM_FILENAME_LENGTH + 1];
	wchar_t display_name[MAXIMUM_MAP_NAME_LENGTH + 1];
	wchar_t description[MAXIMUM_DESCRIPTION_LENGTH + 1];
	boolean picture_read;
	struct bitmap_data *picture;
};

struct custom_edition_maps_globals
{
	boolean looked_for;
	short map_count;
	struct custom_edition_map maps[CUSTOM_EDITION_MAPS_MAXIMUM];
	/* campaign maps, kept out of the level list */
	short campaign_count;
	struct custom_edition_map campaigns[MAXIMUM_CUSTOM_EDITION_CAMPAIGNS];
	/* the latest level list: the Xbox levels, then the CE maps */
	short xbox_level_count;
	char *levels[MAXIMUM_XBOX_LEVELS + CUSTOM_EDITION_MAPS_MAXIMUM];
};

/* ---------- globals */

static struct custom_edition_maps_globals custom_edition_maps_globals;

/* Halo PC's own multiplayer maps (not on the Xbox), with their menu names
from ui.map's mp_map_list. They are listed as VANILLA. */
static struct
{
	char const *file;
	wchar_t const *name;
} const stock_maps[] =
{
	{ "icefields", L"Ice Fields" }, { "deathisland", L"Death Island" }, { "dangercanyon", L"Danger Canyon" },
	{ "infinity", L"Infinity" }, { "timberland", L"Timberland" }, { "gephyrophobia", L"Gephyrophobia" },
};

/* for a map without a description file */
static wchar_t const default_description[] = L"Halo Custom\r\nEdition map";

/* ---------- private code */

static void custom_edition_maps_forget(
	void)
{
	struct custom_edition_maps_globals *globals = &custom_edition_maps_globals;
	short map_index;

	for (map_index = 0; map_index < globals->map_count; map_index++)
	{
		if (globals->maps[map_index].picture)
			bitmap_delete(globals->maps[map_index].picture);
	}
	for (map_index = 0; map_index < globals->campaign_count; map_index++)
	{
		if (globals->campaigns[map_index].picture)
			bitmap_delete(globals->campaigns[map_index].picture);
	}
	globals->map_count = 0;
	globals->campaign_count = 0;

	return;
}

static boolean xbox_level_named(
	char const *name)
{
	struct custom_edition_maps_globals *globals = &custom_edition_maps_globals;
	short level_index;

	for (level_index = 0; level_index < globals->xbox_level_count; level_index++)
	{
		if (!csstrcasecmp(tag_name_strip_path(globals->levels[level_index]), name))
		{
			return TRUE;
		}
	}

	return FALSE;
}

/* the map's index in stock_maps, or NONE */
static short stock_map_index(
	char const *name)
{
	short index;

	for (index = 0; index < NUMBEROF(stock_maps); index++)
	{
		if (!csstrcasecmp(name, stock_maps[index].file))
			return index;
	}
	return NONE;
}

/* A map's menu name: Halo PC's name for its own maps, otherwise the file
name with underscores as spaces and words capitalized ("the_bay_of_pigs"
becomes "The Bay Of Pigs"). */
static void display_name_make(
	char const *name,
	wchar_t *display_name)
{
	boolean word_start = TRUE;
	short index = stock_map_index(name);

	if (index != NONE)
	{
		ustrcpy(display_name, stock_maps[index].name);
		return;
	}

	for (index = 0; name[index]; index++)
	{
		unsigned char character = (unsigned char)name[index];

		if (character == '_')
		{
			character = ' ';
		}
		else if (word_start && character >= 'a' && character <= 'z')
		{
			character = (unsigned char)(character - 'a' + 'A');
		}
		word_start = character == ' ';
		display_name[index] = (wchar_t)character;
	}
	display_name[index] = 0;

	return;
}

/* Reads <name>.txt into map->description: line ends become \r\n (as in the
menus' strings), tabs become spaces, and non-ASCII characters become '?'.
A missing or empty file gives the default description. */
static void custom_edition_map_description_read(
	struct custom_edition_map *map)
{
	char path[MAXIMUM_FILENAME_LENGTH + 1];
	byte text[MAXIMUM_DESCRIPTION_FILE_BYTES];
	FILE *stream;
	long text_size = 0;
	long text_index = 0;
	short length = 0;

	csprintf(path, "%s%s%s", map->folder, map->name, DESCRIPTION_EXTENSION);
	stream = fopen(path, "rb");
	if (stream)
	{
		text_size = (long)fread(text, 1, sizeof(text), stream);
		fclose(stream);
	}
	/* the byte order mark of UTF-8 text */
	if (text_size >= 3 && text[0] == 0xEF && text[1] == 0xBB && text[2] == 0xBF)
	{
		text_index = 3;
	}
	for (; text_index < text_size && length < MAXIMUM_DESCRIPTION_LENGTH; text_index++)
	{
		byte character = text[text_index];

		if (character == '\n')
		{
			if (length + 2 > MAXIMUM_DESCRIPTION_LENGTH)
			{
				break;
			}
			map->description[length++] = '\r';
			map->description[length++] = '\n';
		}
		else if (character == '\t')
		{
			map->description[length++] = ' ';
		}
		else if (character >= ' ' && character < 0x7F)
		{
			map->description[length++] = character;
		}
		else if (character >= 0xC0)
		{
			/* the lead byte of a multi-byte UTF-8 character */
			map->description[length++] = '?';
		}
	}
	while (length > 0 &&
		(map->description[length - 1] == ' ' ||
			map->description[length - 1] == '\r' ||
			map->description[length - 1] == '\n'))
	{
		length--;
	}
	map->description[length] = 0;

	if (!length)
	{
		csmemcpy(map->description, default_description, sizeof(default_description));
	}

	return;
}

/* Adds the file `name`.`extension` from `folder` if it is a CE map that
isn't listed yet. */
static void custom_edition_map_add(
	char const *folder,
	char const *name,
	char const *extension)
{
	struct custom_edition_maps_globals *globals = &custom_edition_maps_globals;
	struct custom_edition_map *map;
	boolean campaign;
	short map_index;

	if (csstrcasecmp(extension, "map"))
	{
		return;
	}
	for (map_index = 0; map_index < globals->map_count; map_index++)
	{
		if (!csstrcasecmp(globals->maps[map_index].name, name))
		{
			return;
		}
	}
	for (map_index = 0; map_index < globals->campaign_count; map_index++)
	{
		if (!csstrcasecmp(globals->campaigns[map_index].name, name))
		{
			return;
		}
	}
	if (xbox_level_named(name))
	{
		return;
	}
	/* campaign maps go to co-op; anything that isn't multiplayer either is skipped */
	campaign = custom_edition_cache_campaign(name);
	if (!campaign && !custom_edition_cache_multiplayer(name))
	{
		return;
	}
	if (campaign)
	{
		if (globals->campaign_count == MAXIMUM_CUSTOM_EDITION_CAMPAIGNS || csstrlen(name) > MAXIMUM_MAP_NAME_LENGTH)
		{
			error(_error_silent, "custom edition: the campaign map '%s' is not listed (too many, or too long a name)",
				name);
			return;
		}
		map = &globals->campaigns[globals->campaign_count++];
	}
	else if (csstrlen(name) > MAXIMUM_MAP_NAME_LENGTH)
	{
		error(
			_error_silent,
			"custom edition: the map '%s' is not in the level list: its name is longer than %d characters",
			name,
			MAXIMUM_MAP_NAME_LENGTH);
		return;
	}
	else if (globals->map_count == CUSTOM_EDITION_MAPS_MAXIMUM)
	{
		error(
			_error_silent,
			"custom edition: the map '%s' is not in the level list, which holds %d of them",
			name,
			CUSTOM_EDITION_MAPS_MAXIMUM);
		return;
	}
	else
	{
		map = &globals->maps[globals->map_count++];
	}
	csmemset(map, 0, sizeof(*map));
	csstrcpy(map->name, name);
	csstrncpy(map->folder, folder, sizeof(map->folder) - 1);
	if (csstrlen(name) <= LONGEST_LEVEL_NAME_FORMAT_NAME)
	{
		csprintf(map->level_name, LEVEL_NAME_FORMAT, name, name);
	}
	else
	{
		csprintf(map->level_name, LONG_LEVEL_NAME_FORMAT, name);
	}
	display_name_make(name, map->display_name);
	custom_edition_map_description_read(map);

	return;
}

static int custom_edition_map_compare(
	void const *first,
	void const *second)
{
	return (int)csstrcasecmp(
		((struct custom_edition_map const *)first)->name,
		((struct custom_edition_map const *)second)->name);
}

static void custom_edition_maps_look_for(
	void)
{
	struct custom_edition_maps_globals *globals = &custom_edition_maps_globals;
	struct file_reference directory;
	struct file_reference file;
	char name[MAXIMUM_FILENAME_LENGTH + 1];
	char extension[MAXIMUM_FILENAME_LENGTH + 1];
	char const *folders[] = { cache_files_map_directory(), CUSTOM_EDITION_INSTALL_MAP_DIRECTORY };
	short folder_index;

	custom_edition_maps_forget();
	globals->looked_for = TRUE;
	if (!halo_custom_edition_tag_cache())
	{
		return;
	}

	/* the game's own folder first, so its copy wins */
	for (folder_index = 0; folder_index < NUMBEROF(folders); folder_index++)
	{
		/* (the install's only when there is one) */
		if (folder_index && !custom_edition_install_present())
			continue;
		file_reference_create_from_path(&directory, folders[folder_index], TRUE);
		find_files_start(0, &directory);
		while (find_files_next(&file, NULL))
		{
			file_reference_get_name(&file, FLAG(_name_filename_bit), name);
			file_reference_get_name(&file, FLAG(_name_extension_bit), extension);
			custom_edition_map_add(folders[folder_index], name, extension);
		}
	}
	qsort(globals->maps, globals->map_count, sizeof(globals->maps[0]), custom_edition_map_compare);
	qsort(globals->campaigns, globals->campaign_count, sizeof(globals->campaigns[0]), custom_edition_map_compare);
	error(_error_silent, "custom edition: %d multiplayer maps for the level list, %d campaign maps", globals->map_count,
		globals->campaign_count);

	return;
}

/* Loads the map's picture as a texture. Returns NULL if it has none, or if
it can't be shown (logged). */
static struct bitmap_data *custom_edition_map_picture_read(
	struct custom_edition_map const *map)
{
	char path[MAXIMUM_FILENAME_LENGTH + 1];
	struct bmp_file_picture picture;
	enum bmp_file_status status;
	struct bitmap_data *bitmap = NULL;
	uint8_t *file = NULL;
	FILE *stream;
	long size = 0;

	csprintf(path, "%s%s%s", map->folder, map->name, PICTURE_EXTENSION);
	stream = fopen(path, "rb");
	if (!stream)
	{
		return NULL;
	}
	if (fseek(stream, 0, SEEK_END) == 0 &&
		(size = ftell(stream)) >= 0 &&
		size <= MAXIMUM_PICTURE_FILE_BYTES &&
		fseek(stream, 0, SEEK_SET) == 0)
	{
		/* +1 so an empty file doesn't look like a failed allocation */
		file = malloc((size_t)size + 1);
		if (file && fread(file, 1, (size_t)size, stream) != (size_t)size)
		{
			free(file);
			file = NULL;
		}
	}
	fclose(stream);
	if (!file)
	{
		error(_error_silent, "custom edition: the picture '%s' could not be read", path);
		return NULL;
	}

	status = bmp_file_open(file, (uint32_t)size, &picture);
	if (status == _bmp_file_status_ok)
	{
		bitmap = bitmap_2d_new(PICTURE_TEXTURE_SIZE, PICTURE_TEXTURE_SIZE, 0, _bitmap_format_a8r8g8b8);
		if (bitmap && bitmap->base_address)
		{
			bmp_file_fit(
				file,
				&picture,
				LEVEL_PICTURE_SHAPE_WIDTH,
				LEVEL_PICTURE_SHAPE_HEIGHT,
				PICTURE_TEXTURE_SIZE,
				PICTURE_TEXTURE_SIZE,
				XBOX_POINTER(uint32_t, bitmap->base_address));
			bitmap_rebuild(bitmap);
		}
		if (bitmap && !bitmap->hardware_format)
		{
			bitmap_delete(bitmap);
			bitmap = NULL;
		}
	}
	else
	{
		error(_error_silent, "custom edition: the picture '%s' cannot be shown: %s", path, bmp_file_status_describe(status));
	}
	free(file);

	return bitmap;
}

/* the CE map with this display index, or NULL */
static struct custom_edition_map *custom_edition_map_get(
	short display_index)
{
	struct custom_edition_maps_globals *globals = &custom_edition_maps_globals;
	short map_index = display_index - FIRST_DISPLAY_INDEX;
	short campaign_index = display_index - FIRST_CUSTOM_CAMPAIGN_DISPLAY_INDEX;

	if (campaign_index >= 0 && campaign_index < globals->campaign_count)
		return &globals->campaigns[campaign_index];
	return map_index >= 0 && map_index < globals->map_count ? &globals->maps[map_index] : NULL;
}

/* the stock campaign level with this display index, or NONE */
static short campaign_level_get(
	short display_index)
{
	short level = display_index - FIRST_CAMPAIGN_DISPLAY_INDEX;

	return level >= 0 && level < NUMBER_OF_SINGLE_PLAYER_LEVELS ? level : NONE;
}

/* the stock campaign level a level name refers to (levels\a10\a10 is the
first), or NONE */
static short campaign_level_from_name(
	char const *level_name)
{
	char const *name = tag_name_strip_path(level_name);
	short level;

	for (level = 0; level < NUMBER_OF_SINGLE_PLAYER_LEVELS; level++)
	{
		if (!csstrcasecmp(tag_name_strip_path(main_get_solo_level_name(level)), name))
			return level;
	}

	return NONE;
}

/* ---------- public code */

char **custom_edition_maps_level_list(
	char **xbox_levels,
	short xbox_level_count,
	short *level_count)
{
	struct custom_edition_maps_globals *globals = &custom_edition_maps_globals;
	short previous_xbox_level_count = globals->xbox_level_count;
	short level_index;
	short map_index;

	globals->xbox_level_count = MIN(xbox_level_count, MAXIMUM_XBOX_LEVELS);
	for (level_index = 0; level_index < globals->xbox_level_count; level_index++)
	{
		globals->levels[level_index] = xbox_levels[level_index];
	}
	/* (the menus ask for the list every tick, so the folders are scanned
	once, again when a map list opens (custom_edition_maps_look_again),
	and again once the Xbox levels, which a CE map's name can't take, are
	known) */
	if (!globals->looked_for || globals->xbox_level_count != previous_xbox_level_count)
	{
		custom_edition_maps_look_for();
	}
	for (map_index = 0; map_index < globals->map_count; map_index++)
	{
		globals->levels[globals->xbox_level_count + map_index] = globals->maps[map_index].level_name;
	}
	*level_count = globals->xbox_level_count + globals->map_count;

	return globals->levels;
}

void custom_edition_maps_look_again(
	void)
{
	custom_edition_maps_globals.looked_for = FALSE;

	return;
}

short custom_edition_maps_level_display_index(
	short level_index)
{
	struct custom_edition_maps_globals *globals = &custom_edition_maps_globals;
	short map_index = level_index - globals->xbox_level_count;

	return map_index >= 0 && map_index < globals->map_count ? FIRST_DISPLAY_INDEX + map_index : level_index;
}

short custom_edition_maps_display_index(
	char const *level_name)
{
	struct custom_edition_maps_globals *globals = &custom_edition_maps_globals;
	char const *name = tag_name_strip_path(level_name);
	short level = campaign_level_from_name(level_name);
	short map_index;

	if (level != NONE)
	{
		return FIRST_CAMPAIGN_DISPLAY_INDEX + level;
	}
	if (!globals->looked_for)
	{
		custom_edition_maps_look_for();
	}
	for (map_index = 0; map_index < globals->map_count; map_index++)
	{
		if (!csstrcasecmp(globals->maps[map_index].name, name))
		{
			return FIRST_DISPLAY_INDEX + map_index;
		}
	}
	for (map_index = 0; map_index < globals->campaign_count; map_index++)
	{
		if (!csstrcasecmp(globals->campaigns[map_index].name, name))
		{
			return FIRST_CUSTOM_CAMPAIGN_DISPLAY_INDEX + map_index;
		}
	}

	return NONE;
}

boolean custom_edition_maps_campaign(
	short display_index)
{
	return campaign_level_get(display_index) != NONE ||
		(display_index >= FIRST_CUSTOM_CAMPAIGN_DISPLAY_INDEX && custom_edition_map_get(display_index));
}

short custom_edition_maps_campaign_level(
	short display_index)
{
	return campaign_level_get(display_index);
}

short custom_edition_maps_custom_campaigns(
	short *display_indices,
	short maximum)
{
	struct custom_edition_maps_globals *globals = &custom_edition_maps_globals;
	short count;

	if (!globals->looked_for)
	{
		custom_edition_maps_look_for();
	}
	for (count = 0; count < globals->campaign_count && count < maximum; count++)
	{
		display_indices[count] = FIRST_CUSTOM_CAMPAIGN_DISPLAY_INDEX + count;
	}

	return count;
}

char const *custom_edition_maps_level_name(
	short display_index)
{
	struct custom_edition_map *map = custom_edition_map_get(display_index);
	short level = campaign_level_get(display_index);

	if (level != NONE)
	{
		return main_get_solo_level_name(level);
	}

	return map ? map->level_name : NULL;
}

boolean custom_edition_maps_stock(
	short display_index)
{
	struct custom_edition_map *map = custom_edition_map_get(display_index);

	return map && stock_map_index(map->name) != NONE;
}

wchar_t *custom_edition_maps_name(
	short display_index)
{
	struct custom_edition_map *map = custom_edition_map_get(display_index);
	short level = campaign_level_get(display_index);

	if (level != NONE)
	{
		return campaign_level_names[level];
	}

	return map ? map->display_name : NULL;
}

wchar_t *custom_edition_maps_description(
	short display_index)
{
	struct custom_edition_map *map = custom_edition_map_get(display_index);

	if (campaign_level_get(display_index) != NONE)
	{
		return campaign_level_description;
	}

	return map ? map->description : NULL;
}

struct bitmap_data *custom_edition_maps_picture(
	long bitmap_tag_index,
	short *frame_index)
{
	struct custom_edition_map *map;
	short level = campaign_level_get(*frame_index);

	if (level != NONE &&
		bitmap_tag_index != NONE &&
		!csstrcasecmp(tag_get_name(bitmap_tag_index), LEVEL_PICTURES_TAG_NAME))
	{
		long pictures = tag_loaded('bitm', CAMPAIGN_LEVEL_PICTURES_TAG_NAME);

		*frame_index = UNKNOWN_LEVEL_FRAME;
		return pictures != NONE ? bitmap_group_get_bitmap_from_sequence(pictures, 0, level) : NULL;
	}
	if (*frame_index < FIRST_DISPLAY_INDEX ||
		bitmap_tag_index == NONE ||
		csstrcasecmp(tag_get_name(bitmap_tag_index), LEVEL_PICTURES_TAG_NAME))
	{
		return NULL;
	}

	map = custom_edition_map_get(*frame_index);
	*frame_index = UNKNOWN_LEVEL_FRAME;
	if (!map)
	{
		return NULL;
	}
	if (!map->picture_read)
	{
		map->picture = custom_edition_map_picture_read(map);
		map->picture_read = TRUE;
	}

	return map->picture;
}
