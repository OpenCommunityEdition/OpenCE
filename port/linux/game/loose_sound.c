/* Desktop HEK .sound overrides for sound tags already present in a cache.
   Invader serializes the root, pitch ranges, permutations and raw payloads
   consecutively. Bounds are checked before any in-memory tag is installed. */

#include "cache/loose_sound.h"

#if defined(HALO_NATIVE_AUDIO) && !defined(HALO_ANDROID)

#include "cache/cache_files.h"
#include "cache/sound_cache.h"
#include "main/console.h"
#include "../src/port_config.h"
#include "sound/sound_definitions.h"
#include "sound/sound_manager.h"
#include "tag_files/tag_files.h"

#include <math.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOOSE_SOUND_TAG_SLOTS 0x10000u
#define LOOSE_SOUND_MAX_FILE (32u * 1024u * 1024u)
#define LOOSE_SOUND_MAX_RETAINED (192u * 1024u * 1024u)
#define LOOSE_SOUND_HEADER 0x40u
#define LOOSE_SOUND_ROOT 0xA4u
#define LOOSE_SOUND_RANGE 0x48u
#define LOOSE_SOUND_PERMUTATION 0x7Cu

struct loose_sound_version
{
	struct loose_sound_version *next;
	struct sound_definition definition;
	struct sound_definition *original;
	struct sound_pitch_range *ranges;
	struct sound_permutation *permutations[MAXIMUM_PITCH_RANGES_PER_SOUND];
	byte *file_data;
	size_t retained_size;
	long tag_index;
};

/* Old versions remain valid until map unload. An in-flight DirectSound
   packet or sound-cache entry can still refer to their permutations. */
static struct loose_sound_version *loose_versions;
static struct loose_sound_version *loose_active[LOOSE_SOUND_TAG_SLOTS];
static size_t loose_retained_bytes;
static boolean loose_ready;

static uint16_t loose_be16(byte const *p)
{
	return (uint16_t)((uint16_t)p[0] << 8 | p[1]);
}

static uint32_t loose_be32(byte const *p)
{
	return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 |
		(uint32_t)p[2] << 8 | p[3];
}

static float loose_float(byte const *p)
{
	uint32_t bits = loose_be32(p);
	float value;
	memcpy(&value, &bits, sizeof(value));
	return value;
}

static boolean loose_fits(size_t position, size_t size, size_t length)
{
	return position <= length && size <= length - position;
}

static void loose_free_version(struct loose_sound_version *version)
{
	int i;

	if (!version)
		return;
	for (i = 0; i < MAXIMUM_PITCH_RANGES_PER_SOUND; i++)
		if (version->permutations[i])
			free(version->permutations[i]);
	if (version->ranges)
		free(version->ranges);
	if (version->file_data)
		free(version->file_data);
	free(version);
}

void loose_sound_close(void)
{
	struct loose_sound_version *version = loose_versions;

	memset(loose_active, 0, sizeof(loose_active));
	loose_ready = FALSE;
	while (version)
	{
		struct loose_sound_version *next = version->next;
		loose_free_version(version);
		version = next;
	}
	loose_versions = NULL;
	loose_retained_bytes = 0;
}

void loose_sound_open(void)
{
	/* scenario_tags_unload releases the old versions after the cache closes. */
	memset(loose_active, 0, sizeof(loose_active));
	loose_ready = TRUE;
}

void *loose_sound_tag_get(long tag_index, void *original)
{
	unsigned int slot = (unsigned int)tag_index & 0xFFFFu;
	struct loose_sound_version *version = slot < LOOSE_SOUND_TAG_SLOTS ?
		loose_active[slot] : NULL;

	return version && version->tag_index == tag_index ? &version->definition : original;
}

