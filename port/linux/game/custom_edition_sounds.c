/*
CUSTOM_EDITION_SOUNDS.C

The sounds of Halo Custom Edition maps this build would not play
(custom_edition_cache.h).

This build plays Xbox ADPCM only, mono at 22 kHz or stereo at 22 or 44 kHz
(sound_manager.c), and refuses any other sound each time it is played.
Halo PC also has Ogg Vorbis (music, dialogue, the announcer), uncompressed
samples and mono sounds at 44 kHz. When a map is loaded, each such
permutation is decoded here (Ogg Vorbis with stb_vorbis.c, public domain),
brought to the sound's channel count at a rate this build plays, and
encoded again as Xbox ADPCM into one buffer kept for as long as the map is
loaded. The permutation is then an Xbox ADPCM one whose samples lie in a
region of the combined offset space of its own (custom_edition_cache_read
serves it from the buffer), so the sound cache and the mixer see nothing
new.

Xbox ADPCM is IMA ADPCM in blocks of 64 samples per channel: a 4-byte
header per channel (the predictor and step index the block starts from),
then 4-byte groups of eight nibbles, low nibble first, alternating between
the channels (port/linux/src/dsound_sdl.c decodes it).
*/

/* ---------- headers */

#include "cseries.h"
#include "errors.h"
#include "tag_files/tag_groups.h"
#include "sound/sound_definitions.h"
#include "cache_file_formats.h"
#include "custom_edition_cache.h"

#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

#include <stdlib.h>
#include <string.h>

/* ---------- constants */

#define SOUND_COMPRESSION_NONE 0
#define SOUND_COMPRESSION_XBOX_ADPCM 1
#define SOUND_COMPRESSION_OGG_VORBIS 3

#define ADPCM_BLOCK_SAMPLES 64
#define ADPCM_BLOCK_BYTES 36
#define ADPCM_STEP_INDEX_MAXIMUM 88

/* the decoded buffer grows by this much at a time */
#define DECODED_GROWTH 0x100000

/* an Ogg Vorbis stream is decoded into a buffer that grows by this many
frames; no packet decodes to more than 4096 frames */
#define VORBIS_GROWTH_FRAMES 4096
/* The most frames a permutation has at any stage (decoded, or brought to
the rate this build plays): over six minutes at 44 kHz, and 64 MB of
stereo samples. Every buffer of frames is sized from a count within it
(frames_allocate), so that count * channels * 2 cannot wrap the 32-bit
size_t of this build: a stream that said it was at 1 Hz once made a count
whose bytes wrapped to a few KB, which the resampler then wrote 4 GB into */
#define MAXIMUM_FRAMES 0x1000000L
/* the highest rate a stream is taken to be at (its own rate is read from
it, and the resampler scales by it) */
#define MAXIMUM_SAMPLE_RATE 192000

/* ---------- globals */

static int const adpcm_step_table[ADPCM_STEP_INDEX_MAXIMUM + 1] =
{
	7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
	50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230,
	253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963,
	1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327,
	3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442,
	11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
	32767,
};

static int const adpcm_index_table[8] = { -1, -1, -1, -1, 2, 4, 6, 8 };

static struct
{
	byte *decoded;
	unsigned long decoded_bytes;
	unsigned long decoded_capacity;
	/* the most the region of the combined offset space for them holds */
	unsigned long decoded_limit;
} custom_edition_sounds_globals;

/* ---------- private code */

struct adpcm_state
{
	int predictor;
	int step_index;
};

/* The nibble that brings `state` nearest `sample`, moving it as the decoder will. */
static byte adpcm_encode_sample(
	struct adpcm_state *state,
	int sample)
{
	int step = adpcm_step_table[state->step_index];
	int difference = sample - state->predictor;
	int change = step >> 3;
	byte nibble = 0;

	if (difference < 0)
	{
		nibble = 8;
		difference = -difference;
	}
	if (difference >= step)
	{
		nibble |= 4;
		difference -= step;
		change += step;
	}
	if (difference >= step >> 1)
	{
		nibble |= 2;
		difference -= step >> 1;
		change += step >> 1;
	}
	if (difference >= step >> 2)
	{
		nibble |= 1;
		change += step >> 2;
	}

	state->predictor += (nibble & 8) ? -change : change;
	state->predictor = PIN(state->predictor, -32768, 32767);
	state->step_index += adpcm_index_table[nibble & 7];
	state->step_index = PIN(state->step_index, 0, ADPCM_STEP_INDEX_MAXIMUM);

	return nibble;
}

