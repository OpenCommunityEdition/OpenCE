/*
MCC_MAPS.C

The independent MCC menu catalog. Only version 13 Halo 1 MCC caches from
d:\mcc_maps are admitted. The on-disk filename is the level identity; the
cache's internal scenario name may differ. No Xbox or Custom Edition map
list, loader, resource directory or display index is used here.
*/

#include "cseries.h"
#include "errors.h"
#include "mcc_cache_format.h"
#include "mcc_maps.h"
#include "mcc_package.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xtl.h>

/* main_set_map_name retains 63 characters including the namespace. */
#define MCC_MAP_FILENAME_MAXIMUM (63 - (sizeof(MCC_MAPS_LEVEL_PREFIX) - 1))
#define MCC_MAP_DESCRIPTION_MAXIMUM 127

struct mcc_menu_map
{
	char file_name[MCC_MAP_FILENAME_MAXIMUM + 1];
	char level_name[64];
	wchar_t name[64];
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
	char path[128], candidate[128], canonical[64];
	char const *display;
	unsigned char header[MCC_CACHE_HEADER_BYTES];
	struct mcc_cache_identity identity;
	struct mcc_menu_map *map;
	FILE *file;
	long file_size;
	size_t name_length = strlen(name), index;
	boolean start_word = TRUE;

	if (!name_length || name_length > MCC_MAP_FILENAME_MAXIMUM || mcc_menu_catalog.count == MCC_MAPS_MAXIMUM)
        return;
    snprintf(candidate, sizeof(candidate), "%s%s", MCC_MAPS_LEVEL_PREFIX, name);
    if (!mcc_package_name(candidate, canonical)) return;
    name = canonical + sizeof(MCC_MAPS_LEVEL_PREFIX) - 1;
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
	display = strrchr(name, '\\');
	display = display ? display + 1 : name;
	for (index = 0; display[index]; index++)
	{
		unsigned char character = (unsigned char)display[index];

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

/* Exactly one package level; directory entries never become arbitrary paths. */
static void mcc_maps_scan_directory(char const *package)
{
    WIN32_FIND_DATAA data;
    HANDLE find;
    char pattern[128];
    snprintf(pattern,sizeof(pattern),"%s%s%s*.*",MCC_MAPS_DIRECTORY,package,package[0] ? "\\" : "");
    find=FindFirstFileA(pattern,&data);
    if (find==INVALID_HANDLE_VALUE) return;
    do {
        char name[sizeof(data.cFileName)+1], relative[128], candidate[128], canonical[64];
        char *extension;
        size_t length;
        memcpy(name,data.cFileName,sizeof(data.cFileName));
        name[sizeof(data.cFileName)]=0;
        length=strlen(name);
        if (!length || strpbrk(name,"/\\") || length>58) continue;
        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (package[0]) continue;
            snprintf(candidate,sizeof(candidate),"%s%s\\x",MCC_MAPS_LEVEL_PREFIX,name);
            if (mcc_package_name(candidate,canonical)) mcc_maps_scan_directory(name);
            continue;
        }
        extension=strrchr(name,'.');
        if (!extension || csstrcasecmp(extension+1,"map")) continue;
        *extension=0;
        if (!csstrcasecmp(name,"sounds") || !csstrcasecmp(name,"bitmaps") || !csstrcasecmp(name,"loc")) continue;
        snprintf(relative,sizeof(relative),"%s%s%s",package,package[0] ? "\\" : "",name);
        mcc_map_register(relative);
    } while (FindNextFileA(find,&data));
    CloseHandle(find);
}

static void mcc_maps_scan(void)
{
    short i;
    if (mcc_menu_catalog.scanned) return;
    mcc_menu_catalog.scanned=TRUE;
    mcc_menu_catalog.count=0;
    mcc_maps_scan_directory("");
    qsort(mcc_menu_catalog.maps,(size_t)mcc_menu_catalog.count,sizeof(mcc_menu_catalog.maps[0]),mcc_map_order);
    /* Show the owning package for every packaged level. */
    for (i=0;i<mcc_menu_catalog.count;i++) {
        struct mcc_menu_map *map=&mcc_menu_catalog.maps[i];
        char const *base=strrchr(map->file_name,'\\');
        if (base) {
            size_t n,k;
            for (n=0;map->name[n];n++) {}
            map->name[n++]=' ';map->name[n++]='[';
            for (k=0;k<(size_t)(base-map->file_name);k++) map->name[n++]=(unsigned char)map->file_name[k];
            map->name[n++]=']';map->name[n]=0;
        }
    }
    error(_error_silent,"mcc maps: %d maps found in mcc_maps and its packages",mcc_menu_catalog.count);
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
    char canonical[64];

	if (!mcc_package_name(level_name, canonical))
		return NONE;
	mcc_maps_scan();
	for (index = 0; index < mcc_menu_catalog.count; index++)
	{
		if (!csstrcasecmp(canonical, mcc_menu_catalog.maps[index].level_name))
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
