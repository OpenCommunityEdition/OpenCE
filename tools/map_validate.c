/*
MAP_VALIDATE.C

The tag validator (port/linux/game/tag_validate.c) run alone on map files,
as the game runs it when a map loads: each map's tag data is read (and
inflated) to where the game reads it, its tags checked, then each of its
structure bsps where it loads. Prints every correction and refusal, then a
line for each map:
	<map>: <corrections> corrections
	<map>: refused
and exits 1 if any map was refused (or, with --strict, corrected: the retail
maps need no correction, which tools/test_linux_port.py checks).

Built by ninja linux as build/linux/map_validate, with the platform layer's
POSIX flags (it calls the validator's few functions, whose arguments are
pointers and longs alike in both ABIs).

With --fuzz, each map is instead checked iterations times with a few of its
words changed at random (from --seed on): the validator must never crash or
hang, and a map it lets through must be clean when checked again.

With --log, each run that was corrected is written to a file as the map's
name, the seed and the changed words (offset:value in the inflated file).

A Halo Custom Edition map is checked as the game checks one
(port/linux/game/custom_edition_cache.c): its loader reads it, with the
resource maps beside it (or in --maps), into its own tag cache, its bytes
are converted for this build, then the validator checks its tags and each
of its structure bsps. Fuzzing changes words of its file, so that the
loader and the conversion are tried too. (The game also decodes its Ogg
Vorbis sounds and converts its scripts and geometry, from tags the validator
passed: not here.) Resource maps given as maps are passed over; OpenSauce
maps are refused, as the game refuses them.

usage: map_validate [--strict] [--quiet] [--maps folder] [--fuzz iterations [--seed n] [--log file]] map.map...
*/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>

#include "zlib_prefixed.h"
#include "../port/linux/game/cache_file_formats.h"

/* ---------- constants */

enum
{
	/* where the game reads a map's tags, and how many bytes there are room
	for (physical_memory_map.c, physical_memory_map.h) */
	TAG_CACHE_BASE_ADDRESS = 0x803A6000,
	TAG_CACHE_SIZE = 0x1600000,

	CACHE_FILE_HEADER_SIZE = 0x800,
	CACHE_FILE_HEADER_SIGNATURE = 'head',
	CACHE_FILE_FOOTER_SIGNATURE = 'foot',
	SCENARIO_GROUP_TAG = 'scnr',
	/* what the game rounds a read up to (cache_files.c) */
	CACHE_FILE_SECTOR_SIZE = 512,
	/* the scenario's structure bsps block (scenario_definitions.h) */
	SCENARIO_STRUCTURE_BSPS_OFFSET = 0x5A4,
	MAXIMUM_STRUCTURE_BSPS_PER_SCENARIO = 32,
};

/* ---------- structures */

struct cache_file_header
{
	unsigned long header_signature;
	long version;
	long file_length;
	unsigned long reserved0C;
	long tag_data_offset;
	long tag_data_size;
	unsigned char reserved18[8];
	char name[0x20];
	char build[0x20];
	unsigned char reserved60[0x79C];
	unsigned long footer_signature;
};

struct tag_instance
{
	unsigned long group_tag;
	unsigned long parent_group_tags[2];
	long tag_index;
	char *name;
	void *base_address;
	unsigned long unused[2];
};

struct tag_header
{
	struct tag_instance *instances;
	long scenario_tag_index;
	unsigned long checksum;
	long tag_count;
};

struct tag_block
{
	long count;
	void *address;
	void *definition;
};

struct scenario_structure_bsp_reference
{
	long file_offset;
	long file_size;
	void *base_address;
	unsigned char unused[4];
	unsigned long group_tag;
	char *name;
	long name_length;
	long tag_index;
};

typedef char verify_cache_file_header_size[sizeof(struct cache_file_header) == CACHE_FILE_HEADER_SIZE ? 1 : -1];
typedef char verify_scenario_structure_bsp_reference_size[
	sizeof(struct scenario_structure_bsp_reference) == 0x20 ? 1 : -1];

/* ---------- the validator's (port/linux/game/tag_validate.c) */

unsigned char tag_validate_tags(void *tag_header, long tag_data_size, long file_length, char const *map_name);
unsigned char tag_validate_structure_bsp(long tag_index, void *base, long size);
long tag_validate_corrections(void);
unsigned char tag_validate_claimed(void const *address);
struct tag_validate_file_range
{
	unsigned long offset;
	unsigned long size;
};
unsigned char tag_validate_custom_edition_tags(void *tag_header, long loaded_size, unsigned long tag_cache_size,
	struct tag_validate_file_range const *file_ranges, short file_range_count, char const *map_name);

static int quiet;
/* (fuzzing: each bsp is checked twice, and whether a second check of one
found more to correct) */
static int recheck;
static int recheck_failed;
static unsigned long fuzz_seed;