/* Encodes `frame_count` frames of interleaved 16-bit samples as Xbox ADPCM into
`output`, which holds adpcm_encoded_bytes(frame_count, channel_count) bytes.
A last block short of 64 frames is filled with the final sample. */
static unsigned long adpcm_encoded_bytes(
	long frame_count,
	long channel_count)
{
	return (unsigned long)((frame_count + ADPCM_BLOCK_SAMPLES - 1) / ADPCM_BLOCK_SAMPLES) *
		ADPCM_BLOCK_BYTES * channel_count;
}

static void adpcm_encode(
	short const *samples,
	long frame_count,
	long channel_count,
	byte *output)
{
	struct adpcm_state states[2] = { { 0, 0 }, { 0, 0 } };
	long block_count = (frame_count + ADPCM_BLOCK_SAMPLES - 1) / ADPCM_BLOCK_SAMPLES;
	long block_index;

	for (block_index = 0; block_index < block_count; block_index++)
	{
		byte *block = output + block_index * ADPCM_BLOCK_BYTES * channel_count;
		byte *nibbles = block + 4 * channel_count;
		long channel;

		for (channel = 0; channel < channel_count; channel++)
		{
			struct adpcm_state *state = &states[channel];
			byte *header = block + channel * 4;
			long group;

			header[0] = (byte)(state->predictor & 0xFF);
			header[1] = (byte)((state->predictor >> 8) & 0xFF);
			header[2] = (byte)state->step_index;
			header[3] = 0;
			for (group = 0; group < 8; group++)
			{
				byte *group_bytes = nibbles + (group * channel_count + channel) * 4;
				long byte_index;

				for (byte_index = 0; byte_index < 4; byte_index++)
				{
					long frame = block_index * ADPCM_BLOCK_SAMPLES + group * 8 + byte_index * 2;
					long first = MIN(frame, frame_count - 1);
					long second = MIN(frame + 1, frame_count - 1);
					byte low = adpcm_encode_sample(state, samples[first * channel_count + channel]);
					byte high = adpcm_encode_sample(state, samples[second * channel_count + channel]);

					group_bytes[byte_index] = (byte)(low | (high << 4));
				}
			}
		}
	}

	return;
}

/* 16-bit frames: their samples, channel count and rate */
struct frames
{
	short *samples;
	long count;
	long channels;
	long rate;
};

static void frames_free(
	struct frames *frames)
{
	free(frames->samples);
	frames->samples = NULL;
}

/* room for `count` frames of `channels` channels, or NULL when there are
none, too many (MAXIMUM_FRAMES), or no memory: the only way a buffer of
frames is made */
static short *frames_allocate(
	long count,
	long channels)
{
	if (count <= 0 || count > MAXIMUM_FRAMES || channels < 1 || channels > 2)
	{
		return NULL;
	}

	return malloc((size_t)count * (size_t)channels * sizeof(short));
}

/* the Ogg Vorbis stream `data`, at its own channel count and rate; FALSE
when it cannot be decoded. It is decoded a packet at a time, so a stream
of more channels than a sound has, or one longer than any sound, is
refused as soon as that shows, before it has used the memory. */
static boolean vorbis_decode(
	byte const *data,
	long data_bytes,
	struct frames *frames)
{
	int error = 0;
	stb_vorbis *vorbis = stb_vorbis_open_memory(data, (int)data_bytes, &error, NULL);
	stb_vorbis_info info;
	long capacity;
	boolean decoded = FALSE;