byte const *loose_sound_samples(struct sound_permutation const *permutation)
{
	struct loose_sound_version *version;
	int range;

	if (permutation->samples.file_offset != NONE || !permutation->samples.address)
		return NULL;
	for (version = loose_versions; version; version = version->next)
		for (range = 0; range < version->definition.pitch_ranges.count; range++)
		{
			struct sound_pitch_range const *pitch = &version->ranges[range];
			long i;

			for (i = 0; i < pitch->permutations.count; i++)
				if (&version->permutations[range][i] == permutation)
					return permutation->samples.address;
		}
	return NULL;
}

static boolean loose_path(char *output, size_t capacity, char const *name)
{
	size_t position = 0;
	char const *p;

	if (!name || !*name || strlen(name) + sizeof("tags/.sound") >= capacity)
		return FALSE;
	memcpy(output, "tags/", 5);
	position = 5;
	for (p = name; *p; p++)
	{
		/* A cache tag is a relative tag path, never a filesystem escape. */
		if (*p == ':' || (*p == '/' && p[1] == '/') ||
			(*p == '.' && (p == name || p[-1] == '/' || p[-1] == '\\') && p[1] == '.'))
			return FALSE;
		output[position++] = *p == '\\' ? '/' : *p;
	}
	if (position == 5 || output[position - 1] == '/')
		return FALSE;
	memcpy(output + position, ".sound", sizeof(".sound"));
	return TRUE;
}

/* Return 1 for a new override, 2 for unchanged, 0 for a missing file,
   and -1 for invalid. An invalid file leaves the old override active. */
