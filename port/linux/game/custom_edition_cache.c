/*
CUSTOM_EDITION_CACHE.C

Halo Custom Edition caches in the native builds' cache file loader
(custom_edition_cache.h). The loader runs only Xbox caches of this build;
without this unit it rejects a Custom Edition cache as "an old version".
OpenSauce caches are refused (cache_file_formats.c), and ".yelo" files are
never looked for.

With the game.custom_edition setting on (the platform then reserves the
Custom Edition tag cache, port/linux/src/xbox_memory.c), a Custom Edition
map is loaded instead: it is read in place rather than copied to the cache
partition, and its tags go to 0x40440000 through cache_file_formats.c, which
also converts what only needs their bytes changed. custom_edition_bitmaps.c
and custom_edition_geometry.c convert the rest with the game's own
functions.
Every read the game makes of the map (structure BSPs, bitmap pixels, sound
samples) is served from the map, bitmaps.map or sounds.map according to
where its offset falls in their combined offset space.
*/

/* ---------- headers */

#include "cseries.h"
#include "errors.h"
#include "tag_files/files.h"
#include "tag_files/tag_files.h"
#include "cache/cache_files.h"
#include "scenario/scenario_definitions.h"
#include "cache_file_formats.h"
#include "custom_edition_cache.h"
#include "tag_schema.h"

#include <stdlib.h>

/* ---------- constants */

#define MAP_FILE_EXTENSION ".map"
/* as the cache file loader's own map paths */
#define MAP_PATH_SIZE 256
/* where a cache header keeps the file's length (cache_files.c) */
#define CACHE_FILE_HEADER_FILE_LENGTH_OFFSET 0x08

/* (the validator has room for the tag cache's tags) */
typedef char verify_custom_edition_tag_cache_bytes[
	CUSTOM_EDITION_TAG_CACHE_BYTES <= TAG_VALIDATE_MAXIMUM_TAG_CACHE_SIZE ? 1 : -1];

/* the combined offset space: the map (CUSTOM_EDITION_CACHE_FILE_MAXIMUM_BYTES
at most), the Ogg Vorbis sounds decoded at load (custom_edition_sounds.c)
after the largest map it takes, bitmaps.map, sounds.map */
#define COMBINED_DECODED_OFFSET 0x30000000UL
#define COMBINED_BITMAPS_OFFSET 0x40000000UL
#define COMBINED_SOUNDS_OFFSET 0x60000000UL
#define COMBINED_OFFSET_LIMIT 0x80000000UL

/* The renderer write-protects the memory it has made textures of and learns
of changes to it from the faults writes take (port/linux/src/memory_watch.c);
the kernel fails a read into such memory instead of faulting. Reads for the
game therefore land here first and are copied, as the platform's own file
layer does (port/linux/src/xbox_files.c, read_at). */
#define READ_STAGING_BYTES 0x10000

/* ---------- structures */

struct custom_edition_file
{
	FILE *stream;
	struct cache_file_source source;
};

struct custom_edition_cache_globals
{
	boolean tags_loaded;
	/* the Halo PC behaviours the loaded map relies on (enum
	custom_edition_behaviour, flags) */
	uint32_t behaviours;
	/* the tag cache and the bytes of it the loaded tags use */
	uint8_t *tag_cache;
	uint32_t loaded_bytes;
	/* the tag cache the map's structure BSPs load to the top of, and the
	length of its cache data */
	uint32_t tag_cache_bytes;
	uint32_t file_length;
	struct custom_edition_file map;
	struct custom_edition_file resource_files[NUMBER_OF_RESOURCE_MAP_TYPES];
	struct resource_map resource_map_storage[NUMBER_OF_RESOURCE_MAP_TYPES];
	struct resource_map *resource_maps[NUMBER_OF_RESOURCE_MAP_TYPES];
	byte read_staging[READ_STAGING_BYTES];
};

/* ---------- globals */

static struct custom_edition_cache_globals custom_edition_cache_globals;

/* ---------- private code */

/* (port_config.c) */
int config_boolean(char const *name);

/* why Custom Edition maps cannot run at all, when a cache's own checks
found nothing wrong: no tag cache to load them into */
static char const *custom_edition_unavailable_reason(
	void)
{
#ifdef HALO_64BIT
	return "Custom Edition maps are not supported on the 64-bit builds yet";
#else
	return config_boolean("game.custom_edition") ?
		"the Custom Edition tag cache could not be reserved at startup (the log's first lines say why)" :
		"Custom Edition maps are turned off (game.custom_edition)";
#endif
}

