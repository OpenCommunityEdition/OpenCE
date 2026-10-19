/*
HOST_AUDIO_TEST.C

A native test of port/macos/host/host_audio.c (tools/test_macos_vm.py): with
the app's audio installed, a playback stream with a callback is the app's,
its guest data is converted to float frames as it is put in, the engine's
renders take them a channel each (silence for what the stream lacks), and
the guest's callback is asked for what the stream lacks of its target, on a
thread of the stream's own once the stream plays. A stream without a
callback (the microphone's), and any stream without the app's audio, is
SDL's. SDL's imports and the guest's callback are stand-ins here.
*/

#include "host.h"
#include "halo_host.h"

#include <SDL3/SDL.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define CHECK(condition) do { if (!(condition)) { printf("failed: %s (line %d)\n", #condition, __LINE__); exit(1); } } while (0)

/* ---------- SDL's, as host_sdl.c has them: one stream, handle 5 */

static int sdl_opens;
static int sdl_puts;
static int sdl_destroys;

uint32_t host_sdl_open_audio_stream(uint32_t device, const void *spec, uint32_t callback, uint32_t userdata)
{
	(void)device;
	(void)spec;
	(void)callback;
	(void)userdata;
	sdl_opens++;
	return 5;
}

int host_sdl_put_audio_stream_data(uint32_t stream, const void *data, int length)
{
	(void)data;
	(void)length;
	CHECK(stream == 5);
	sdl_puts++;
	return 1;
}

int host_sdl_resume_audio_stream_device(uint32_t stream)
{
	CHECK(stream == 5);
	return 1;
}

int host_sdl_get_audio_stream_data(uint32_t stream, void *data, int length)
{
	(void)data;
	CHECK(stream == 5);
	return length;
}

int host_sdl_get_audio_stream_available(uint32_t stream)
{
	CHECK(stream == 5);
	return 77;
}

void host_sdl_destroy_audio_stream(uint32_t stream)
{
	CHECK(stream == 5);
	sdl_destroys++;
}

/* ---------- the guest's callback: it puts guest_frames frames of its
pattern, in its stream's format, as the game's mixer does */

#define CALLBACK_ADDRESS 0x1234
#define USERDATA_ADDRESS 0x5678

static pthread_mutex_t guest_lock = PTHREAD_MUTEX_INITIALIZER;
static int guest_calls;
static uint32_t guest_stream;
static uint32_t guest_additional;
static int guest_frames;

void host_logf(int priority, const char *format, ...)
{
	(void)priority;
	(void)format;
}

void host_fatal(const char *format, ...)
{
	printf("fatal: %s\n", format);
	exit(1);
}

uint32_t host_call_guest(uint32_t function, uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
	float frames[2 * 64];
	int frame, count;

	CHECK(function == CALLBACK_ADDRESS);
	CHECK(a == USERDATA_ADDRESS);
	pthread_mutex_lock(&guest_lock);
	guest_calls++;
	guest_stream = b;
	guest_additional = c;
	count = guest_frames;
	pthread_mutex_unlock(&guest_lock);
	/* (SDL's total is the additional amount here: nothing is queued
	elsewhere) */
	CHECK(d == c);
	for (frame = 0; frame < count && frame < 64; frame++)
	{
		frames[frame * 2] = 0.25f;
		frames[frame * 2 + 1] = -0.25f;
	}
	if (count)
		CHECK(host_audio_put_stream_data(b, frames, count * 2 * (int)sizeof(float)));
	return 0;
}

int host_native_thread_create(void *(*function)(void *), void *argument, size_t stack_size)
{
	pthread_attr_t attributes;
	pthread_t thread;
	int error;

	(void)stack_size;
	pthread_attr_init(&attributes);
	pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
	error = pthread_create(&thread, &attributes, function, argument);
	pthread_attr_destroy(&attributes);
	return error;
}

/* ---------- the app's audio: what it is asked to open, play and close */

static int app_opens;
static uint32_t app_stream;
static int app_channels;
static int app_rate;
static int app_resumes;
static int app_closes;
static int app_refuses;

static int app_open(void *context, uint32_t stream, int channels, int rate)
{
	CHECK(context == &app_opens);
	if (app_refuses)
		return 0;
	app_opens++;
	app_stream = stream;
	app_channels = channels;
	app_rate = rate;
	return 1;
}

static int app_resume(void *context, uint32_t stream)
{
	(void)context;
	CHECK(stream == app_stream);
	app_resumes++;
	return 1;
}

static void app_close(void *context, uint32_t stream)
{
	(void)context;
	CHECK(stream == app_stream);
	app_closes++;
}

static SDL_AudioSpec spec_of(SDL_AudioFormat format, int channels, int rate)
{
	SDL_AudioSpec spec;

	spec.format = format;
	spec.channels = channels;
	spec.freq = rate;
	return spec;
}