static int loose_reload_one(long tag_index, char const *name)
{
	unsigned int slot = (unsigned int)tag_index & 0xFFFFu;
	struct loose_sound_version *version = NULL;
	struct loose_sound_version *old;
	struct sound_definition *original;
	char path[512];
	byte const *root;
	FILE *file;
	long length;
	size_t cursor, size, range_count;
	uint32_t path_length;
	char const *reason = "invalid layout or memory limit";
	int i;

	if (slot >= LOOSE_SOUND_TAG_SLOTS || !loose_path(path, sizeof(path), name))
		return -1;
	old = loose_active[slot];
	original = old ? old->original : sound_definition_get(tag_index);
	file = fopen(path, "rb");
	if (!file)
	{
		if (errno == ENOENT)
		{
			loose_active[slot] = NULL;
			return 0;
		}
		console_warning("Could not open loose sound: %s", path);
		return -1;
	}
	if (fseek(file, 0, SEEK_END) != 0 || (length = ftell(file)) < 0 ||
		length > LOOSE_SOUND_MAX_FILE || length < LOOSE_SOUND_HEADER + LOOSE_SOUND_ROOT ||
		fseek(file, 0, SEEK_SET) != 0)
		goto invalid;
	size = (size_t)length;
	/* cseries replaces free() with debug_free(); matching allocations must
	   use the game allocator too (CRT calloc() has no debug header). */
	version = debug_malloc(sizeof(*version), TRUE, __FILE__, __LINE__);
	if (!version)
		goto invalid;
	version->file_data = malloc(size);
	if (!version->file_data || fread(version->file_data, 1, size, file) != size)
		goto invalid;
	fclose(file);
	file = NULL;
	if (old && old->retained_size == size &&
		memcmp(old->file_data, version->file_data, size) == 0)
	{
		loose_free_version(version);
		return 2;
	}
	if (loose_retained_bytes > LOOSE_SOUND_MAX_RETAINED - size)
		goto invalid;

	if (memcmp(version->file_data + 0x24, "snd!", 4) ||
		memcmp(version->file_data + 0x3C, "blam", 4) ||
		loose_be32(version->file_data + 0x2C) != LOOSE_SOUND_HEADER ||
		loose_be16(version->file_data + 0x38) != SOUND_DEFINITION_VERSION)
		goto invalid;
	root = version->file_data + LOOSE_SOUND_HEADER;
	range_count = loose_be32(root + offsetof(struct sound_definition, pitch_ranges));
	if (range_count != (size_t)original->pitch_ranges.count)
	{
		reason = "pitch range count differs from the cached sound";
		goto invalid;
	}
	if (loose_be16(root + offsetof(struct sound_definition, compression)) > 1)
	{
		reason = "only 16-bit PCM and Xbox ADPCM have playback channels";
		goto invalid;
	}
	if (!range_count || range_count > MAXIMUM_PITCH_RANGES_PER_SOUND ||
		!original->pitch_ranges.address ||
		loose_be16(root + offsetof(struct sound_definition, sound_class)) >= 51 ||
		loose_be16(root + offsetof(struct sound_definition, sample_rate)) > 1 ||
		loose_be16(root + offsetof(struct sound_definition, encoding)) > 1)
		goto invalid;

	version->definition = *original;
	version->original = original;
	version->tag_index = tag_index;
	version->definition.longest_permutation_length = 0;
	version->definition.flags = loose_be32(root + offsetof(struct sound_definition, flags));
	version->definition.sound_class = loose_be16(root + offsetof(struct sound_definition, sound_class));
	version->definition.sample_rate = loose_be16(root + offsetof(struct sound_definition, sample_rate));
	version->definition.encoding = loose_be16(root + offsetof(struct sound_definition, encoding));
	version->definition.compression = loose_be16(root + offsetof(struct sound_definition, compression));
	/* Existing voices retain their original channel format until they stop.
	 * A format change during a live reload must use sound_restart instead. */
	if (sound_definition_has_live_instances(tag_index) &&
		(version->definition.compression != (old ? old->definition.compression : original->compression) ||
		 version->definition.encoding != (old ? old->definition.encoding : original->encoding) ||
		 version->definition.sample_rate != (old ? old->definition.sample_rate : original->sample_rate)))
	{
		reason = "format changed while sound is playing; use sound_restart";
		goto invalid;
	}
#define READ_SOUND_FLOAT(field) \
	version->definition.field = loose_float(root + offsetof(struct sound_definition, field))
	READ_SOUND_FLOAT(minimum_distance);
	READ_SOUND_FLOAT(maximum_distance);
	READ_SOUND_FLOAT(skip_fraction);
	READ_SOUND_FLOAT(inner_cone_angle);
	READ_SOUND_FLOAT(outer_cone_angle);
	READ_SOUND_FLOAT(outer_cone_gain);
	READ_SOUND_FLOAT(gain_modifier);
	READ_SOUND_FLOAT(zero_skip_fraction_modifier);
	READ_SOUND_FLOAT(zero_gain_modifier);
	READ_SOUND_FLOAT(zero_pitch_modifier);
	READ_SOUND_FLOAT(one_skip_fraction_modifier);
	READ_SOUND_FLOAT(one_gain_modifier);
	READ_SOUND_FLOAT(one_pitch_modifier);
#undef READ_SOUND_FLOAT
	version->definition.random_pitch_bounds.lower =
		loose_float(root + offsetof(struct sound_definition, random_pitch_bounds));
	version->definition.random_pitch_bounds.upper =
		loose_float(root + offsetof(struct sound_definition, random_pitch_bounds) + 4);
	{
		float bend = loose_float(root + offsetof(struct sound_definition, maximum_bend_per_second));
		if (!__builtin_isfinite(bend) || bend < 0.0f)
			goto invalid;
		version->definition.maximum_bend_per_second = bend ? powf(bend, 1.0f / 30.0f) : 0.0f;
		if (!__builtin_isfinite(version->definition.maximum_bend_per_second))
			goto invalid;
	}
	if (!__builtin_isfinite(version->definition.minimum_distance) ||
		!__builtin_isfinite(version->definition.maximum_distance) ||
		version->definition.minimum_distance < 0.0f ||
		(version->definition.maximum_distance != 0.0f &&
		version->definition.maximum_distance < version->definition.minimum_distance) ||
		!__builtin_isfinite(version->definition.skip_fraction) ||
		version->definition.skip_fraction < 0.0f ||
		version->definition.skip_fraction > 1.0f ||
		!__builtin_isfinite(version->definition.inner_cone_angle) ||
		!__builtin_isfinite(version->definition.outer_cone_angle) ||
		!__builtin_isfinite(version->definition.outer_cone_gain) ||
		!__builtin_isfinite(version->definition.gain_modifier) ||
		!__builtin_isfinite(version->definition.zero_skip_fraction_modifier) ||
		!__builtin_isfinite(version->definition.one_skip_fraction_modifier) ||
		!__builtin_isfinite(version->definition.zero_gain_modifier) ||
		!__builtin_isfinite(version->definition.one_gain_modifier) ||
		!__builtin_isfinite(version->definition.zero_pitch_modifier) ||
		!__builtin_isfinite(version->definition.one_pitch_modifier) ||
		!__builtin_isfinite(version->definition.random_pitch_bounds.lower) ||
		!__builtin_isfinite(version->definition.random_pitch_bounds.upper))
		goto invalid;
	if (version->definition.random_pitch_bounds.lower == 0.0f)
		version->definition.random_pitch_bounds.lower = 1.0f;
	if (version->definition.random_pitch_bounds.upper == 0.0f)
		version->definition.random_pitch_bounds.upper = 1.0f;
	if (version->definition.random_pitch_bounds.lower < 0.0f ||
		version->definition.random_pitch_bounds.upper < 0.0f ||
		version->definition.zero_pitch_modifier < 0.0f ||
		version->definition.one_pitch_modifier < 0.0f)
		goto invalid;
	if (version->definition.zero_pitch_modifier == 0.0f &&
		version->definition.one_pitch_modifier == 0.0f)
		version->definition.zero_pitch_modifier =
			version->definition.one_pitch_modifier = 1.0f;
	if (version->definition.zero_skip_fraction_modifier == 0.0f &&
		version->definition.one_skip_fraction_modifier == 0.0f)
		version->definition.zero_skip_fraction_modifier =
			version->definition.one_skip_fraction_modifier = 1.0f;
	if (version->definition.zero_gain_modifier == 0.0f &&
		version->definition.one_gain_modifier == 0.0f)
	{
		/* Match the class-dependent defaults applied by Invader's map build. */
		switch (version->definition.sound_class)
		{
		case 13: case 14: case 15: case 19: case 32:
		case 33: case 34: case 35: case 44: case 45: case 46: case 47:
			version->definition.zero_gain_modifier = 0.0f;
			break;
		default:
			version->definition.zero_gain_modifier = 1.0f;
			break;
		}
		version->definition.one_gain_modifier = 1.0f;
	}
	version->definition.promotion_count =
		loose_be16(root + offsetof(struct sound_definition, promotion_count));
	path_length = loose_be32(root + offsetof(struct sound_definition, promotion_sound) + 8);
	cursor = LOOSE_SOUND_HEADER + LOOSE_SOUND_ROOT;
	if (path_length)
	{
		char promotion[256];
		long promotion_index;

		if (path_length >= sizeof(promotion) || !loose_fits(cursor, path_length + 1, size) ||
			version->file_data[cursor + path_length] != 0 ||
			memchr(version->file_data + cursor, 0, path_length) != NULL)
			goto invalid;
		memcpy(promotion, version->file_data + cursor, path_length);
		promotion[path_length] = 0;
		promotion_index = tag_loaded(SOUND_DEFINITION_TAG, promotion);
		if (promotion_index == NONE)
			goto invalid;
		version->definition.promotion_sound.index = promotion_index;
		version->definition.promotion_sound.name = tag_get_name(promotion_index);
		version->definition.promotion_sound.name_length = (long)path_length;
		cursor += path_length + 1;
	}
	else
	{
		version->definition.promotion_sound.index = NONE;
		version->definition.promotion_sound.name = NULL;
		version->definition.promotion_sound.name_length = 0;
	}

	version->ranges = debug_malloc(range_count * sizeof(*version->ranges), TRUE,
		__FILE__, __LINE__);
	if (!version->ranges || !loose_fits(cursor, range_count * LOOSE_SOUND_RANGE, size))
		goto invalid;
	version->definition.pitch_ranges.count = (long)range_count;
	version->definition.pitch_ranges.address = version->ranges;
	{
		byte const *range_bytes = version->file_data + cursor;

		cursor += range_count * LOOSE_SOUND_RANGE;
		for (i = 0; i < (int)range_count; i++)
		{
			struct sound_pitch_range *pitch = &version->ranges[i];
			struct sound_pitch_range const *stock =
				&((struct sound_pitch_range const *)original->pitch_ranges.address)[i];
			byte const *r = range_bytes + i * LOOSE_SOUND_RANGE;
			uint32_t count = loose_be32(r + offsetof(struct sound_pitch_range, permutations));
			long active_count = old ? old->ranges[i].permutations.count :
				stock->permutations.count;
			int j;

			/* Existing sound instances store permutation indices, so a live
			   count change must wait until this definition is no longer in use. */
			if (count != (uint32_t)active_count &&
				sound_definition_has_live_instances(tag_index))
			{
				reason = "permutation count changed while sound is playing; stop it and reload";
				goto invalid;
			}
			if (!count || count > MAXIMUM_PERMUTATIONS_PER_PITCH_RANGE ||
				!loose_fits(cursor, (size_t)count * LOOSE_SOUND_PERMUTATION, size))
				goto invalid;
			memcpy(pitch->name, r, sizeof(pitch->name));
			pitch->name[sizeof(pitch->name) - 1] = 0;
			pitch->natural_pitch = loose_float(r + offsetof(struct sound_pitch_range, natural_pitch));
			pitch->bend_bounds.lower = loose_float(r + offsetof(struct sound_pitch_range, bend_bounds));
			pitch->bend_bounds.upper = loose_float(r + offsetof(struct sound_pitch_range, bend_bounds) + 4);
			pitch->actual_permutation_count =
				loose_be16(r + offsetof(struct sound_pitch_range, actual_permutation_count));
			if (!__builtin_isfinite(pitch->natural_pitch) ||
				!__builtin_isfinite(pitch->bend_bounds.lower) || !__builtin_isfinite(pitch->bend_bounds.upper))
				goto invalid;
			if (!(version->definition.flags & (1u << _sound_definition_linked_permutations_bit)))
				pitch->actual_permutation_count = (short)count;
			if (!pitch->actual_permutation_count ||
				pitch->actual_permutation_count > MAXIMUM_PERMUTATIONS_PER_RANDOM_PITCH_RANGE ||
				pitch->actual_permutation_count > count)
				goto invalid;
			if (pitch->natural_pitch <= 0.0f)
				pitch->natural_pitch = 1.0f;
			if (pitch->bend_bounds.lower > pitch->natural_pitch)
				pitch->bend_bounds.lower = pitch->natural_pitch;
			if (pitch->bend_bounds.upper < pitch->natural_pitch)
				pitch->bend_bounds.upper = pitch->natural_pitch;
			if (pitch->bend_bounds.lower < 0.0f)
				goto invalid;
			pitch->playback_rate = 1.0f / pitch->natural_pitch;
			pitch->played_permutation_mask = 0;
			pitch->previous_permutation_index = NONE;
			pitch->forced_permutation_index = NONE;
			pitch->permutations.count = count;
			pitch->permutations.definition = stock->permutations.definition;
			version->permutations[i] = debug_malloc(count * sizeof(struct sound_permutation),
				TRUE, __FILE__, __LINE__);
			if (!version->permutations[i])
				goto invalid;
			pitch->permutations.address = version->permutations[i];
			{
				byte const *permutation_bytes = version->file_data + cursor;

				cursor += (size_t)count * LOOSE_SOUND_PERMUTATION;
				for (j = 0; j < (int)count; j++)
				{
					struct sound_permutation *p = &version->permutations[i][j];
					byte const *d = permutation_bytes + j * LOOSE_SOUND_PERMUTATION;
					uint32_t samples = loose_be32(d + offsetof(struct sound_permutation, samples));
					uint32_t mouth = loose_be32(d + offsetof(struct sound_permutation, mouth_data));
					uint32_t subtitle = loose_be32(d + offsetof(struct sound_permutation, subtitle_data));
					uint16_t next = loose_be16(d + offsetof(struct sound_permutation, next_permutation_index));
					unsigned channels = version->definition.encoding + 1;
					unsigned block = version->definition.compression ? 36u * channels : 2u * channels;
					uint64_t frames;
					double milliseconds;
					float pitch_min;

					if (!samples || samples > MAXIMUM_SOUND_DATA_SIZE || samples % block ||
					mouth > MAXIMUM_SOUND_MOUTH_DATA_SIZE ||
					subtitle > MAXIMUM_SOUND_SUBTITLE_DATA_SIZE ||
					loose_be16(d + offsetof(struct sound_permutation, compression)) !=
						version->definition.compression ||
					(next != 0xFFFFu && next >= count) ||
					!loose_fits(cursor, (size_t)samples + mouth + subtitle, size))
						goto invalid;
					memcpy(p->name, d, sizeof(p->name));
					p->name[sizeof(p->name) - 1] = 0;
					p->skip_fraction = loose_float(d + offsetof(struct sound_permutation, skip_fraction));
					p->gain = loose_float(d + offsetof(struct sound_permutation, gain));
					if (!__builtin_isfinite(p->gain) || !__builtin_isfinite(p->skip_fraction) ||
						p->gain < 0.0f || p->skip_fraction < 0.0f || p->skip_fraction > 1.0f)
						goto invalid;
					p->compression = version->definition.compression;
					p->next_permutation_index =
						version->definition.flags & (1u << _sound_definition_linked_permutations_bit) ?
						(short)next : NONE;
					if (!(version->definition.flags & (1u << _sound_definition_linked_permutations_bit)) &&
						p->gain == 0.0f)
						p->gain = 1.0f;
					p->unknown0 = NONE;
					p->unknown1 = 0;
					p->unknown2 = tag_index;
					p->unknown3 = tag_index;
					p->sample_buffer_size = 0;
					p->samples.size = samples;
					p->samples.file_offset = NONE;
					p->samples.address = version->file_data + cursor;
					cursor += samples;
					p->mouth_data.size = mouth;
					p->mouth_data.address = version->file_data + cursor;
					cursor += mouth;
					p->subtitle_data.size = subtitle;
					p->subtitle_data.address = version->file_data + cursor;
					cursor += subtitle;
					frames = version->definition.compression ?
						(uint64_t)samples / block * 64 : (uint64_t)samples / block;
					pitch_min = fminf(version->definition.random_pitch_bounds.lower,
						version->definition.random_pitch_bounds.upper) *
						fminf(version->definition.zero_pitch_modifier,
							version->definition.one_pitch_modifier);
					if (pitch_min <= 0.0f)
						goto invalid;
					milliseconds = (double)frames * 1000.0 * pitch->natural_pitch /
						((version->definition.sample_rate ? 44100u : 22050u) * pitch_min);
					if (!__builtin_isfinite(milliseconds) || milliseconds >= INT_MAX)
						goto invalid;
					if ((long)milliseconds > version->definition.longest_permutation_length)
						version->definition.longest_permutation_length = (long)milliseconds;
				}
			}
			/* A linked permutation chain must terminate before cycling. */
			for (j = 0; j < pitch->actual_permutation_count; j++)
			{
				short current = (short)j;
				unsigned steps = 0;
				while (current != NONE && steps++ <= count)
					current = version->permutations[i][current].next_permutation_index;
				if (current != NONE)
					goto invalid;
			}
			if (version->definition.flags & (1u << _sound_definition_linked_permutations_bit))
				for (j = 0; j < pitch->actual_permutation_count; j++)
				{
				short next;
				struct sound_permutation *start = &version->permutations[i][j];
				if (start->gain == 0.0f)
					start->gain = 1.0f;
				for (next = start->next_permutation_index; next != NONE;
					next = version->permutations[i][next].next_permutation_index)
				{
					version->permutations[i][next].gain = start->gain;
					version->permutations[i][next].skip_fraction = 0.0f;
				}
			}
		}
	}
	if (cursor != size)
		goto invalid;
	version->retained_size = size;
	version->next = loose_versions;
	loose_versions = version;
	loose_retained_bytes += size;
	loose_active[slot] = version;
	return 1;

invalid:
	if (file)
		fclose(file);
	loose_free_version(version);
	console_warning("Loose sound skipped (%s): %s", reason, path);
	return -1;
}

