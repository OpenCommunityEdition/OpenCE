/*
MCC_IMPORT_SOUNDS.C

The sounds of a map the MCC import writes (mcc_import.c), taken from Halo:
The Master Chief Collection's FMOD banks as the Xbox disc's maps hold them
(.local/step4_sounds.md: the rules below were found there, on every sound of
the 24 maps).

MCC keeps Halo 1's sounds in banks (the data root's sound\pc\<bank>.fsb,
FSB5), each with an index of tag paths (sound\pc\lst\<bank>.lst.bin). Its
maps' sound tags still describe Halo PC's samples; MCC's load (mcc_sounds_
mute, cache_file_formats.c) leaves them without pitch ranges, which this
file gives back and fills:

- a tag's classic (2001) samples are under its own path, or the old_sfx or
  old_music one (the same-name entries of the music and stream banks are the
  remastered sounds, never used, nor is sounds_debug);
- a bank sample is one permutation, or a whole chain of a linked sound (its
  music, cut here into the disc's pieces of 65520 bytes);
- the dialogue bank holds a pitch range's takes sorted by name, the others in
  the tag's order; a sample far from the length MCC's tag gives its unit is
  given the unit of the nearest length;
- the effects bank is Xbox ADPCM already, copied; dialogue and music are
  CELT 0.11 (port/third_party/celt), decoded and encoded as Xbox ADPCM in the
  tag's format (custom_edition_sounds_encode_frames).

Not the disc's bytes for CELT sounds (it is lossy): the same recordings. A
sound whose samples are not found stays silent (5 of the 24 maps' 2889).
Desktop builds only: Android and the web import nothing.
*/

/* ---------- headers */

#include "cseries.h"
#include "cseries/cseries_windows.h"
#include "errors.h"
#include "tag_files/tag_groups.h"
#include "cache_file_formats.h"
#include "custom_edition_cache.h"
#include "mcc_import.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(HALO_ANDROID) && !defined(HALO_WEB)
#include "../../third_party/celt/celt_prefixed.h"
#include "../../third_party/celt/libcelt/celt.h"
#define MCC_IMPORT_SOUNDS
#endif

/* ---------- constants */

enum
{
	MCC_SOUND_BANK_COUNT = 4,
	MCC_SOUND_MAXIMUM_MUTED = 8192,
	MCC_SOUND_PIECE_BYTES = 65520,
	MCC_SOUND_CELT_FRAME = 512,
	/* the CELT decoder's delay: the frames before the sound (measured) */
	MCC_SOUND_CELT_SKIP = 128,
	MCC_SOUND_ADPCM_BLOCK_FRAMES = 64,
	MCC_SOUND_ADPCM_BLOCK_BYTES = 36,
	MCC_SOUND_MAXIMUM_FRAMES = 0x1000000,

	/* a sound (snd!) */
	SOUND_FLAGS_OFFSET = 0x00,
	SOUND_SAMPLE_RATE_OFFSET = 0x06,
	SOUND_ENCODING_OFFSET = 0x6C,
	SOUND_COMPRESSION_OFFSET = 0x6E,
	SOUND_LONGEST_OFFSET = 0x84,
	SOUND_PITCH_RANGES_OFFSET = 0x98,
	SOUND_FIT_TO_BLOCK_BIT = 0,
	SOUND_LINKED_BIT = 1,
	/* a pitch range */
	PITCH_RANGE_BYTES = 0x48,
	PITCH_RANGE_ACTUAL_COUNT_OFFSET = 0x2C,
	PITCH_RANGE_PERMUTATIONS_OFFSET = 0x3C,
	/* a permutation */
	PERMUTATION_BYTES = 0x7C,
	PERMUTATION_COMPRESSION_OFFSET = 0x28,
	PERMUTATION_NEXT_OFFSET = 0x2A,
	PERMUTATION_BUFFER_SIZE_OFFSET = 0x38,
	PERMUTATION_SAMPLES_OFFSET = 0x40,
	PERMUTATION_MOUTH_DATA_OFFSET = 0x54,
	PERMUTATION_SUBTITLES_OFFSET = 0x68,
	SOUND_COMPRESSION_XBOX_ADPCM = 1,

	FSB_MODE_ADPCM = 7,
	FSB_MODE_CELT = 12,
	CELT_SYNC = 0xF30DC317,
};

enum
{
	_bank_adpcm,
	_bank_dialogs,
	_bank_music,
	_bank_stream,
};

static char const *const mcc_sound_bank_names[MCC_SOUND_BANK_COUNT] =
{
	"sounds_adpcm", "sounds_dialogs", "sounds_music", "sounds_stream",
};

/* ---------- structures */

struct mcc_sound_sample
{
	long rate;
	short channels;
	unsigned long offset;
	unsigned long size;
	unsigned long frames;
};

struct mcc_sound_entry
{
	char const *name;
	long first;
	long count;
};

struct mcc_sound_bank
{
	FILE *file;
	long mode;
	unsigned long data_start;
	long sample_count;
	struct mcc_sound_sample *samples;
	char *names;
	struct mcc_sound_entry *entries;
	long entry_count;
	/* the entries by name (open addressing, a power of two) */
	long *slots;
	long slot_count;
};