static int custom_edition_file_read(
	void *context,
	uint32_t offset,
	uint32_t size,
	void *buffer)
{
	FILE *stream = context;

	return fseek(stream, (long)offset, SEEK_SET) == 0 && fread(buffer, 1, size, stream) == size;
}

static boolean custom_edition_file_open(
	struct custom_edition_file *file,
	char const *path)
{
	long size;

	file->stream = fopen(path, "rb");
	if (!file->stream)
	{
		return FALSE;
	}
	if (fseek(file->stream, 0, SEEK_END) != 0 || (size = ftell(file->stream)) < 0)
	{
		fclose(file->stream);
		file->stream = NULL;
		return FALSE;
	}
	file->source.context = file->stream;
	file->source.read = custom_edition_file_read;
	file->source.size = (uint32_t)size;

	return TRUE;
}

static void custom_edition_file_close(
	struct custom_edition_file *file)
{
	if (file->stream)
	{
		fclose(file->stream);
		file->stream = NULL;
	}

	return;
}

static boolean file_path_exists(
	char const *path)
{
	struct file_reference reference;

	return file_exists(file_reference_create_from_path(&reference, path, FALSE));
}

/* whether a Halo Custom Edition install is set (paths.custom_edition), the
platform's h:\ (port/linux/src/xbox_files.c) */
boolean custom_edition_install_present(
	void)
{
	extern char const *platform_custom_edition_root(void);

	return platform_custom_edition_root()[0] != 0;
}

/* the maps folders looked in, in order: the game's custom_maps, then the
Custom Edition install's */
static char const *maps_folder(
	short index)
{
	return index == 0 ? CUSTOM_EDITION_MAP_DIRECTORY : CUSTOM_EDITION_INSTALL_MAP_DIRECTORY;
}
#define NUMBER_OF_MAPS_FOLDERS 2

/* whether <folder><name><extension>, made in path, exists */
static boolean maps_folder_has(
	char const *folder,
	char const *name,
	char const *extension,
	char *path)
{
	/* (the install's only when there is one: an Xbox drive of the save
	folder is made when first looked in, port/linux/src/xbox_files.c) */
	if (strlen(folder) + strlen(name) + strlen(extension) >= MAP_PATH_SIZE ||
		(folder == maps_folder(1) && !custom_edition_install_present()))
	{
		return FALSE;
	}
	sprintf(path, "%s%s%s", folder, name, extension);

	return file_path_exists(path);
}

/* the file that holds the map `map_name` names (a level name or a file
name): <custom maps folder>\<name>.map */
static boolean custom_edition_map_path(
	char const *map_name,
	char *path)
{
	char const *name = tag_name_strip_path(map_name);
	short folder;

	for (folder = 0; folder < NUMBER_OF_MAPS_FOLDERS; folder++)
	{
		if (maps_folder_has(maps_folder(folder), name, MAP_FILE_EXTENSION, path))
		{
			return TRUE;
		}
	}

	return FALSE;
}

/* A cache's resource map of `type`, bitmaps.map and so on, in the first maps
folder that has it; the game's when none does, for the message. */
static boolean custom_edition_resource_map_path(
	enum resource_map_type type,
	char *path)
{
	char const *name = resource_map_type_describe(type);
	short folder;

	for (folder = 0; folder < NUMBER_OF_MAPS_FOLDERS; folder++)
	{
		if (maps_folder_has(maps_folder(folder), name, MAP_FILE_EXTENSION, path))
		{
			return TRUE;
		}
	}
	sprintf(path, "%s%s%s", maps_folder(0), name, MAP_FILE_EXTENSION);

	return TRUE;
}

static void custom_edition_cache_files_close(
	void)
{
	struct custom_edition_cache_globals *globals = &custom_edition_cache_globals;
	short type;

	for (type = _resource_map_bitmaps; type < NUMBER_OF_RESOURCE_MAP_TYPES; type++)
	{
		if (globals->resource_maps[type])
		{
			resource_map_close(globals->resource_maps[type]);
			globals->resource_maps[type] = NULL;
		}
		custom_edition_file_close(&globals->resource_files[type]);
	}
	custom_edition_file_close(&globals->map);

	return;
}