boolean loose_sound_reload(char const *name)
{
	struct tag_iterator iterator;
	long index;
	int loaded = 0, unchanged = 0, removed = 0, invalid = 0;
	char selected[512];

	if (!loose_ready)
	{
		console_warning("Load a map before reloading loose sounds");
		return FALSE;
	}
	if (!config_boolean("audio.loose_sounds"))
	{
		if (name)
			console_warning("Loose sounds are disabled in config.toml; enable audio.loose_sounds and restart");
		return TRUE;
	}
	selected[0] = 0;
	if (name && *name)
	{
		size_t length;

		while (*name == ' ' || *name == '\t') name++;
		length = strlen(name);
		while (length && (name[length - 1] == ' ' || name[length - 1] == '\t')) length--;
		if (length >= sizeof(selected))
			return FALSE;
		memcpy(selected, name, length);
		selected[length] = 0;
		if (length > 1 && selected[0] == '"' && selected[length - 1] == '"')
		{
			memmove(selected, selected + 1, length - 2);
			selected[length - 2] = 0;
		}
		length = strlen(selected);
		if (length >= 6 && !_stricmp(selected + length - 6, ".sound"))
			selected[length - 6] = 0;
		for (length = 0; selected[length]; length++)
			if (selected[length] == '/') selected[length] = '\\';
		if (!selected[0])
			return FALSE;
	}

	tag_iterator_new(&iterator, SOUND_DEFINITION_TAG);
	for (index = tag_iterator_next(&iterator); index != NONE;
		index = tag_iterator_next(&iterator))
	{
		char const *path = tag_get_name(index);
		int result;

		if (selected[0] && _stricmp(path, selected))
			continue;
		result = loose_reload_one(index, path);
		if (result == 1) loaded++;
		else if (result == 2) unchanged++;
		else if (result < 0) invalid++;
		else removed++;
		if (selected[0])
			break;
	}
	if (selected[0] && loaded + unchanged + removed + invalid == 0)
	{
		console_warning("Sound is not in this map: %s", selected);
		return FALSE;
	}
	if (selected[0] || loaded || invalid)
		console_printf(FALSE, "Loose sounds: %d loaded, %d unchanged, %d stock, %d invalid; %lu MiB retained",
			loaded, unchanged, removed, invalid, (unsigned long)(loose_retained_bytes >> 20));
	return invalid == 0 && (!selected[0] || loaded + unchanged != 0);
}

boolean loose_sound_restart(void)
{
	if (!loose_ready)
	{
		console_warning("Load a map before restarting sound");
		return FALSE;
	}

	/* Stop before changing definitions: channels, queued packets and the
	 * sound cache may all still point into a previous loose tag version. */
	sound_stop_all();
	sound_cache_flush();
	if (config_boolean("audio.loose_sounds"))
		return loose_sound_reload(NULL);

	/* Retain the old allocations until map unload in case a cache block is
	 * still referenced. New tag lookups now resolve to the map's original. */
	memset(loose_active, 0, sizeof(loose_active));
	return TRUE;
}

#endif