	if (!vorbis)
	{
		return FALSE;
	}
	info = stb_vorbis_get_info(vorbis);
	frames->count = 0;
	frames->channels = info.channels;
	frames->rate = (long)info.sample_rate;
	capacity = VORBIS_GROWTH_FRAMES;
	frames->samples = NULL;
	if (info.channels >= 1 && info.channels <= 2 && info.sample_rate > 0 && info.sample_rate <= MAXIMUM_SAMPLE_RATE)
	{
		frames->samples = frames_allocate(capacity, info.channels);
	}
	while (frames->samples)
	{
		int count;

		if (frames->count + VORBIS_GROWTH_FRAMES > capacity)
		{
			short *larger;

			/* (the limit is a power of two, so doubling reaches it exactly) */
			if (capacity >= MAXIMUM_FRAMES)
			{
				break;
			}
			larger = realloc(frames->samples, (size_t)capacity * 2 * (size_t)info.channels * sizeof(short));
			if (!larger)
			{
				break;
			}
			frames->samples = larger;
			capacity *= 2;
		}
		count = stb_vorbis_get_frame_short_interleaved(vorbis, info.channels,
			frames->samples + frames->count * info.channels,
			(int)((capacity - frames->count) * info.channels));
		if (count == 0)
		{
			decoded = frames->count > 0;
			break;
		}
		frames->count += count;
	}
	stb_vorbis_close(vorbis);
	if (!decoded)
	{
		frames_free(frames);
	}

	return decoded;
}

/* uncompressed samples, 16-bit little-endian as Halo PC keeps them */
static boolean pcm_decode(
	byte const *data,
	long data_bytes,
	long channels,
	long rate,
	struct frames *frames)
{
	long index;

	frames->count = data_bytes / (2 * channels);
	frames->channels = channels;
	frames->rate = rate;
	frames->samples = frames_allocate(frames->count, channels);
	for (index = 0; frames->samples && index < frames->count * channels; index++)
	{
		frames->samples[index] = (short)(data[2 * index] | (data[2 * index + 1] << 8));
	}

	return frames->samples != NULL;
}

/* Xbox ADPCM, as the mixer decodes it (port/linux/src/dsound_sdl.c) */
static int adpcm_decode_nibble(
	struct adpcm_state *state,
	int nibble)
{
	int step = adpcm_step_table[state->step_index];
	int difference = step >> 3;

	if (nibble & 1)
		difference += step >> 2;
	if (nibble & 2)
		difference += step >> 1;
	if (nibble & 4)
		difference += step;
	state->predictor += (nibble & 8) ? -difference : difference;
	state->predictor = PIN(state->predictor, -32768, 32767);
	state->step_index = PIN(state->step_index + adpcm_index_table[nibble & 7], 0, ADPCM_STEP_INDEX_MAXIMUM);

	return state->predictor;
}

static boolean adpcm_decode(
	byte const *data,
	long data_bytes,
	long channels,
	long rate,
	struct frames *frames)
{
	long block_count = data_bytes / (ADPCM_BLOCK_BYTES * channels);
	long block_index;

	frames->count = block_count * ADPCM_BLOCK_SAMPLES;
	frames->channels = channels;
	frames->rate = rate;
	frames->samples = frames_allocate(frames->count, channels);
	for (block_index = 0; frames->samples && block_index < block_count; block_index++)
	{
		byte const *block = data + block_index * ADPCM_BLOCK_BYTES * channels;
		short *output = frames->samples + block_index * ADPCM_BLOCK_SAMPLES * channels;
		long channel;

		for (channel = 0; channel < channels; channel++)
		{
			byte const *header = block + channel * 4;
			struct adpcm_state state;
			long group;

			state.predictor = (short)(header[0] | (header[1] << 8));
			state.step_index = MIN(header[2], ADPCM_STEP_INDEX_MAXIMUM);
			for (group = 0; group < 8; group++)
			{
				byte const *group_bytes = block + 4 * channels + (group * channels + channel) * 4;
				long byte_index;

				for (byte_index = 0; byte_index < 4; byte_index++)
				{
					long frame = group * 8 + byte_index * 2;

					output[frame * channels + channel] = (short)adpcm_decode_nibble(&state, group_bytes[byte_index] & 0xF);
					output[(frame + 1) * channels + channel] = (short)adpcm_decode_nibble(&state, group_bytes[byte_index] >> 4);
				}
			}
		}
	}

	return frames->samples != NULL;
}

