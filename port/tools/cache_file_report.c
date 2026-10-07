/*
CACHE_FILE_REPORT.C

Reports what the native builds can do with Halo 1 map files: which format
each file is, whether it can be loaded, and why not
(port/linux/game/cache_file_formats.c; docs/custom_edition_caches.md).

	cache_file_report [--maps DIRECTORY] [--dump-tags FILE] FILE...

A Custom Edition cache is loaded with the resource maps it needs, looked for
in DIRECTORY (by default the cache's own directory): bitmaps.map, sounds.map
and loc.map. OpenSauce caches are refused. A cache that loads is then
converted for this build as far as its bytes alone go
(custom_edition_cache_convert), and --dump-tags writes the converted tags,
as they would sit at 0x40440000, to FILE. Every file gets a block of
"key: value" lines; the exit status is 0 when every file was recognized and
every Custom Edition cache loaded and converted.
*/

/* ---------- headers */

#include "cache_file_formats.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- constants */

#define PATH_BYTES 1024

/* ---------- structures */

struct stdio_source
{
	FILE *file;
	struct cache_file_source source;
};

/* ---------- private code */

static int stdio_read(
	void *context,
	uint32_t offset,
	uint32_t size,
	void *buffer)
{
	FILE *file = context;

	return fseek(file, (long)offset, SEEK_SET) == 0 && fread(buffer, 1, size, file) == size;
}

/* opens `path` as a source, returning zero when it cannot be read or is
larger than 2 GB, beyond any map and beyond what fseek can address here */
static int stdio_source_open(
	struct stdio_source *source,
	char const *path)
{
	long size;

	source->file = fopen(path, "rb");
	if (!source->file)
	{
		return 0;
	}
	if (fseek(source->file, 0, SEEK_END) != 0 || (size = ftell(source->file)) < 0)
	{
		fclose(source->file);
		source->file = NULL;
		return 0;
	}
	source->source.context = source->file;
	source->source.read = stdio_read;
	source->source.size = (uint32_t)size;

	return 1;
}

static void stdio_source_close(
	struct stdio_source *source)
{
	if (source->file)
	{
		fclose(source->file);
		source->file = NULL;
	}

	return;
}

/* the directory part of `path`, with its separator */
static void directory_of(
	char const *path,
	char *directory)
{
	char const *slash = strrchr(path, '/');
	char const *backslash = strrchr(path, '\\');
	char const *end = slash > backslash ? slash : backslash;
	size_t length = end ? (size_t)(end - path) + 1 : 0;

	memcpy(directory, path, length);
	directory[length] = 0;

	return;
}

static void print_flags(
	char const *key,
	uint32_t flags,
	char const *const *names,
	int name_count)
{
	int bit;
	int printed = 0;

	printf("%s:", key);
	for (bit = 0; bit < name_count; bit++)
	{
		if ((flags >> bit) & 1)
		{
			printf("%s%s", printed ? ", " : " ", names[bit]);
			printed = 1;
		}
	}
	printf("%s\n", printed ? "" : " none");

	return;
}

static void print_identity(
	struct cache_file_identity const *identity)
{
	printf("format: %s\n", cache_file_format_describe(identity->format));
	printf("file_size: 0x%" PRIx32 "\n", identity->file_size);
	if (identity->format == _cache_file_format_resource_map)
	{
		printf("resource_map_type: %s\n", resource_map_type_describe(identity->resource_map_type));
		printf("resource_items: %" PRId32 "\n", identity->resource_item_count);
		return;
	}
	if (identity->format == _cache_file_format_unrecognized)
	{
		return;
	}
	printf("version: %" PRId32 "\n", identity->version);
	printf("build: %s\n", identity->build);
	printf("name: %s\n", identity->name);
	printf("scenario_type: %d\n", identity->scenario_type);
	printf("file_length: 0x%" PRIx32 "\n", identity->file_length);
	printf("tag_data: 0x%" PRIx32 "+0x%" PRIx32 "\n", identity->tag_data_offset, identity->tag_data_size);
	printf("checksum: 0x%08" PRIx32 "\n", identity->checksum);

	return;
}

/* the path of the resource map of `type` a cache needs */
static void resource_map_path(
	char const *maps_directory,
	enum resource_map_type type,
	char *path)
{
	snprintf(path, PATH_BYTES, "%s%s.map", maps_directory, resource_map_type_describe(type));

	return;
}