/* a tag's units: each a bank sample and the permutations it is (one, or a
whole chain) */
struct mcc_sound_unit
{
	long sample;
	short pitch_range;
	short permutation_count;
	short permutations[256];
};

/* ---------- globals */

static struct
{
	boolean open;
	struct mcc_sound_bank banks[MCC_SOUND_BANK_COUNT];
	/* the sounds MCC's load muted, and their pitch range counts */
	byte *muted[MCC_SOUND_MAXIMUM_MUTED];
	long muted_counts[MCC_SOUND_MAXIMUM_MUTED];
	void *muted_addresses[MCC_SOUND_MAXIMUM_MUTED];
	long muted_count;
	/* the permutations made for a map, let go once it is written */
	void *made[MCC_SOUND_MAXIMUM_MUTED];
	long made_count;
	long converted;
	long silent;
} mcc_sounds;

/* ---------- private code */

#define mcc_sounds_log(...) (error(_error_silent, __VA_ARGS__), fprintf(stderr, __VA_ARGS__), fputc(10, stderr))

static unsigned long mcc_sounds_u32(
	byte const *bytes)
{
	return (unsigned long)bytes[0] | (unsigned long)bytes[1] << 8 | (unsigned long)bytes[2] << 16 |
		(unsigned long)bytes[3] << 24;
}

static unsigned long mcc_sounds_hash(
	char const *name)
{
	unsigned long hash = 5381;

	while (*name)
		hash = hash * 33 + (unsigned char)*name++;

	return hash;
}

static unsigned long mcc_sounds_length(
	char const *text)
{
	unsigned long length = 0;

	while (text[length])
		length++;

	return length;
}

/* the bank's index (sound\pc\lst\<bank>.lst.bin): u32 1, u32 count, then
each entry's length, its tag path and its first sample and sample count */
static boolean mcc_sounds_index_read(
	struct mcc_sound_bank *bank,
	char const *bank_name)
{
	char path[256];
	long size, at = 8, entry_index;
	byte *bytes;

	snprintf(path, sizeof(path), "%ssound\\pc\\lst\\%s.lst.bin", mcc_source_directory(), bank_name);
	bytes = (byte *)mcc_import_file_read(path, &size);
	if (!bytes || size < 8 || mcc_sounds_u32(bytes) != 1)
	{
		if (bytes)
			free(bytes);
		return FALSE;
	}
	bank->entry_count = (long)mcc_sounds_u32(bytes + 4);
	if (bank->entry_count <= 0 || bank->entry_count > 0x100000)
	{
		free(bytes);
		return FALSE;
	}
	bank->entries = malloc((size_t)bank->entry_count * sizeof(*bank->entries));
	for (bank->slot_count = 1; bank->slot_count < bank->entry_count * 2; bank->slot_count *= 2)
	{
	}
	bank->slots = malloc((size_t)bank->slot_count * sizeof(long));
	if (!bank->entries || !bank->slots)
	{
		free(bytes);
		return FALSE;
	}
	csmemset(bank->slots, 0xFF, (unsigned long)bank->slot_count * sizeof(long));
	for (entry_index = 0; entry_index < bank->entry_count; entry_index++)
	{
		unsigned long length;
		struct mcc_sound_entry *entry = &bank->entries[entry_index];
		long slot;

		if (at + 4 > size)
			break;
		length = mcc_sounds_u32(bytes + at);
		at += 4;
		if (length == 0 || length > (unsigned long)(size - at) || (unsigned long)(size - at) - length < 8)
			break;
		/* (the name ends within its length: the bytes are the file's, kept) */
		bytes[at + length - 1] = 0;
		entry->name = (char const *)bytes + at;
		at += (long)length;
		entry->first = (long)mcc_sounds_u32(bytes + at);
		entry->count = (long)mcc_sounds_u32(bytes + at + 4);
		at += 8;
		for (slot = (long)(mcc_sounds_hash(entry->name) & (unsigned long)(bank->slot_count - 1));
			bank->slots[slot] != NONE; slot = (slot + 1) & (bank->slot_count - 1))
		{
		}
		bank->slots[slot] = entry_index;
	}
	if (entry_index < bank->entry_count)
	{
		mcc_sounds_log("mcc import: the index of %s is damaged", bank_name);
		free(bytes);
		return FALSE;
	}
	bank->names = (char *)bytes;

	return TRUE;
}

static struct mcc_sound_entry const *mcc_sounds_entry(
	struct mcc_sound_bank const *bank,
	char const *name)
{
	long slot;

	if (!bank->slots)
		return NULL;
	for (slot = (long)(mcc_sounds_hash(name) & (unsigned long)(bank->slot_count - 1)); bank->slots[slot] != NONE;
		slot = (slot + 1) & (bank->slot_count - 1))
	{
		if (!strcmp(bank->entries[bank->slots[slot]].name, name))
			return &bank->entries[bank->slots[slot]];
	}

	return NULL;
}

