/*
COOP_SOUND_SYNC.C

A co-op client that joins during a looping sound that plays once (a
cutscene's music or dialogue: a fake impulse one) starts it where the host
has reached, rather than from its beginning, out of step with the scene.

The host's resent start says how long ago the host started it, in
milliseconds (network_coop.c's host_send_looping_sounds: the event's
target). Here that time is walked through the parts the client's sound
manager is about to play (sound_manager.c's
update_potentially_audible_looping_sound), track by track:
	- a start sound that has played out is skipped, and its time taken from
	  the loop's (unless the loop fades in at the start, with it),
	- the part that is playing starts at the permutation of its linked chain
	  the time falls in, a block's start into that permutation's samples
	  (update_channel_for_looping_sound, dsound_port_queue_sound_from); a loop
	  whose permutations are not linked repeats, so the time wraps around it,
	- a loop whose chain has played out has the whole sound played out: it
	  does not start, nor do its stop sounds.

Each machine picks its pitches and permutations itself, as it always has,
and the time is walked through the client's own, so the sound ends when it
would have, had it started with the host's. A permutation whose samples'
time is not known here (a compression other than none or Xbox ADPCM, which
the channels play) starts at its beginning.

Only the scripts' own instance of the sound is placed (the looping sound
definition's runtime_scripting_sound_index), and only in the refresh that
first starts something of it: a part started later (a pitch range's
crossfade, an alternate) starts at its beginning, as it always has.
*/

#include "cseries.h"
#include "cseries/cseries_windows.h"
#include "cseries/errors.h"
#include "sound/sound_definitions.h"
#include "sound/sound_dsound.h"
#include "tag_files/tag_files.h"
#include "tag_files/tag_groups.h"
#include "coop_sound_sync.h"

/* ---------- constants */

enum
{
	/* network_coop.c's MAXIMUM_LOOPING_SOUNDS */
	MAXIMUM_COOP_SOUND_SYNCS = 32,
	MAXIMUM_COOP_SOUND_OFFSETS = 16,
	/* the most of the host's time taken: a day */
	MAXIMUM_COOP_SOUND_SYNC_MILLISECONDS = 24 * 60 * 60 * 1000,
	/* the most a permutation's time is taken as: an hour */
	MAXIMUM_COOP_SOUND_PERMUTATION_MILLISECONDS = 60 * 60 * 1000,
};

/* a sound's encoding and compression, as its tag has them */
enum
{
	COOP_SOUND_ENCODING_STEREO = 1,
	COOP_SOUND_COMPRESSION_NONE = 0,
	COOP_SOUND_COMPRESSION_XBOX_ADPCM = 1,
	/* (sound_dsound_xbox.c's SOUND_COMPRESSED_BLOCK_SIZE, a channel's, and
	SOUND_COMPRESSED_SAMPLES_PER_BLOCK) */
	COOP_SOUND_ADPCM_BLOCK_BYTES = 36,
	COOP_SOUND_ADPCM_BLOCK_SAMPLES = 64,
	/* a 16-bit sample of each of the stream's two channels */
	COOP_SOUND_PCM_BLOCK_BYTES = 4,
	/* 22 and 44 kHz (sound_samples_per_second) */
	COOP_SOUND_SAMPLE_RATES = 2,
	/* sound_manager.c's _fade_in_at_start_bit: a track whose loop fades in
	with its start sound, rather than after it */
	COOP_SOUND_TRACK_FADE_IN_AT_START_BIT = 0,
};

/* the lowest pitch a time is worked out at */
#define COOP_SOUND_MINIMUM_PITCH 0.01f

/* ---------- globals */

static struct coop_sound_sync
{
	/* NONE: unused */
	long definition_index;
	long elapsed_milliseconds;
	/* (system_milliseconds when the host's time came) */
	unsigned long received;
	/* its first refresh is over: nothing more is placed */
	boolean placed;
	/* its loop had played out on the host: its stop sounds do not play */
	boolean finished;
	/* each track's start sound that was skipped, its time */
	long start_milliseconds[MAXIMUM_TRACKS_PER_LOOPING_SOUND];
} coop_sound_syncs[MAXIMUM_COOP_SOUND_SYNCS];

