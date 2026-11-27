/*
FORGE_SIDECAR.C (test)

Tests custom_edition_map_format_forge_summary from port/linux/game/custom_edition_maps.c
against synthetic `.txt` Forge sidecar files in a temporary directory.
*/

#include "harness.h"

#include <stdint.h>

#ifndef _WIN32
#include <strings.h>
#define _stricmp strcasecmp
#define _strnicmp strncasecmp
#endif

#define MAXIMUM_FILENAME_LENGTH 256
#define DESCRIPTION_EXTENSION ".txt"
#define csprintf sprintf
#define csstrcpy(dst, src) strcpy((dst), (src))
#define csmemcpy memcpy

static int map_share_name_hashed(char const *name)
{
	(void)name;
	return 0;
}

static size_t map_share_name_base_length(char const *name)
{
	return strlen(name);
}

#include "under_test.inc"

static void write_temp_txt(char const *path, char const *content)
{
	FILE *f = fopen(path, "wb");
	CHECK(f != NULL, "failed to create temporary sidecar file");
	fwrite(content, 1, strlen(content), f);
	fclose(f);
}

static void build_tmp_prefix(char *dir_out, size_t dir_sz, char *full_out, size_t full_sz, char const *leaf)
{
#ifdef _WIN32
	char const *tmp = getenv("TEMP");
	if (!tmp || !tmp[0]) tmp = ".";
	snprintf(dir_out, dir_sz, "%s\\", tmp);
#else
	snprintf(dir_out, dir_sz, "/tmp/");
#endif
	snprintf(full_out, full_sz, "%s%s.txt", dir_out, leaf);
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";
	char dir[256], path[256], summary[512];

	CASE("basic")
	{
		build_tmp_prefix(dir, sizeof(dir), path, sizeof(path), "forge_test_basic");
		write_temp_txt(path,
			"[OpenCE Forge Map]\r\n"
			"Map Name: forge_test_basic\r\n"
			"Base Map: bloodgulch\r\n"
			"Objects Placed: 4\r\n");
		CHECK(custom_edition_map_format_forge_summary(dir, "forge_test_basic", summary, sizeof(summary)),
			"header-only Forge sidecar should parse");
		remove(path);
		CHECK(strstr(summary, "Forge (Base: bloodgulch)") != NULL, "summary should name base map");
		CHECK(strstr(summary, "Objects Placed: 4") != NULL, "header object count should be preserved");
		CHECK(strstr(summary, "Modes: All Modes") != NULL, "default multiplayer map supports All Modes");
	}
	else CASE("modes")
	{
		build_tmp_prefix(dir, sizeof(dir), path, sizeof(path), "forge_test_modes");
		write_temp_txt(path,
			"[OpenCE Forge Map]\r\n"
			"Base Map: levels\\test\\sidewinder\\sidewinder\r\n"
			"[ForgeState]\r\n"
			"OBJECT|1935893870|3|0|0|0|0|0|0|0|0|scenery\\rock\\rock\r\n"
			"OBJECT_GAMETYPE|1\r\n"
			"REPLACE_NETGAME_FLAGS|1\r\n"
			"NETGAME_FLAG|0|0|0|0|0|0\r\n"
			"NETGAME_FLAG|1|0|0|0|0|1\r\n"
			"NETGAME_FLAG|2|0|0|0|8|0\r\n");
		CHECK(custom_edition_map_format_forge_summary(dir, "forge_test_modes", summary, sizeof(summary)),
			"ForgeState sidecar should parse");
		remove(path);
		CHECK(strstr(summary, "Base: sidewinder") != NULL, "directory prefix stripped from Base Map");
		CHECK(strstr(summary, "Objects Placed: 1") != NULL, "counted OBJECT| line");
		CHECK(strstr(summary, "Slayer, CTF, KOTH") != NULL && strstr(summary, "Oddball") == NULL,
			"detected Slayer + CTF + KOTH only");
	}
	else CASE("sp_default")
	{
		build_tmp_prefix(dir, sizeof(dir), path, sizeof(path), "forge_test_sp");
		write_temp_txt(path,
			"[OpenCE Forge Map]\r\n"
			"Base Map: a10\r\n"
			"[ForgeState]\r\n");
		CHECK(custom_edition_map_format_forge_summary(dir, "forge_test_sp", summary, sizeof(summary)),
			"single-player base map sidecar should parse");
		remove(path);
		CHECK(strstr(summary, "Modes: Slayer") != NULL && strstr(summary, "CTF") == NULL,
			"single-player base map defaults to Slayer unless objective flags exist");
	}
	else CASE("invalid")
	{
		build_tmp_prefix(dir, sizeof(dir), path, sizeof(path), "forge_test_invalid");
		write_temp_txt(path, "Just a plain text file without Forge headers.\r\n");
		CHECK(!custom_edition_map_format_forge_summary(dir, "forge_test_invalid", summary, sizeof(summary)),
			"non-Forge text file must be rejected");
		remove(path);
	}
	return 0;
}
