/*
CUSTOM_EDITION_CACHE.C

Halo Custom Edition and OpenSauce caches in the native builds' cache file
loader (custom_edition_cache.h). The loader runs only Xbox caches of this
build; without this unit it rejects a Custom Edition cache as "an old
version" and never looks for a ".yelo" file at all.
*/

/* ---------- headers */

#include "cseries.h"
#include "errors.h"
#include "tag_files/files.h"
#include "cache_file_formats.h"
#include "custom_edition_cache.h"

/* ---------- constants */

#define MAP_FILE_EXTENSION ".map"
#define OPENSAUCE_MAP_FILE_EXTENSION ".yelo"

/* ---------- public code */

boolean custom_edition_cache_refuse(
	void const *header,
	char const *build,
	char const *scenario_name,
	boolean fatal)
{
	int has_opensauce_header;

	if (cache_file_header_format(header, &has_opensauce_header) != _cache_file_format_custom_edition_cache)
	{
		return FALSE;
	}

	/* temporary holds 256 characters: bound the name and build strings */
	csprintf(
		temporary,
		"'%.96s' is a Halo Custom Edition cache%s (build %.31s): this build recognizes it but cannot run it (docs/custom_edition_caches.md)",
		scenario_name,
		has_opensauce_header ? " with an OpenSauce header" : "",
		build);
	error(_error_silent, "%s", temporary);
	if (fatal)
	{
		vassert(FALSE, temporary);
	}

	return TRUE;
}

void opensauce_cache_path_find(
	char *path,
	long path_size)
{
	long stem_length = (long)strlen(path) - (long)strlen(MAP_FILE_EXTENSION);
	struct file_reference reference;

	/* OpenSauce looks for the .map first, then the .yelo
	(cache_files_yelo.cpp, c_map_file_finder::SearchPath) */
	if (stem_length < 0 ||
		strcmp(path + stem_length, MAP_FILE_EXTENSION) ||
		stem_length + (long)strlen(OPENSAUCE_MAP_FILE_EXTENSION) >= path_size ||
		file_exists(file_reference_create_from_path(&reference, path, FALSE)))
	{
		return;
	}

	strcpy(path + stem_length, OPENSAUCE_MAP_FILE_EXTENSION);
	if (!file_exists(file_reference_create_from_path(&reference, path, FALSE)))
	{
		strcpy(path + stem_length, MAP_FILE_EXTENSION);
	}

	return;
}