/* Converts the loaded map's models for this build from its model data,
which is read for the purpose and let go. */
static boolean custom_edition_cache_models_convert(
	uint8_t *tag_cache,
	struct custom_edition_load_report const *report)
{
	struct custom_edition_file const *map = &custom_edition_cache_globals.map;
	byte *model_data = malloc(report->model_data_bytes + 1);
	boolean success = FALSE;

	if (!model_data)
	{
		error(_error_silent, "custom edition: out of memory for 0x%lX bytes of model data", (unsigned long)report->model_data_bytes);
	}
	else if (!map->source.read(map->source.context, report->model_data_offset, report->model_data_bytes, model_data))
	{
		error(_error_silent, "custom edition: cannot read the model data");
	}
	else
	{
		success = custom_edition_models_convert(
			tag_cache,
			report->tag_data_bytes + report->resource_tag_bytes,
			report,
			model_data);
	}
	free(model_data);

	return success;
}

/* the Halo PC behaviours this build follows (cache_file_formats.c and
hud_draw.c) */
#define FOLLOWED_BEHAVIOURS ( \
	1U << _custom_edition_behaviour_gearbox_multitexture_blend_modes | \
	1U << _custom_edition_behaviour_invert_detail_after_reflection | \
	1U << _custom_edition_behaviour_hud_number_scale | \
	1U << _custom_edition_behaviour_disable_bitmap_hud_scale_flags | \
	1U << _custom_edition_behaviour_block_multitexture_overlays)

/* logs each Halo PC behaviour the map relies on, and whether this build
follows it */
static void custom_edition_behaviours_log(
	uint32_t behaviours)
{
	short behaviour;

	for (behaviour = 0; behaviour < NUMBER_OF_CUSTOM_EDITION_BEHAVIOURS; behaviour++)
	{
		if (!((behaviours >> behaviour) & 1))
			continue;
		error(
			_error_silent,
			(FOLLOWED_BEHAVIOURS >> behaviour) & 1 ?
				"custom edition: relies on Halo PC's %s (Chimera's map list), which this build follows" :
				"custom edition: relies on Halo PC's %s (Chimera's map list), which this build does not do yet",
			custom_edition_behaviour_name(behaviour));
	}

	return;
}

/* The tags custom_edition_cache_load loaded and custom_edition_cache_convert
converted, checked as an Xbox map's are before the game reads them
(port/linux/game/tag_validate.c): their pixels and samples in the files of
the combined offset space, and the sounds decoded at load. */
static boolean custom_edition_cache_tags_validate(
	uint8_t *tag_cache,
	struct custom_edition_load_report const *report)
{
	struct custom_edition_cache_globals *globals = &custom_edition_cache_globals;
	struct tag_validate_file_range ranges[MAXIMUM_TAG_VALIDATE_FILE_RANGES];
	short range_count = 0;

	ranges[range_count].offset = 0;
	ranges[range_count++].size = report->identity.file_length;
	ranges[range_count].offset = COMBINED_DECODED_OFFSET;
	ranges[range_count++].size = custom_edition_sounds_decoded_bytes();
	if (globals->resource_files[_resource_map_bitmaps].stream)
	{
		ranges[range_count].offset = COMBINED_BITMAPS_OFFSET;
		ranges[range_count++].size = globals->resource_files[_resource_map_bitmaps].source.size;
	}
	if (globals->resource_files[_resource_map_sounds].stream)
	{
		ranges[range_count].offset = COMBINED_SOUNDS_OFFSET;
		ranges[range_count++].size = globals->resource_files[_resource_map_sounds].source.size;
	}
	if (!tag_validate_custom_edition_tags(
		tag_cache,
		(long)(report->tag_data_bytes + report->resource_tag_bytes),
		report->tag_cache_bytes,
		ranges,
		range_count,
		report->identity.name))
	{
		return FALSE;
	}
	if (tag_validate_corrections())
	{
		error(_error_silent, "custom edition: the map's tags needed %ld corrections (above)",
			tag_validate_corrections());
	}

	return TRUE;
}

/* Makes the tags custom_edition_cache_load loaded into `tag_cache` this
build's: their resource offsets combined, their bytes converted, then
checked as an Xbox map's tags are; then their bitmaps checked, their
scripts given this build's functions and their models converted, all from
tags the checks passed. */
static boolean custom_edition_cache_tags_convert(
	uint8_t *tag_cache,
	struct custom_edition_load_report const *report)
{
	uint32_t loaded_bytes = report->tag_data_bytes + report->resource_tag_bytes;
	struct custom_edition_conversion_report conversion;
	enum cache_file_status status;