/* the bank's samples (FSB5: a 0x3C header, a sample header each, its
names, its data) */
static boolean mcc_sounds_bank_read(
	struct mcc_sound_bank *bank,
	char const *bank_name)
{
	static long const rates[16] = { 0, 8000, 11000, 11025, 16000, 22050, 24000, 32000, 44100, 48000 };
	char path[256];
	byte header[0x3C];
	byte *headers;
	unsigned long header_bytes, name_bytes, data_bytes, at = 0;
	long sample_index;
	long file_size;

	snprintf(path, sizeof(path), "%ssound\\pc\\%s.fsb", mcc_source_directory(), bank_name);
	bank->file = fopen(path, "rb");
	if (!bank->file || fread(header, 1, sizeof(header), bank->file) != sizeof(header) || memcmp(header, "FSB5", 4))
		return FALSE;
	fseek(bank->file, 0, SEEK_END);
	file_size = ftell(bank->file);
	bank->sample_count = (long)mcc_sounds_u32(header + 8);
	header_bytes = mcc_sounds_u32(header + 12);
	name_bytes = mcc_sounds_u32(header + 16);
	data_bytes = mcc_sounds_u32(header + 20);
	bank->mode = (long)mcc_sounds_u32(header + 24);
	bank->data_start = 0x3C + header_bytes + name_bytes;
	if (bank->sample_count <= 0 || bank->sample_count > 0x100000 || header_bytes > 0x1000000 ||
		file_size < 0 || bank->data_start + data_bytes > (unsigned long)file_size)
	{
		return FALSE;
	}
	headers = malloc(header_bytes + 1);
	bank->samples = malloc((size_t)bank->sample_count * sizeof(*bank->samples));
	fseek(bank->file, 0x3C, SEEK_SET);
	if (!headers || !bank->samples || fread(headers, 1, header_bytes, bank->file) != header_bytes)
	{
		if (headers)
			free(headers);
		return FALSE;
	}
	for (sample_index = 0; sample_index < bank->sample_count; sample_index++)
	{
		struct mcc_sound_sample *sample = &bank->samples[sample_index];
		unsigned long low, high;
		boolean more;

		if (at + 8 > header_bytes)
			break;
		low = mcc_sounds_u32(headers + at);
		high = mcc_sounds_u32(headers + at + 4);
		at += 8;
		more = (low & 1) != 0;
		sample->rate = rates[(low >> 1) & 0xF];
		sample->channels = ((short const []){ 1, 2, 6, 8 })[(low >> 5) & 3];
		sample->offset = ((low >> 7) & 0x1FFFFFF) << 5 | ((high & 0x3) << 30);
		sample->frames = (high >> 2) & 0x3FFFFFFF;
		while (more)
		{
			unsigned long chunk, chunk_size;

			if (at + 4 > header_bytes)
				break;
			chunk = mcc_sounds_u32(headers + at);
			more = (chunk & 1) != 0;
			chunk_size = (chunk >> 1) & 0xFFFFFF;
			if (((chunk >> 25) & 0x7F) == 1 && at + 4 < header_bytes)
				sample->channels = headers[at + 4];
			at += 4 + chunk_size;
		}
	}
	free(headers);
	if (sample_index < bank->sample_count)
		return FALSE;
	for (sample_index = 0; sample_index < bank->sample_count; sample_index++)
	{
		unsigned long end = sample_index + 1 < bank->sample_count ? bank->samples[sample_index + 1].offset : data_bytes;

		bank->samples[sample_index].size = end >= bank->samples[sample_index].offset ?
			end - bank->samples[sample_index].offset : 0;
	}

	return mcc_sounds_index_read(bank, bank_name);
}

/* a bank sample's stored bytes, or NULL */
static byte *mcc_sounds_sample_bytes(
	struct mcc_sound_bank const *bank,
	struct mcc_sound_sample const *sample)
{
	byte *bytes;

	if (!sample->size || sample->size > 0x8000000)
		return NULL;
	bytes = malloc(sample->size);
	if (bytes && (fseek(bank->file, (long)(bank->data_start + sample->offset), SEEK_SET) ||
		fread(bytes, 1, sample->size, bank->file) != sample->size))
	{
		free(bytes);
		bytes = NULL;
	}

	return bytes;
}