/* renders `frames` frames of `stream` to two channels */
static void render(uint32_t stream, float *left, float *right, int frames)
{
	float *channels[2];

	channels[0] = left;
	channels[1] = right;
	halo_host_audio_render(stream, channels, 2, frames);
}

static int near(float value, float expected)
{
	return value > expected - 1e-6f && value < expected + 1e-6f;
}

int main(void)
{
	struct halo_host_audio_backend backend;
	SDL_AudioSpec spec;
	float left[8], right[8];
	uint32_t stream, microphone;
	int index;

	/* without the app's audio, every stream is SDL's */
	spec = spec_of(SDL_AUDIO_F32, 2, 48000);
	stream = host_audio_open_stream(0, &spec, CALLBACK_ADDRESS, USERDATA_ADDRESS);
	CHECK(stream == 5 && sdl_opens == 1);
	CHECK(host_audio_put_stream_data(stream, left, 8) && sdl_puts == 1);
	CHECK(host_audio_get_stream_available(stream) == 77);
	host_audio_destroy_stream(stream);
	CHECK(sdl_destroys == 1);

	memset(&backend, 0, sizeof(backend));
	backend.context = &app_opens;
	backend.open = app_open;
	backend.resume = app_resume;
	backend.close = app_close;
	halo_host_set_audio_backend(&backend);

	/* the microphone's stream (no callback) stays SDL's */
	spec = spec_of(SDL_AUDIO_F32, 1, 16000);
	microphone = host_audio_open_stream(0, &spec, 0, 0);
	CHECK(microphone == 5 && sdl_opens == 2 && app_opens == 0);
	CHECK(host_audio_get_stream_data(microphone, left, 4) == 4);
	host_audio_destroy_stream(microphone);
	CHECK(sdl_destroys == 2);

	/* a format the host cannot convert, or a bad spec, is refused */
	spec = spec_of((SDL_AudioFormat)0x1234, 2, 48000);
	CHECK(host_audio_open_stream(0, &spec, CALLBACK_ADDRESS, USERDATA_ADDRESS) == 0);
	spec = spec_of(SDL_AUDIO_F32, 0, 48000);
	CHECK(host_audio_open_stream(0, &spec, CALLBACK_ADDRESS, USERDATA_ADDRESS) == 0);
	spec = spec_of(SDL_AUDIO_F32, 9, 48000);
	CHECK(host_audio_open_stream(0, &spec, CALLBACK_ADDRESS, USERDATA_ADDRESS) == 0);
	spec = spec_of(SDL_AUDIO_F32, 2, 0);
	CHECK(host_audio_open_stream(0, &spec, CALLBACK_ADDRESS, USERDATA_ADDRESS) == 0);
	CHECK(host_audio_open_stream(0, NULL, CALLBACK_ADDRESS, USERDATA_ADDRESS) == 0);
	/* as is one the app cannot open */
	app_refuses = 1;
	spec = spec_of(SDL_AUDIO_F32, 2, 48000);
	CHECK(host_audio_open_stream(0, &spec, CALLBACK_ADDRESS, USERDATA_ADDRESS) == 0);
	app_refuses = 0;
	CHECK(app_opens == 0 && sdl_opens == 2);

	/* ---------- the game's stream, 16-bit: the app's */
	spec = spec_of(SDL_AUDIO_S16, 2, 44100);
	stream = host_audio_open_stream(0, &spec, CALLBACK_ADDRESS, USERDATA_ADDRESS);
	CHECK(stream >= HOST_AUDIO_HANDLE_BASE && stream != 5);
	CHECK(app_opens == 1 && app_stream == stream && app_channels == 2 && app_rate == 44100);
	{
		int16_t samples[6] = { 16384, -16384, 32767, -32768, 0, 8192 };

		CHECK(host_audio_put_stream_data(stream, samples, (int)sizeof(samples)));
		/* (a partial frame is not taken) */
		CHECK(host_audio_put_stream_data(stream, samples, 3) == 0);
		CHECK(host_audio_put_stream_data(stream, samples, -2) == 0);
	}
	/* what is available is in the stream's own format */
	CHECK(host_audio_get_stream_available(stream) == 3 * 2 * 2);
	/* nothing is read from an app stream, which only plays */
	CHECK(host_audio_get_stream_data(stream, left, 4) == -1);
	/* a render takes the frames, a channel each, then silence */
	for (index = 0; index < 8; index++)
		left[index] = right[index] = 9.0f;
	render(stream, left, right, 5);
	CHECK(near(left[0], 0.5f) && near(right[0], -0.5f));
	CHECK(near(left[1], 32767.0f / 32768.0f) && near(right[1], -1.0f));
	CHECK(near(left[2], 0.0f) && near(right[2], 0.25f));
	CHECK(left[3] == 0.0f && right[3] == 0.0f && left[4] == 0.0f && right[4] == 0.0f);
	CHECK(left[5] == 9.0f);
	CHECK(host_audio_get_stream_available(stream) == 0);
	/* a render of a stream that is not there, or of more channels than the
	stream has, gives silence for what it lacks */
	{
		float a[2] = { 9.0f, 9.0f }, b[2] = { 9.0f, 9.0f }, c[2] = { 9.0f, 9.0f };
		float *three[3];
		int16_t one[2] = { 16384, 16384 };

		three[0] = a;
		three[1] = b;
		three[2] = c;
		halo_host_audio_render(HOST_AUDIO_HANDLE_BASE + 99, three, 3, 2);
		CHECK(a[0] == 0.0f && b[1] == 0.0f && c[0] == 0.0f);
		CHECK(host_audio_put_stream_data(stream, one, (int)sizeof(one)));
		a[0] = b[0] = c[0] = 9.0f;
		halo_host_audio_render(stream, three, 3, 1);
		CHECK(near(a[0], 0.5f) && near(b[0], 0.5f) && c[0] == 0.0f);
	}

	/* the callback is asked for what the stream lacks of its target, in the
	stream's own format (16-bit stereo: 4 bytes a frame) */
	pthread_mutex_lock(&guest_lock);
	guest_frames = 0;
	pthread_mutex_unlock(&guest_lock);
	CHECK(host_audio_fill(stream) == 1);
	CHECK(guest_calls == 1 && guest_stream == stream);
	CHECK(guest_additional > 0 && guest_additional % 4 == 0);
	CHECK(guest_additional / 4 <= HOST_AUDIO_RING_FRAMES);
	/* the ring keeps no more than it holds: a stream put past it keeps its
	first frames */
	{
		static int16_t many[2 * (HOST_AUDIO_RING_FRAMES + 16)];

		for (index = 0; index < 2 * (HOST_AUDIO_RING_FRAMES + 16); index++)
			many[index] = 16384;
		CHECK(host_audio_put_stream_data(stream, many, (int)sizeof(many)));
		CHECK(host_audio_get_stream_available(stream) == HOST_AUDIO_RING_FRAMES * 4);
	}
	/* a full stream asks the callback for nothing */
	CHECK(host_audio_fill(stream) == 0 && guest_calls == 1);
	host_audio_destroy_stream(stream);
	CHECK(app_closes == 1);
	CHECK(host_audio_get_stream_available(stream) == -1);
	CHECK(host_audio_put_stream_data(stream, left, 8) == 0);

	/* ---------- the game's stream as it plays: float, on its own thread */
	spec = spec_of(SDL_AUDIO_F32, 2, 48000);
	stream = host_audio_open_stream(0, &spec, CALLBACK_ADDRESS, USERDATA_ADDRESS);
	CHECK(stream >= HOST_AUDIO_HANDLE_BASE && app_opens == 2);
	pthread_mutex_lock(&guest_lock);
	guest_calls = 0;
	guest_frames = 64;
	pthread_mutex_unlock(&guest_lock);
	/* (no callback before the stream plays) */
	CHECK(host_audio_get_stream_available(stream) == 0 && guest_calls == 0);
	CHECK(host_audio_resume_stream_device(stream) == 1 && app_resumes == 1);
	{
		struct timespec wait = { 0, 5000000 };
		int tries;

		for (tries = 0; tries < 400; tries++)
		{
			int calls;

			pthread_mutex_lock(&guest_lock);
			calls = guest_calls;
			pthread_mutex_unlock(&guest_lock);
			if (calls >= 2 && host_audio_get_stream_available(stream) > 0)
				break;
			nanosleep(&wait, NULL);
		}
		CHECK(tries < 400);
	}
	render(stream, left, right, 2);
	CHECK(near(left[0], 0.25f) && near(right[0], -0.25f) && near(left[1], 0.25f));
	/* (destroying it ends its thread, after which no callback is made) */
	host_audio_destroy_stream(stream);
	CHECK(app_closes == 2);
	{
		struct timespec wait = { 0, 20000000 };
		int calls;

		pthread_mutex_lock(&guest_lock);
		calls = guest_calls;
		pthread_mutex_unlock(&guest_lock);
		nanosleep(&wait, NULL);
		pthread_mutex_lock(&guest_lock);
		CHECK(guest_calls == calls);
		pthread_mutex_unlock(&guest_lock);
	}

	/* without the app's audio again, streams are SDL's */
	halo_host_set_audio_backend(NULL);
	stream = host_audio_open_stream(0, &spec, CALLBACK_ADDRESS, USERDATA_ADDRESS);
	CHECK(stream == 5 && sdl_opens == 3);
	puts("ok");
	return 0;
}