void tag_validate_report(char const *message)
{
	if (!quiet)
		printf("%s\n", message);
}

/* (the game's printf family, which the validator calls by these names:
port/linux/include/stdio.h) */
int halo_linux_vsnprintf(char *buffer, size_t count, char const *format, va_list arguments)
{
	return vsnprintf(buffer, count, format, arguments);
}

int halo_linux_snprintf(char *buffer, size_t count, char const *format, ...)
{
	va_list arguments;
	int result;

	va_start(arguments, format);
	result = vsnprintf(buffer, count, format, arguments);
	va_end(arguments);
	return result;
}

/* (the game's string functions, which its units call by these names:
cseries.h) */
void *csmemset(void *buffer, long c, unsigned long size)
{
	return memset(buffer, (int)c, size);
}

void *csmemcpy(void *destination, void const *source, unsigned long size)
{
	return memcpy(destination, source, size);
}

unsigned long csstrlen(char const *s)
{
	return strlen(s);
}

char *csstrcat(char *s1, char const *s2)
{
	return strcat(s1, s2);
}

long csstrcmp(char const *s1, char const *s2)
{
	return strcmp(s1, s2);
}

/* ---------- code */

/* the map's bytes as the game sees them (its tag data and bsps at their file
offsets): the file, inflated after its header if it is compressed */
static unsigned char *map_read(char const *path, struct cache_file_header *header)
{
	FILE *file = fopen(path, "rb");
	unsigned char *bytes = NULL;
	unsigned char *compressed = NULL;
	long size;

	if (!file)
		return NULL;
	if (fseek(file, 0, SEEK_END) || (size = ftell(file)) < CACHE_FILE_HEADER_SIZE || fseek(file, 0, SEEK_SET) ||
		fread(header, CACHE_FILE_HEADER_SIZE, 1, file) != 1 ||
		header->header_signature != CACHE_FILE_HEADER_SIGNATURE ||
		header->footer_signature != CACHE_FILE_FOOTER_SIGNATURE ||
		header->file_length < CACHE_FILE_HEADER_SIZE)
	{
		fclose(file);
		return NULL;
	}
	bytes = malloc(header->file_length);
	compressed = malloc(size - CACHE_FILE_HEADER_SIZE);
	if (bytes && compressed && fread(compressed, size - CACHE_FILE_HEADER_SIZE, 1, file) == 1)
	{
		memcpy(bytes, header, CACHE_FILE_HEADER_SIZE);
		if (size == header->file_length)
		{
			memcpy(bytes + CACHE_FILE_HEADER_SIZE, compressed, size - CACHE_FILE_HEADER_SIZE);
		}
		else
		{
			uLongf inflated = header->file_length - CACHE_FILE_HEADER_SIZE;

			if (uncompress(bytes + CACHE_FILE_HEADER_SIZE, &inflated, compressed, size - CACHE_FILE_HEADER_SIZE) != Z_OK ||
				inflated != (uLongf)(header->file_length - CACHE_FILE_HEADER_SIZE))
			{
				free(bytes);
				bytes = NULL;
			}
		}
	}
	else
	{
		free(bytes);
		bytes = NULL;
	}
	free(compressed);
	fclose(file);

	return bytes;
}