/* sounds whose first permutation starts into its samples, till a channel
takes them (coop_sound_sync_take_offset) */
static struct
{
	long sound_index;
	struct sound_permutation const *permutation;
	long byte_offset;
} coop_sound_offsets[MAXIMUM_COOP_SOUND_OFFSETS];
static short coop_sound_next_offset;

static boolean coop_sound_syncs_initialized;

/* ---------- private code */

static void coop_sound_syncs_initialize(
	void)
{
	short index;

	if (coop_sound_syncs_initialized)
		return;
	for (index = 0; index < MAXIMUM_COOP_SOUND_SYNCS; index++)
		coop_sound_syncs[index].definition_index = NONE;
	for (index = 0; index < MAXIMUM_COOP_SOUND_OFFSETS; index++)
		coop_sound_offsets[index].sound_index = NONE;
	coop_sound_next_offset = 0;
	coop_sound_syncs_initialized = TRUE;
}

static struct coop_sound_sync *coop_sound_sync_find(
	long looping_definition_index)
{
	short index;

	if (!coop_sound_syncs_initialized || looping_definition_index == NONE)
		return NULL;
	for (index = 0; index < MAXIMUM_COOP_SOUND_SYNCS; index++)
	{
		if (coop_sound_syncs[index].definition_index == looping_definition_index)
			return &coop_sound_syncs[index];
	}
	return NULL;
}

/* the host's time now: as it sent it, and since it came */
static long coop_sound_sync_elapsed(
	struct coop_sound_sync const *sync)
{
	unsigned long since = system_milliseconds() - sync->received;
	long elapsed = sync->elapsed_milliseconds;

	if (since > (unsigned long)(MAXIMUM_COOP_SOUND_SYNC_MILLISECONDS - elapsed))
		return MAXIMUM_COOP_SOUND_SYNC_MILLISECONDS;
	return elapsed + (long)since;
}

/* a permutation's sample frames (one sample of each of its channels), or 0
for a compression whose time is not known here */
static long coop_sound_permutation_frames(
	struct sound_definition const *definition,
	struct sound_permutation const *permutation)
{
	long channels = definition->encoding == COOP_SOUND_ENCODING_STEREO ? 2 : 1;
	long size = permutation->samples.size;

	if (size <= 0)
		return 0;
	if (definition->compression == COOP_SOUND_COMPRESSION_NONE)
		return size / (2 * channels);
	if (definition->compression == COOP_SOUND_COMPRESSION_XBOX_ADPCM)
		return size / (COOP_SOUND_ADPCM_BLOCK_BYTES * channels) * COOP_SOUND_ADPCM_BLOCK_SAMPLES;
	return 0;
}

/* the frames a permutation plays each second at played_pitch, or 0 */
static real coop_sound_frames_per_second(
	struct sound_definition const *definition,
	real played_pitch)
{
	if (!VALID_INDEX(definition->sample_rate, COOP_SOUND_SAMPLE_RATES) ||
		!(played_pitch >= COOP_SOUND_MINIMUM_PITCH))
	{
		return 0.f;
	}
	return (real)sound_samples_per_second(definition->sample_rate) * played_pitch;
}

/* how long a permutation plays at played_pitch, in milliseconds, or 0 when
that is not known */
static long coop_sound_permutation_milliseconds(
	struct sound_definition const *definition,
	struct sound_permutation const *permutation,
	real played_pitch)
{
	real rate = coop_sound_frames_per_second(definition, played_pitch);
	long frames = coop_sound_permutation_frames(definition, permutation);
	real milliseconds;

	if (rate <= 0.f || frames <= 0)
		return 0;
	milliseconds = (real)frames * 1000.f / rate;
	if (milliseconds >= (real)MAXIMUM_COOP_SOUND_PERMUTATION_MILLISECONDS)
		return MAXIMUM_COOP_SOUND_PERMUTATION_MILLISECONDS;
	return MAX((long)milliseconds, 1);
}

