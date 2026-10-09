/*
LOOSE_SOUNDS.C

Sound tags played over a map's, for those making them (audio.loose_sounds,
off unless set). When a map's tags load (scenario_tags_load), each of its
sound tags that has a tag file of its name in the data root's tags folder
(tags/sound/.../name.sound, as the Halo Editing Kit and Invader write them)
is played from that file instead of the map, and the script function
loose_sounds_reload, typed at the console, reads them again: a changed
file is taken, a removed one gives the map's sound back. loose_sounds false
gives every sound back until loose_sounds true.

A tag file is Halo PC's (big-endian, version 4): the sound, the path of its
promotion sound, its pitch ranges, then each range's permutations, each
followed by its samples, mouth data and subtitle data. Every count, size and
offset is checked against the file, which must end where the last of them
does, and every value the game divides by or steps through against what the
game takes; a file that is not is said and left out. Its samples (16-bit
PCM, Xbox ADPCM or Ogg Vorbis, mono or stereo at 22 or 44 kHz) are made the
Xbox ADPCM this build plays as a Custom Edition map's are
(custom_edition_sounds_encode: mono at 22 kHz, stereo at its own rate), so
they play through the map's channels as its sounds do. The values a map's
build (tool.exe, Invader) fills in are filled in as it does: the defaults
of the sound's class, the pitch ranges' playback rates, the per-tick bend.
The longest permutation length is Tool's measure of a permutation, a split
(linked) sound's chain of them summed: the whole chain plays, and the
scripts and AI speech that wait for a sound wait for it.

The map's definition takes the file's values in place, so everything that
holds the sound keeps holding it; only what the game counts as it plays
(promotion counter and time, scripting time and sound) stays the map's
definition's own, kept across a reload or switch. The map's own values come
back when the file goes. The samples are read into the sound cache from
memory (loose_sounds_read, xbox_sound_cache.c).

A sound playing holds its permutation by index and the sound cache by
address, so nothing is changed under one: when a reload changes anything,
every sound stops and the sound cache lets go what is not playing first,
then the definitions change, and the permutations that went are freed once
no cache block holds them (at the latest when the map goes).
*/

/* ---------- headers */

#include "cseries.h"
#include "errors.h"
#include "cache/cache_files.h"
#include "cache/sound_cache.h"
#include "main/console.h"
#include "sound/sound_definitions.h"
#include "sound/sound_manager.h"
#include "tag_files/tag_groups.h"
#include "custom_edition_cache.h"

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* the platform layer's (port/linux/src) */
int config_boolean(char const *name);

/* the game's (port) */
char *tag_get_name(long tag_index);

/* ---------- constants */

/* where a sound tag's file is: tags\\<its name>.sound in the data root */
#define TAG_FILE_FOLDER "d:\\tags\\"
#define TAG_FILE_EXTENSION ".sound"

/* a tag file's header, and the sizes of its sound, pitch range and
permutation (sound_definitions.h's structures, as the file has them) */
#define TAG_FILE_HEADER_SIZE 0x40
#define TAG_FILE_GROUP_OFFSET 0x24
#define TAG_FILE_HEADER_SIZE_OFFSET 0x2C
#define TAG_FILE_VERSION_OFFSET 0x38
#define TAG_FILE_SIGNATURE_OFFSET 0x3C
#define SOUND_SIZE 0xA4
#define PITCH_RANGE_SIZE 0x48
#define PERMUTATION_SIZE 0x7C

/* the largest file read (the largest of Halo PC's sounds is a few MB), and
the most the files and their samples together keep */
#define MAXIMUM_FILE_SIZE (64UL * 1024 * 1024)
#define MAXIMUM_KEPT_BYTES (512UL * 1024 * 1024)

#define SOUND_COMPRESSION_NONE 0
#define SOUND_COMPRESSION_XBOX_ADPCM 1
#define SOUND_COMPRESSION_OGG_VORBIS 3
#define ADPCM_BLOCK_BYTES 36

/* (sound_classes.c) */
#define NUMBER_OF_SOUND_CLASSES 51

enum
{
	_class_projectile_impact = 0,
	_class_projectile_detonation = 1,
	_class_weapon_fire = 4,
	_class_weapon_ready = 5,
	_class_weapon_reload = 6,
	_class_weapon_empty = 7,
	_class_weapon_charge = 8,
	_class_weapon_overheat = 9,
	_class_weapon_idle = 10,
	_class_object_impacts = 13,
	_class_particle_impacts = 14,
	_class_slow_particle_impacts = 15,
	_class_unit_footsteps = 18,
	_class_unit_dialog = 19,
	_class_vehicle_collision = 22,
	_class_vehicle_engine = 23,
	_class_device_door = 26,
	_class_device_force_field = 27,
	_class_device_machinery = 28,
	_class_device_nature = 29,
	_class_device_computers = 30,
	_class_music = 32,
	_class_ambient_nature = 33,
	_class_ambient_machinery = 34,
	_class_ambient_computers = 35,
	_class_first_person_damage = 39,
	_class_scripted_dialog_player = 44,
	_class_scripted_effect = 45,
	_class_scripted_dialog_other = 46,
	_class_scripted_dialog_force_unspatialized = 47,
	_class_game_event = 50,
};