	custom_edition_cache_combine_resource_offsets(
		tag_cache,
		loaded_bytes,
		COMBINED_BITMAPS_OFFSET,
		COMBINED_SOUNDS_OFFSET);
	/* (before the conversion, which silences sounds this build cannot play) */
	custom_edition_sounds_decode(tag_cache, loaded_bytes, (long)COMBINED_DECODED_OFFSET,
		COMBINED_BITMAPS_OFFSET - COMBINED_DECODED_OFFSET);
	status = custom_edition_cache_convert(tag_cache, loaded_bytes, report->identity.name, &conversion);
	if (status != _cache_file_status_ok)
	{
		error(
			_error_silent,
			"custom edition: cannot convert '%s': %s",
			custom_edition_cache_tag_name(tag_cache, loaded_bytes, conversion.problem_tag_index),
			cache_file_status_describe(status));
		return FALSE;
	}
	error(
		_error_silent,
		"custom edition: %ld shaders renumbered, %ld transparent chicago extended shaders made transparent chicago shaders, %ld bitmaps prepared",
		(long)conversion.shaders_retyped,
		(long)conversion.chicago_extended_shaders,
		(long)conversion.bitmaps_prepared);
	custom_edition_cache_globals.behaviours = conversion.behaviours;
	custom_edition_behaviours_log(conversion.behaviours);
	if (conversion.shaders_mistyped)
	{
		error(_error_silent, "custom edition: %ld shaders whose type was not their group's were given their group's",
			(long)conversion.shaders_mistyped);
	}
	if (conversion.bitmaps_made_linear)
	{
		error(_error_silent, "custom edition: %ld bitmaps of sides only Halo PC draws are drawn linear (their first level)",
			(long)conversion.bitmaps_made_linear);
	}
	if (conversion.node_links_cut)
	{
		error(_error_silent, "custom edition: %ld model and animation node links that looped or pointed past the nodes were cut",
			(long)conversion.node_links_cut);
	}
	if (conversion.animation_overlays_disabled)
	{
		error(
			_error_silent,
			"custom edition: %ld animation overlays named animations their graphs do not have and were disabled",
			(long)conversion.animation_overlays_disabled);
	}
	if (conversion.sounds_undecodable)
	{
		error(
			_error_silent,
			"custom edition: %ld sounds use a compression this build cannot decode (Custom Edition's Ogg Vorbis) and will not play",
			(long)conversion.sounds_undecodable);
	}
	if (conversion.hud_placements_rescaled)
	{
		error(
			_error_silent,
			"custom edition: %ld HUD elements drawn from Halo PC's double resolution bitmaps were given half their scale",
			(long)conversion.hud_placements_rescaled);
	}
	if (conversion.score_hint_converted)
	{
		error(_error_silent, "custom edition: the multiplayer score hint names the BACK button where Halo PC names a key");
	}
	if (conversion.widget_functions_cleared || conversion.pause_menu_trimmed)
	{
		error(
			_error_silent,
			"custom edition: %ld menu event handlers of Halo PC's own functions run none%s",
			(long)conversion.widget_functions_cleared,
			conversion.pause_menu_trimmed ? "; the multiplayer pause menu is the Xbox's resume and quit" : "");
	}

	return custom_edition_cache_tags_validate(tag_cache, report) &&
		custom_edition_bitmaps_verify(tag_cache, loaded_bytes) &&
		custom_edition_reordered_bitmaps_find(tag_cache, loaded_bytes) &&
		custom_edition_scripts_convert(tag_cache, loaded_bytes) &&
		custom_edition_cache_models_convert(tag_cache, report);
}

/* Whether Custom Edition maps may run (game.custom_edition) and the map
`map_name` names is a Custom Edition cache, whose header is then described
in `identity`. */
static boolean custom_edition_cache_identify(
	char const *map_name,
	struct cache_file_identity *identity)
{
	char path[MAP_PATH_SIZE];
	struct custom_edition_file file;
	boolean identified = FALSE;