/* the map's tags (bytes, as map_read gives them) checked where the game
reads them, then each of its structure bsps where it loads: 0 if the map
plays (its corrections counted), 1 if it is refused */
static int map_check(struct cache_file_header const *header, unsigned char const *bytes, unsigned char *tag_cache,
	char const *name, long *corrections)
{
	struct tag_header *tags = (struct tag_header *)tag_cache;
	struct tag_block *bsps;
	long bsp_count;
	long index;
	int refused = 0;

	*corrections = 0;
	memset(tag_cache, 0xCD, TAG_CACHE_SIZE);
	memcpy(tag_cache, bytes + header->tag_data_offset, header->tag_data_size);
	if (!tag_validate_tags(tag_cache, header->tag_data_size, header->file_length, name))
		return 1;
	*corrections = tag_validate_corrections();

	/* each bsp, where it loads (scenario_structure_bsp_load): the reference
	is the validated scenario's, which is a scenario (as
	cache_file_tag_header_verify checks) */
	{
		short scenario_index = (short)tags->scenario_tag_index;
		struct tag_instance *scenario;

		if (scenario_index < 0 || scenario_index >= tags->tag_count ||
			tags->instances[scenario_index].tag_index != tags->scenario_tag_index ||
			tags->instances[scenario_index].group_tag != SCENARIO_GROUP_TAG)
		{
			if (!quiet)
				printf("%s: its tag header names no scenario\n", name);
			return 1;
		}
		scenario = &tags->instances[scenario_index];

		bsps = (struct tag_block *)((unsigned char *)scenario->base_address + SCENARIO_STRUCTURE_BSPS_OFFSET);
		bsp_count = bsps->count < MAXIMUM_STRUCTURE_BSPS_PER_SCENARIO ? bsps->count : MAXIMUM_STRUCTURE_BSPS_PER_SCENARIO;
	}
	for (index = 0; index < bsp_count && !refused; index++)
	{
		struct scenario_structure_bsp_reference *reference =
			(struct scenario_structure_bsp_reference *)bsps->address + index;
		unsigned long load = (unsigned long)reference->base_address - TAG_CACHE_BASE_ADDRESS;
		/* (the game reads whole sectors) */
		unsigned long read_size = ((unsigned long)reference->file_size + CACHE_FILE_SECTOR_SIZE - 1) &
			~(unsigned long)(CACHE_FILE_SECTOR_SIZE - 1);

		/* (what cache_file_structure_bsp_reference_verify checks) */
		if (reference->file_offset < 0 || reference->file_size <= 0 ||
			reference->file_offset > header->file_length - reference->file_size ||
			load < (unsigned long)header->tag_data_size || load > TAG_CACHE_SIZE ||
			read_size > TAG_CACHE_SIZE - load)
		{
			if (!quiet)
				printf("%s: its structure bsp %ld is outside it\n", name, index);
			refused = 1;
			break;
		}
		memset(tag_cache + header->tag_data_size, 0xCD, TAG_CACHE_SIZE - header->tag_data_size);
		memcpy(reference->base_address, bytes + reference->file_offset, reference->file_size);
		if (!tag_validate_structure_bsp(reference->tag_index, reference->base_address, reference->file_size))
		{
			refused = 1;
		}
		/* (fuzzing: the bsp as corrected, checked again, needs no more) */
		else if (recheck)
		{
			long before = tag_validate_corrections();

			if (!tag_validate_structure_bsp(reference->tag_index, reference->base_address, reference->file_size) ||
				tag_validate_corrections() != before)
			{
				printf("%s: fuzz seed %lu: structure bsp %ld as corrected is not clean when checked again\n", name,
					fuzz_seed, index);
				recheck_failed = 1;
			}
		}
	}
	*corrections = tag_validate_corrections();

	return refused;
}

static int map_validate(char const *path, unsigned char *tag_cache, long *corrections)
{
	struct cache_file_header header;
	unsigned char *bytes = map_read(path, &header);
	char const *name = strrchr(path, '/') ? strrchr(path, '/') + 1 : path;
	int refused;

	*corrections = 0;
	if (!bytes)
	{
		printf("%s: cannot be read\n", name);
		return 1;
	}
	if (header.tag_data_offset < 0 || header.tag_data_size <= 0 || header.tag_data_size > TAG_CACHE_SIZE ||
		header.tag_data_offset > header.file_length - header.tag_data_size)
	{
		printf("%s: its tag data is outside it\n", name);
		free(bytes);
		return 1;
	}
	refused = map_check(&header, bytes, tag_cache, name, corrections);
	free(bytes);

	return refused;
}

/* ---------- fuzzing */

/* the run being fuzzed, for the signal handlers' message */
static char const *fuzz_name = "";

static unsigned long fuzz_state;
static FILE *fuzz_log;

static unsigned long fuzz_random(void)
{
	/* (xorshift32) */
	fuzz_state ^= fuzz_state << 13;
	fuzz_state ^= fuzz_state >> 17;
	fuzz_state ^= fuzz_state << 5;
	return fuzz_state;
}

static void fuzz_signal(int signal_number)
{
	char message[160];
	int length = snprintf(message, sizeof(message), "map_validate: %s, fuzz seed %lu: %s\n", fuzz_name, fuzz_seed,
		signal_number == SIGALRM ? "hangs" : "crashes");

	if (write(2, message, length) < 0)
		_exit(3);
	_exit(3);
}

/* a value to put in a field: one of those that break a reader of tags most */
static unsigned long fuzz_value(struct cache_file_header const *header)
{
	switch (fuzz_random() % 9)
	{
	case 0: return fuzz_random();
	case 1: return fuzz_random() % 8;
	case 2: return fuzz_random() % 300;
	case 3: return 0xFFFFFFFFUL;
	case 4: return 0x80000000UL;
	case 5: return 0x7FFFFFFFUL;
	case 6: return 0xFFFF;
	case 7: return TAG_CACHE_BASE_ADDRESS + fuzz_random() % (unsigned long)header->tag_data_size;
	default: return TAG_CACHE_BASE_ADDRESS + fuzz_random() % TAG_CACHE_SIZE;
	}
}