/* `frames` at `channels` and `rate` (a mono stream fills both channels, a
stereo one is averaged for mono, and another rate is resampled by the
nearest frame); FALSE when it comes to nothing */
static boolean frames_conform(
	struct frames *frames,
	long channels,
	long rate)
{
	double scaled_count = (double)frames->count * rate / frames->rate;
	long count;
	short *samples;
	long frame;

	if (frames->channels == channels && frames->rate == rate)
	{
		return frames->count > 0;
	}
	/* (the count at the new rate, refused before it is a long when it is
	more than any sound has: a stream's own rate can be anything) */
	if (frames->rate <= 0 || !(scaled_count >= 0.0) || scaled_count > (double)MAXIMUM_FRAMES)
	{
		frames_free(frames);
		return FALSE;
	}
	count = (long)scaled_count;
	samples = frames_allocate(count, channels);
	for (frame = 0; samples && frame < count; frame++)
	{
		long source = MIN((long)((double)frame * frames->rate / rate), frames->count - 1);
		short const *in = frames->samples + source * frames->channels;
		short *out = samples + frame * channels;

		if (channels == frames->channels)
			memcpy(out, in, channels * sizeof(*out));
		else if (channels == 2)
			out[0] = out[1] = in[0];
		else
			out[0] = (short)((in[0] + in[1]) / 2);
	}
	frames_free(frames);
	frames->samples = samples;
	frames->count = count;
	frames->channels = channels;
	frames->rate = rate;

	return samples != NULL;
}

/* Room for `bytes` more in the decoded buffer: its offset, or NONE when the
memory or the region for them runs out. */
static long decoded_reserve(
	unsigned long bytes)
{
	unsigned long needed = custom_edition_sounds_globals.decoded_bytes + bytes;

	if (needed < bytes || needed > custom_edition_sounds_globals.decoded_limit)
	{
		return NONE;
	}
	if (needed > custom_edition_sounds_globals.decoded_capacity)
	{
		unsigned long capacity = (needed + DECODED_GROWTH - 1) / DECODED_GROWTH * DECODED_GROWTH;
		byte *grown = realloc(custom_edition_sounds_globals.decoded, capacity);

		if (!grown)
		{
			return NONE;
		}
		custom_edition_sounds_globals.decoded = grown;
		custom_edition_sounds_globals.decoded_capacity = capacity;
	}

	return (long)custom_edition_sounds_globals.decoded_bytes;
}

/* the format this build plays a sound in (sound_manager.c): Xbox ADPCM,
mono at 22 kHz or stereo at 22 or 44 kHz */
static long playable_rate(
	struct sound_definition const *sound)
{
	return sound->encoding == 0 ? 22050 : (sound->sample_rate == 0 ? 22050 : 44100);
}

/* whether a permutation of `sound` must be made playable here: Ogg Vorbis
and uncompressed ones, and mono ones at 44 kHz */
static boolean permutation_needs_conversion(
	struct sound_definition const *sound,
	struct sound_permutation const *permutation)
{
	return permutation->compression == SOUND_COMPRESSION_OGG_VORBIS ||
		permutation->compression == SOUND_COMPRESSION_NONE ||
		(sound->encoding == 0 && sound->sample_rate != 0);
}

/* Makes the permutation `permutation` of `sound` an Xbox ADPCM one in the
format this build plays it, whose samples are in the decoded buffer; FALSE
when it cannot be. */
static boolean permutation_convert(
	struct sound_definition const *sound,
	struct sound_permutation *permutation,
	long decoded_offset)
{
	long channels = sound->encoding == 0 ? 1 : 2;
	long rate = sound->sample_rate == 0 ? 22050 : 44100;
	struct frames frames = { 0 };
	boolean decoded = FALSE;
	unsigned long encoded_bytes;
	byte *data;
	long offset = NONE;

	/* (the loader checked the samples lie in the file; none at all is no sound) */
	if (permutation->samples.size <= 0 || !(data = malloc(permutation->samples.size)))
	{
		return FALSE;
	}
	custom_edition_cache_read(NONE, permutation->samples.file_offset, permutation->samples.size, data);
	switch (permutation->compression)
	{
	case SOUND_COMPRESSION_OGG_VORBIS:
		decoded = vorbis_decode(data, permutation->samples.size, &frames);
		break;
	case SOUND_COMPRESSION_NONE:
		decoded = pcm_decode(data, permutation->samples.size, channels, rate, &frames);
		break;
	case SOUND_COMPRESSION_XBOX_ADPCM:
		decoded = adpcm_decode(data, permutation->samples.size, channels, rate, &frames);
		break;
	}
	free(data);

	if (decoded && frames_conform(&frames, channels, playable_rate(sound)))
	{
		encoded_bytes = adpcm_encoded_bytes(frames.count, channels);
		offset = decoded_reserve(encoded_bytes);
		if (offset != NONE)
		{
			adpcm_encode(frames.samples, frames.count, channels, custom_edition_sounds_globals.decoded + offset);
			custom_edition_sounds_globals.decoded_bytes += encoded_bytes;
			permutation->compression = SOUND_COMPRESSION_XBOX_ADPCM;
			permutation->samples.file_offset = decoded_offset + offset;
			permutation->samples.size = (long)encoded_bytes;
			permutation->sample_buffer_size = 0;
		}
	}
	frames_free(&frames);

	return offset != NONE;
}