#ifdef MCC_IMPORT_SOUNDS
/* a CELT sample's frames (its packets: a sync word, a size, the bytes), at
44.1 kHz, without the decoder's delay; NULL when it is not one of one or two
channels */
static short *mcc_sounds_celt_decode(
	byte const *bytes,
	unsigned long size,
	struct mcc_sound_sample const *sample,
	long *frame_count)
{
	CELTMode *mode;
	CELTDecoder *decoder;
	short *pcm;
	short frame[MCC_SOUND_CELT_FRAME * 2];
	unsigned long at = 0;
	long capacity, decoded = 0;
	int status;

	if (sample->channels < 1 || sample->channels > 2 || sample->frames > MCC_SOUND_MAXIMUM_FRAMES)
		return NULL;
	capacity = ((long)sample->frames + MCC_SOUND_CELT_FRAME - 1) / MCC_SOUND_CELT_FRAME * MCC_SOUND_CELT_FRAME +
		MCC_SOUND_CELT_FRAME;
	pcm = malloc((size_t)capacity * (size_t)sample->channels * sizeof(short));
	mode = celt_mode_create(44100, MCC_SOUND_CELT_FRAME, &status);
	if (!pcm || !mode || status != CELT_OK)
	{
		if (pcm)
			free(pcm);
		if (mode)
			celt_mode_destroy(mode);
		return NULL;
	}
	decoder = celt_decoder_create_custom(mode, sample->channels, &status);
	while (decoder && decoded + MCC_SOUND_CELT_FRAME <= capacity && at + 8 <= size &&
		mcc_sounds_u32(bytes + at) == CELT_SYNC)
	{
		unsigned long packet = mcc_sounds_u32(bytes + at + 4);

		if (packet > size - at - 8)
			break;
		if (celt_decode(decoder, bytes + at + 8, (int)packet, frame, MCC_SOUND_CELT_FRAME) != CELT_OK)
			csmemset(frame, 0, sizeof(frame));
		csmemcpy(pcm + decoded * sample->channels, frame,
			(unsigned long)MCC_SOUND_CELT_FRAME * sample->channels * sizeof(short));
		decoded += MCC_SOUND_CELT_FRAME;
		at += 8 + packet;
	}
	if (decoder)
		celt_decoder_destroy(decoder);
	celt_mode_destroy(mode);
	/* (the sample's own length: its last packet is padded; less the delay) */
	decoded = MIN(decoded, (long)sample->frames);
	if (decoded <= MCC_SOUND_CELT_SKIP)
	{
		free(pcm);
		return NULL;
	}
	memmove(pcm, pcm + MCC_SOUND_CELT_SKIP * sample->channels,
		(size_t)(decoded - MCC_SOUND_CELT_SKIP) * (size_t)sample->channels * sizeof(short));
	*frame_count = decoded - MCC_SOUND_CELT_SKIP;

	return pcm;
}
#endif

/* a bank sample as Xbox ADPCM of `channels` at `rate`: the game's
allocator's bytes, or NULL */
static byte *mcc_sounds_convert(
	struct mcc_sound_bank const *bank,
	long sample_index,
	short channels,
	long rate,
	unsigned long *converted_bytes,
	boolean *celt)
{
	struct mcc_sound_sample const *sample = &bank->samples[sample_index];
	byte *bytes = mcc_sounds_sample_bytes(bank, sample);
	byte *converted = NULL;

	*celt = FALSE;
	if (!bytes)
		return NULL;
	if (bank->mode == FSB_MODE_ADPCM && sample->channels == channels && sample->rate == rate)
	{
		/* (the bank pads each sample: its frames give its blocks) */
		unsigned long size = (sample->frames + MCC_SOUND_ADPCM_BLOCK_FRAMES - 1) / MCC_SOUND_ADPCM_BLOCK_FRAMES *
			MCC_SOUND_ADPCM_BLOCK_BYTES * (unsigned long)channels;

		if (size <= sample->size)
		{
			converted = bytes;
			bytes = NULL;
			*converted_bytes = size;
		}
	}
#ifdef MCC_IMPORT_SOUNDS
	else if (bank->mode == FSB_MODE_CELT)
	{
		long frame_count = 0;
		short *pcm = mcc_sounds_celt_decode(bytes, sample->size, sample, &frame_count);

		if (pcm)
		{
			converted = custom_edition_sounds_encode_frames(pcm, frame_count, sample->channels, sample->rate,
				channels, rate, converted_bytes);
			free(pcm);
			*celt = TRUE;
		}
	}
#endif
	if (bytes)
		free(bytes);

	return converted;
}

/* the (bank, index name) pairs that may hold a tag's classic samples, best
first */
static short mcc_sounds_classic_names(
	char const *tag_name,
	short *banks,
	char names[][300])
{
	short count = 0;
	unsigned long length = mcc_sounds_length(tag_name);

	if (!strncmp(tag_name, "sound\\sfx\\", 10))
	{
		banks[count] = _bank_adpcm;
		snprintf(names[count++], 300, "sound\\old_sfx\\%s", tag_name + 10);
	}
	if (!strncmp(tag_name, "sound\\", 6))
	{
		banks[count] = _bank_music;
		snprintf(names[count++], 300, "sound\\old_music\\%s", tag_name + 6);
		banks[count] = _bank_stream;
		snprintf(names[count++], 300, "sound\\old_music\\%s", tag_name + 6);
	}
	if (!strncmp(tag_name, "sound\\dialog\\", 13))
	{
		banks[count] = _bank_dialogs;
		snprintf(names[count++], 300, "%s", tag_name);
	}
	if (!strncmp(tag_name, "sound\\sinomatixx_foley\\", 23) ||
		(!strncmp(tag_name, "sound\\sinomatixx\\", 17) && (length < 6 || strcmp(tag_name + length - 6, "_music"))))
	{
		banks[count] = _bank_stream;
		snprintf(names[count++], 300, "%s", tag_name);
	}

	return count;
}

static byte *mcc_sounds_pitch_range(
	byte *sound,
	long index)
{
	return (byte *)((struct tag_block *)(sound + SOUND_PITCH_RANGES_OFFSET))->address + index * PITCH_RANGE_BYTES;
}

