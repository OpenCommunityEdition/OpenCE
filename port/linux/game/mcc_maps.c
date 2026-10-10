/*
MCC_MAPS.C

The independent MCC menu catalog. Only version 13 Halo 1 MCC caches from
d:\mcc_maps are admitted. The on-disk filename is the level identity; the
cache's internal scenario name may differ. No Xbox or Custom Edition map
list, loader, resource directory or display index is used here.
*/

#include "cseries.h"
#include "errors.h"
#include "tag_files/files.h"
#include "tag_files/tag_files.h"
#include "mcc_cache_format.h"
#include "mcc_maps.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* main_set_map_name retains 63 characters including the namespace. */
#define MCC_MAP_FILENAME_MAXIMUM (63 - (sizeof(MCC_MAPS_LEVEL_PREFIX) - 1))
#define MCC_MAP_DESCRIPTION_MAXIMUM 127

struct mcc_menu_map
{
	char file_name[MCC_MAP_FILENAME_MAXIMUM + 1];
	char level_name[64];
	wchar_t name[MCC_MAP_FILENAME_MAXIMUM + 1];
	wchar_t description[MCC_MAP_DESCRIPTION_MAXIMUM + 1];
	boolean campaign;
};

static struct
{
	boolean scanned;
	short count;
	struct mcc_menu_map maps[MCC_MAPS_MAXIMUM];
} mcc_menu_catalog;

typedef char mcc_display_indices_are_separate[
	MCC_MAPS_FIRST_DISPLAY_INDEX + MCC_MAPS_MAXIMUM <= 0x3000 ? 1 : -1];

static void mcc_map_description_read(struct mcc_menu_map *map)
{
	char path[128];
	unsigned char bytes[1024];
	FILE *file;
	size_t length = 0, position;
	short used = 0;

	snprintf(path, sizeof(path), "%s%s.txt", MCC_MAPS_DIRECTORY, map->file_name);
	file = fopen(path, "rb");
	if (file)
	{
		length = fread(bytes, 1, sizeof(bytes), file);
		fclose(file);
	}
	position = length >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF ? 3 : 0;
	for (; position < length && used < MCC_MAP_DESCRIPTION_MAXIMUM; position++)
	{
		unsigned char value = bytes[position];

		if (value == '\n')
		{
			if (used + 2 > MCC_MAP_DESCRIPTION_MAXIMUM)
				break;
			map->description[used++] = '\r';
			map->description[used++] = '\n';
		}
		else if (value == '\t')
			map->description[used++] = ' ';
		else if (value >= 32 && value < 127)
			map->description[used++] = value;
		else if (value >= 0xC0)
			map->description[used++] = '?';
	}
	while (used && (map->description[used - 1] == ' ' || map->description[used - 1] == '\r' ||
		map->description[used - 1] == '\n'))
		used--;
	map->description[used] = 0;
	if (!used)
	{
		wchar_t const *fallback = map->campaign ? L"Halo 1 MCC\r\nCampaign map" : L"Halo 1 MCC\r\nMultiplayer map";

		for (used = 0; fallback[used]; used++)
			map->description[used] = fallback[used];
		map->description[used] = 0;
	}
}

static void mcc_map_register(char const *name)
{
	char path[128];
	unsigned char header[MCC_CACHE_HEADER_BYTES];
	struct mcc_cache_identity identity;
	struct mcc_menu_map *map;
	FILE *file;
	long file_size;
	size_t name_length = strlen(name), index;
	boolean start_word = TRUE;

	if (!name_length || name_length > MCC_MAP_FILENAME_MAXIMUM || mcc_menu_catalog.count == MCC_MAPS_MAXIMUM ||
		name[name_length - 1] == '.' || name[name_length - 1] == ' ')
		return;
	for (index = 0; index < name_length; index++)
	{
		unsigned char character = (unsigned char)name[index];

		if (character < 32 || strchr("/\\:*?\"<>|", character))
			return;
	}
	for (index = 0; index < (size_t)mcc_menu_catalog.count; index++)
	{
		if (!csstrcasecmp(mcc_menu_catalog.maps[index].file_name, name))
			return;
	}
	snprintf(path, sizeof(path), "%s%s.map", MCC_MAPS_DIRECTORY, name);
	file = fopen(path, "rb");
	if (!file)
		return;
	if (fseek(file, 0, SEEK_END) || (file_size = ftell(file)) < 0 || fseek(file, 0, SEEK_SET) ||
		fread(header, 1, sizeof(header), file) != sizeof(header))
	{
		fclose(file);
		return;
	}
	fclose(file);
	if (mcc_cache_identify(header, sizeof(header), (uint64_t)file_size, &identity) != MCC_CACHE_OK)
		return;
	map = &mcc_menu_catalog.maps[mcc_menu_catalog.count++];
	memset(map, 0, sizeof(*map));
	memcpy(map->file_name, name, name_length + 1);
	snprintf(map->level_name, sizeof(map->level_name), "%s%s", MCC_MAPS_LEVEL_PREFIX, name);
	map->campaign = identity.scenario_type == 0;
	for (index = 0; index < name_length; index++)
	{
		unsigned char character = (unsigned char)name[index];

		if (character == '_')
			character = ' ';
		else if (start_word && character >= 'a' && character <= 'z')
			character -= 'a' - 'A';
		map->name[index] = character >= 32 && character < 127 ? character : '?';
		start_word = character == ' ';
	}
	mcc_map_description_read(map);
}