/* the start of the block milliseconds into a permutation playing at
played_pitch, in bytes into its samples (0: its beginning) */
static long coop_sound_permutation_byte_offset(
	struct sound_definition const *definition,
	struct sound_permutation const *permutation,
	real played_pitch,
	long milliseconds)
{
	long channels = definition->encoding == COOP_SOUND_ENCODING_STEREO ? 2 : 1;
	long frames = coop_sound_permutation_frames(definition, permutation);
	real rate = coop_sound_frames_per_second(definition, played_pitch);
	real into = (real)milliseconds * rate / 1000.f;
	long offset;

	if (frames <= 0 || rate <= 0.f || !(into > 0.f))
		return 0;
	if (into >= (real)frames)
		into = (real)(frames - 1);
	if (definition->compression == COOP_SOUND_COMPRESSION_XBOX_ADPCM)
	{
		offset = (long)into / COOP_SOUND_ADPCM_BLOCK_SAMPLES * COOP_SOUND_ADPCM_BLOCK_BYTES * channels;
	}
	else
	{
		offset = (long)into * 2 * channels;
		offset -= offset % COOP_SOUND_PCM_BLOCK_BYTES;
	}
	return offset > 0 && offset < permutation->samples.size ? offset : 0;
}

/* a pitch a sound's tag gives, or 1 for none */
static real coop_sound_pitch_or_one(
	real pitch)
{
	return pitch > 0.f ? pitch : 1.f;
}

static void coop_sound_offset_note(
	long sound_index,
	struct sound_permutation const *permutation,
	long byte_offset)
{
	short index;

	for (index = 0; index < MAXIMUM_COOP_SOUND_OFFSETS; index++)
	{
		if (coop_sound_offsets[index].sound_index == sound_index)
			break;
	}
	if (index == MAXIMUM_COOP_SOUND_OFFSETS)
	{
		index = coop_sound_next_offset;
		coop_sound_next_offset = (short)((coop_sound_next_offset + 1) % MAXIMUM_COOP_SOUND_OFFSETS);
	}
	coop_sound_offsets[index].sound_index = sound_index;
	coop_sound_offsets[index].permutation = permutation;
	coop_sound_offsets[index].byte_offset = byte_offset;
}

/* (debug.txt: where a part was placed) */
static void coop_sound_sync_log(
	long looping_definition_index,
	short track_index,
	short part,
	long elapsed,
	char const *placement,
	short permutation_index,
	long byte_offset)
{
	error(_error_silent, "co-op: %s's track %d %s, %ld ms in: %s (permutation %d, byte %ld)",
		tag_get_name(looping_definition_index), track_index,
		part == _coop_sound_sync_start_part ? "start" : "loop", elapsed, placement, permutation_index, byte_offset);
}

/* ---------- public code */

void coop_sound_sync_begin(
	long looping_definition_index,
	long elapsed_milliseconds)
{
	struct coop_sound_sync *sync;
	short index;

	coop_sound_syncs_initialize();
	coop_sound_sync_end(looping_definition_index);
	/* (just now: from its beginning, as it always started) */
	if (looping_definition_index == NONE || elapsed_milliseconds <= 0)
		return;
	for (index = 0; index < MAXIMUM_COOP_SOUND_SYNCS; index++)
	{
		if (coop_sound_syncs[index].definition_index == NONE)
			break;
	}
	if (index == MAXIMUM_COOP_SOUND_SYNCS)
		return;
	sync = &coop_sound_syncs[index];
	csmemset(sync, 0, sizeof(*sync));
	sync->definition_index = looping_definition_index;
	sync->elapsed_milliseconds = MIN(elapsed_milliseconds, MAXIMUM_COOP_SOUND_SYNC_MILLISECONDS);
	sync->received = system_milliseconds();
}

