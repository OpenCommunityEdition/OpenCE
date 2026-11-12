/*
CUSTOM_EDITION_MAPS.C

Lists Custom Edition maps for the menus (custom_edition_maps.h).

Whenever a map list opens, the custom_maps folder and then the Halo Custom
Edition install's maps folder are scanned for CE caches (.map; OpenSauce
caches are refused by the loader, so not listed). Multiplayer maps join the
level list after the Xbox levels, and the map lists' CUSTOM MULTIPLAYER;
campaign maps (solo scenarios) are the map lists' CUSTOM SINGLEPLAYER,
played alone or as network co-op. The stock campaign levels get display
indices here too, so the menus can show them the same way.

A map's level name is custom_maps\<name> (CUSTOM_EDITION_LEVEL_NAME_PREFIX),
which the cache loader reads from the custom maps folders alone: a map named
as one of the game's own levels (a30.map, bloodgulch.map) is listed and
played as itself. The game engine keeps 63 characters of a level name, so a
map whose name is longer than 51 is skipped. A map keeps its name: a network
game names it so, and says which version it is on by its hash (map_share.h).
A downloaded map of a name another version has is named <name>_<hash>.map;
the menus show it without the hash, with (2), (3) and so on after the first
of the name. A map a build of before named for its hash has its name given
back (map_share_map_found).

Each map can have, beside it in its folder:
- <name>.bmp, its picture (bmp_files.c; for a map named for its hash, a
  picture named without the hash also does). The middle is cropped to the
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
#include "map_share.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <xtl.h>
#define FORGE_CE_IO_STDIO 1
#include "forge_ce_io_redirect.h"

/* ---------- constants */

/* Custom Edition campaign maps, for co-op */
#define MAXIMUM_CUSTOM_EDITION_CAMPAIGNS 1024
#define FIRST_CUSTOM_CAMPAIGN_DISPLAY_INDEX 0x6000
/* room for the level list's Xbox levels (ui_widget_event_handler_functions.c has 13) */
#define MAXIMUM_XBOX_LEVELS 16

/* the longest map file name whose level name
(CUSTOM_EDITION_LEVEL_NAME_PREFIX and it) the game engine keeps whole */
#define MAXIMUM_MAP_NAME_LENGTH \
	(CUSTOM_EDITION_MAXIMUM_LEVEL_NAME_LENGTH - (NUMBEROF(CUSTOM_EDITION_LEVEL_NAME_PREFIX) - 1))

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
#define MAXIMUM_DESCRIPTION_LENGTH 255

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
	/* (and room for "[MCC] " and a version's number, " (2)") */
	wchar_t display_name[MAXIMUM_MAP_NAME_LENGTH + 16];
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

/* the map's index in stock_maps, or NONE (by its name without its hash) */
static short stock_map_index(
	char const *name)
{
	short length = map_share_name_base_length(name);
	short index;

	for (index = 0; index < NUMBEROF(stock_maps); index++)
	{
		if ((short)csstrlen(stock_maps[index].file) == length && !_strnicmp(name, stock_maps[index].file, length))
			return index;
	}
	return NONE;
}