static byte *mcc_sounds_permutation(
	byte *pitch_range,
	long index)
{
	return (byte *)((struct tag_block *)(pitch_range + PITCH_RANGE_PERMUTATIONS_OFFSET))->address +
		index * PERMUTATION_BYTES;
}

static long mcc_sounds_permutation_count(
	byte *pitch_range)
{
	return ((struct tag_block *)(pitch_range + PITCH_RANGE_PERMUTATIONS_OFFSET))->count;
}

/* each chain of a pitch range: its permutations, from a head below the
actual count along next_permutation_index */
static short mcc_sounds_chain(
	byte *pitch_range,
	short head,
	short *permutations)
{
	long count = mcc_sounds_permutation_count(pitch_range);
	short length = 0, index = head;

	while (index >= 0 && index < count && length < 256)
	{
		permutations[length++] = index;
		index = *(short *)(mcc_sounds_permutation(pitch_range, index) + PERMUTATION_NEXT_OFFSET);
	}

	return length;
}

static short mcc_sounds_chain_count(
	byte *pitch_range)
{
	short actual = *(short *)(pitch_range + PITCH_RANGE_ACTUAL_COUNT_OFFSET);

	return mcc_sounds_permutation_count(pitch_range) ? MAX(actual, 1) : 0;
}

/* sorted as the dialogue bank sorts a range's takes: by name as a file
name ("<name>.", lower case), then index */
static int mcc_sounds_name_compare(
	byte *pitch_range,
	short a,
	short b)
{
	char keys[2][34];
	short which, at;
	int order;

	for (which = 0; which < 2; which++)
	{
		char const *name = (char const *)mcc_sounds_permutation(pitch_range, which ? b : a);

		for (at = 0; at < 32 && name[at]; at++)
			keys[which][at] = (char)(name[at] >= 'A' && name[at] <= 'Z' ? name[at] + ('a' - 'A') : name[at]);
		keys[which][at++] = '.';
		keys[which][at] = 0;
	}
	order = strcmp(keys[0], keys[1]);

	return order ? order : a - b;
}

/* The tag's units in the bank's order: one a chain (when the bank has a
sample per chain) or a permutation (one per permutation); 0 when neither
fits. */
static long mcc_sounds_units(
	byte *sound,
	long sample_count,
	boolean by_name,
	struct mcc_sound_unit *units,
	long maximum,
	boolean *per_chain_out)
{
	long pitch_range_count = ((struct tag_block *)(sound + SOUND_PITCH_RANGES_OFFSET))->count;
	long chains = 0, permutations = 0, count = 0, range;
	boolean per_chain;

	for (range = 0; range < pitch_range_count; range++)
	{
		chains += mcc_sounds_chain_count(mcc_sounds_pitch_range(sound, range));
		permutations += mcc_sounds_permutation_count(mcc_sounds_pitch_range(sound, range));
	}
	if (sample_count != chains && sample_count != permutations)
		return 0;
	per_chain = sample_count == chains;
	*per_chain_out = per_chain;
	for (range = 0; range < pitch_range_count; range++)
	{
		byte *pitch_range = mcc_sounds_pitch_range(sound, range);
		short heads[256];
		short head_count = per_chain ? mcc_sounds_chain_count(pitch_range) :
			(short)MIN(mcc_sounds_permutation_count(pitch_range), 256);
		short i, j;

		for (i = 0; i < head_count; i++)
			heads[i] = i;
		/* (an insertion sort: a range has few) */
		for (i = 1; by_name && i < head_count; i++)
		{
			short head = heads[i];

			for (j = i; j > 0 && mcc_sounds_name_compare(pitch_range, heads[j - 1], head) > 0; j--)
				heads[j] = heads[j - 1];
			heads[j] = head;
		}
		for (i = 0; i < head_count && count < maximum; i++, count++)
		{
			units[count].pitch_range = (short)range;
			if (per_chain)
			{
				units[count].permutation_count = mcc_sounds_chain(pitch_range, heads[i], units[count].permutations);
			}
			else
			{
				units[count].permutation_count = 1;
				units[count].permutations[0] = heads[i];
			}
		}
	}

	return count == sample_count ? count : 0;
}