/* iterations runs of the map, each with a few of its tags' and bsps' words
changed: the validator must never crash or hang, and the tags it lets
through, checked again, must need no more corrections */
static int map_fuzz(char const *path, unsigned char *tag_cache, unsigned long seed, long iterations)
{
	struct cache_file_header header;
	unsigned char *bytes = map_read(path, &header);
	char const *name = strrchr(path, '/') ? strrchr(path, '/') + 1 : path;
	long iteration;
	long refusals = 0;
	long corrected = 0;
	int failed = 0;
	unsigned long *claimed = NULL;
	long claimed_count = 0;

	if (!bytes)
	{
		printf("%s: cannot be read\n", name);
		return 1;
	}
	if (header.tag_data_offset < 0 || header.tag_data_size <= 0 || header.tag_data_size > TAG_CACHE_SIZE ||
		header.tag_data_offset > header.file_length - header.tag_data_size)
	{
		printf("%s: its tag data is outside it\n", name);
		free(bytes);
		return 1;
	}
	fuzz_name = name;
	recheck = 1;
	/* (the words of the tags' roots, blocks and data, where nearly every
	field the game trusts is: most changes go there) */
	{
		long corrections;
		long offset;

		map_check(&header, bytes, tag_cache, name, &corrections);
		claimed = malloc(sizeof(*claimed) * (header.tag_data_size / 4 + 1));
		for (offset = 0; claimed && offset + 4 <= header.tag_data_size; offset += 4)
		{
			if (tag_validate_claimed(tag_cache + offset))
				claimed[claimed_count++] = header.tag_data_offset + offset;
		}
	}
	for (iteration = 0; iteration < iterations; iteration++)
	{
		enum { MAXIMUM_CHANGES = 8 };
		unsigned long offsets[MAXIMUM_CHANGES];
		unsigned long values[MAXIMUM_CHANGES];
		long changes = 1 + fuzz_random() % MAXIMUM_CHANGES;
		long corrections;
		long change;

		fuzz_seed = seed + iteration;
		fuzz_state = fuzz_seed * 2654435761UL + 1;
		changes = 1 + fuzz_random() % MAXIMUM_CHANGES;
		for (change = 0; change < changes; change++)
		{
			/* (most in the tags' roots, blocks and data, some anywhere in the
			tags, some anywhere in the file: its bsps) */
			unsigned long choice = fuzz_random() % 8;
			unsigned long offset = choice < 5 && claimed_count ? claimed[fuzz_random() % claimed_count] :
				choice < 7 ? header.tag_data_offset + (fuzz_random() % (unsigned long)header.tag_data_size) :
				fuzz_random() % (unsigned long)header.file_length;

			offset &= ~3UL;
			if (offset + 4 > (unsigned long)header.file_length)
				offset = header.tag_data_offset;
			offsets[change] = offset;
			memcpy(&values[change], bytes + offset, 4);
			{
				unsigned long value = fuzz_value(&header);

				memcpy(bytes + offset, &value, 4);
			}
		}

		alarm(60);
		if (map_check(&header, bytes, tag_cache, name, &corrections))
		{
			refusals++;
		}
		else
		{
			long again;

			if (corrections)
			{
				corrected++;
				/* (with --log, the changes of a run that was corrected, for
				playing it in the game: "offset value" in the file) */
				if (fuzz_log)
				{
					fprintf(fuzz_log, "%s %lu", name, fuzz_seed);
					for (change = 0; change < changes; change++)
					{
						unsigned long value;

						memcpy(&value, bytes + offsets[change], 4);
						fprintf(fuzz_log, " %lx:%lx", offsets[change], value);
					}
					fprintf(fuzz_log, "\n");
				}
			}
			/* (the tags as corrected, checked again: nothing more to correct) */
			if (!tag_validate_tags(tag_cache, header.tag_data_size, header.file_length, name) ||
				(again = tag_validate_corrections()) != 0)
			{
				printf("%s: fuzz seed %lu: the corrected tags are not clean when checked again:\n", name, fuzz_seed);
				failed = 1;
				/* (what was corrected, then what is left) */
				quiet = 0;
				map_check(&header, bytes, tag_cache, name, &corrections);
				printf("checked again:\n");
				tag_validate_tags(tag_cache, header.tag_data_size, header.file_length, name);
				quiet = 1;
			}
		}
		alarm(0);

		/* (undone, last first) */
		for (change = changes - 1; change >= 0; change--)
			memcpy(bytes + offsets[change], &values[change], 4);
	}
	recheck = 0;
	if (recheck_failed)
		failed = 1;
	printf("%s: %ld runs: %ld refused, %ld corrected, %ld unchanged\n", name, iterations, refusals, corrected,
		iterations - refusals - corrected);
	free(claimed);
	free(bytes);

	return failed;
}

/* ---------- Custom Edition maps */

enum
{
	/* where a Custom Edition map's bitmaps.map and sounds.map count in the
	offsets of its tags, after the map itself (as the game has them,
	custom_edition_cache.c) */
	CUSTOM_EDITION_BITMAPS_OFFSET = 0x40000000,
	CUSTOM_EDITION_SOUNDS_OFFSET = 0x60000000,
};