	if (!halo_custom_edition_tag_cache() ||
		!custom_edition_map_path(map_name, path) ||
		!custom_edition_file_open(&file, path))
	{
		return FALSE;
	}
	if (cache_file_identify(&file.source, identity) == _cache_file_status_ok &&
		identity->format == _cache_file_format_custom_edition_cache)
	{
		identified = TRUE;
	}
	custom_edition_file_close(&file);

	return identified;
}

static void custom_edition_cache_report_log(
	struct custom_edition_load_report const *report)
{
	error(
		_error_silent,
		"custom edition: %ld tags, 0x%lX bytes of tag data and 0x%lX of tags from resource maps (bitmaps %ld, sounds %ld, loc %ld)",
		(long)report->tag_count,
		(unsigned long)report->tag_data_bytes,
		(unsigned long)report->resource_tag_bytes,
		(long)report->resource_tag_counts[_resource_map_bitmaps],
		(long)report->resource_tag_counts[_resource_map_sounds],
		(long)report->resource_tag_counts[_resource_map_locale]);
	error(
		_error_silent,
		"custom edition: %ld structure BSPs (%ld materials), %ld bitmap and %ld sound ranges checked, %ld pointers relocated, checksum %s",
		(long)report->structure_bsp_count,
		(long)report->structure_bsp_materials_checked,
		(long)report->bitmap_data_ranges_checked,
		(long)report->sound_sample_ranges_checked,
		(long)report->relocated_pointer_count,
		TEST_FLAG(report->warnings, _custom_edition_warning_checksum_mismatch_bit) ? "mismatched" : "matched");
	if (report->scenario_regrouped)
		error(_error_silent, "custom edition: the scenario tag had another group (map protection) and was given the scenario's");

	return;
}

/* ---------- public code */

boolean custom_edition_cache_refuse(
	void const *header,
	char const *build,
	char const *path)
{
	struct custom_edition_file file;
	struct cache_file_identity identity;
	enum cache_file_status status = _cache_file_status_read_failed;
	char map_path[MAP_PATH_SIZE];

	if (cache_file_header_format(header) != _cache_file_format_custom_edition_cache)
	{
		return FALSE;
	}
	/* (why the Custom Edition loader passed it by: what its checks found, or
	why it cannot run any when they found nothing. It reaches the Xbox loader
	from the game's own maps folder, by `path`, a file's or a scenario's) */
	snprintf(map_path, sizeof(map_path), "%s%s%s", cache_files_map_directory(), tag_name_strip_path(path),
		MAP_FILE_EXTENSION);
	if (custom_edition_file_open(&file, path) || custom_edition_file_open(&file, map_path))
	{
		status = cache_file_identify(&file.source, &identity);
		custom_edition_file_close(&file);
	}
	error(
		_error_silent,
		"'%.96s' is a Halo Custom Edition cache (build %.31s) this build cannot run: %s (docs/custom_edition_caches.md)",
		path,
		build,
		status != _cache_file_status_ok ? cache_file_status_describe(status) :
		halo_custom_edition_tag_cache() ? "Custom Edition maps are played from the custom_maps folder" :
		custom_edition_unavailable_reason());

	return TRUE;
}

boolean custom_edition_level_name(
	char const *level_name)
{
	return level_name &&
		!_strnicmp(level_name, CUSTOM_EDITION_LEVEL_NAME_PREFIX, csstrlen(CUSTOM_EDITION_LEVEL_NAME_PREFIX));
}

boolean custom_edition_map_file_present(
	char const *map_name)
{
	char path[MAP_PATH_SIZE];

	return custom_edition_map_path(map_name, path);
}

unsigned long custom_edition_map_checksum(
	char const *level_name)
{
	char path[MAP_PATH_SIZE];
	struct custom_edition_file file;
	struct cache_file_identity identity;
	unsigned long checksum = 0;

	if (custom_edition_map_path(level_name, path) && custom_edition_file_open(&file, path))
	{
		if (cache_file_identify(&file.source, &identity) == _cache_file_status_ok &&
			identity.format == _cache_file_format_custom_edition_cache)
		{
			checksum = identity.checksum;
		}
		custom_edition_file_close(&file);
	}

	return checksum;
}