/* where a sample's length is far from its unit's (Halo PC's buffer sizes,
in MCC's tag), each sample of the range takes the unit of the nearest
length */
static void mcc_sounds_match_lengths(
	byte *sound,
	struct mcc_sound_bank const *bank,
	long const *samples,
	struct mcc_sound_unit *units,
	long unit_count)
{
	short channels = *(short *)(sound + SOUND_ENCODING_OFFSET) == 1 ? 2 : 1;
	long tag_rate = *(short *)(sound + SOUND_SAMPLE_RATE_OFFSET) == 1 ? 44100 : 22050;
	double expected[1024];
	boolean far = FALSE, taken[1024];
	long unit, other;

	if (unit_count > 1024)
		return;
	for (unit = 0; unit < unit_count; unit++)
	{
		byte *pitch_range = mcc_sounds_pitch_range(sound, units[unit].pitch_range);
		double bytes = 0.0;
		double frames = (double)bank->samples[samples[unit]].frames;
		short k;

		for (k = 0; k < units[unit].permutation_count; k++)
		{
			unsigned long buffer = *(unsigned long *)(mcc_sounds_permutation(pitch_range, units[unit].permutations[k]) +
				PERMUTATION_BUFFER_SIZE_OFFSET);

			if (!buffer)
				return;
			bytes += (double)buffer;
		}
		expected[unit] = bytes / (2.0 * channels) * bank->samples[samples[0]].rate / tag_rate;
		if (frames - expected[unit] > MAX(1500.0, 0.02 * expected[unit]) ||
			expected[unit] - frames > MAX(1500.0, 0.02 * expected[unit]))
		{
			far = TRUE;
		}
	}
	if (!far)
		return;
	/* (each range on its own: the longest sample first takes the nearest
	free unit) */
	{
		struct mcc_sound_unit fixed[1024];
		long order[1024];

		for (unit = 0; unit < unit_count; unit++)
		{
			taken[unit] = FALSE;
			order[unit] = unit;
		}
		for (unit = 1; unit < unit_count; unit++)
		{
			long slot = order[unit], j;

			for (j = unit; j > 0 && bank->samples[samples[order[j - 1]]].frames < bank->samples[samples[slot]].frames; j--)
				order[j] = order[j - 1];
			order[j] = slot;
		}
		for (unit = 0; unit < unit_count; unit++)
		{
			long slot = order[unit], best = NONE;
			double frames = (double)bank->samples[samples[slot]].frames;

			for (other = 0; other < unit_count; other++)
			{
				double distance, best_distance;

				if (taken[other] || units[other].pitch_range != units[slot].pitch_range)
					continue;
				distance = expected[other] > frames ? expected[other] - frames : frames - expected[other];
				best_distance = best == NONE ? 0.0 :
					(expected[best] > frames ? expected[best] - frames : frames - expected[best]);
				if (best == NONE || distance < best_distance)
					best = other;
			}
			if (best != NONE)
			{
				taken[best] = TRUE;
				fixed[slot] = units[best];
			}
			else
			{
				fixed[slot] = units[slot];
			}
		}
		csmemcpy(units, fixed, (unsigned long)unit_count * sizeof(*units));
	}

	return;
}

/* a permutation's samples set: Xbox ADPCM at `file_offset` */
static void mcc_sounds_permutation_set(
	byte *permutation,
	unsigned long size,
	unsigned long file_offset)
{
	struct tag_data *samples = (struct tag_data *)(permutation + PERMUTATION_SAMPLES_OFFSET);

	*(short *)(permutation + PERMUTATION_COMPRESSION_OFFSET) = SOUND_COMPRESSION_XBOX_ADPCM;
	*(unsigned long *)(permutation + PERMUTATION_BUFFER_SIZE_OFFSET) = 0;
	samples->size = (long)size;
	samples->pad = 0;
	samples->file_offset = (long)file_offset;
	samples->address = NULL;
	samples->definition = NULL;

	return;
}