/* ---------- public code */

boolean custom_edition_sounds_decode(
	byte *tag_cache,
	unsigned long loaded_bytes,
	long decoded_offset,
	unsigned long decoded_limit)
{
	struct sound_definition *sound;
	int32_t tag_index = NONE;
	long converted_count = 0;
	long failed_count = 0;

	custom_edition_sounds_globals.decoded_limit = decoded_limit;
	while ((sound = custom_edition_cache_tag_next(tag_cache, loaded_bytes, SOUND_DEFINITION_TAG, sizeof(*sound), &tag_index)) != NULL)
	{
		boolean converted = TRUE;
		boolean any = FALSE;
		long range_index;

		for (range_index = 0; range_index < sound->pitch_ranges.count; range_index++)
		{
			struct sound_pitch_range *range = custom_edition_cache_block_element(
				tag_cache, loaded_bytes, &sound->pitch_ranges, range_index, sizeof(*range));
			long permutation_index;

			for (permutation_index = 0; range && permutation_index < range->permutations.count; permutation_index++)
			{
				struct sound_permutation *permutation = custom_edition_cache_block_element(
					tag_cache, loaded_bytes, &range->permutations, permutation_index, sizeof(*permutation));

				if (!permutation || !permutation_needs_conversion(sound, permutation))
				{
					continue;
				}
				any = TRUE;
				if (permutation_convert(sound, permutation, decoded_offset))
				{
					converted_count++;
				}
				else
				{
					converted = FALSE;
				}
			}
		}
		if (any || sound->compression != SOUND_COMPRESSION_XBOX_ADPCM)
		{
			sound->compression = SOUND_COMPRESSION_XBOX_ADPCM;
			if (sound->encoding == 0)
				sound->sample_rate = 0;
		}
		if (!converted)
		{
			/* with no pitch ranges the game neither plays nor loads the sound */
			error(_error_silent, "custom edition: cannot convert the sound '%s'; it will not play",
				custom_edition_cache_tag_name(tag_cache, loaded_bytes, tag_index));
			sound->pitch_ranges.count = 0;
			failed_count++;
		}
	}
	if (converted_count || failed_count)
	{
		error(_error_silent, "custom edition: %ld sound permutations converted to %lu bytes of Xbox ADPCM%s",
			converted_count, custom_edition_sounds_globals.decoded_bytes,
			failed_count ? " (some sounds could not be converted)" : "");
	}

	return TRUE;
}

unsigned long custom_edition_sounds_decoded_bytes(
	void)
{
	return custom_edition_sounds_globals.decoded_bytes;
}

boolean custom_edition_sounds_read(
	long offset,
	long size,
	void *buffer)
{
	if (offset < 0 || size < 0 ||
		(unsigned long)offset + (unsigned long)size > custom_edition_sounds_globals.decoded_bytes)
	{
		return FALSE;
	}
	memcpy(buffer, custom_edition_sounds_globals.decoded + offset, size);

	return TRUE;
}

void custom_edition_sounds_dispose(
	void)
{
	free(custom_edition_sounds_globals.decoded);
	memset(&custom_edition_sounds_globals, 0, sizeof(custom_edition_sounds_globals));

	return;
}