/* a file read whole */
struct memory_file
{
	unsigned char *bytes;
	unsigned long size;
	struct cache_file_source source;
};

/* the folder resource maps are looked in, when not beside the map (--maps) */
static char const *resource_maps_folder;

static int memory_file_read(void *context, uint32_t offset, uint32_t size, void *buffer)
{
	struct memory_file const *file = context;

	if (offset > file->size || size > file->size - offset)
		return 0;
	memcpy(buffer, file->bytes + offset, size);
	return 1;
}

static int memory_file_open(struct memory_file *file, char const *path)
{
	FILE *stream = fopen(path, "rb");
	long size;

	memset(file, 0, sizeof(*file));
	if (!stream)
		return 0;
	if (fseek(stream, 0, SEEK_END) || (size = ftell(stream)) < 0 || fseek(stream, 0, SEEK_SET) ||
		!(file->bytes = malloc((size_t)size + 1)) || fread(file->bytes, 1, (size_t)size, stream) != (size_t)size)
	{
		free(file->bytes);
		file->bytes = NULL;
		fclose(stream);
		return 0;
	}
	fclose(stream);
	file->size = (unsigned long)size;
	file->source.context = file;
	file->source.read = memory_file_read;
	file->source.size = (uint32_t)size;
	return 1;
}

static void memory_file_close(struct memory_file *file)
{
	free(file->bytes);
	memset(file, 0, sizeof(*file));
}

/* a map's resource maps (bitmaps.map, sounds.map, loc.map), beside it or in
--maps; those missing are left closed, and the map refused if it needs them */
struct custom_edition_resources
{
	struct memory_file files[NUMBER_OF_RESOURCE_MAP_TYPES];
	struct resource_map storage[NUMBER_OF_RESOURCE_MAP_TYPES];
	struct resource_map *maps[NUMBER_OF_RESOURCE_MAP_TYPES];
};

static void custom_edition_resources_open(struct custom_edition_resources *resources, char const *map_path)
{
	short type;

	memset(resources, 0, sizeof(*resources));
	for (type = _resource_map_bitmaps; type < NUMBER_OF_RESOURCE_MAP_TYPES; type++)
	{
		char path[1024];
		char const *slash = strrchr(map_path, '/');
		int folder_length = resource_maps_folder ? (int)strlen(resource_maps_folder) : slash ? (int)(slash - map_path) : 1;
		char const *folder = resource_maps_folder ? resource_maps_folder : slash ? map_path : ".";

		snprintf(path, sizeof(path), "%.*s/%s.map", folder_length, folder,
			resource_map_type_describe((enum resource_map_type)type));
		if (memory_file_open(&resources->files[type], path) &&
			resource_map_open(&resources->files[type].source, (enum resource_map_type)type, &resources->storage[type]) ==
				_cache_file_status_ok)
		{
			resources->maps[type] = &resources->storage[type];
		}
	}
}

static void custom_edition_resources_close(struct custom_edition_resources *resources)
{
	short type;

	for (type = _resource_map_bitmaps; type < NUMBER_OF_RESOURCE_MAP_TYPES; type++)
	{
		if (resources->maps[type])
			resource_map_close(resources->maps[type]);
		memory_file_close(&resources->files[type]);
	}
}