/* one sound's samples, written; FALSE when it stays silent */
static boolean mcc_sounds_sound(
	byte *sound,
	char const *tag_name,
	long pitch_range_count,
	void *pitch_ranges_address,
	mcc_import_sound_write_proc write,
	void *context)
{
	static struct mcc_sound_unit units[1024];
	static long samples[1024];
	short banks[6];
	char names[6][300];
	short name_count = mcc_sounds_classic_names(tag_name, banks, names), name_index;
	struct mcc_sound_bank const *bank = NULL;
	struct tag_block *pitch_ranges = (struct tag_block *)(sound + SOUND_PITCH_RANGES_OFFSET);
	short channels = *(short *)(sound + SOUND_ENCODING_OFFSET) == 1 ? 2 : 1;
	long rate = channels == 1 || *(short *)(sound + SOUND_SAMPLE_RATE_OFFSET) == 0 ? 22050 : 44100;
	boolean linked = TEST_FLAG(*(unsigned long *)(sound + SOUND_FLAGS_OFFSET), SOUND_LINKED_BIT);
	long sample_count = 0, unit_count, unit, range;
	unsigned long longest_frames = 0;
	boolean any_celt = FALSE, whole_chains, per_chain = FALSE;

	pitch_ranges->count = pitch_range_count;
	pitch_ranges->address = pitch_ranges_address;
	if (!pitch_ranges_address)
	{
		pitch_ranges->count = 0;
		return FALSE;
	}
	/* the bank entries: the tag's, or one per pitch range named after it */
	for (name_index = 0; name_index < name_count && !sample_count; name_index++)
	{
		struct mcc_sound_bank const *candidate = &mcc_sounds.banks[banks[name_index]];
		struct mcc_sound_entry const *entry = mcc_sounds_entry(candidate, names[name_index]);

		if (entry)
		{
			for (unit = 0; unit < entry->count && unit < 1024; unit++)
				samples[unit] = entry->first + unit;
			sample_count = MIN(entry->count, 1024);
			bank = candidate;
		}
		else if (pitch_range_count > 0)
		{
			for (range = 0; range < pitch_range_count; range++)
			{
				char name[340];

				snprintf(name, sizeof(name), "%s\\%s", names[name_index], (char const *)mcc_sounds_pitch_range(sound, range));
				entry = mcc_sounds_entry(candidate, name);
				if (!entry)
					break;
				for (unit = 0; unit < entry->count && sample_count < 1024; unit++)
					samples[sample_count++] = entry->first + unit;
			}
			if (range < pitch_range_count)
				sample_count = 0;
			else
				bank = candidate;
		}
	}
	for (unit = 0; unit < sample_count; unit++)
	{
		if (samples[unit] < 0 || samples[unit] >= bank->sample_count)
			sample_count = 0;
	}
	unit_count = sample_count ? mcc_sounds_units(sound, sample_count, bank == &mcc_sounds.banks[_bank_dialogs], units, 1024,
		&per_chain) : 0;
	if (!unit_count)
	{
		mcc_sounds_log("mcc import: %s: %s", tag_name, sample_count ? "the bank's samples are neither its chains nor its permutations" :
			"no classic samples in MCC's banks: silent");
		pitch_ranges->count = 0;
		return FALSE;
	}
	if (bank == &mcc_sounds.banks[_bank_dialogs])
		mcc_sounds_match_lengths(sound, bank, samples, units, unit_count);
	/* a linked sound whose samples are whole chains is cut into the disc's
	pieces, its permutations made again: each range's heads first, then
	each chain's pieces */
	/* (a chain's sample is a whole variant: a linked sound's, or one of
	several permutations) */
	whole_chains = linked && per_chain;
	for (unit = 0; unit < unit_count; unit++)
	{
		if (units[unit].permutation_count > 1)
			whole_chains = TRUE;
	}
	for (range = 0; range < pitch_range_count; range++)
	{
		byte *pitch_range = mcc_sounds_pitch_range(sound, range);
		struct tag_block *permutations = (struct tag_block *)(pitch_range + PITCH_RANGE_PERMUTATIONS_OFFSET);
		short heads = mcc_sounds_chain_count(pitch_range);
		byte *made = NULL;
		long made_count = 0, made_maximum = 0;

		for (unit = 0; unit < unit_count; unit++)
		{
			struct mcc_sound_unit const *u = &units[unit];
			unsigned long size = 0, at;
			boolean celt = FALSE;
			byte *bytes;

			if (u->pitch_range != range)
				continue;
			bytes = mcc_sounds_convert(bank, samples[unit], channels, rate, &size, &celt);
			if (!bytes)
			{
				mcc_sounds_log("mcc import: %s: a sample cannot be converted: silent", tag_name);
				pitch_ranges->count = 0;
				if (made)
					free(made);
				return FALSE;
			}
			any_celt = any_celt || celt;
			if (!whole_chains)
			{
				byte *permutation = mcc_sounds_permutation(pitch_range, u->permutations[0]);
				unsigned long frames = size / (MCC_SOUND_ADPCM_BLOCK_BYTES * channels) * MCC_SOUND_ADPCM_BLOCK_FRAMES;

				mcc_sounds_permutation_set(permutation, size, write(context, bytes, size));
				longest_frames = MAX(longest_frames, frames);
			}
			else
			{
				long pieces = (long)((size + MCC_SOUND_PIECE_BYTES - 1) / MCC_SOUND_PIECE_BYTES), piece;
				byte const *head = mcc_sounds_permutation(pitch_range, u->permutations[0]);
				short head_index = u->permutations[0];

				if (!made)
				{
					/* (the heads first, as MCC's tag has them, then the
					pieces) */
					made_maximum = heads + 64;
					made = malloc((size_t)made_maximum * PERMUTATION_BYTES);
					if (made)
						csmemcpy(made, mcc_sounds_permutation(pitch_range, 0), (unsigned long)heads * PERMUTATION_BYTES);
					made_count = heads;
				}
				if (made && made_count + pieces > made_maximum)
				{
					byte *grown;

					made_maximum = made_count + pieces + 64;
					grown = realloc(made, (size_t)made_maximum * PERMUTATION_BYTES);
					/* (a failed realloc leaves the old block, freed below) */
					if (!grown)
						free(made);
					made = grown;
				}
				if (!made || head_index >= heads || made_count + pieces > 0x7FFF)
				{
					free(bytes);
					if (made)
						free(made);
					pitch_ranges->count = 0;
					mcc_sounds_log("mcc import: %s: its chains cannot be made: silent", tag_name);
					return FALSE;
				}
				for (piece = 0; piece < pieces; piece++)
				{
					long index = piece == 0 ? head_index : made_count++;
					byte *permutation = made + index * PERMUTATION_BYTES;
					unsigned long piece_size = MIN(MCC_SOUND_PIECE_BYTES, size - (unsigned long)piece * MCC_SOUND_PIECE_BYTES);

					csmemcpy(permutation, head, PERMUTATION_BYTES);
					if (piece > 0)
					{
						/* (the lip sync and subtitles are the chain's head's) */
						csmemset(permutation + PERMUTATION_MOUTH_DATA_OFFSET, 0, sizeof(struct tag_data));
						csmemset(permutation + PERMUTATION_SUBTITLES_OFFSET, 0, sizeof(struct tag_data));
					}
					at = write(context, bytes + (unsigned long)piece * MCC_SOUND_PIECE_BYTES, piece_size);
					mcc_sounds_permutation_set(permutation, piece_size, at);
					*(short *)(permutation + PERMUTATION_NEXT_OFFSET) = (short)(piece + 1 < pieces ? made_count : NONE);
					longest_frames = MAX(longest_frames,
						piece_size / (MCC_SOUND_ADPCM_BLOCK_BYTES * channels) * MCC_SOUND_ADPCM_BLOCK_FRAMES);
				}
			}
			free(bytes);
		}
		if (made)
		{
			permutations->count = made_count;
			permutations->address = made;
			permutations->definition = NULL;
			*(short *)(pitch_range + PITCH_RANGE_ACTUAL_COUNT_OFFSET) = heads;
			if (mcc_sounds.made_count < MCC_SOUND_MAXIMUM_MUTED)
				mcc_sounds.made[mcc_sounds.made_count++] = made;
		}
	}
	*(short *)(sound + SOUND_COMPRESSION_OFFSET) = SOUND_COMPRESSION_XBOX_ADPCM;
	if (whole_chains)
		*(unsigned long *)(sound + SOUND_FLAGS_OFFSET) |= FLAG(SOUND_FIT_TO_BLOCK_BIT);
	/* (the longest single permutation, in milliseconds times 1.125, as the
	disc's tools; an effect's is MCC's already) */
	if (any_celt)
		*(unsigned long *)(sound + SOUND_LONGEST_OFFSET) = (unsigned long)((double)longest_frames * 1125.0 / rate);

	return TRUE;
}