boolean custom_edition_cache_present(
	char const *level_name,
	unsigned long checksum,
	char *message,
	long message_size)
{
	char const *name = tag_name_strip_path(level_name);
	char path[MAP_PATH_SIZE];
	struct custom_edition_file file;
	struct cache_file_identity identity;
	enum cache_file_status status = _cache_file_status_read_failed;
	short folder;
	short type;

	if (!custom_edition_map_path(level_name, path))
	{
		snprintf(message, message_size, "You don't have the map %.64s.map. If you have it, copy it into custom_maps.",
			name);
		return FALSE;
	}
	if (!halo_custom_edition_tag_cache())
	{
		error(_error_silent, "custom edition: the map '%s' cannot be played: %s", level_name,
			custom_edition_unavailable_reason());
		snprintf(message, message_size, "The map %.64s can't be played: Custom Edition maps can't run (see debug.txt).",
			name);
		return FALSE;
	}
	if (custom_edition_file_open(&file, path))
	{
		status = cache_file_identify(&file.source, &identity);
		custom_edition_file_close(&file);
	}
	if (status != _cache_file_status_ok || identity.format != _cache_file_format_custom_edition_cache)
	{
		error(_error_silent, "custom edition: '%s' cannot be played: %s", path,
			status != _cache_file_status_ok ? cache_file_status_describe(status) :
			"it is not a Halo Custom Edition cache");
		snprintf(message, message_size, "Your %.64s.map can't be played (debug.txt says why).", name);
		return FALSE;
	}
	/* (another version's tags are not the host's: the objects the host sends
	would be other things here) */
	if (checksum && identity.checksum != checksum)
	{
		error(_error_silent, "custom edition: '%s' is another version of the host's map (checksum %08lX, the host's %08lX)",
			path, (unsigned long)identity.checksum, checksum);
		snprintf(message, message_size,
			"Your %.64s.map is a different version from the host's. Copy the host's into custom_maps.", name);
		return FALSE;
	}
	/* (the resource maps every Custom Edition map's tags are read from) */
	for (type = _resource_map_bitmaps; type < NUMBER_OF_RESOURCE_MAP_TYPES; type++)
	{
		char const *resource_name = resource_map_type_describe((enum resource_map_type)type);
		char resource_path[MAP_PATH_SIZE];

		for (folder = 0; folder < NUMBER_OF_MAPS_FOLDERS; folder++)
		{
			if (maps_folder_has(maps_folder(folder), resource_name, MAP_FILE_EXTENSION, resource_path))
				break;
		}
		if (folder == NUMBER_OF_MAPS_FOLDERS)
		{
			error(_error_silent, "custom edition: the map '%s' needs %s.map, which no maps folder has", level_name,
				resource_name);
			snprintf(message, message_size,
				"The map %.64s needs Custom Edition's bitmaps.map, sounds.map and loc.map in custom_maps.", name);
			return FALSE;
		}
	}

	return TRUE;
}

boolean custom_edition_cache_playable(
	char const *level_name)
{
	struct cache_file_identity identity;

	return custom_edition_level_name(level_name) && custom_edition_cache_identify(level_name, &identity);
}

boolean custom_edition_cache_campaign(
	char const *map_name)
{
	struct cache_file_identity identity;

	return custom_edition_cache_identify(map_name, &identity) &&
		identity.scenario_type == _scenario_type_solo;
}

boolean custom_edition_cache_multiplayer(
	char const *map_name)
{
	struct cache_file_identity identity;

	return custom_edition_cache_identify(map_name, &identity) &&
		identity.scenario_type == _scenario_type_multiplayer;
}