/* ---------- structures */

/* a sound tag file read, and what it makes */
struct loose_sound
{
	struct loose_sound *next;
	long tag_index;
	/* the file, kept to tell whether it changed */
	byte *file;
	unsigned long file_size;
	/* the definition it makes (its counters unused), its pitch ranges and
	their permutations, and each permutation's samples */
	struct sound_definition definition;
	struct sound_pitch_range *ranges;
	struct sound_permutation *permutations;
	long permutation_count;
	unsigned long kept_bytes;
};

/* a sound of the map played from a file: the map's own values, given back
when it goes */
struct loose_override
{
	struct loose_sound *sound;
	struct sound_definition map_definition;
};

static struct
{
	boolean map_loaded;
	boolean enabled;
	/* by the map's sound tag, its override (none: the map's own) */
	struct loose_override *overrides;
	long override_count;
	/* the sounds made and not yet let go: those of the overrides, and those
	gone but still in the sound cache */
	struct loose_sound *sounds;
	unsigned long kept_bytes;
} loose_sounds_globals;

/* ---------- private code */

static unsigned long read_be16(
	byte const *bytes)
{
	return ((unsigned long)bytes[0] << 8) | bytes[1];
}

static unsigned long read_be32(
	byte const *bytes)
{
	return ((unsigned long)bytes[0] << 24) | ((unsigned long)bytes[1] << 16) |
		((unsigned long)bytes[2] << 8) | bytes[3];
}

static short read_short(
	byte const *bytes)
{
	return (short)read_be16(bytes);
}

static real read_real(
	byte const *bytes)
{
	unsigned long bits = read_be32(bytes);
	real value;

	memcpy(&value, &bits, sizeof(value));

	return value;
}

/* whether `size` bytes at `offset` lie within `file_size` */
static boolean in_file(
	unsigned long offset,
	unsigned long size,
	unsigned long file_size)
{
	return offset <= file_size && size <= file_size - offset;
}

static boolean finite_real(
	real value)
{
	return value == value && value - value == 0.f;
}

/* the tag file of the sound tag `name`, in the data root's tags folder (an
Xbox path, which the game's fopen finds there: port/linux/src/xbox_files.c);
FALSE when the name could reach outside it */
static boolean tag_file_path(
	char const *name,
	char *path,
	size_t path_size)
{
	char const *character;

	if (!name || !*name || strlen(name) + sizeof(TAG_FILE_FOLDER TAG_FILE_EXTENSION) > path_size)
	{
		return FALSE;
	}
	/* (a name is relative and of the folder: no drive, no parent, no
	absolute or empty part) */
	for (character = name; *character; character++)
	{
		boolean separator = *character == '\\' || *character == '/';

		if (*character == ':' ||
			(separator && (character == name || !character[1] || character[1] == '\\' || character[1] == '/')) ||
			(*character == '.' && character[1] == '.' &&
				(character == name || character[-1] == '\\' || character[-1] == '/')))
		{
			return FALSE;
		}
	}
	sprintf(path, "%s%s%s", TAG_FILE_FOLDER, name, TAG_FILE_EXTENSION);

	return TRUE;
}

/* the file at `path` in a buffer of the game's allocator, or NULL (no file:
*missing) */
static byte *file_read(
	char const *path,
	unsigned long *size,
	boolean *missing)
{
	FILE *file = fopen(path, "rb");
	byte *data = NULL;
	long length;

	*missing = file == NULL;
	if (!file)
	{
		return NULL;
	}
	if (fseek(file, 0, SEEK_END) == 0 &&
		(length = ftell(file)) >= TAG_FILE_HEADER_SIZE + SOUND_SIZE &&
		(unsigned long)length <= MAXIMUM_FILE_SIZE &&
		fseek(file, 0, SEEK_SET) == 0 &&
		(data = malloc((size_t)length)) != NULL)
	{
		if (fread(data, 1, (size_t)length, file) == (size_t)length)
		{
			*size = (unsigned long)length;
		}
		else
		{
			free(data);
			data = NULL;
		}
	}
	fclose(file);

	return data;
}

static void loose_sound_free(
	struct loose_sound *sound)
{
	long permutation_index;

	for (permutation_index = 0; permutation_index < sound->permutation_count; permutation_index++)
	{
		if (sound->permutations[permutation_index].samples.address)
			free(sound->permutations[permutation_index].samples.address);
	}
	/* (the game's free, debug_free, does not take NULL) */
	if (sound->permutations)
		free(sound->permutations);
	if (sound->ranges)
		free(sound->ranges);
	if (sound->file)
		free(sound->file);
	free(sound);

	return;
}