static int mcc_map_order(void const *left, void const *right)
{
	return csstrcasecmp(((struct mcc_menu_map const *)left)->file_name,
		((struct mcc_menu_map const *)right)->file_name);
}

static void mcc_maps_scan(void)
{
	struct file_reference directory, file;
	char name[MAXIMUM_FILENAME_LENGTH + 1], extension[MAXIMUM_FILENAME_LENGTH + 1];

	if (mcc_menu_catalog.scanned)
		return;
	mcc_menu_catalog.scanned = TRUE;
	mcc_menu_catalog.count = 0;
	file_reference_create_from_path(&directory, MCC_MAPS_DIRECTORY, TRUE);
	find_files_start(0, &directory);
	while (find_files_next(&file, NULL))
	{
		file_reference_get_name(&file, FLAG(_name_extension_bit), extension);
		if (csstrcasecmp(extension, "map"))
			continue;
		file_reference_get_name(&file, FLAG(_name_filename_bit), name);
		mcc_map_register(name);
	}
	qsort(mcc_menu_catalog.maps, (size_t)mcc_menu_catalog.count, sizeof(mcc_menu_catalog.maps[0]), mcc_map_order);
	error(_error_silent, "mcc maps: %d maps found in mcc_maps", mcc_menu_catalog.count);
}

void mcc_maps_rescan(void)
{
	mcc_menu_catalog.scanned = FALSE;
}

short mcc_maps_count(boolean multiplayer_only)
{
	short index, count = 0;

	mcc_maps_scan();
	for (index = 0; index < mcc_menu_catalog.count; index++)
	{
		if (!multiplayer_only || !mcc_menu_catalog.maps[index].campaign)
			count++;
	}
	return count;
}

short mcc_maps_index(short row, boolean multiplayer_only)
{
	short index;

	mcc_maps_scan();
	if (row < 0)
		return NONE;
	for (index = 0; index < mcc_menu_catalog.count; index++)
	{
		if ((!multiplayer_only || !mcc_menu_catalog.maps[index].campaign) && row-- == 0)
			return index;
	}
	return NONE;
}

short mcc_maps_type_count(boolean campaign)
{
	short index, count = 0;

	mcc_maps_scan();
	for (index = 0; index < mcc_menu_catalog.count; index++)
	{
		if (mcc_menu_catalog.maps[index].campaign == campaign)
			count++;
	}
	return count;
}

short mcc_maps_type_index(short row, boolean campaign)
{
	short index;

	mcc_maps_scan();
	if (row < 0)
		return NONE;
	for (index = 0; index < mcc_menu_catalog.count; index++)
	{
		if (mcc_menu_catalog.maps[index].campaign == campaign && row-- == 0)
			return index;
	}
	return NONE;
}

short mcc_maps_find(char const *level_name)
{
	short index;

	if (!level_name || _strnicmp(level_name, MCC_MAPS_LEVEL_PREFIX, sizeof(MCC_MAPS_LEVEL_PREFIX) - 1))
		return NONE;
	mcc_maps_scan();
	for (index = 0; index < mcc_menu_catalog.count; index++)
	{
		if (!csstrcasecmp(level_name, mcc_menu_catalog.maps[index].level_name))
			return index;
	}
	return NONE;
}

char const *mcc_maps_level_name(short index)
{
	mcc_maps_scan();
	return index >= 0 && index < mcc_menu_catalog.count ? mcc_menu_catalog.maps[index].level_name : NULL;
}

wchar_t const *mcc_maps_name(short index)
{
	mcc_maps_scan();
	return index >= 0 && index < mcc_menu_catalog.count ? mcc_menu_catalog.maps[index].name : L"";
}

wchar_t const *mcc_maps_description(short index)
{
	mcc_maps_scan();
	return index >= 0 && index < mcc_menu_catalog.count ? mcc_menu_catalog.maps[index].description : L"";
}

boolean mcc_maps_campaign(short index)
{
	mcc_maps_scan();
	return index >= 0 && index < mcc_menu_catalog.count && mcc_menu_catalog.maps[index].campaign;
}

boolean mcc_maps_level_campaign(char const *level_name)
{
	return mcc_maps_campaign(mcc_maps_find(level_name));
}