struct cache_file_tag_header *custom_edition_cache_tags_load(
	char const *map_name,
	void *header)
{
	struct custom_edition_cache_globals *globals = &custom_edition_cache_globals;
	uint8_t *tag_cache = halo_custom_edition_tag_cache();
	struct custom_edition_load_report report;
	struct cache_file_identity identity;
	char path[MAP_PATH_SIZE];
	enum cache_file_status status;
	short type;

	assert(!globals->tags_loaded);
	if (!tag_cache)
	{
		error(_error_silent, "custom edition: cannot load the map '%s': %s", map_name,
			custom_edition_unavailable_reason());
		return NULL;
	}
	if (!custom_edition_map_path(map_name, path) || !custom_edition_file_open(&globals->map, path))
	{
		error(_error_silent, "custom edition: cannot open the map '%s' (it is looked for in the custom_maps folder)",
			map_name);
		return NULL;
	}
	status = cache_file_identify(&globals->map.source, &identity);
	if (status != _cache_file_status_ok ||
		identity.format != _cache_file_format_custom_edition_cache ||
		identity.file_size > COMBINED_DECODED_OFFSET ||
		!globals->map.source.read(globals->map.source.context, 0, CACHE_FILE_HEADER_BYTES, header))
	{
		error(_error_silent, "custom edition: '%s' is not a loadable cache (%s)", path, cache_file_status_describe(status));
		custom_edition_cache_files_close();
		return NULL;
	}
	/* (the length the loader went by: the whole file's where Invader left 0) */
	{
		uint32_t file_length = identity.file_length;

		memcpy((byte *)header + CACHE_FILE_HEADER_FILE_LENGTH_OFFSET, &file_length, sizeof(file_length));
	}
	error(_error_silent, "custom edition: loading '%s' (build %s)", path, identity.build);

	for (type = _resource_map_bitmaps; type < NUMBER_OF_RESOURCE_MAP_TYPES; type++)
	{
		char resource_path[MAP_PATH_SIZE];

		if (!custom_edition_resource_map_path((enum resource_map_type)type, resource_path) ||
			!custom_edition_file_open(&globals->resource_files[type], resource_path))
		{
			error(_error_silent, "custom edition: no resource map '%s'", resource_path);
			continue;
		}
		status = resource_map_open(
			&globals->resource_files[type].source,
			(enum resource_map_type)type,
			&globals->resource_map_storage[type]);
		if (status != _cache_file_status_ok)
		{
			error(_error_silent, "custom edition: resource map '%s': %s", resource_path, cache_file_status_describe(status));
			continue;
		}
		globals->resource_maps[type] = &globals->resource_map_storage[type];
	}

	/* the combined offset space serves bitmaps.map and sounds.map after the
	map itself */
	if ((globals->resource_files[_resource_map_bitmaps].stream &&
		globals->resource_files[_resource_map_bitmaps].source.size > COMBINED_SOUNDS_OFFSET - COMBINED_BITMAPS_OFFSET) ||
		(globals->resource_files[_resource_map_sounds].stream &&
		globals->resource_files[_resource_map_sounds].source.size > COMBINED_OFFSET_LIMIT - COMBINED_SOUNDS_OFFSET))
	{
		error(_error_silent, "custom edition: a resource map is too large for this loader");
		custom_edition_cache_files_close();
		return NULL;
	}

	status = custom_edition_cache_load(
		&globals->map.source,
		globals->resource_maps,
		tag_cache,
		CUSTOM_EDITION_TAG_CACHE_BYTES,
		&report);
	if (status != _cache_file_status_ok)
	{
		error(
			_error_silent,
			"custom edition: cannot load '%s': %s (tag %ld, 0x%08lX)",
			path,
			cache_file_status_describe(status),
			(long)report.problem_tag_index,
			(unsigned long)report.problem_location);
		custom_edition_cache_files_close();
		return NULL;
	}
	custom_edition_cache_report_log(&report);
	if (!custom_edition_cache_tags_convert(tag_cache, &report))
	{
		error(_error_silent, "custom edition: cannot run '%s'", path);
		custom_edition_sounds_dispose();
		custom_edition_models_dispose();
		custom_edition_bitmaps_dispose();
		custom_edition_cache_files_close();
		return NULL;
	}
	globals->tag_cache = tag_cache;
	globals->loaded_bytes = report.tag_data_bytes + report.resource_tag_bytes;
	globals->tag_cache_bytes = report.tag_cache_bytes;
	globals->file_length = report.identity.file_length;
	globals->tags_loaded = TRUE;

	return (struct cache_file_tag_header *)tag_cache;
}

boolean custom_edition_cache_stock_tag(
	long tag_index)
{
	struct custom_edition_cache_globals *globals = &custom_edition_cache_globals;

	return globals->tags_loaded && tag_index != NONE &&
		custom_edition_cache_tag_in_resource_map(globals->tag_cache, globals->loaded_bytes,
			(int32_t)DATUM_INDEX_TO_ABSOLUTE_INDEX(tag_index));
}

boolean custom_edition_structure_bsp_reference_valid(
	struct scenario_structure_bsp_reference const *reference)
{
	struct custom_edition_cache_globals *globals = &custom_edition_cache_globals;
	unsigned long address = (unsigned long)reference->base_address;
	unsigned long top = CUSTOM_EDITION_TAG_CACHE_ADDRESS + globals->tag_cache_bytes;