/* the defaults of a class for a sound's distances, which a map's build
fills in for none (Invader's, as tool.exe's) */
static void class_distances(
	short sound_class,
	real *minimum,
	real *maximum)
{
	switch (sound_class)
	{
	case _class_device_machinery: case _class_device_force_field: case _class_ambient_machinery:
	case _class_ambient_nature: case _class_device_door: case _class_music: case _class_device_nature:
		*minimum = 0.9f; *maximum = 5.f;
		break;
	case _class_weapon_empty: case _class_weapon_idle: case _class_weapon_ready:
	case _class_weapon_reload: case _class_weapon_charge: case _class_weapon_overheat:
		*minimum = 1.f; *maximum = 9.f;
		break;
	case _class_scripted_dialog_other: case _class_scripted_dialog_player: case _class_game_event:
	case _class_unit_dialog: case _class_scripted_dialog_force_unspatialized:
		*minimum = 3.f; *maximum = 20.f;
		break;
	case _class_first_person_damage: case _class_object_impacts: case _class_ambient_computers:
	case _class_particle_impacts: case _class_device_computers: case _class_slow_particle_impacts:
		*minimum = 0.5f; *maximum = 3.f;
		break;
	case _class_vehicle_engine: case _class_projectile_impact: case _class_vehicle_collision:
		*minimum = 1.4f; *maximum = 8.f;
		break;
	case _class_weapon_fire:
		*minimum = 4.f; *maximum = 70.f;
		break;
	case _class_scripted_effect:
		*minimum = 2.f; *maximum = 5.f;
		break;
	case _class_projectile_detonation:
		*minimum = 8.f; *maximum = 120.f;
		break;
	case _class_unit_footsteps:
		*minimum = 0.9f; *maximum = 10.f;
		break;
	default:
		*minimum = 0.f; *maximum = 0.f;
		break;
	}

	return;
}

/* the defaults a map's build fills in for none (Invader's, as tool.exe's) */
static void definition_defaults(
	struct sound_definition *definition)
{
	real minimum_distance;
	real maximum_distance;

	if (definition->zero_skip_fraction_modifier == 0.f && definition->one_skip_fraction_modifier == 0.f)
	{
		definition->zero_skip_fraction_modifier = 1.f;
		definition->one_skip_fraction_modifier = 1.f;
	}
	if (definition->random_pitch_bounds.lower == 0.f)
		definition->random_pitch_bounds.lower = 1.f;
	if (definition->random_pitch_bounds.upper == 0.f)
		definition->random_pitch_bounds.upper = 1.f;
	if (definition->zero_gain_modifier == 0.f && definition->one_gain_modifier == 0.f)
	{
		definition->one_gain_modifier = 1.f;
		switch (definition->sound_class)
		{
		case _class_object_impacts: case _class_particle_impacts: case _class_slow_particle_impacts:
		case _class_unit_dialog: case _class_music: case _class_ambient_nature: case _class_ambient_machinery:
		case _class_ambient_computers: case _class_scripted_dialog_player: case _class_scripted_dialog_other:
		case _class_scripted_dialog_force_unspatialized: case _class_scripted_effect:
			definition->zero_gain_modifier = 0.f;
			break;
		default:
			definition->zero_gain_modifier = 1.f;
			break;
		}
	}
	if (definition->zero_pitch_modifier == 0.f && definition->one_pitch_modifier == 0.f)
	{
		definition->zero_pitch_modifier = 1.f;
		definition->one_pitch_modifier = 1.f;
	}
	class_distances(definition->sound_class, &minimum_distance, &maximum_distance);
	if (definition->minimum_distance == 0.f)
		definition->minimum_distance = minimum_distance;
	if (definition->maximum_distance == 0.f)
		definition->maximum_distance = maximum_distance;
	/* (a map holds the bend a tick, the file a second) */
	definition->maximum_bend_per_second = definition->maximum_bend_per_second > 0.f ?
		(real)pow(definition->maximum_bend_per_second, 1.0 / 30.0) : 0.f;

	return;
}

/* Tool's measure of a permutation of `size` bytes in `compression` (and,
for Ogg Vorbis, of `buffer_size` decoded), in milliseconds (Invader's
SoundPermutation length, as tool.exe's) */
static real permutation_length(
	struct sound_definition const *definition,
	struct sound_pitch_range const *range,
	short compression,
	unsigned long size,
	unsigned long buffer_size,
	long rate)
{
	real channel_modifier = definition->encoding == 0 ? 1.f : 0.5f;
	real samples;
	real format_modifier;
	real slowest = MIN(definition->random_pitch_bounds.lower, definition->random_pitch_bounds.upper) *
		MIN(definition->zero_pitch_modifier, definition->one_pitch_modifier);

	switch (compression)
	{
	case SOUND_COMPRESSION_NONE:
		samples = (real)size;
		format_modifier = 0.5f;
		break;
	case SOUND_COMPRESSION_OGG_VORBIS:
		samples = (real)buffer_size / 4.f;
		format_modifier = 2.2f;
		break;
	default:
		samples = (real)size;
		format_modifier = 2.f;
		break;
	}

	return samples * 1000.f * channel_modifier * format_modifier * range->natural_pitch / ((real)rate * slowest);
}