static int report_custom_edition_cache(
	struct cache_file_source *source,
	char const *maps_directory,
	char const *dump_path)
{
	static char const *const warning_names[NUMBER_OF_CUSTOM_EDITION_WARNINGS] =
	{
		"checksum mismatch",
		"trailing data",
	};
	struct stdio_source resource_sources[NUMBER_OF_RESOURCE_MAP_TYPES];
	struct resource_map resource_map_storage[NUMBER_OF_RESOURCE_MAP_TYPES];
	struct resource_map *resource_maps[NUMBER_OF_RESOURCE_MAP_TYPES];
	struct custom_edition_load_report report;
	uint32_t tag_cache_bytes = CUSTOM_EDITION_TAG_CACHE_BYTES;
	uint8_t *tag_cache;
	enum cache_file_status status;
	int type;

	memset(resource_sources, 0, sizeof(resource_sources));
	memset(resource_maps, 0, sizeof(resource_maps));
	for (type = _resource_map_bitmaps; type < NUMBER_OF_RESOURCE_MAP_TYPES; type++)
	{
		char path[PATH_BYTES];

		resource_map_path(maps_directory, (enum resource_map_type)type, path);
		if (!stdio_source_open(&resource_sources[type], path))
		{
			printf("resource_map.%s: %s (not found)\n", resource_map_type_describe((enum resource_map_type)type), path);
			continue;
		}
		status = resource_map_open(&resource_sources[type].source, (enum resource_map_type)type, &resource_map_storage[type]);
		printf("resource_map.%s: %s (%s)\n",
			resource_map_type_describe((enum resource_map_type)type),
			path,
			cache_file_status_describe(status));
		if (status == _cache_file_status_ok)
		{
			resource_maps[type] = &resource_map_storage[type];
		}
	}

	tag_cache = calloc(tag_cache_bytes, 1);
	if (!tag_cache)
	{
		status = _cache_file_status_out_of_memory;
		printf("load: %s\n", cache_file_status_describe(status));
	}
	else
	{
		status = custom_edition_cache_load(source, resource_maps, tag_cache, tag_cache_bytes, &report);
		printf("load: %s\n", cache_file_status_describe(status));
		if (status != _cache_file_status_ok)
		{
			printf("load_problem_tag: %" PRId32 "\n", report.problem_tag_index);
			printf("load_problem_location: 0x%" PRIx32 "\n", report.problem_location);
		}
		printf("tag_cache_bytes: 0x%" PRIx32 "\n", report.tag_cache_bytes);
		printf("tags: %" PRId32 "\n", report.tag_count);
		printf("scenario_tag: %" PRId32 "\n", report.scenario_tag_index);
		printf("tag_data_bytes: 0x%" PRIx32 "\n", report.tag_data_bytes);
		printf("resource_tag_bytes: 0x%" PRIx32 "\n", report.resource_tag_bytes);
		printf("resource_tags: bitmaps %" PRId32 ", sounds %" PRId32 ", loc %" PRId32 "\n",
			report.resource_tag_counts[_resource_map_bitmaps],
			report.resource_tag_counts[_resource_map_sounds],
			report.resource_tag_counts[_resource_map_locale]);
		printf("structure_bsps: %" PRId32 "\n", report.structure_bsp_count);
		printf("largest_structure_bsp_bytes: 0x%" PRIx32 "\n", report.largest_structure_bsp_bytes);
		printf("lowest_structure_bsp_address: 0x%" PRIx32 "\n", report.lowest_structure_bsp_address);
		printf("structure_bsp_materials_checked: %" PRId32 "\n", report.structure_bsp_materials_checked);
		printf("model_data: 0x%" PRIx32 " bytes at 0x%" PRIx32 ", strips from 0x%" PRIx32 "\n",
			report.model_data_bytes,
			report.model_data_offset,
			report.model_index_data_offset);
		printf("bitmap_data_ranges_checked: %" PRId32 "\n", report.bitmap_data_ranges_checked);
		printf("sound_sample_ranges_checked: %" PRId32 "\n", report.sound_sample_ranges_checked);
		printf("relocated_pointers: %" PRId32 "\n", report.relocated_pointer_count);
		printf("scenario_regrouped: %" PRId32 "\n", report.scenario_regrouped);
		printf("computed_checksum: 0x%08" PRIx32 "\n", report.computed_checksum);
		printf("trailing_bytes: 0x%" PRIx32 "\n", report.trailing_bytes);
		print_flags("warnings", report.warnings, warning_names, NUMBER_OF_CUSTOM_EDITION_WARNINGS);
		if (status == _cache_file_status_ok)
		{
			uint32_t loaded_bytes = report.tag_data_bytes + report.resource_tag_bytes;
			struct custom_edition_conversion_report conversion;
			enum cache_file_status conversion_status = custom_edition_cache_convert(tag_cache, loaded_bytes, report.identity.name, &conversion);
			int behaviour;

			printf("convert: %s\n", cache_file_status_describe(conversion_status));
			if (conversion_status != _cache_file_status_ok)
			{
				printf("convert_problem_tag: %" PRId32 "\n", conversion.problem_tag_index);
			}
			printf("shaders_renumbered: %" PRId32 "\n", conversion.shaders_retyped);
			printf("chicago_extended_shaders_converted: %" PRId32 "\n", conversion.chicago_extended_shaders);
			printf("shaders_mistyped: %" PRId32 "\n", conversion.shaders_mistyped);
			printf("bitmaps_prepared: %" PRId32 "\n", conversion.bitmaps_prepared);
			printf("bitmaps_made_linear: %" PRId32 "\n", conversion.bitmaps_made_linear);
			printf("animation_overlays_disabled: %" PRId32 "\n", conversion.animation_overlays_disabled);
			printf("node_links_cut: %" PRId32 "\n", conversion.node_links_cut);
			printf("sounds_undecodable: %" PRId32 "\n", conversion.sounds_undecodable);
			printf("hud_placements_rescaled: %" PRId32 "\n", conversion.hud_placements_rescaled);
			printf("score_hint_converted: %" PRId32 "\n", conversion.score_hint_converted);
			printf("widget_functions_cleared: %" PRId32 "\n", conversion.widget_functions_cleared);
			printf("pause_menu_trimmed: %" PRId32 "\n", conversion.pause_menu_trimmed);
			printf("halo_pc_behaviours:");
			for (behaviour = 0; behaviour < NUMBER_OF_CUSTOM_EDITION_BEHAVIOURS; behaviour++)
			{
				if ((conversion.behaviours >> behaviour) & 1)
					printf(" %s", custom_edition_behaviour_name((short)behaviour));
			}
			printf("%s\n", conversion.behaviours ? "" : " none");
			if (dump_path)
			{
				FILE *dump = fopen(dump_path, "wb");
				int written = dump && fwrite(tag_cache, 1, loaded_bytes, dump) == loaded_bytes;

				if (dump && fclose(dump))
				{
					written = 0;
				}
				printf("dump: %s\n", written ? dump_path : "failed");
			}
			status = conversion_status;
		}
		free(tag_cache);
	}
	printf("milestone.recognize: yes\n");
	printf("milestone.load: %s\n", status == _cache_file_status_ok ? "yes" : "no");
	printf("milestone.run: not observed by this tool (the game converts the rest as it loads a map; docs/custom_edition_caches.md)\n");

	for (type = _resource_map_bitmaps; type < NUMBER_OF_RESOURCE_MAP_TYPES; type++)
	{
		if (resource_maps[type])
		{
			resource_map_close(resource_maps[type]);
		}
		stdio_source_close(&resource_sources[type]);
	}

	return status == _cache_file_status_ok;
}