	/* (in the map's cache data, read to the top of its tag cache, above its
	tags, as its loader found them) */
	if (!globals->tags_loaded ||
		reference->file_offset < CACHE_FILE_HEADER_BYTES ||
		reference->file_size < 0x18 ||
		(unsigned long)reference->file_offset > globals->file_length ||
		(unsigned long)reference->file_size > globals->file_length - (unsigned long)reference->file_offset ||
		address < CUSTOM_EDITION_TAG_CACHE_ADDRESS + globals->loaded_bytes ||
		address > top ||
		(unsigned long)reference->file_size > top - address)
	{
		error(_error_silent, "custom edition: a structure bsp's %08lx bytes at %08lx would load to %08lx, outside its tag cache",
			(unsigned long)reference->file_size, (unsigned long)reference->file_offset, address);
		return FALSE;
	}

	return TRUE;
}

boolean custom_edition_cache_tags_loaded(
	void)
{
	return custom_edition_cache_globals.tags_loaded;
}

boolean custom_edition_cache_relies_on(
	short behaviour)
{
	return custom_edition_cache_globals.tags_loaded &&
		behaviour >= 0 && behaviour < NUMBER_OF_CUSTOM_EDITION_BEHAVIOURS &&
		((custom_edition_cache_globals.behaviours >> behaviour) & 1);
}

void custom_edition_cache_tags_unload(
	void)
{
	/* the structure BSP goes first, as scenario_structure_bsp_unload would
	have released it */
	custom_edition_structure_bsp_unload();
	custom_edition_sounds_dispose();
	custom_edition_models_dispose();
	custom_edition_bitmaps_dispose();
	custom_edition_cache_files_close();
	custom_edition_cache_globals.tags_loaded = FALSE;
	custom_edition_cache_globals.behaviours = 0;
	custom_edition_cache_globals.tag_cache = NULL;
	custom_edition_cache_globals.loaded_bytes = 0;

	return;
}

void custom_edition_cache_read(
	long tag_index,
	long offset,
	long size,
	void *buffer)
{
	struct custom_edition_cache_globals *globals = &custom_edition_cache_globals;
	struct custom_edition_file *file;
	unsigned long file_offset;
	long read_bytes;
	boolean read;

	if ((unsigned long)offset >= COMBINED_SOUNDS_OFFSET)
	{
		file = &globals->resource_files[_resource_map_sounds];
		file_offset = (unsigned long)offset - COMBINED_SOUNDS_OFFSET;
	}
	else if ((unsigned long)offset >= COMBINED_BITMAPS_OFFSET)
	{
		file = &globals->resource_files[_resource_map_bitmaps];
		file_offset = (unsigned long)offset - COMBINED_BITMAPS_OFFSET;
	}
	else if ((unsigned long)offset >= COMBINED_DECODED_OFFSET)
	{
		if (!custom_edition_sounds_read((long)((unsigned long)offset - COMBINED_DECODED_OFFSET), size, buffer))
			csmemset(buffer, 0, size);
		return;
	}
	else
	{
		file = &globals->map;
		file_offset = (unsigned long)offset;
	}

	/* the game reads its map from its main thread only (scenario and
	structure BSP loading, the texture and sound caches), so the streams and
	the staging buffer need no lock */
	read = file->stream && size >= 0;
	for (read_bytes = 0; read && read_bytes < size; read_bytes += READ_STAGING_BYTES)
	{
		long chunk_bytes = MIN(size - read_bytes, READ_STAGING_BYTES);

		read = file->source.read(file->source.context, file_offset + read_bytes, (uint32_t)chunk_bytes, globals->read_staging);
		if (read)
		{
			csmemcpy((byte *)buffer + read_bytes, globals->read_staging, chunk_bytes);
		}
	}
	if (!read)
	{
		/* the loader checked every range the tags give, so this is an I/O
		failure: the reader gets zeros rather than whatever was there */
		error(_error_silent, "custom edition: cannot read 0x%lX bytes at 0x%08lX", (unsigned long)size, (unsigned long)offset);
		if (size > 0)
		{
			csmemset(buffer, 0, size);
		}
	}
	else if (tag_index != NONE)
	{
		custom_edition_bitmap_pixels_arrived(globals->tag_cache, globals->loaded_bytes, tag_index, offset, buffer);
	}

	return;
}