/* a Custom Edition map (map, with its resources) loaded, converted and
checked where the game does it, then each of its structure bsps: 0 if it
plays (its corrections counted), 1 if it is refused. *loaded_bytes is how
many bytes of the tag cache its tags take. */
static int custom_edition_check(struct memory_file *map, struct custom_edition_resources *resources,
	unsigned char *tag_cache, char const *name, long *corrections, unsigned long *loaded_bytes)
{
	struct custom_edition_load_report report;
	struct custom_edition_conversion_report conversion;
	struct tag_validate_file_range ranges[3];
	short range_count = 0;
	enum cache_file_status status;
	struct tag_header *tags = (struct tag_header *)tag_cache;
	struct tag_block *bsps;
	long bsp_count;
	long index;
	int refused = 0;

	*corrections = 0;
	*loaded_bytes = 0;
	memset(tag_cache, 0xCD, CUSTOM_EDITION_TAG_CACHE_BYTES);
	status = custom_edition_cache_load(&map->source, resources->maps, tag_cache, CUSTOM_EDITION_TAG_CACHE_BYTES,
		&report);
	if (status != _cache_file_status_ok)
	{
		if (!quiet)
		{
			printf("%s: its loader refuses it: %s (tag %ld, %08lx)\n", name, cache_file_status_describe(status),
				(long)report.problem_tag_index, (unsigned long)report.problem_location);
		}
		return 1;
	}
	*loaded_bytes = report.tag_data_bytes + report.resource_tag_bytes;
	custom_edition_cache_combine_resource_offsets(tag_cache, *loaded_bytes, CUSTOM_EDITION_BITMAPS_OFFSET,
		CUSTOM_EDITION_SOUNDS_OFFSET);
	status = custom_edition_cache_convert(tag_cache, *loaded_bytes, report.identity.name, &conversion);
	if (status != _cache_file_status_ok)
	{
		if (!quiet)
			printf("%s: its conversion refuses it: %s\n", name, cache_file_status_describe(status));
		return 1;
	}
	ranges[range_count].offset = 0;
	ranges[range_count++].size = report.identity.file_length;
	if (resources->maps[_resource_map_bitmaps])
	{
		ranges[range_count].offset = CUSTOM_EDITION_BITMAPS_OFFSET;
		ranges[range_count++].size = resources->files[_resource_map_bitmaps].size;
	}
	if (resources->maps[_resource_map_sounds])
	{
		ranges[range_count].offset = CUSTOM_EDITION_SOUNDS_OFFSET;
		ranges[range_count++].size = resources->files[_resource_map_sounds].size;
	}
	if (!tag_validate_custom_edition_tags(tag_cache, (long)*loaded_bytes, report.tag_cache_bytes, ranges, range_count,
		name))
	{
		return 1;
	}
	*corrections = tag_validate_corrections();

	/* each bsp, where it loads: the scenario's references, as its loader
	found them (custom_edition_structure_bsp_reference_valid) */
	{
		short scenario_index = (short)tags->scenario_tag_index;

		if (scenario_index < 0 || scenario_index >= tags->tag_count ||
			tags->instances[scenario_index].group_tag != SCENARIO_GROUP_TAG)
		{
			if (!quiet)
				printf("%s: its tag header names no scenario\n", name);
			return 1;
		}
		bsps = (struct tag_block *)((unsigned char *)tags->instances[scenario_index].base_address +
			SCENARIO_STRUCTURE_BSPS_OFFSET);
		bsp_count = bsps->count < MAXIMUM_STRUCTURE_BSPS_PER_SCENARIO ? bsps->count : MAXIMUM_STRUCTURE_BSPS_PER_SCENARIO;
	}
	for (index = 0; index < bsp_count && !refused; index++)
	{
		struct scenario_structure_bsp_reference *reference =
			(struct scenario_structure_bsp_reference *)bsps->address + index;
		unsigned long address = (unsigned long)reference->base_address;
		unsigned long top = CUSTOM_EDITION_TAG_CACHE_ADDRESS + report.tag_cache_bytes;

		if (reference->file_offset < CACHE_FILE_HEADER_SIZE || reference->file_size < 0x18 ||
			(unsigned long)reference->file_offset > report.identity.file_length ||
			(unsigned long)reference->file_size > report.identity.file_length - (unsigned long)reference->file_offset ||
			address < CUSTOM_EDITION_TAG_CACHE_ADDRESS + *loaded_bytes || address > top ||
			(unsigned long)reference->file_size > top - address)
		{
			if (!quiet)
				printf("%s: its structure bsp %ld is outside it\n", name, index);
			refused = 1;
			break;
		}
		memset(tag_cache + *loaded_bytes, 0xCD, report.tag_cache_bytes - *loaded_bytes);
		memcpy(reference->base_address, map->bytes + reference->file_offset, reference->file_size);
		if (!tag_validate_structure_bsp(reference->tag_index, reference->base_address, reference->file_size))
		{
			refused = 1;
		}
		else if (recheck)
		{
			long before = tag_validate_corrections();

			if (!tag_validate_structure_bsp(reference->tag_index, reference->base_address, reference->file_size) ||
				tag_validate_corrections() != before)
			{
				printf("%s: fuzz seed %lu: structure bsp %ld as corrected is not clean when checked again\n", name,
					fuzz_seed, index);
				recheck_failed = 1;
			}
		}
	}
	*corrections = tag_validate_corrections();

	return refused;
}

