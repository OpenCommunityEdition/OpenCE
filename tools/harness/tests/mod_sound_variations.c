/* MOD_SOUND_VARIATIONS.C: the real mod loader and permutation picker (OpenH1 #1170). */

#include "harness.h"
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#ifdef _WIN32
#define strcasecmp _stricmp
#else
#include <strings.h>
#endif

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define PIN(x, lo, hi) ((x) < (lo) ? (lo) : ((x) > (hi) ? (hi) : (x)))
#define VALID_INDEX(i, c) ((i) >= 0 && (i) < (c))
#define SET_FLAG(f, b, v) ((f) = (v) ? (f) | (1U << (b)) : (f) & ~(1U << (b)))
#define TAG_BLOCK_GET_ELEMENT(b, i, t) (&((t *)(b)->address)[i])
#define _error_silent 0
#define error(priority, ...) ((void)(priority))
typedef void *byte_swap_data_proc;

#include "types.inc"

/* ---------- the game's, faked */

/* (no map's sounds here: custom_edition_sounds_decode is not called) */
static void custom_edition_cache_read(long tag_index, long offset, long size, void *buffer)
{
	(void)tag_index; (void)offset; (void)size; (void)buffer;
	CHECK(FALSE, "a map read");
}
static void *custom_edition_cache_tag_next(byte *tag_cache, unsigned long loaded_bytes, unsigned long group_tag,
	unsigned long size, int32_t *tag_index)
{
	(void)tag_cache; (void)loaded_bytes; (void)group_tag; (void)size; (void)tag_index;
	return NULL;
}
static void *custom_edition_cache_block_element(byte *tag_cache, unsigned long loaded_bytes, struct tag_block *block,
	long index, unsigned long size)
{
	(void)tag_cache; (void)loaded_bytes; (void)block; (void)index; (void)size;
	return NULL;
}
static char const *custom_edition_cache_tag_name(byte *tag_cache, unsigned long loaded_bytes, int32_t tag_index)
{
	(void)tag_cache; (void)loaded_bytes; (void)tag_index;
	return "sound";
}

/* stb_vorbis, faked: the "stream" is a fake_vorbis record of 16-bit frames at its own rate */
struct fake_vorbis
{
	short const *samples;
	int frames;
	int channels;
	unsigned int rate;
	int read;
};
typedef struct fake_vorbis stb_vorbis;
typedef struct { unsigned int sample_rate; int channels; } stb_vorbis_info;
static stb_vorbis *stb_vorbis_open_memory(byte const *data, int length, int *error, void *alloc)
{
	static struct fake_vorbis opened;

	(void)length; (void)alloc;
	*error = 0;
	memcpy(&opened, data, sizeof(opened));
	opened.read = 0;
	return &opened;
}
static stb_vorbis_info stb_vorbis_get_info(stb_vorbis *vorbis)
{
	stb_vorbis_info info;

	info.sample_rate = vorbis->rate;
	info.channels = vorbis->channels;
	return info;
}
static int stb_vorbis_get_frame_short_interleaved(stb_vorbis *vorbis, int channels, short *buffer, int shorts)
{
	int frames = MIN(MIN(vorbis->frames - vorbis->read, shorts / channels), 1024);

	memcpy(buffer, vorbis->samples + (long)vorbis->read * channels, (size_t)frames * channels * sizeof(short));
	vorbis->read += frames;
	return frames;
}
static void stb_vorbis_close(stb_vorbis *vorbis) { (void)vorbis; }

typedef int (*cache_file_read_proc)(void *context, uint32_t offset, uint32_t size, void *buffer);
struct cache_file_source { void *context; cache_file_read_proc read; uint32_t size; };
struct resource_map_item { uint32_t data_offset; uint32_t size; char const *name; };
struct resource_map { struct cache_file_source *source; int type; int32_t item_count; struct resource_map_item *items;
	char *names; };
enum { _resource_map_sounds = 2, _cache_file_status_ok = 0 };
static int resource_map_open(struct cache_file_source *source, int type, struct resource_map *map)
{ (void)source; (void)type; (void)map; return 1; }
static void resource_map_close(struct resource_map *map) { (void)map; }
static boolean custom_edition_resource_map_file_path(short type, char *path) { (void)type; (void)path; return FALSE; }
#define csstrcasecmp strcasecmp