/* The sound the tag file `file` makes for the map's sound tag
`tag_index`, or NULL with *reason saying why not. */
static struct loose_sound *loose_sound_new(
	long tag_index,
	byte *file,
	unsigned long file_size,
	char const **reason)
{
	struct loose_sound *sound;
	struct sound_definition *definition;
	byte const *root = file + TAG_FILE_HEADER_SIZE;
	unsigned long cursor = TAG_FILE_HEADER_SIZE + SOUND_SIZE;
	unsigned long range_count;
	unsigned long promotion_length;
	unsigned long range_index;
	long permutation_total = 0;
	boolean linked;
	long rate;
	long encoded_rate;
	long channels;
	real longest = 0.f;

	*reason = "not a sound tag file of this version";
	if (memcmp(file + TAG_FILE_GROUP_OFFSET, "snd!", 4) ||
		memcmp(file + TAG_FILE_SIGNATURE_OFFSET, "blam", 4) ||
		read_be32(file + TAG_FILE_HEADER_SIZE_OFFSET) != TAG_FILE_HEADER_SIZE ||
		read_be16(file + TAG_FILE_VERSION_OFFSET) != SOUND_DEFINITION_VERSION)
	{
		return NULL;
	}
	sound = malloc(sizeof(*sound));
	if (!sound)
	{
		*reason = "out of memory";
		return NULL;
	}
	memset(sound, 0, sizeof(*sound));
	sound->tag_index = tag_index;
	definition = &sound->definition;

	/* the sound's values */
#define SOUND_SHORT(field) definition->field = read_short(root + offsetof(struct sound_definition, field))
#define SOUND_REAL(field) definition->field = read_real(root + offsetof(struct sound_definition, field))
	definition->flags = read_be32(root + offsetof(struct sound_definition, flags));
	SOUND_SHORT(sound_class);
	SOUND_SHORT(sample_rate);
	SOUND_SHORT(encoding);
	SOUND_SHORT(compression);
	SOUND_SHORT(promotion_count);
	SOUND_REAL(minimum_distance);
	SOUND_REAL(maximum_distance);
	SOUND_REAL(skip_fraction);
	SOUND_REAL(random_pitch_bounds.lower);
	SOUND_REAL(random_pitch_bounds.upper);
	SOUND_REAL(inner_cone_angle);
	SOUND_REAL(outer_cone_angle);
	SOUND_REAL(outer_cone_gain);
	SOUND_REAL(gain_modifier);
	SOUND_REAL(maximum_bend_per_second);
	SOUND_REAL(zero_skip_fraction_modifier);
	SOUND_REAL(zero_gain_modifier);
	SOUND_REAL(zero_pitch_modifier);
	SOUND_REAL(one_skip_fraction_modifier);
	SOUND_REAL(one_gain_modifier);
	SOUND_REAL(one_pitch_modifier);
#undef SOUND_SHORT
#undef SOUND_REAL
	*reason = "a value of the sound is out of range";
	if (definition->sound_class < 0 || definition->sound_class >= NUMBER_OF_SOUND_CLASSES ||
		(definition->sample_rate != 0 && definition->sample_rate != 1) ||
		(definition->encoding != 0 && definition->encoding != 1) ||
		definition->promotion_count < 0 ||
		!finite_real(definition->minimum_distance) || !finite_real(definition->maximum_distance) ||
		definition->minimum_distance < 0.f || definition->maximum_distance < 0.f ||
		!finite_real(definition->skip_fraction) ||
		!finite_real(definition->random_pitch_bounds.lower) || !finite_real(definition->random_pitch_bounds.upper) ||
		definition->random_pitch_bounds.lower < 0.f || definition->random_pitch_bounds.upper < 0.f ||
		!finite_real(definition->inner_cone_angle) || !finite_real(definition->outer_cone_angle) ||
		!finite_real(definition->outer_cone_gain) || !finite_real(definition->gain_modifier) ||
		!finite_real(definition->maximum_bend_per_second) || definition->maximum_bend_per_second < 0.f ||
		!finite_real(definition->zero_skip_fraction_modifier) || !finite_real(definition->one_skip_fraction_modifier) ||
		!finite_real(definition->zero_gain_modifier) || !finite_real(definition->one_gain_modifier) ||
		!finite_real(definition->zero_pitch_modifier) || !finite_real(definition->one_pitch_modifier) ||
		definition->zero_pitch_modifier < 0.f || definition->one_pitch_modifier < 0.f)
	{
		goto refused;
	}
	*reason = "its samples are IMA ADPCM, which this build does not decode";
	if (definition->compression != SOUND_COMPRESSION_NONE &&
		definition->compression != SOUND_COMPRESSION_XBOX_ADPCM &&
		definition->compression != SOUND_COMPRESSION_OGG_VORBIS)
	{
		goto refused;
	}
	definition_defaults(definition);
	*reason = "a value of the sound is out of range";
	if (!finite_real(definition->maximum_bend_per_second) ||
		MIN(definition->random_pitch_bounds.lower, definition->random_pitch_bounds.upper) *
			MIN(definition->zero_pitch_modifier, definition->one_pitch_modifier) <= 0.f)
	{
		goto refused;
	}
	linked = TEST_FLAG(definition->flags, _sound_definition_linked_permutations_bit);
	channels = definition->encoding == 0 ? 1 : 2;
	rate = definition->sample_rate == 0 ? 22050 : 44100;
	/* (what this build plays: mono at 22 kHz, stereo at either) */
	encoded_rate = channels == 1 ? 22050 : rate;

	/* the promotion sound: one of the map's, or none */
	promotion_length = read_be32(root + offsetof(struct sound_definition, promotion_sound.name_length));
	definition->promotion_sound.group_tag = SOUND_DEFINITION_TAG;
	definition->promotion_sound.index = NONE;
	definition->promotion_sound.name = NULL;
	definition->promotion_sound.name_length = 0;
	if (promotion_length)
	{
		char promotion[256];
		long promotion_index;

		*reason = "the promotion sound's name is not in the file";
		if (promotion_length >= sizeof(promotion) || !in_file(cursor, promotion_length + 1, file_size) ||
			file[cursor + promotion_length] != 0 || memchr(file + cursor, 0, promotion_length))
		{
			goto refused;
		}
		memcpy(promotion, file + cursor, promotion_length + 1);
		cursor += promotion_length + 1;
		promotion_index = tag_loaded(SOUND_DEFINITION_TAG, promotion);
		if (promotion_index != NONE)
		{
			definition->promotion_sound.index = promotion_index;
			definition->promotion_sound.name = tag_get_name(promotion_index);
			definition->promotion_sound.name_length = (long)strlen(definition->promotion_sound.name);
		}
	}

	/* the pitch ranges, then each one's permutations and their data */
	range_count = read_be32(root + offsetof(struct sound_definition, pitch_ranges.count));
	*reason = "its pitch ranges are not in the file";
	if (range_count < 1 || range_count > MAXIMUM_PITCH_RANGES_PER_SOUND ||
		!in_file(cursor, range_count * PITCH_RANGE_SIZE, file_size))
	{
		goto refused;
	}
	for (range_index = 0; range_index < range_count; range_index++)
	{
		/* (no more than a range takes: a larger count is refused below) */
		permutation_total += (long)MIN(read_be32(file + cursor + range_index * PITCH_RANGE_SIZE +
			offsetof(struct sound_pitch_range, permutations.count)), MAXIMUM_PERMUTATIONS_PER_PITCH_RANGE);
	}
	sound->ranges = malloc(range_count * sizeof(*sound->ranges));
	sound->permutations = malloc((size_t)MAX(permutation_total, 1) * sizeof(*sound->permutations));
	*reason = "out of memory";
	if (!sound->ranges || !sound->permutations)
	{
		goto refused;
	}
	memset(sound->ranges, 0, range_count * sizeof(*sound->ranges));
	memset(sound->permutations, 0, (size_t)MAX(permutation_total, 1) * sizeof(*sound->permutations));
	{
		byte const *ranges = file + cursor;

		cursor += range_count * PITCH_RANGE_SIZE;
		for (range_index = 0; range_index < range_count; range_index++)
		{
			byte const *range_bytes = ranges + range_index * PITCH_RANGE_SIZE;
			struct sound_pitch_range *range = &sound->ranges[range_index];
			unsigned long count = read_be32(range_bytes + offsetof(struct sound_pitch_range, permutations.count));
			struct sound_permutation *permutations = sound->permutations + sound->permutation_count;
			byte const *permutation_bytes = file + cursor;
			unsigned long permutation_index;
			real *lengths;

			*reason = "a pitch range's permutations are not in the file";
			if (count < 1 || count > MAXIMUM_PERMUTATIONS_PER_PITCH_RANGE ||
				!in_file(cursor, count * PERMUTATION_SIZE, file_size))
			{
				goto refused;
			}
			cursor += count * PERMUTATION_SIZE;
			memcpy(range->name, range_bytes, sizeof(range->name));
			range->name[sizeof(range->name) - 1] = 0;
			range->natural_pitch = read_real(range_bytes + offsetof(struct sound_pitch_range, natural_pitch));
			range->bend_bounds.lower = read_real(range_bytes + offsetof(struct sound_pitch_range, bend_bounds.lower));
			range->bend_bounds.upper = read_real(range_bytes + offsetof(struct sound_pitch_range, bend_bounds.upper));
			range->actual_permutation_count = linked ?
				read_short(range_bytes + offsetof(struct sound_pitch_range, actual_permutation_count)) : (short)count;
			*reason = "a value of a pitch range is out of range";
			if (!finite_real(range->natural_pitch) || !finite_real(range->bend_bounds.lower) ||
				!finite_real(range->bend_bounds.upper) ||
				range->actual_permutation_count < 1 ||
				range->actual_permutation_count > MAXIMUM_PERMUTATIONS_PER_RANDOM_PITCH_RANGE ||
				(unsigned long)range->actual_permutation_count > count)
			{
				goto refused;
			}
			/* (as a map's build makes them) */
			if (range->natural_pitch <= 0.f)
				range->natural_pitch = 1.f;
			range->bend_bounds.lower = MIN(range->bend_bounds.lower, range->natural_pitch);
			range->bend_bounds.upper = MAX(range->bend_bounds.upper, range->natural_pitch);
			if (range->bend_bounds.lower < 0.f)
				goto refused;
			range->playback_rate = 1.f / range->natural_pitch;
			range->played_permutation_mask = 0xFFFFFFFF;
			range->previous_permutation_index = NONE;
			range->forced_permutation_index = NONE;
			range->permutations.count = (long)count;
			range->permutations.address = permutations;
			range->permutations.definition = NULL;

			/* each permutation, its samples made this build's */
			lengths = (real *)malloc(count * sizeof(real));
			if (!lengths)
			{
				*reason = "out of memory";
				goto refused;
			}
			for (permutation_index = 0; permutation_index < count; permutation_index++)
			{
				byte const *bytes = permutation_bytes + permutation_index * PERMUTATION_SIZE;
				struct sound_permutation *permutation = &permutations[permutation_index];
				unsigned long samples_size = read_be32(bytes + offsetof(struct sound_permutation, samples.size));
				unsigned long mouth_size = read_be32(bytes + offsetof(struct sound_permutation, mouth_data.size));
				unsigned long subtitle_size = read_be32(bytes + offsetof(struct sound_permutation, subtitle_data.size));
				unsigned long buffer_size = read_be32(bytes + offsetof(struct sound_permutation, sample_buffer_size));
				short compression = read_short(bytes + offsetof(struct sound_permutation, compression));
				short next = read_short(bytes + offsetof(struct sound_permutation, next_permutation_index));
				byte const *samples = file + cursor;
				unsigned long encoded_bytes = 0;

				sound->permutation_count++;
				*reason = "a permutation's data is not in the file";
				if (samples_size == 0 || samples_size > MAXIMUM_FILE_SIZE ||
					mouth_size > MAXIMUM_SOUND_MOUTH_DATA_SIZE || subtitle_size > MAXIMUM_SOUND_SUBTITLE_DATA_SIZE ||
					!in_file(cursor, samples_size, file_size) ||
					!in_file(cursor + samples_size, mouth_size, file_size) ||
					!in_file(cursor + samples_size + mouth_size, subtitle_size, file_size))
				{
					free(lengths);
					goto refused;
				}
				*reason = "a permutation's values are out of range";
				memcpy(permutation->name, bytes, sizeof(permutation->name));
				permutation->name[sizeof(permutation->name) - 1] = 0;
				permutation->skip_fraction = read_real(bytes + offsetof(struct sound_permutation, skip_fraction));
				permutation->gain = read_real(bytes + offsetof(struct sound_permutation, gain));
				if (compression != definition->compression ||
					!finite_real(permutation->skip_fraction) || !finite_real(permutation->gain) ||
					(linked && next != NONE && (next < 0 || (unsigned long)next >= count)) ||
					(compression == SOUND_COMPRESSION_XBOX_ADPCM && samples_size % (ADPCM_BLOCK_BYTES * channels)) ||
					(compression == SOUND_COMPRESSION_NONE && samples_size % (2 * channels)))
				{
					free(lengths);
					goto refused;
				}
				permutation->next_permutation_index = linked ? next : NONE;
				lengths[permutation_index] = permutation_length(definition, range, compression,
					samples_size, buffer_size, rate);

				/* the samples (Xbox ADPCM at the rate played is played as it is) */
				if (compression == SOUND_COMPRESSION_XBOX_ADPCM && rate == encoded_rate)
				{
					permutation->samples.address = malloc(samples_size);
					if (permutation->samples.address)
					{
						memcpy(permutation->samples.address, samples, samples_size);
						encoded_bytes = samples_size;
					}
				}
				else
				{
					permutation->samples.address = custom_edition_sounds_encode(samples, (long)samples_size,
						compression, TRUE, channels, rate, encoded_rate, &encoded_bytes);
				}
				if (!permutation->samples.address || encoded_bytes > MAXIMUM_SOUND_DATA_SIZE)
				{
					*reason = permutation->samples.address ?
						"a permutation is longer than a sound cache block takes" :
						"a permutation's samples cannot be decoded";
					free(lengths);
					goto refused;
				}
				permutation->samples.size = (long)encoded_bytes;
				permutation->samples.file_offset = 0;
				permutation->compression = SOUND_COMPRESSION_XBOX_ADPCM;
				permutation->sample_buffer_size = 0;
				sound->kept_bytes += encoded_bytes;
				cursor += samples_size;
				/* (the mouth and subtitle data stay in the file kept) */
				permutation->mouth_data.size = (long)mouth_size;
				permutation->mouth_data.address = mouth_size ? file + cursor : NULL;
				cursor += mouth_size;
				permutation->subtitle_data.size = (long)subtitle_size;
				permutation->subtitle_data.address = subtitle_size ? file + cursor : NULL;
				cursor += subtitle_size;
				/* the sound cache's (xbox_sound_cache.c: block, address, tag) */
				permutation->unknown0 = NONE;
				permutation->unknown1 = 0;
				permutation->unknown2 = (unsigned long)tag_index;
				permutation->unknown3 = (unsigned long)tag_index;
			}

			/* a map's build's gains, and the longest the range plays: a
			permutation, or a chain of them that must end */
			for (permutation_index = 0; permutation_index < (unsigned long)range->actual_permutation_count; permutation_index++)
			{
				struct sound_permutation *first = &permutations[permutation_index];
				short link = first->next_permutation_index;
				real length = lengths[permutation_index];
				unsigned long steps = 0;

				if (first->gain == 0.f)
					first->gain = 1.f;
				while (link != NONE)
				{
					if (++steps > count)
					{
						*reason = "a chain of permutations never ends";
						free(lengths);
						goto refused;
					}
					permutations[link].gain = first->gain;
					permutations[link].skip_fraction = 0.f;
					length += lengths[link];
					link = permutations[link].next_permutation_index;
				}
				longest = MAX(longest, length);
			}
			if (!linked)
			{
				for (permutation_index = 0; permutation_index < count; permutation_index++)
				{
					longest = MAX(longest, lengths[permutation_index]);
				}
			}
			free(lengths);
		}
	}
	*reason = "the file goes on past its last permutation";
	if (cursor != file_size || !(longest < 2147483647.f))
	{
		goto refused;
	}

	definition->longest_permutation_length = (long)longest;
	definition->compression = SOUND_COMPRESSION_XBOX_ADPCM;
	definition->sample_rate = encoded_rate == 44100 ? 1 : 0;
	definition->pitch_ranges.count = (long)range_count;
	definition->pitch_ranges.address = sound->ranges;
	sound->file = file;
	sound->file_size = file_size;
	sound->kept_bytes += file_size;

	return sound;

refused:
	loose_sound_free(sound);

	return NULL;
}