/* the Custom Edition map at path checked (custom_edition_check), or, with
iterations, fuzzed as map_fuzz fuzzes an Xbox map, its file's words changed */
static int custom_edition_validate(char const *path, unsigned char *tag_cache, long *corrections, unsigned long seed,
	long iterations)
{
	char const *name = strrchr(path, '/') ? strrchr(path, '/') + 1 : path;
	struct memory_file map;
	struct custom_edition_resources resources;
	struct cache_file_identity identity;
	enum cache_file_status status;
	unsigned long loaded_bytes;
	unsigned long *claimed = NULL;
	long claimed_count = 0;
	long refusals = 0;
	long corrected = 0;
	long iteration;
	int failed = 0;

	*corrections = 0;
	if (!memory_file_open(&map, path))
	{
		printf("%s: cannot be read\n", name);
		return 1;
	}
	status = cache_file_identify(&map.source, &identity);
	if (status != _cache_file_status_ok)
	{
		printf("%s: its loader refuses it: %s\n", name, cache_file_status_describe(status));
		memory_file_close(&map);
		return 1;
	}
	custom_edition_resources_open(&resources, path);
	if (!iterations)
	{
		failed = custom_edition_check(&map, &resources, tag_cache, name, corrections, &loaded_bytes);
		custom_edition_resources_close(&resources);
		memory_file_close(&map);
		return failed;
	}

	fuzz_name = name;
	recheck = 1;
	/* (the words of its tag data that the tags' roots, blocks and data
	hold, where most changes go) */
	if (!custom_edition_check(&map, &resources, tag_cache, name, corrections, &loaded_bytes))
	{
		unsigned long offset;

		claimed = malloc(sizeof(*claimed) * (identity.tag_data_size / 4 + 1));
		for (offset = 0; claimed && offset + 4 <= identity.tag_data_size; offset += 4)
		{
			if (tag_validate_claimed(tag_cache + offset))
				claimed[claimed_count++] = identity.tag_data_offset + offset;
		}
	}
	for (iteration = 0; iteration < iterations; iteration++)
	{
		enum { MAXIMUM_CHANGES = 8 };
		unsigned long offsets[MAXIMUM_CHANGES];
		unsigned long values[MAXIMUM_CHANGES];
		long changes;
		long change;

		fuzz_seed = seed + iteration;
		fuzz_state = fuzz_seed * 2654435761UL + 1;
		changes = 1 + fuzz_random() % MAXIMUM_CHANGES;
		for (change = 0; change < changes; change++)
		{
			/* (most in the tags' roots, blocks and data, some anywhere in the
			tag data, some anywhere in the file: its bsps, its model data,
			its header) */
			unsigned long choice = fuzz_random() % 8;
			unsigned long offset = choice < 5 && claimed_count ? claimed[fuzz_random() % claimed_count] :
				choice < 7 ? identity.tag_data_offset + fuzz_random() % identity.tag_data_size :
				fuzz_random() % map.size;
			unsigned long value;

			offset &= ~3UL;
			if (offset + 4 > map.size)
				offset = identity.tag_data_offset;
			offsets[change] = offset;
			memcpy(&values[change], map.bytes + offset, 4);
			switch (fuzz_random() % 9)
			{
			case 0: value = fuzz_random(); break;
			case 1: value = fuzz_random() % 8; break;
			case 2: value = fuzz_random() % 300; break;
			case 3: value = 0xFFFFFFFFUL; break;
			case 4: value = 0x80000000UL; break;
			case 5: value = 0x7FFFFFFFUL; break;
			case 6: value = 0xFFFF; break;
			case 7: value = CUSTOM_EDITION_TAG_CACHE_ADDRESS + fuzz_random() % identity.tag_data_size; break;
			default: value = CUSTOM_EDITION_TAG_CACHE_ADDRESS + fuzz_random() % CUSTOM_EDITION_TAG_CACHE_BYTES; break;
			}
			memcpy(map.bytes + offset, &value, 4);
		}

		alarm(120);
		if (custom_edition_check(&map, &resources, tag_cache, name, corrections, &loaded_bytes))
		{
			refusals++;
		}
		else
		{
			if (*corrections)
			{
				corrected++;
				if (fuzz_log)
				{
					fprintf(fuzz_log, "%s %lu", name, fuzz_seed);
					for (change = 0; change < changes; change++)
					{
						unsigned long value;

						memcpy(&value, map.bytes + offsets[change], 4);
						fprintf(fuzz_log, " %lx:%lx", offsets[change], value);
					}
					fprintf(fuzz_log, "\n");
				}
			}
			/* (the tags as corrected, checked again: nothing more to correct;
			the files' ranges do not matter to the tags' own) */
			{
				struct tag_validate_file_range everything = { 0, 0xFFFFFFFFUL };

				if (!tag_validate_custom_edition_tags(tag_cache, (long)loaded_bytes, CUSTOM_EDITION_TAG_CACHE_BYTES,
						&everything, 1, name) ||
					tag_validate_corrections() != 0)
				{
					printf("%s: fuzz seed %lu: the corrected tags are not clean when checked again:\n", name, fuzz_seed);
					failed = 1;
					/* (what was corrected, then what is left) */
					quiet = 0;
					custom_edition_check(&map, &resources, tag_cache, name, corrections, &loaded_bytes);
					printf("checked again:\n");
					tag_validate_custom_edition_tags(tag_cache, (long)loaded_bytes, CUSTOM_EDITION_TAG_CACHE_BYTES,
						&everything, 1, name);
					quiet = 1;
				}
			}
		}
		alarm(0);
		for (change = changes - 1; change >= 0; change--)
			memcpy(map.bytes + offsets[change], &values[change], 4);
	}
	recheck = 0;
	if (recheck_failed)
		failed = 1;
	printf("%s: %ld runs: %ld refused, %ld corrected, %ld unchanged\n", name, iterations, refusals, corrected,
		iterations - refusals - corrected);
	free(claimed);
	custom_edition_resources_close(&resources);
	memory_file_close(&map);

	return failed;
}

