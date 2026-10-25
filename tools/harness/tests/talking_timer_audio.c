/*
TALKING_TIMER_AUDIO.C (test)
*/

#include "harness.h"
#include "lines.inc"

#include <math.h>
#include <pthread.h>

typedef int opus_int32;
typedef struct OpusDecoder OpusDecoder;
#define OPUS_BUFFER_TOO_SMALL (-2)
#define OPUS_INVALID_PACKET (-4)
#define OPUS_RESET_STATE 4028
#define CORRUPT 0xFF

static char decoder;
static unsigned long decoded;
static int logs, lock_depth;

static float fake_sample(int n, unsigned long k)
{
	return n / 100.0f + k / 65536.0f;
}

static OpusDecoder *opus_decoder_create(opus_int32 rate, int channels, int *error)
{
	(void)rate;
	(void)channels;
	*error = 0;
	return (OpusDecoder *)&decoder;
}

#define opus_decoder_ctl(decoder, request) ((void)((request) == OPUS_RESET_STATE && (decoded = 0)))
static int opus_decode_float(OpusDecoder *decoder, const unsigned char *data, opus_int32 length, float *pcm,
	int frame_size, int fec);
#define pthread_mutex_lock(...) ((void)lock_depth++)
#define pthread_mutex_unlock(...) ((void)lock_depth--)

static void platform_log(const char *format, ...)
{
	(void)format;
	logs++;
}

static struct talking_timer_clip talking_timer_clips[NUMBER_OF_TALKING_TIMER_LINES];

#include "under_test.inc"

static int opus_decode_float(OpusDecoder *decoder, const unsigned char *data, opus_int32 length, float *pcm,
	int frame_size, int fec)
{
	float *end = NULL;
	int buffer, samples, sample;

	(void)decoder;
	(void)fec;
	for (buffer = 0; buffer < 2; buffer++)
	{
		if (pcm >= talking_timer_buffers[buffer] && pcm < talking_timer_buffers[buffer] + TALKING_TIMER_MAXIMUM_SAMPLES)
			end = talking_timer_buffers[buffer] + TALKING_TIMER_MAXIMUM_SAMPLES;
	}
	CHECK(end, "decodes into a buffer");
	CHECK(!talking_timer_length || pcm < talking_timer_buffers[talking_timer_playing]
		|| pcm >= talking_timer_buffers[talking_timer_playing] + TALKING_TIMER_MAXIMUM_SAMPLES,
		"decodes into the buffer not playing");
	CHECK(frame_size <= end - pcm, "frame size %d, room for %ld", frame_size, (long)(end - pcm));
	CHECK(length == 1, "a packet of a byte: %d", length);
	if (data[0] == CORRUPT)
		return OPUS_INVALID_PACKET;
	samples = data[0] * 48;
	if (samples > frame_size)
		return OPUS_BUFFER_TOO_SMALL;
	for (sample = 0; sample < samples; sample++)
		pcm[sample] = fake_sample(data[0], decoded + (unsigned long)sample);
	decoded += (unsigned long)samples;
	return samples;
}

#define LONG_PACKETS (TALKING_TIMER_MAXIMUM_SAMPLES / 960 + 10)
static unsigned int clip_words[5][LONG_PACKETS];

static void clip(int line, int packets, int n)
{
	unsigned char *bytes = (unsigned char *)clip_words[line];
	int packet;

	for (packet = 0; packet < packets; packet++)
	{
		bytes[packet * 3] = 1;
		bytes[packet * 3 + 1] = 0;
		bytes[packet * 3 + 2] = (unsigned char)n;
	}
	talking_timer_clips[line].words = clip_words[line];
	talking_timer_clips[line].size = (unsigned long)packets * 3;
}

#define LONG_FRAMES (TALKING_TIMER_MAXIMUM_SAMPLES + 1000)
static float output[2 * LONG_FRAMES];

static void check_mixed(unsigned long from, unsigned long to, int n, unsigned long first, float gain, float under)
{
	unsigned long frame;

	for (frame = from; frame < to; frame++)
	{
		float expected = under + gain * fake_sample(n, first + frame - from);

		CHECK(fabsf(output[frame * 2] - expected) < 1e-6f, "frame %lu left: %f, not %f", frame, output[frame * 2],
			expected);
		CHECK(output[frame * 2 + 1] == output[frame * 2], "frame %lu: right %f, left %f", frame,
			output[frame * 2 + 1], output[frame * 2]);
	}
}

static void check_silent(unsigned long from, unsigned long to, float under)
{
	unsigned long frame;

	for (frame = from; frame < to; frame++)
		CHECK(output[frame * 2] == under && output[frame * 2 + 1] == under, "frame %lu: %f, %f, not %f", frame,
			output[frame * 2], output[frame * 2 + 1], under);
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";

	/* (read here so a negative control that drops their other read still builds with -Werror) */
	(void)talking_timer_gain;
	(void)talking_timer_logged;

	clip(0, 2, 10);
	clip(1, 2, 20);
	clip(2, 1, CORRUPT);
	clip(3, 0, 0);
	clip(4, LONG_PACKETS, 20);

	CASE("play")
	{
		talking_timer_audio_play(0, 1.0f);
		talking_timer_audio_mix(output, 480);
		check_mixed(0, 480, 10, 0, 1.0f, 0.0f);
		memset(output, 0, sizeof(output));
		talking_timer_audio_mix(output, 480);
		check_mixed(0, 480, 10, 480, 1.0f, 0.0f);
	}
	CASE("volume")
	{
		talking_timer_audio_play(0, 0.25f);
		talking_timer_audio_mix(output, 960);
		check_mixed(0, 960, 10, 0, 0.25f, 0.0f);
	}
	CASE("cut")
	{
		talking_timer_audio_play(0, 1.0f);
		talking_timer_audio_mix(output, 100);
		talking_timer_audio_play(1, 1.0f);
		memset(output, 0, sizeof(output));
		talking_timer_audio_mix(output, 100);
		check_mixed(0, 100, 20, 0, 1.0f, 0.0f);
	}
	CASE("end")
	{
		unsigned long frame;

		for (frame = 0; frame < 2000 * 2; frame++)
			output[frame] = 0.5f;
		talking_timer_audio_play(0, 1.0f);
		talking_timer_audio_mix(output, 2000);
		check_mixed(0, 960, 10, 0, 1.0f, 0.5f);
		check_silent(960, 2000, 0.5f);
	}
	CASE("missing")
	{
		talking_timer_audio_play(3, 1.0f);
		talking_timer_audio_play(2, 1.0f);
		talking_timer_audio_mix(output, 960);
		check_silent(0, 960, 0.0f);
		CHECK(logs == 1, "logged once: %d", logs);
	}
	CASE("long")
	{
		talking_timer_audio_play(4, 1.0f);
		talking_timer_audio_mix(output, LONG_FRAMES);
		check_mixed(0, TALKING_TIMER_MAXIMUM_SAMPLES, 20, 0, 1.0f, 0.0f);
		check_silent(TALKING_TIMER_MAXIMUM_SAMPLES, LONG_FRAMES, 0.0f);
	}
	CHECK(lock_depth == 0, "the lock released: %d", lock_depth);
	return 0;
}