/* ---------- public code */

void mcc_import_sounds_muted(
	unsigned char *sound,
	long pitch_range_count)
{
	if (mcc_sounds.muted_count < MCC_SOUND_MAXIMUM_MUTED)
	{
		/* (the block's address too: the conversion after the load clears
		an empty block's) */
		mcc_sounds.muted[mcc_sounds.muted_count] = sound;
		mcc_sounds.muted_addresses[mcc_sounds.muted_count] =
			((struct tag_block *)(sound + SOUND_PITCH_RANGES_OFFSET))->address;
		mcc_sounds.muted_counts[mcc_sounds.muted_count++] = pitch_range_count;
	}

	return;
}

boolean mcc_import_sounds_open(
	void)
{
	short bank_index;

	if (mcc_sounds.open)
		return TRUE;
#ifdef MCC_IMPORT_SOUNDS
	for (bank_index = 0; bank_index < MCC_SOUND_BANK_COUNT; bank_index++)
	{
		if (!mcc_sounds_bank_read(&mcc_sounds.banks[bank_index], mcc_sound_bank_names[bank_index]))
		{
			mcc_sounds_log("mcc import: MCC's sound bank %s cannot be read (sound\\pc): the maps' sounds are silent",
				mcc_sound_bank_names[bank_index]);
			mcc_import_sounds_close();
			return FALSE;
		}
	}
	mcc_sounds.open = TRUE;
#else
	(void)bank_index;
#endif

	return mcc_sounds.open;
}

void mcc_import_sounds_close(
	void)
{
	short bank_index;

	for (bank_index = 0; bank_index < MCC_SOUND_BANK_COUNT; bank_index++)
	{
		struct mcc_sound_bank *bank = &mcc_sounds.banks[bank_index];

		if (bank->file)
			fclose(bank->file);
		if (bank->samples)
			free(bank->samples);
		if (bank->names)
			free(bank->names);
		if (bank->entries)
			free(bank->entries);
		if (bank->slots)
			free(bank->slots);
		csmemset(bank, 0, sizeof(*bank));
	}
	mcc_sounds.open = FALSE;

	return;
}

long mcc_import_sounds_write(
	struct mcc_import_tags *tags,
	long const *old_to_new,
	mcc_import_sound_write_proc write,
	void *context)
{
	long tag_index, written = 0;

	mcc_sounds.silent = 0;
	for (tag_index = 0; tag_index < tags->count; tag_index++)
	{
		struct mcc_import_tag_instance *instance = &tags->instances[tag_index];
		long muted;

		if (instance->group_tag != 'snd!' || !instance->base_address || old_to_new[tag_index] == NONE)
			continue;
		for (muted = 0; muted < mcc_sounds.muted_count; muted++)
		{
			if (mcc_sounds.muted[muted] == instance->base_address)
				break;
		}
		if (muted == mcc_sounds.muted_count)
			continue;
		if (mcc_sounds.open &&
			mcc_sounds_sound(instance->base_address, instance->name, mcc_sounds.muted_counts[muted],
				mcc_sounds.muted_addresses[muted], write, context))
		{
			written++;
		}
		else
		{
			mcc_sounds.silent++;
		}
	}

	return written;
}

long mcc_import_sounds_silent(
	void)
{
	return mcc_sounds.silent;
}

void mcc_import_sounds_map_done(
	void)
{
	long index;

	for (index = 0; index < mcc_sounds.made_count; index++)
		free(mcc_sounds.made[index]);
	mcc_sounds.made_count = 0;
	mcc_sounds.muted_count = 0;

	return;
}