static int report_file(
	char const *path,
	char const *maps_directory_option,
	char const *dump_path)
{
	struct stdio_source source;
	struct cache_file_identity identity;
	enum cache_file_status status;
	char maps_directory[PATH_BYTES];
	int succeeded;

	printf("file: %s\n", path);
	if (!stdio_source_open(&source, path))
	{
		printf("identify: %s\n\n", cache_file_status_describe(_cache_file_status_read_failed));
		return 0;
	}
	status = cache_file_identify(&source.source, &identity);
	printf("identify: %s\n", cache_file_status_describe(status));
	print_identity(&identity);
	succeeded = status == _cache_file_status_ok && identity.format != _cache_file_format_unrecognized;
	if (succeeded && identity.format == _cache_file_format_custom_edition_cache)
	{
		if (maps_directory_option)
		{
			size_t length = strlen(maps_directory_option);

			snprintf(maps_directory, sizeof(maps_directory), "%s%s",
				maps_directory_option,
				length && (maps_directory_option[length - 1] == '/' || maps_directory_option[length - 1] == '\\') ? "" : "/");
		}
		else
		{
			directory_of(path, maps_directory);
		}
		succeeded = report_custom_edition_cache(&source.source, maps_directory, dump_path);
	}
	else if (succeeded && identity.format == _cache_file_format_resource_map)
	{
		struct resource_map map;

		status = resource_map_open(&source.source, identity.resource_map_type, &map);
		printf("resource_map: %s\n", cache_file_status_describe(status));
		succeeded = status == _cache_file_status_ok;
		if (succeeded)
		{
			resource_map_close(&map);
		}
	}
	stdio_source_close(&source);
	printf("\n");

	return succeeded;
}

/* ---------- public code */

int main(
	int argument_count,
	char **arguments)
{
	char const *maps_directory = NULL;
	char const *dump_path = NULL;
	int all_succeeded = 1;
	int file_count = 0;
	int argument_index;

	for (argument_index = 1; argument_index < argument_count; argument_index++)
	{
		if (!strcmp(arguments[argument_index], "--maps") && argument_index + 1 < argument_count)
		{
			maps_directory = arguments[++argument_index];
			continue;
		}
		if (!strcmp(arguments[argument_index], "--dump-tags") && argument_index + 1 < argument_count)
		{
			dump_path = arguments[++argument_index];
			continue;
		}
		if (!report_file(arguments[argument_index], maps_directory, dump_path))
		{
			all_succeeded = 0;
		}
		file_count++;
	}
	if (!file_count)
	{
		fprintf(stderr, "usage: cache_file_report [--maps DIRECTORY] [--dump-tags FILE] FILE...\n");
		return 2;
	}

	return all_succeeded ? 0 : 1;
}