/* the map's tags and the mods' memory */
static long tag_loaded(unsigned long group_tag, char const *name) { (void)group_tag; (void)name; return NONE; }
static char *tag_get_name(long tag_index) { (void)tag_index; return "sound\\test"; }
static unsigned long memory_left = 512UL * 1024 * 1024;
static unsigned long mods_memory_left(void) { return memory_left; }
/* the game's seeded random (real_math.c's: a linear congruential step) */
static short seed_random_range(unsigned long *seed, short lower, short upper)
{
	*seed = *seed * 1664525UL + 1013904223UL;
	return (short)(lower + (short)(((*seed >> 16) & 0xFFFF) * (unsigned long)(upper - lower) >> 16));
}
static real real_seed_random(unsigned long *seed)
{
	*seed = *seed * 1664525UL + 1013904223UL;
	return (real)((*seed >> 16) & 0xFFFF) / 65536.f;
}

static unsigned long current_seed;
static unsigned long *get_global_local_random_seed_address(void) { return &current_seed; }

#include "under_test.inc"

/* ---------- a sound tag file */

enum { MONO = 0, STEREO = 1, RATE_22 = 0, RATE_44 = 1, PCM = 0, OGG = 3 };
#define MOUTH_BYTES 30
#define MOST_PERMUTATIONS 8

/* a permutation of the file: PCM frames (`frames` of `channels`, native shorts), or a fake Ogg Vorbis stream */
struct source
{
	short *samples;
	long frames;
	long channels;
	unsigned int rate;
	short next;
};

static byte file_bytes[0x100000];
static unsigned long file_size;

static void put_be16(unsigned long at, unsigned value) { file_bytes[at] = (byte)(value >> 8); file_bytes[at + 1] = (byte)value; }
static void put_be32(unsigned long at, unsigned long value)
{
	put_be16(at, (unsigned)(value >> 16));
	put_be16(at + 2, (unsigned)(value & 0xFFFF));
}
static void put_real(unsigned long at, float value)
{
	uint32_t bits;

	memcpy(&bits, &value, sizeof(bits));
	put_be32(at, bits);
}

/* The tag file of a sound: `encoding`, its tag's rate, `compression`, linked or not, of one pitch range of `count`
permutations, `actual` of them picked; the first permutation has MOUTH_BYTES of mouth data. */
static void file_make(short encoding, short sample_rate, short compression, boolean linked, short actual,
	struct source const *sources, int count)
{
	unsigned long root = 0x40, range = root + 0xA4, permutations = range + 0x48, cursor;
	int index;

	CHECK(sizeof(struct sound_definition) == 0xA4 && sizeof(struct sound_pitch_range) == 0x48 &&
		sizeof(struct sound_permutation) == 0x7C, "not the game's 32-bit structures");
	memset(file_bytes, 0, permutations + (unsigned long)count * 0x7C);
	put_be32(MOD_TAG_FILE_GROUP_OFFSET, SOUND_DEFINITION_TAG);
	put_be32(MOD_TAG_FILE_HEADER_SIZE_OFFSET, MOD_TAG_FILE_HEADER_SIZE);
	put_be16(MOD_TAG_FILE_VERSION_OFFSET, SOUND_DEFINITION_VERSION);
	memcpy(file_bytes + MOD_TAG_FILE_SIGNATURE_OFFSET, "blam", 4);
	put_be32(root + offsetof(struct sound_definition, flags), linked ? 1UL << _sound_definition_linked_permutations_bit : 0);
	put_be16(root + offsetof(struct sound_definition, sound_class), 34);
	put_be16(root + offsetof(struct sound_definition, sample_rate), (unsigned)sample_rate);
	put_be16(root + offsetof(struct sound_definition, encoding), (unsigned)encoding);
	put_be16(root + offsetof(struct sound_definition, compression), (unsigned)compression);
	put_real(root + offsetof(struct sound_definition, random_pitch_bounds.lower), 1.f);
	put_real(root + offsetof(struct sound_definition, random_pitch_bounds.upper), 1.f);
	put_be32(root + offsetof(struct sound_definition, pitch_ranges.count), 1);
	memcpy(file_bytes + range, "default", 7);
	put_real(range + offsetof(struct sound_pitch_range, natural_pitch), 1.f);
	put_be16(range + offsetof(struct sound_pitch_range, actual_permutation_count), (unsigned)actual);
	put_be32(range + offsetof(struct sound_pitch_range, permutations.count), (unsigned long)count);
	cursor = permutations + (unsigned long)count * 0x7C;
	for (index = 0; index < count; index++)
	{
		unsigned long at = permutations + (unsigned long)index * 0x7C;
		struct source const *source = &sources[index];
		unsigned long bytes;

		sprintf((char *)file_bytes + at, "piece %d", index);
		put_real(at + offsetof(struct sound_permutation, gain), 1.f);
		put_be16(at + offsetof(struct sound_permutation, compression), (unsigned)compression);
		put_be16(at + offsetof(struct sound_permutation, next_permutation_index), (unsigned)(unsigned short)source->next);
		if (compression == OGG)
		{
			struct fake_vorbis vorbis = { source->samples, (int)source->frames, (int)source->channels, source->rate, 0 };

			bytes = sizeof(vorbis);
			memcpy(file_bytes + cursor, &vorbis, bytes);
			put_be32(at + offsetof(struct sound_permutation, sample_buffer_size),
				(unsigned long)source->frames * 2 * (unsigned long)source->channels);
		}
		else
		{
			long sample;

			bytes = (unsigned long)source->frames * (unsigned long)source->channels * 2;
			CHECK(cursor + bytes + MOUTH_BYTES <= sizeof(file_bytes), "the file is too small for the test");
			for (sample = 0; sample < source->frames * source->channels; sample++)
				put_be16(cursor + (unsigned long)sample * 2, (unsigned)(unsigned short)source->samples[sample]);
		}
		put_be32(at + offsetof(struct sound_permutation, samples.size), bytes);
		cursor += bytes;
		if (index == 0)
		{
			put_be32(at + offsetof(struct sound_permutation, mouth_data.size), MOUTH_BYTES);
			memset(file_bytes + cursor, 7, MOUTH_BYTES);
			cursor += MOUTH_BYTES;
		}
	}
	file_size = cursor;
}