long coop_sound_sync_sound_milliseconds(
	long sound_index)
{
	struct sound_definition *definition;
	real slowest;
	long longest = 0;
	short range_index;

	if (sound_index == NONE)
		return 0;
	definition = sound_definition_get(sound_index);
	/* (sound_manager.c: its random pitch's lower bound, by the smaller of the
	scale's modifiers, by a range's playback rate) */
	slowest = coop_sound_pitch_or_one(definition->random_pitch_bounds.lower) *
		coop_sound_pitch_or_one(MIN(definition->zero_pitch_modifier, definition->one_pitch_modifier));
	for (range_index = 0; range_index < definition->pitch_ranges.count; range_index++)
	{
		struct sound_pitch_range *range =
			TAG_BLOCK_GET_ELEMENT(&definition->pitch_ranges, range_index, struct sound_pitch_range);
		real played_pitch = slowest * coop_sound_pitch_or_one(range->playback_rate);
		short first;

		/* (sound_definition_next_permutation: the first is one of the
		actual ones, and a linked one is followed by its next, till NONE) */
		for (first = 0; first < MAX(range->actual_permutation_count, 1) && first < range->permutations.count; first++)
		{
			long length = 0;
			short index = first;
			short count;

			for (count = 0; count < range->permutations.count && VALID_INDEX(index, range->permutations.count); count++)
			{
				struct sound_permutation *permutation =
					TAG_BLOCK_GET_ELEMENT(&range->permutations, index, struct sound_permutation);
				long part = coop_sound_permutation_milliseconds(definition, permutation, played_pitch);

				/* (one whose time is not known here: the longest the tag says one is) */
				if (part <= 0)
					part = PIN(definition->longest_permutation_length, 0, MAXIMUM_COOP_SOUND_PERMUTATION_MILLISECONDS);
				length = MIN(length + part, MAXIMUM_COOP_SOUND_PERMUTATION_MILLISECONDS);
				if (!TEST_FLAG(definition->flags, _sound_definition_linked_permutations_bit))
					break;
				index = permutation->next_permutation_index;
			}
			longest = MAX(longest, length);
		}
	}
	return longest;
}

void coop_sound_sync_end(
	long looping_definition_index)
{
	struct coop_sound_sync *sync = coop_sound_sync_find(looping_definition_index);

	if (sync)
		sync->definition_index = NONE;
}

void coop_sound_sync_reset(
	void)
{
	coop_sound_syncs_initialized = FALSE;
	coop_sound_syncs_initialize();
}