/* the format of the file at path, from its first bytes */
static enum cache_file_format file_format(char const *path)
{
	unsigned char header[CACHE_FILE_HEADER_SIZE];
	FILE *file = fopen(path, "rb");
	size_t read;

	if (!file)
		return _cache_file_format_unrecognized;
	read = fread(header, 1, sizeof(header), file);
	fclose(file);
	if (read < 16)
		return _cache_file_format_unrecognized;
	if (read == sizeof(header) && cache_file_header_format(header) != _cache_file_format_unrecognized)
		return cache_file_header_format(header);
	{
		unsigned long type = header[0] | header[1] << 8 | (unsigned long)header[2] << 16 | (unsigned long)header[3] << 24;

		return type > _resource_map_none && type < NUMBER_OF_RESOURCE_MAP_TYPES ?
			_cache_file_format_resource_map : _cache_file_format_unrecognized;
	}
}

int main(int argc, char **argv)
{
	unsigned char *tag_cache;
	unsigned char *custom_edition_tag_cache;
	int strict = 0;
	int failed = 0;
	long fuzz_iterations = 0;
	unsigned long seed = 1;
	int argument;

	tag_cache = mmap((void *)TAG_CACHE_BASE_ADDRESS, TAG_CACHE_SIZE, PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
	if (tag_cache != (unsigned char *)TAG_CACHE_BASE_ADDRESS)
	{
		fprintf(stderr, "map_validate: the tag cache's address (%08x) is taken\n", TAG_CACHE_BASE_ADDRESS);
		return 2;
	}
	custom_edition_tag_cache = mmap((void *)CUSTOM_EDITION_TAG_CACHE_ADDRESS, CUSTOM_EDITION_TAG_CACHE_BYTES,
		PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_FIXED_NOREPLACE, -1, 0);
	if (custom_edition_tag_cache != (unsigned char *)CUSTOM_EDITION_TAG_CACHE_ADDRESS)
	{
		fprintf(stderr, "map_validate: the Custom Edition tag cache's address (%08lx) is taken\n",
			(unsigned long)CUSTOM_EDITION_TAG_CACHE_ADDRESS);
		return 2;
	}
	for (argument = 1; argument < argc; argument++)
	{
		long corrections;

		if (!strcmp(argv[argument], "--strict"))
		{
			strict = 1;
			continue;
		}
		if (!strcmp(argv[argument], "--quiet"))
		{
			quiet = 1;
			continue;
		}
		if (!strcmp(argv[argument], "--fuzz") && argument + 1 < argc)
		{
			fuzz_iterations = strtol(argv[++argument], NULL, 10);
			quiet = 1;
			signal(SIGSEGV, fuzz_signal);
			signal(SIGBUS, fuzz_signal);
			signal(SIGFPE, fuzz_signal);
			signal(SIGALRM, fuzz_signal);
			continue;
		}
		if (!strcmp(argv[argument], "--log") && argument + 1 < argc)
		{
			fuzz_log = fopen(argv[++argument], "w");
			continue;
		}
		if (!strcmp(argv[argument], "--seed") && argument + 1 < argc)
		{
			seed = strtoul(argv[++argument], NULL, 10);
			continue;
		}
		if (!strcmp(argv[argument], "--maps") && argument + 1 < argc)
		{
			resource_maps_folder = argv[++argument];
			continue;
		}
		switch (file_format(argv[argument]))
		{
		case _cache_file_format_resource_map:
			if (!quiet)
				printf("%s: a resource map, passed over\n", argv[argument]);
			continue;
		case _cache_file_format_custom_edition_cache:
			if (custom_edition_validate(argv[argument], custom_edition_tag_cache, &corrections, seed, fuzz_iterations))
			{
				if (!fuzz_iterations)
					printf("%s: refused\n", argv[argument]);
				failed = 1;
			}
			else if (!fuzz_iterations)
			{
				printf("%s: %ld corrections\n", argv[argument], corrections);
				if (strict && corrections)
					failed = 1;
			}
			fflush(stdout);
			continue;
		default:
			break;
		}
		if (fuzz_iterations)
		{
			if (map_fuzz(argv[argument], tag_cache, seed, fuzz_iterations))
				failed = 1;
			fflush(stdout);
			continue;
		}
		if (map_validate(argv[argument], tag_cache, &corrections))
		{
			printf("%s: refused\n", argv[argument]);
			failed = 1;
		}
		else
		{
			printf("%s: %ld corrections\n", argv[argument], corrections);
			if (strict && corrections)
				failed = 1;
		}
		fflush(stdout);
	}

	return failed;
}