/* the sound mod_sound_new makes of the file (a copy of it, which the sound keeps) */
static struct mod_sound *sound_load(void)
{
	byte *file = malloc(file_size);
	char const *reason = NULL;
	struct mod_sound *sound;

	CHECK(file, "no memory");
	memcpy(file, file_bytes, file_size);
	sound = mod_sound_new(1, file, file_size, &reason);
	CHECK(sound, "the sound was left out: %s", reason ? reason : "?");
	return sound;
}

/* frames of a sweep of `channels` (each channel its own), every sample distinct from its neighbours */
static short *sweep_make(long frames, long channels)
{
	short *samples = malloc((size_t)(frames * channels) * sizeof(short));
	long sample;

	CHECK(samples, "no memory");
	for (sample = 0; sample < frames * channels; sample++)
		samples[sample] = (short)(lround(12000.0 * sin(0.001 * (double)sample * (1.0 + sample % 7))) + sample % 13);
	return samples;
}

static struct sound_pitch_range *the_range(struct mod_sound *sound)
{
	CHECK(sound->definition.pitch_ranges.count == 1, "%ld pitch ranges", (long)sound->definition.pitch_ranges.count);
	return (struct sound_pitch_range *)sound->definition.pitch_ranges.address;
}

static struct sound_permutation *permutation_get(struct mod_sound *sound, short index)
{
	struct sound_pitch_range *range = the_range(sound);

	CHECK(VALID_INDEX(index, range->permutations.count), "permutation %d of %ld", index, (long)range->permutations.count);
	return TAG_BLOCK_GET_ELEMENT(&range->permutations, index, struct sound_permutation);
}

int main(void)
{
	{
		/* a mod's sound of 40 short permutations, not linked (OpenH1 issue 1170): kept, all 40 picked from, and
		each played once before any repeats, round after round (sound_definitions.c's played bits past 32) */
		struct source sources[40];
		short *samples = sweep_make(2205, 1);
		struct mod_sound *sound;
		current_seed = 3;
		short index, round, last = NONE;

		for (index = 0; index < 40; index++)
		{
			struct source source = { samples, 2205, 1, 22050, NONE };

			sources[index] = source;
		}
		file_make(MONO, RATE_22, PCM, FALSE, 40, sources, 40);
		sound = sound_load();
		CHECK(the_range(sound)->actual_permutation_count == 40, "%d picked from",
			the_range(sound)->actual_permutation_count);
		sound_definitions_played_beyond_clear();
		for (round = 0; round < 4; round++)
		{
			boolean seen[40] = { 0 };
			short count = round ? 39 : 40;

			for (index = 0; index < count; index++)
			{
				short pick = sound_definition_next_permutation(&sound->definition, 0, NONE);

				CHECK(VALID_INDEX(pick, 40) && !seen[pick] && pick != last,
					"round %d, pick %d: permutation %d again", round, index, pick);
				seen[pick] = TRUE;
				last = pick;
			}
		}
		mod_sound_free(sound);
		free(samples);
		sound_definitions_played_beyond_clear();
	}
	return 0;
}