/* A map's menu name: Halo PC's name for its own maps, otherwise the file
name (without a hash it ends in) with underscores as spaces and words
capitalized ("the_bay_of_pigs" becomes "The Bay Of Pigs"). */
static void display_name_make(
	char const *name,
	wchar_t *display_name)
{
	boolean word_start = TRUE;
	short index = stock_map_index(name);
	short length = map_share_name_base_length(name);

	if (index != NONE)
	{
		ustrcpy(display_name, stock_maps[index].name);
		return;
	}

	for (index = 0; index < length; index++)
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

boolean custom_edition_map_format_forge_summary(
	char const *folder,
	char const *name,
	char *out_summary,
	unsigned int out_summary_size)
{
	static char const *const sp_stems[10] = {
		"a10", "a30", "a50", "b30", "b40", "c10", "c20", "c40", "d20", "d40"
	};
	char path[MAXIMUM_FILENAME_LENGTH + 1];
	char *buf = NULL;
	FILE *stream;
	char const *bm;
	char const *op;
	char const *ap;
	char const *ln;
	char base_short[40];
	char modes_str[64];
	int is_sp_base = 0;
	int header_obj_cnt = 0;
	int header_ai_cnt = 0;
	int placed_obj_cnt = 0;
	int ai_cnt = 0;
	int has_replace_flags = 0;
	int has_ctf_red = 0, has_ctf_blue = 0, has_koth = 0, has_oddball = 0, has_race = 0;
	int has_ctf_obj = 0, has_koth_obj = 0, has_oddball_obj = 0, has_race_obj = 0;
	int sup_slayer = 1, sup_ctf = 0, sup_koth = 0, sup_oddball = 0, sup_race = 0;
	int si;

	if (!folder || !name || !out_summary || out_summary_size == 0)
		return FALSE;
	out_summary[0] = 0;

	/* 1. Try <folder><name>.txt sidecar first (up to 128 KB) */
	csprintf(path, "%s%s%s", folder, name, DESCRIPTION_EXTENSION);
	stream = fopen(path, "rb");
	if (!stream && map_share_name_hashed(name))
	{
		csprintf(path, "%s%.*s%s", folder, (int)map_share_name_base_length(name), name, DESCRIPTION_EXTENSION);
		stream = fopen(path, "rb");
	}
	if (stream)
	{
		long f_sz = 0;
		if (fseek(stream, 0, SEEK_END) == 0)
			f_sz = ftell(stream);
		if (f_sz > 0 && fseek(stream, 0, SEEK_SET) == 0)
		{
			long rd_sz = (f_sz > 131072L) ? 131072L : f_sz;
			buf = (char *)malloc((size_t)rd_sz + 1u);
			if (buf)
			{
				size_t got = fread(buf, 1, (size_t)rd_sz, stream);
				buf[got] = 0;
				if (strstr(buf, "[OpenCE Forge Map]") == NULL && strstr(buf, "[ForgeState]") == NULL)
				{
					free(buf);
					buf = NULL;
				}
			}
		}
		fclose(stream);
	}

	/* 2. Fallback: Check <folder><name>.map embedded FLG1 Forge metadata trailer */
	if (!buf)
	{
		csprintf(path, "%s%s.map", folder, name);
		stream = fopen(path, "rb");
		if (!stream && map_share_name_hashed(name))
		{
			csprintf(path, "%s%.*s.map", folder, (int)map_share_name_base_length(name), name);
			stream = fopen(path, "rb");
		}
		if (stream)
		{
			uint8_t hdr[0x800];
			if (fread(hdr, 1, sizeof(hdr), stream) == sizeof(hdr) &&
				*(uint32_t const *)(hdr + 0x1FC) == 0x464C4731u)
			{
				uint32_t state_off = *(uint32_t const *)(hdr + 0x1E4);
				uint32_t state_sz = *(uint32_t const *)(hdr + 0x1E8);
				long file_sz = 0;
				if (state_off >= 0x800u && state_sz > 0u && state_sz <= 262144u &&
					fseek(stream, 0, SEEK_END) == 0)
				{
					file_sz = ftell(stream);
					if (file_sz > 0 && (uint64_t)state_off + (uint64_t)state_sz <= (uint64_t)file_sz &&
						fseek(stream, (long)state_off, SEEK_SET) == 0)
					{
						buf = (char *)malloc((size_t)state_sz + 1u);
						if (buf)
						{
							size_t got = fread(buf, 1, (size_t)state_sz, stream);
							buf[got] = 0;
							if (strstr(buf, "[OpenCE Forge Map]") == NULL && strstr(buf, "[ForgeState]") == NULL)
							{
								free(buf);
								buf = NULL;
							}
						}
					}
				}
			}
			fclose(stream);
		}
	}

	if (!buf)
		return FALSE;

	/* 3. Extract clean Base Map stem */
	csstrcpy(base_short, name);
	bm = strstr(buf, "Base Map:");
	if (bm)
	{
		char const *p = bm + 9;
		char const *end;
		char const *last_sep = NULL;
		char const *q;
		size_t len;

		while (*p == ' ' || *p == '\t')
			p++;
		for (q = p; *q && *q != '\r' && *q != '\n'; q++)
		{
			if (*q == '\\' || *q == '/')
				last_sep = q;
		}
		if (last_sep)
			p = last_sep + 1;
		end = p;
		while (*end && *end != '\r' && *end != '\n')
			end++;
		len = (size_t)(end - p);
		if (len > 4 && _strnicmp(p + len - 4, ".map", 4) == 0)
			len -= 4;
		if (len > 0)
		{
			if (len >= sizeof(base_short))
				len = sizeof(base_short) - 1;
			csmemcpy(base_short, p, len);
			base_short[len] = 0;
		}
	}
	for (si = 0; si < 10; si++)
	{
		if (_stricmp(base_short, sp_stems[si]) == 0)
		{
			is_sp_base = 1;
			break;
		}
	}

	op = strstr(buf, "Objects Placed:");
	if (op)
		header_obj_cnt = atoi(op + 15);
	ap = strstr(buf, "AI Squads:");
	if (ap)
		header_ai_cnt = atoi(ap + 10);

	/* 4. Scan [ForgeState] entries for Objects, AI Squads, and Objective/Gametype compatibility */
	ln = buf;
	while (*ln)
	{
		char const *eol = ln;
		while (*eol && *eol != '\n')
			eol++;
		while (ln < eol && (*ln == ' ' || *ln == '\t' || *ln == '\r'))
			ln++;

		if ((size_t)(eol - ln) >= 23 && strncmp(ln, "REPLACE_NETGAME_FLAGS|1", 23) == 0)
		{
			has_replace_flags = 1;
		}
		else if ((size_t)(eol - ln) >= 7 && strncmp(ln, "OBJECT|", 7) == 0)
		{
			char const *p = ln + 7;
			int pipes = 0;
			placed_obj_cnt++;
			/* Support legacy inline 9th-pipe gametype_flags if present */
			while (p < eol && pipes < 8)
			{
				if (*p == '|')
					pipes++;
				p++;
			}
			if (pipes == 8 && p < eol)
			{
				char const *p2 = p;
				while (p2 < eol && *p2 != '|')
					p2++;
				if (p2 < eol)
				{
					int gflags = atoi(p2 + 1);
					if (gflags == 0x0001) has_ctf_obj = 1;
					else if (gflags == 0x0004) has_oddball_obj = 1;
					else if (gflags == 0x0008) has_koth_obj = 1;
					else if (gflags == 0x0010) has_race_obj = 1;
				}
			}
		}
		else if ((size_t)(eol - ln) >= 16 && strncmp(ln, "OBJECT_GAMETYPE|", 16) == 0)
		{
			int gflags = atoi(ln + 16);
			if (gflags == 0x0001) has_ctf_obj = 1;
			else if (gflags == 0x0004) has_oddball_obj = 1;
			else if (gflags == 0x0008) has_koth_obj = 1;
			else if (gflags == 0x0010) has_race_obj = 1;
		}
		else if ((size_t)(eol - ln) >= 17 && strncmp(ln, "DEFAULT_GAMETYPE|", 17) == 0)
		{
			char const *bar = (char const *)memchr(ln + 17, '|', (size_t)(eol - (ln + 17)));
			int gflags = bar ? atoi(bar + 1) : atoi(ln + 17);
			if (gflags == 0x0001) has_ctf_obj = 1;
			else if (gflags == 0x0004) has_oddball_obj = 1;
			else if (gflags == 0x0008) has_koth_obj = 1;
			else if (gflags == 0x0010) has_race_obj = 1;
		}
		else if (((size_t)(eol - ln) >= 3 && strncmp(ln, "AI|", 3) == 0) ||
			((size_t)(eol - ln) >= 9 && strncmp(ln, "AI_SPAWN|", 9) == 0))
		{
			ai_cnt++;
		}
		else if ((size_t)(eol - ln) >= 13 && strncmp(ln, "NETGAME_FLAG|", 13) == 0)
		{
			int nf_type = -1;
			int nf_usage = 0;
			if (sscanf(ln + 13, "%*f|%*f|%*f|%*f|%d|%d", &nf_type, &nf_usage) == 2 ||
				sscanf(ln + 13, "%d|%d", &nf_type, &nf_usage) == 2)
			{
				has_replace_flags = 1;
				if (nf_type == 0)
				{
					if (nf_usage == 0) has_ctf_red = 1;
					else if (nf_usage == 1) has_ctf_blue = 1;
				}
				else if (nf_type == 1)
				{
					has_ctf_blue = 1;
				}
				else if (nf_type == 2)
				{
					has_oddball = 1;
				}
				else if (nf_type == 3 || nf_type == 4)
				{
					has_race = 1;
				}
				else if (nf_type == 8)
				{
					has_koth = 1;
				}
			}
		}

		ln = (*eol == '\n') ? (eol + 1) : eol;
	}

	if (placed_obj_cnt == 0 && header_obj_cnt > 0 && strstr(buf, "[ForgeState]") == NULL)
		placed_obj_cnt = header_obj_cnt;
	if (ai_cnt == 0 && header_ai_cnt > 0 && strstr(buf, "[ForgeState]") == NULL)
		ai_cnt = header_ai_cnt;

	if (has_replace_flags || is_sp_base)
	{
		sup_ctf = ((has_ctf_red && has_ctf_blue) || has_ctf_obj) ? 1 : 0;
		sup_koth = (has_koth || has_koth_obj) ? 1 : 0;
		sup_oddball = (has_oddball || has_oddball_obj) ? 1 : 0;
		sup_race = (has_race || has_race_obj) ? 1 : 0;
	}
	else
	{
		sup_ctf = 1;
		sup_koth = 1;
		sup_oddball = 1;
		sup_race = 1;
	}

	if (sup_slayer && sup_ctf && sup_koth && sup_oddball && sup_race)
	{
		csstrcpy(modes_str, "All Modes");
	}
	else
	{
		modes_str[0] = 0;
		if (sup_slayer)
			snprintf(modes_str + strlen(modes_str), sizeof(modes_str) - strlen(modes_str), "%sSlayer", modes_str[0] ? ", " : "");
		if (sup_ctf)
			snprintf(modes_str + strlen(modes_str), sizeof(modes_str) - strlen(modes_str), "%sCTF", modes_str[0] ? ", " : "");
		if (sup_koth)
			snprintf(modes_str + strlen(modes_str), sizeof(modes_str) - strlen(modes_str), "%sKOTH", modes_str[0] ? ", " : "");
		if (sup_oddball)
			snprintf(modes_str + strlen(modes_str), sizeof(modes_str) - strlen(modes_str), "%sOddball", modes_str[0] ? ", " : "");
		if (sup_race)
			snprintf(modes_str + strlen(modes_str), sizeof(modes_str) - strlen(modes_str), "%sRace", modes_str[0] ? ", " : "");
		if (!modes_str[0])
			csstrcpy(modes_str, "Slayer");
	}

	if (ai_cnt > 0)
	{
		snprintf(
			out_summary,
			out_summary_size,
			"Forge (Base: %s)\r\nObjects: %d | AI: %d\r\nModes: %s",
			base_short,
			placed_obj_cnt,
			ai_cnt,
			modes_str);
	}
	else
	{
		snprintf(
			out_summary,
			out_summary_size,
			"Forge (Base: %s)\r\nObjects Placed: %d\r\nModes: %s",
			base_short,
			placed_obj_cnt,
			modes_str);
	}

	free(buf);
	return TRUE;
}

/* Reads <name>.txt into map->description: line ends become \r\n (as in the
menus' strings), tabs become spaces, and non-ASCII characters become '?'.
A missing or empty file gives the default description. */
static void custom_edition_map_description_read(
	struct custom_edition_map *map)
{
	char path[MAXIMUM_FILENAME_LENGTH + 1];
	char forge_summary[192];
	byte text[MAXIMUM_DESCRIPTION_FILE_BYTES];
	FILE *stream;
	long text_size = 0;
	long text_index = 0;
	short length = 0;

	/* Format rich metadata & gametype compatibility badges for OpenCE Forge maps (.txt sidecar OR embedded .map trailer) */
	if (custom_edition_map_format_forge_summary(map->folder, map->name, forge_summary, sizeof(forge_summary)))
	{
		int s_i;
		for (s_i = 0; forge_summary[s_i] && length < MAXIMUM_DESCRIPTION_LENGTH; s_i++)
			map->description[length++] = (wchar_t)(unsigned char)forge_summary[s_i];
		map->description[length] = 0;
		return;
	}

	csprintf(path, "%s%s%s", map->folder, map->name, DESCRIPTION_EXTENSION);
	stream = fopen(path, "rb");
	/* (a map named for its hash: its description named without it too) */
	if (!stream && map_share_name_hashed(map->name))
	{
		csprintf(path, "%s%.*s%s", map->folder, (int)map_share_name_base_length(map->name), map->name,
			DESCRIPTION_EXTENSION);
		stream = fopen(path, "rb");
	}
	if (stream)
	{
		text_size = (long)fread(text, 1, sizeof(text) - 1, stream);
		fclose(stream);
	}
	text[text_size] = 0;
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

static boolean custom_edition_forge_sidecar(
	char const *folder,
	char const *name,
	boolean *out_is_mcc_base)
{
	char path[MAXIMUM_FILENAME_LENGTH + 1];
	char buf[1024];
	FILE *stream;
	size_t n;
	char const *bm;

	if (out_is_mcc_base)
		*out_is_mcc_base = FALSE;

	csprintf(path, "%s%s.map", folder, name);
	stream = fopen(path, "rb");
	if (stream)
	{
		fclose(stream);
		return FALSE;
	}
	csprintf(path, "%s%s.txt", folder, name);
	stream = fopen(path, "rb");
	if (!stream)
		return FALSE;
	n = fread(buf, 1, sizeof(buf) - 1, stream);
	buf[n] = 0;
	fclose(stream);
	if (!strstr(buf, "[OpenCE Forge Map]"))
		return FALSE;
	bm = strstr(buf, "Base Map:");
	if (!bm)
		return FALSE;
	bm += 9;
	while (*bm == ' ' || *bm == '\t')
		bm++;
	if (!*bm || *bm == '\r' || *bm == '\n')
		return FALSE;
	if (out_is_mcc_base &&
		(_strnicmp(bm, "mcc_maps\\", 9) == 0 || _strnicmp(bm, "mcc_maps/", 9) == 0))
	{
		*out_is_mcc_base = TRUE;
	}
	return TRUE;
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
	boolean is_sidecar_only = FALSE;
	boolean is_mcc_base = FALSE;
	short map_index;

	if (!csstrcasecmp(extension, "txt"))
	{
		if (!custom_edition_forge_sidecar(folder, name, &is_mcc_base))
			return;
		is_sidecar_only = TRUE;
	}
	else if (csstrcasecmp(extension, "map"))
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
	if (is_sidecar_only)
	{
		campaign = FALSE;
	}
	else
	{
		if (!halo_custom_edition_tag_cache())
			return;
		/* campaign maps go to co-op; anything that isn't multiplayer either is skipped */
		campaign = custom_edition_cache_campaign(name);
		if (!campaign && !custom_edition_cache_multiplayer(name))
		{
			return;
		}
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
	csprintf(map->level_name, "%s%s", CUSTOM_EDITION_LEVEL_NAME_PREFIX, name);
	display_name_make(name, map->display_name);
	if (is_mcc_base)
	{
		wchar_t tmp_name[NUMBEROF(map->display_name)];
		short i = 0;
		csmemcpy(tmp_name, map->display_name, sizeof(tmp_name));
		map->display_name[0] = L'[';
		map->display_name[1] = L'M';
		map->display_name[2] = L'C';
		map->display_name[3] = L'C';
		map->display_name[4] = L']';
		map->display_name[5] = L' ';
		while (tmp_name[i] && 6 + i + 1 < (short)NUMBEROF(map->display_name))
		{
			map->display_name[6 + i] = tmp_name[i];
			i++;
		}
		map->display_name[6 + i] = 0;
	}
	custom_edition_map_description_read(map);
	/* (named for its hash by a build of before: its name given back,
	map_share_files.c) */
	if (!is_sidecar_only)
		map_share_map_found(folder, name);

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

/* Versions of a map listed (its name the same but for the hash a version's
file name ends in, a download's beside another of its name): the first in
the order the maps are sorted by name (the one of the name alone, which
sorts before them) shown by its name, the others with (2), (3) and so on. */
static void custom_edition_maps_tell_versions_apart(
	struct custom_edition_map *maps,
	short count)
{
	short index;

	for (index = 0; index < count; index++)
	{
		struct custom_edition_map *map = &maps[index];
		short length = map_share_name_base_length(map->name);
		short number = 1;
		short other;

		if (!map_share_name_hashed(map->name))
			continue;
		for (other = 0; other < count; other++)
		{
			struct custom_edition_map const *version = &maps[other];

			if (other == index || map_share_name_base_length(version->name) != length ||
				_strnicmp(version->name, map->name, length))
			{
				continue;
			}
			if (other < index)
				number++;
		}
		if (number > 1)
		{
			char suffix[8];
			short end = 0;
			short character;

			while (map->display_name[end])
				end++;
			csprintf(suffix, " (%d)", (int)MIN(number, 999));
			for (character = 0; suffix[character] && end + 1 < NUMBEROF(map->display_name); character++)
				map->display_name[end++] = (wchar_t)suffix[character];
			map->display_name[end] = 0;
		}
	}
}

static void custom_edition_maps_look_for(
	void)
{
	struct custom_edition_maps_globals *globals = &custom_edition_maps_globals;
	char const *folders[] = { CUSTOM_EDITION_MAP_DIRECTORY, CUSTOM_EDITION_INSTALL_MAP_DIRECTORY };
	short folder_index;

	custom_edition_maps_forget();
	globals->looked_for = TRUE;

	/* the game's custom_maps first, so its copy wins */
	for (folder_index = 0; folder_index < NUMBEROF(folders); folder_index++)
	{
		char pattern[MAXIMUM_FILENAME_LENGTH + 1];
		WIN32_FIND_DATAA data;
		HANDLE find;

		/* (the install's only when there is one) */
		if (folder_index && (!custom_edition_install_present() || !halo_custom_edition_tag_cache()))
			continue;
		/* A folder is listed through a handle of its own, never with
		find_files_start and find_files_next: those keep their place in one
		global, and as the game starts a thread of the menus' deletes the
		files of folders with them (perform_filesystem_initialization in
		ui_widget.c, directory_create_or_delete_contents). A listing here at
		the same time takes that one's place, and the thread then deletes
		the files of this folder: the maps. */
		csprintf(pattern, "%s*.*", folders[folder_index]);
		find = FindFirstFileA(pattern, &data);
		if (find == INVALID_HANDLE_VALUE)
			continue;
		do
		{
			char name[sizeof(data.cFileName) + 1];
			char *extension;

			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				continue;
			csmemcpy(name, data.cFileName, sizeof(data.cFileName));
			name[sizeof(data.cFileName)] = 0;
			extension = strrchr(name, '.');
			if (extension)
			{
				*extension++ = 0;
				custom_edition_map_add(folders[folder_index], name, extension);
			}
		} while (FindNextFileA(find, &data));
		CloseHandle(find);
	}
	qsort(globals->maps, globals->map_count, sizeof(globals->maps[0]), custom_edition_map_compare);
	qsort(globals->campaigns, globals->campaign_count, sizeof(globals->campaigns[0]), custom_edition_map_compare);
	custom_edition_maps_tell_versions_apart(globals->maps, globals->map_count);
	custom_edition_maps_tell_versions_apart(globals->campaigns, globals->campaign_count);
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
	/* (a map named for its hash: its picture named without it too) */
	if (!stream && map_share_name_hashed(map->name))
	{
		csprintf(path, "%s%.*s%s", map->folder, (int)map_share_name_base_length(map->name), map->name,
			PICTURE_EXTENSION);
		stream = fopen(path, "rb");
	}
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
	short level_index;
	short map_index;

	globals->xbox_level_count = MIN(xbox_level_count, MAXIMUM_XBOX_LEVELS);
	for (level_index = 0; level_index < globals->xbox_level_count; level_index++)
	{
		globals->levels[level_index] = xbox_levels[level_index];
	}
	/* (the menus ask for the list every tick, so the folders are scanned
	once, and again when a map list opens: custom_edition_maps_look_again) */
	if (!globals->looked_for)
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
	/* (a network game's map's file, found again) */
	custom_edition_map_paths_forget();

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
	char path[MAP_SHARE_PATH_SIZE];
	short map_index;

	/* (only a custom_maps\ name is a CE map's; any other is the game's own,
	a stock campaign level or none of these) */
	if (!custom_edition_level_name(level_name))
	{
		short level = campaign_level_from_name(level_name);

		return level != NONE ? FIRST_CAMPAIGN_DISPLAY_INDEX + level : NONE;
	}
	if (!globals->looked_for)
	{
		custom_edition_maps_look_for();
	}
	/* (the map by its file: a network game's of its version, which may be
	one of another name, <name>_<hash>.map, shown with its number) */
	if (custom_edition_map_file_path(level_name, path))
	{
		char const *file_name = tag_name_strip_path(path);
		short length = (short)csstrlen(file_name) - (short)(sizeof(".map") - 1);

		for (map_index = 0; length > 0 && map_index < globals->map_count; map_index++)
		{
			if ((short)csstrlen(globals->maps[map_index].name) == length &&
				!_strnicmp(globals->maps[map_index].name, file_name, length))
			{
				return FIRST_DISPLAY_INDEX + map_index;
			}
		}
		for (map_index = 0; length > 0 && map_index < globals->campaign_count; map_index++)
		{
			if ((short)csstrlen(globals->campaigns[map_index].name) == length &&
				!_strnicmp(globals->campaigns[map_index].name, file_name, length))
			{
				return FIRST_CUSTOM_CAMPAIGN_DISPLAY_INDEX + map_index;
			}
		}
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

boolean custom_edition_maps_level_campaign(
	char const *level_name)
{
	return level_name && custom_edition_maps_campaign(custom_edition_maps_display_index(level_name));
}

short custom_edition_maps_count(
	boolean campaign)
{
	struct custom_edition_maps_globals *globals = &custom_edition_maps_globals;

	if (!globals->looked_for)
	{
		custom_edition_maps_look_for();
	}

	return campaign ? globals->campaign_count : globals->map_count;
}

short custom_edition_maps_display_index_of(
	boolean campaign,
	short index)
{
	return index < 0 || index >= custom_edition_maps_count(campaign) ? NONE :
		(campaign ? FIRST_CUSTOM_CAMPAIGN_DISPLAY_INDEX : FIRST_DISPLAY_INDEX) + index;
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