/* Gives `definition` the values `from` has of a sound tag, keeping what
the game counts as it plays the sound (from promotion_counter on). */
static void definition_set(
	struct sound_definition *definition,
	struct sound_definition const *from)
{
	memcpy(definition, from, offsetof(struct sound_definition, promotion_counter));
	definition->pitch_ranges = from->pitch_ranges;

	return;
}

/* Lets go of the sounds not in use: none of an override's, and none that
holds a sound cache block (the cache's blocks point at their permutations). */
static void sounds_let_go(
	boolean all)
{
	struct loose_sound **reference = &loose_sounds_globals.sounds;

	while (*reference)
	{
		struct loose_sound *sound = *reference;
		boolean in_use = FALSE;
		long index;

		if (!all)
		{
			struct loose_override const *override =
				&loose_sounds_globals.overrides[DATUM_INDEX_TO_ABSOLUTE_INDEX(sound->tag_index)];

			in_use = override->sound == sound;
			for (index = 0; !in_use && index < sound->permutation_count; index++)
			{
				in_use = (long)sound->permutations[index].unknown0 != NONE;
			}
		}
		if (in_use)
		{
			reference = &sound->next;
		}
		else
		{
			*reference = sound->next;
			loose_sounds_globals.kept_bytes -= sound->kept_bytes;
			loose_sound_free(sound);
		}
	}

	return;
}