short coop_sound_sync_place(
	long looping_definition_index,
	long loop_identifier,
	short track_index,
	short part,
	long definition_index,
	short pitch_range_index,
	short *permutation_index,
	real played_pitch,
	long sound_index)
{
	struct coop_sound_sync *sync = coop_sound_sync_find(looping_definition_index);
	struct sound_definition *definition;
	struct sound_pitch_range *range;
	struct sound_permutation *permutation;
	boolean linked;
	long into;
	long played = 0;
	short index;
	short count;

	if (!sync || !VALID_INDEX(track_index, MAXIMUM_TRACKS_PER_LOOPING_SOUND) || definition_index == NONE ||
		looping_sound_definition_get(looping_definition_index)->runtime_scripting_sound_index != loop_identifier)
	{
		return _coop_sound_sync_play;
	}
	if (part == _coop_sound_sync_stop_part)
		return sync->finished ? _coop_sound_sync_skip : _coop_sound_sync_play;
	if (sync->placed || (part != _coop_sound_sync_start_part && part != _coop_sound_sync_loop_part))
		return _coop_sound_sync_play;
	definition = sound_definition_get(definition_index);
	if (!VALID_INDEX(pitch_range_index, definition->pitch_ranges.count))
		return _coop_sound_sync_play;
	range = TAG_BLOCK_GET_ELEMENT(&definition->pitch_ranges, pitch_range_index, struct sound_pitch_range);
	if (!VALID_INDEX(*permutation_index, range->permutations.count))
		return _coop_sound_sync_play;
	into = coop_sound_sync_elapsed(sync);
	if (part == _coop_sound_sync_loop_part)
		into -= sync->start_milliseconds[track_index];
	if (into <= 0)
		return _coop_sound_sync_play;

	/* (sound_definition_next_permutation: a linked permutation is followed by
	its next, till NONE; at most each of the range's once, against a chain
	that comes back on itself) */
	linked = TEST_FLAG(definition->flags, _sound_definition_linked_permutations_bit);
	index = *permutation_index;
	for (count = 0; count < range->permutations.count; count++)
	{
		long length;
		long byte_offset;
		short next_index;

		permutation = TAG_BLOCK_GET_ELEMENT(&range->permutations, index, struct sound_permutation);
		length = coop_sound_permutation_milliseconds(definition, permutation, played_pitch);
		if (length <= 0)
			return _coop_sound_sync_play;
		if (into < length)
		{
			byte_offset = coop_sound_permutation_byte_offset(definition, permutation, played_pitch, into);
			*permutation_index = index;
			if (byte_offset > 0)
				coop_sound_offset_note(sound_index, permutation, byte_offset);
			coop_sound_sync_log(looping_definition_index, track_index, part, played + into, "plays",
				index, byte_offset);
			return _coop_sound_sync_play;
		}
		into -= length;
		played += length;
		next_index = linked ? permutation->next_permutation_index : NONE;
		if (!VALID_INDEX(next_index, range->permutations.count))
		{
			if (part == _coop_sound_sync_start_part)
			{
				struct looping_sound_definition *looping_definition =
					looping_sound_definition_get(looping_definition_index);

				/* (the loop starts once the start has played, or with it) */
				if (VALID_INDEX(track_index, looping_definition->tracks.count) &&
					!TEST_FLAG(TAG_BLOCK_GET_ELEMENT(&looping_definition->tracks, track_index,
						struct looping_sound_track)->flags, COOP_SOUND_TRACK_FADE_IN_AT_START_BIT))
				{
					sync->start_milliseconds[track_index] = played;
				}
				coop_sound_sync_log(looping_definition_index, track_index, part, played + into, "played out",
					index, 0);
				return _coop_sound_sync_skip;
			}
			if (!linked)
			{
				/* (a loop of one permutation repeats it) */
				byte_offset = coop_sound_permutation_byte_offset(definition, permutation, played_pitch,
					into % length);
				*permutation_index = index;
				if (byte_offset > 0)
					coop_sound_offset_note(sound_index, permutation, byte_offset);
				coop_sound_sync_log(looping_definition_index, track_index, part, played + into, "repeats",
					index, byte_offset);
				return _coop_sound_sync_play;
			}
			sync->finished = TRUE;
			coop_sound_sync_log(looping_definition_index, track_index, part, played + into, "has played out",
				index, 0);
			return _coop_sound_sync_finished;
		}
		index = next_index;
	}
	/* (a chain that comes back on itself: from where it was) */
	return _coop_sound_sync_play;
}

void coop_sound_sync_refreshed(
	long looping_definition_index,
	long loop_identifier,
	boolean started)
{
	struct coop_sound_sync *sync;

	if (!started || (sync = coop_sound_sync_find(looping_definition_index)) == NULL || sync->placed)
		return;
	if (looping_sound_definition_get(looping_definition_index)->runtime_scripting_sound_index == loop_identifier)
		sync->placed = TRUE;
}

long coop_sound_sync_take_offset(
	long sound_index,
	struct sound_permutation const *permutation)
{
	short index;

	if (!coop_sound_syncs_initialized || sound_index == NONE)
		return 0;
	for (index = 0; index < MAXIMUM_COOP_SOUND_OFFSETS; index++)
	{
		if (coop_sound_offsets[index].sound_index == sound_index)
		{
			long byte_offset = coop_sound_offsets[index].permutation == permutation ?
				coop_sound_offsets[index].byte_offset : 0;

			coop_sound_offsets[index].sound_index = NONE;
			return byte_offset;
		}
	}
	return 0;
}
