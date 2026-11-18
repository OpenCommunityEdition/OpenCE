/*
TALKING_TIMER_AUDIO.C

A line is decoded into the buffer not playing, outside the lock, so the mix
never waits on a decode.
*/

/* ---------- headers */

#include "platform.h"
#include "talking_timer_audio.h"
#include "talking_timer_clips.h"
#include "../../third_party/opus/include/opus.h"

#include <pthread.h>

/* ---------- constants */

#define TALKING_TIMER_RATE 48000
/* (2 s, the longest a line runs, plus the encoder's padding and pre-skip) */
#define TALKING_TIMER_MAXIMUM_SAMPLES (2 * TALKING_TIMER_RATE + 2 * 960)

/* ---------- globals */

static pthread_mutex_t talking_timer_lock = PTHREAD_MUTEX_INITIALIZER;
static float talking_timer_buffers[2][TALKING_TIMER_MAXIMUM_SAMPLES];
static OpusDecoder *talking_timer_decoder;
static int talking_timer_playing;
static unsigned long talking_timer_length, talking_timer_read;
static float talking_timer_gain;
static int talking_timer_logged;

/* ---------- private code */

static unsigned long talking_timer_decode(int line, float *samples)
{
	const unsigned char *data = (const unsigned char *)talking_timer_clips[line].words;
	unsigned long size = talking_timer_clips[line].size, at = 0, total = 0;
	int error;

	if (!talking_timer_decoder)
		talking_timer_decoder = opus_decoder_create(TALKING_TIMER_RATE, 1, &error);
	if (!talking_timer_decoder || !size)
		return 0;
	opus_decoder_ctl(talking_timer_decoder, OPUS_RESET_STATE);
	while (at + 2 <= size)
	{
		unsigned long length = data[at] | (unsigned long)data[at + 1] << 8;
		int decoded;

		at += 2;
		if (at + length > size)
			return 0;
		decoded = opus_decode_float(talking_timer_decoder, data + at, (opus_int32)length, samples + total,
			(int)(TALKING_TIMER_MAXIMUM_SAMPLES - total), 0);
		if (decoded < 0)
			return 0;
		total += (unsigned long)decoded;
		at += length;
		if (total >= TALKING_TIMER_MAXIMUM_SAMPLES)
			break;
	}

	return total;
}

/* ---------- public code */

void talking_timer_audio_play(int line, float volume)
{
	int idle = !talking_timer_playing;
	unsigned long length;

	if (line < 0 || line >= NUMBER_OF_TALKING_TIMER_LINES)
		return;
	length = talking_timer_decode(line, talking_timer_buffers[idle]);
	if (!length)
	{
		if (!talking_timer_logged)
			platform_log("talking timer: line %d could not be decoded", line);
		talking_timer_logged = 1;
		return;
	}
	pthread_mutex_lock(&talking_timer_lock);
	talking_timer_playing = idle;
	talking_timer_length = length;
	talking_timer_read = 0;
	talking_timer_gain = volume;
	pthread_mutex_unlock(&talking_timer_lock);
}

void talking_timer_audio_mix(float *output, unsigned long frames)
{
	unsigned long frame;

	pthread_mutex_lock(&talking_timer_lock);
	for (frame = 0; frame < frames && talking_timer_read < talking_timer_length; frame++)
	{
		float sample = talking_timer_buffers[talking_timer_playing][talking_timer_read++] * talking_timer_gain;

		output[frame * 2] += sample;
		output[frame * 2 + 1] += sample;
	}
	pthread_mutex_unlock(&talking_timer_lock);
}