/* the override of the sound tag `tag_index` */
static struct loose_override *override_get(
	long tag_index)
{
	long absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(tag_index);

	return absolute_index >= 0 && absolute_index < loose_sounds_globals.override_count ?
		&loose_sounds_globals.overrides[absolute_index] : NULL;
}

/* Reads every sound tag's file again (or, `remove_all`, none), and plays
the map's sounds from what changed. As the map loads (`at_load`) nothing
plays yet and nothing is said at the console. */
static void loose_sounds_update(
	boolean remove_all,
	boolean at_load)
{
	struct tag_iterator iterator;
	long tag_index;
	struct loose_sound *made = NULL;
	long loaded_count = 0;
	long removed_count = 0;
	long refused_count = 0;
	boolean stopped = FALSE;

	if (!loose_sounds_globals.map_loaded || !loose_sounds_globals.overrides)
	{
		return;
	}

	/* what each file makes, before anything is changed */
	tag_iterator_new(&iterator, SOUND_DEFINITION_TAG);
	while ((tag_index = tag_iterator_next(&iterator)) != NONE)
	{
		struct loose_override *override = override_get(tag_index);
		char path[1024];
		byte *file = NULL;
		unsigned long file_size = 0;
		boolean missing = TRUE;

		if (!override)
		{
			continue;
		}
		if (!remove_all && tag_file_path(tag_get_name(tag_index), path, sizeof(path)))
		{
			file = file_read(path, &file_size, &missing);
			if (!file && !missing)
			{
				error(_error_silent, "loose sounds: cannot read %s", path);
				refused_count++;
				continue;
			}
		}
		if (file && override->sound && override->sound->file_size == file_size &&
			!memcmp(override->sound->file, file, file_size))
		{
			/* (unchanged) */
			free(file);
			continue;
		}
		if (file)
		{
			char const *reason = NULL;
			struct loose_sound *sound = NULL;

			if (loose_sounds_globals.kept_bytes + file_size > MAXIMUM_KEPT_BYTES)
			{
				reason = "the sounds read already hold too much memory";
			}
			else
			{
				sound = loose_sound_new(tag_index, file, file_size, &reason);
			}
			if (!sound)
			{
				/* (one refused leaves what was played before) */
				error(_error_silent, "loose sounds: %s left out: %s", path, reason);
				console_warning("loose sounds: %s left out: %s", tag_get_name(tag_index), reason);
				free(file);
				refused_count++;
				continue;
			}
			sound->next = made;
			made = sound;
			loose_sounds_globals.kept_bytes += sound->kept_bytes;
		}
		else if (!override->sound)
		{
			continue;
		}

		/* a change: nothing plays while it is made */
		if (!stopped && !at_load)
		{
			sound_stop_all();
			sound_cache_flush();
			stopped = TRUE;
		}
		if (override->sound)
		{
			definition_set(sound_definition_get(tag_index), &override->map_definition);
			override->sound = NULL;
			if (!file)
				removed_count++;
		}
		if (file)
		{
			struct sound_definition *definition = sound_definition_get(tag_index);

			memcpy(&override->map_definition, definition, sizeof(*definition));
			definition_set(definition, &made->definition);
			override->sound = made;
			loaded_count++;
		}
	}

	/* the sounds made join those kept, and those no longer played go */
	while (made)
	{
		struct loose_sound *next = made->next;

		made->next = loose_sounds_globals.sounds;
		loose_sounds_globals.sounds = made;
		made = next;
	}
	sounds_let_go(FALSE);

	if (!at_load || loaded_count || refused_count)
	{
		error(_error_silent, "loose sounds: %ld from tags, %ld given back, %ld left out (%lu KB kept)",
			loaded_count, removed_count, refused_count, loose_sounds_globals.kept_bytes / 1024);
		if (!at_load)
		{
			console_printf(FALSE, "loose sounds: %ld from tags, %ld given back, %ld left out",
				loaded_count, removed_count, refused_count);
		}
	}

	return;
}

/* Makes the table of overrides, one for each of the map's tags up to its
last sound tag; FALSE when it cannot be. */
static boolean overrides_new(
	void)
{
	struct tag_iterator iterator;
	long tag_index;
	long count = 0;

	if (loose_sounds_globals.overrides)
	{
		return TRUE;
	}
	tag_iterator_new(&iterator, SOUND_DEFINITION_TAG);
	while ((tag_index = tag_iterator_next(&iterator)) != NONE)
	{
		count = MAX(count, DATUM_INDEX_TO_ABSOLUTE_INDEX(tag_index) + 1);
	}
	if (count <= 0 || !(loose_sounds_globals.overrides = malloc((size_t)count * sizeof(struct loose_override))))
	{
		return FALSE;
	}
	memset(loose_sounds_globals.overrides, 0, (size_t)count * sizeof(struct loose_override));
	loose_sounds_globals.override_count = count;

	return TRUE;
}

/* ---------- public code */

/* (scenario_tags_load, once a map's tags are loaded) */
void loose_sounds_tags_loaded(
	void)
{
	loose_sounds_globals.map_loaded = TRUE;
	loose_sounds_globals.enabled = config_boolean("audio.loose_sounds") != 0;
	if (loose_sounds_globals.enabled && overrides_new())
	{
		loose_sounds_update(FALSE, TRUE);
	}

	return;
}

/* (scenario_tags_unload, once the sound cache is closed: no block holds a
permutation) */
void loose_sounds_tags_unloaded(
	void)
{
	sounds_let_go(TRUE);
	if (loose_sounds_globals.overrides)
		free(loose_sounds_globals.overrides);
	memset(&loose_sounds_globals, 0, sizeof(loose_sounds_globals));

	return;
}

/* (sound_cache_start_loading_sound) Whether `permutation` is one of a tag
file's, read at once into `buffer`. */
boolean loose_sounds_read(
	struct sound_permutation const *permutation,
	void *buffer)
{
	struct loose_sound const *sound;

	for (sound = loose_sounds_globals.sounds; sound; sound = sound->next)
	{
		if (permutation >= sound->permutations && permutation < sound->permutations + sound->permutation_count)
		{
			memcpy(buffer, permutation->samples.address, (size_t)permutation->samples.size);
			return TRUE;
		}
	}

	return FALSE;
}

/* (the script function loose_sounds_reload) */
void loose_sounds_reload(
	void)
{
	if (!loose_sounds_globals.map_loaded)
	{
		return;
	}
	if (!loose_sounds_globals.enabled)
	{
		console_warning("loose sounds are off (loose_sounds true, or audio.loose_sounds in config.toml)");
		return;
	}
	loose_sounds_update(FALSE, FALSE);

	return;
}

/* (the script function loose_sounds) Plays the sounds of tag files, or
gives the map's back, until the map goes. */
void loose_sounds_enable(
	boolean enabled)
{
	if (!loose_sounds_globals.map_loaded || enabled == loose_sounds_globals.enabled)
	{
		return;
	}
	if (enabled && !overrides_new())
	{
		return;
	}
	loose_sounds_globals.enabled = enabled;
	loose_sounds_update(!enabled, FALSE);

	return;
}
