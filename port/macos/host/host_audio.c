/*
HOST_AUDIO.C

The guest's sound: the app's (port/macos/app's OpenCEAudio, an AVAudioEngine
whose source node pulls from the streams here) once it has installed its
audio with halo_host_set_audio_backend, else SDL's, as the command-line host
has it. Only a playback stream with a callback (the game's,
port/linux/src/dsound_sdl.c) is the app's; one without, the microphone's
(port/linux/src/voice_audio.c), stays SDL's.

An app stream keeps a ring of float frames, which the guest writes and the
engine's render block reads, on Core Audio's real-time thread. That thread
must not wait, and has no vCPU, so it never calls the guest: it takes what
the ring holds (silence for what it lacks) and wakes the stream's own
thread, which calls the guest's callback, as SDL would, for what the ring
lacks of its target. The guest's data is converted to float as it is put in.
*/

#include "host.h"
#include "halo_host.h"

#include <SDL3/SDL.h>
#include <dispatch/dispatch.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#define STREAM_COUNT 8
#define MAXIMUM_CHANNELS 8

struct audio_stream
{
	int used;
	uint32_t handle;
	SDL_AudioFormat format;
	int sample_size;
	int channels;
	int rate;
	uint32_t callback;
	uint32_t userdata;
	/* HOST_AUDIO_RING_FRAMES interleaved frames; read and write count
	frames from the start, and wrap */
	float *ring;
	_Atomic uint32_t read;
	_Atomic uint32_t write;
	/* the stream's thread: woken by renders, told to end by quit */
	dispatch_semaphore_t wake;
	int playing;
	_Atomic int quit;
	pthread_mutex_t lock;
	pthread_cond_t ended;
	int thread_ended;
};

static struct halo_host_audio_backend audio_backend;
static int audio_backend_set;
static struct audio_stream streams[STREAM_COUNT];
static pthread_mutex_t streams_lock = PTHREAD_MUTEX_INITIALIZER;

void halo_host_set_audio_backend(const struct halo_host_audio_backend *backend)
{
	audio_backend_set = backend != NULL;
	if (backend)
		audio_backend = *backend;
}

/* an app stream's, or NULL for a handle that is not one */
static struct audio_stream *stream_get(uint32_t handle)
{
	struct audio_stream *stream;

	if (handle < HOST_AUDIO_HANDLE_BASE || handle >= HOST_AUDIO_HANDLE_BASE + STREAM_COUNT)
		return NULL;
	stream = &streams[handle - HOST_AUDIO_HANDLE_BASE];
	return stream->used ? stream : NULL;
}

/* the bytes of a sample of `format`, or 0 for one the host does not convert */
static int sample_size(SDL_AudioFormat format)
{
	switch (format)
	{
	case SDL_AUDIO_U8:
	case SDL_AUDIO_S8:
		return 1;
	case SDL_AUDIO_S16LE:
		return 2;
	case SDL_AUDIO_S32LE:
	case SDL_AUDIO_F32LE:
		return 4;
	default:
		return 0;
	}
}

static float sample_value(SDL_AudioFormat format, const unsigned char *data)
{
	switch (format)
	{
	case SDL_AUDIO_U8:
		return (float)((int)data[0] - 128) / 128.0f;
	case SDL_AUDIO_S8:
		return (float)(signed char)data[0] / 128.0f;
	case SDL_AUDIO_S16LE:
	{
		int16_t value;

		memcpy(&value, data, sizeof(value));
		return (float)value / 32768.0f;
	}
	case SDL_AUDIO_S32LE:
	{
		int32_t value;

		memcpy(&value, data, sizeof(value));
		return (float)((double)value / 2147483648.0);
	}
	default:
	{
		float value;

		memcpy(&value, data, sizeof(value));
		return value;
	}
	}
}

/* the frames the stream's thread keeps queued for the engine at `rate` Hz:
about 30 ms, and never less than a render of Core Audio's (512 frames) and a
chunk of the game's mixer (1024) together, so that one late callback of the
guest's is not heard */
static uint32_t target_frames(int rate)
{
	uint32_t frames = (uint32_t)rate * 3 / 100;

	if (frames < 1536)
		frames = 1536;
	return frames;
}

/* ---------- the stream's thread */

/* the stream whose callback this thread is running, if any */
static __thread struct audio_stream *calling_back;

int host_audio_fill(uint32_t handle)
{
	struct audio_stream *stream = stream_get(handle);
	uint32_t queued, target;
	int additional;

	if (!stream)
		return 0;
	queued = atomic_load(&stream->write) - atomic_load(&stream->read);
	target = target_frames(stream->rate);
	if (target > HOST_AUDIO_RING_FRAMES)
		target = HOST_AUDIO_RING_FRAMES;
	if (queued >= target)
		return 0;
	additional = (int)(target - queued) * stream->sample_size * stream->channels;
	calling_back = stream;
	host_call_guest(stream->callback, stream->userdata, stream->handle, (uint32_t)additional,
		(uint32_t)additional);
	calling_back = NULL;
	return 1;
}

static void *audio_thread(void *context)
{
	struct audio_stream *stream = context;

	while (!atomic_load(&stream->quit))
	{
		host_audio_fill(stream->handle);
		/* (woken by a render, or after a few milliseconds if the guest put
		less than it was asked for) */
		dispatch_semaphore_wait(stream->wake, dispatch_time(DISPATCH_TIME_NOW, 5 * NSEC_PER_MSEC));
	}
	pthread_mutex_lock(&stream->lock);
	stream->thread_ended = 1;
	pthread_cond_signal(&stream->ended);
	pthread_mutex_unlock(&stream->lock);
	return NULL;
}

/* ---------- the engine's renders (Core Audio's real-time thread: no locks,
no waiting) */

void halo_host_audio_render(uint32_t handle, float *const *channels, int channel_count, int frames)
{
	struct audio_stream *stream = stream_get(handle);
	uint32_t read, available, frame;
	int channel;

	if (frames <= 0)
		return;
	read = stream ? atomic_load_explicit(&stream->read, memory_order_relaxed) : 0;
	available = stream ? atomic_load_explicit(&stream->write, memory_order_acquire) - read : 0;
	if (available > (uint32_t)frames)
		available = (uint32_t)frames;
	for (channel = 0; channel < channel_count; channel++)
	{
		float *out = channels[channel];

		if (!out)
			continue;
		frame = 0;
		if (channel < (stream ? stream->channels : 0))
		{
			for (; frame < available; frame++)
				out[frame] = stream->ring[((read + frame) % HOST_AUDIO_RING_FRAMES) * (uint32_t)stream->channels
					+ (uint32_t)channel];
		}
		for (; frame < (uint32_t)frames; frame++)
			out[frame] = 0.0f;
	}
	if (stream)
	{
		atomic_store_explicit(&stream->read, read + available, memory_order_release);
		dispatch_semaphore_signal(stream->wake);
	}
}

/* ---------- the imports */

uint32_t host_audio_open_stream(uint32_t device, const void *spec, uint32_t callback, uint32_t userdata)
{
	SDL_AudioSpec format;
	struct audio_stream *stream = NULL;
	uint32_t handle;
	int index;

	if (!audio_backend_set || !callback)
		return host_sdl_open_audio_stream(device, spec, callback, userdata);
	if (!spec)
		return 0;
	memcpy(&format, spec, sizeof(format));
	if (!sample_size(format.format) || format.channels < 1 || format.channels > MAXIMUM_CHANNELS
		|| format.freq < 1 || format.freq > 384000)
	{
		host_logf(HOST_LOG_WARN, "an audio stream of format 0x%x, %d channels at %d Hz is refused",
			(unsigned)format.format, format.channels, format.freq);
		return 0;
	}
	pthread_mutex_lock(&streams_lock);
	for (index = 0; index < STREAM_COUNT; index++)
	{
		if (!streams[index].used)
		{
			stream = &streams[index];
			break;
		}
	}
	if (!stream)
	{
		pthread_mutex_unlock(&streams_lock);
		return 0;
	}
	memset(stream, 0, sizeof(*stream));
	stream->ring = calloc(HOST_AUDIO_RING_FRAMES * (size_t)format.channels, sizeof(float));
	stream->wake = dispatch_semaphore_create(0);
	if (!stream->ring || !stream->wake)
	{
		free(stream->ring);
		pthread_mutex_unlock(&streams_lock);
		return 0;
	}
	handle = HOST_AUDIO_HANDLE_BASE + (uint32_t)index;
	stream->handle = handle;
	stream->format = format.format;
	stream->sample_size = sample_size(format.format);
	stream->channels = format.channels;
	stream->rate = format.freq;
	stream->callback = callback;
	stream->userdata = userdata;
	pthread_mutex_init(&stream->lock, NULL);
	pthread_cond_init(&stream->ended, NULL);
	stream->used = 1;
	pthread_mutex_unlock(&streams_lock);
	if (!audio_backend.open(audio_backend.context, handle, format.channels, format.freq))
	{
		pthread_mutex_lock(&streams_lock);
		stream->used = 0;
		pthread_mutex_unlock(&streams_lock);
		pthread_cond_destroy(&stream->ended);
		pthread_mutex_destroy(&stream->lock);
		dispatch_release(stream->wake);
		free(stream->ring);
		return 0;
	}
	return handle;
}

/* converted, as many whole frames as the ring has room for; 1 on success */
int host_audio_put_stream_data(uint32_t handle, const void *data, int length)
{
	struct audio_stream *stream = stream_get(handle);
	const unsigned char *bytes = data;
	uint32_t write, room, frames, frame;
	int frame_size, channel;

	if (handle && handle < HOST_AUDIO_HANDLE_BASE)
		return host_sdl_put_audio_stream_data(handle, data, length);
	if (!stream || length < 0 || (length && !data))
		return 0;
	frame_size = stream->sample_size * stream->channels;
	if (length % frame_size)
		return 0;
	write = atomic_load_explicit(&stream->write, memory_order_relaxed);
	room = HOST_AUDIO_RING_FRAMES - (write - atomic_load_explicit(&stream->read, memory_order_acquire));
	frames = (uint32_t)(length / frame_size);
	if (frames > room)
		frames = room;
	for (frame = 0; frame < frames; frame++)
	{
		float *out = &stream->ring[((write + frame) % HOST_AUDIO_RING_FRAMES) * (uint32_t)stream->channels];

		for (channel = 0; channel < stream->channels; channel++)
			out[channel] = sample_value(stream->format, bytes + frame * (uint32_t)frame_size
				+ (uint32_t)(channel * stream->sample_size));
	}
	atomic_store_explicit(&stream->write, write + frames, memory_order_release);
	return 1;
}

int host_audio_resume_stream_device(uint32_t handle)
{
	struct audio_stream *stream = stream_get(handle);

	if (handle && handle < HOST_AUDIO_HANDLE_BASE)
		return host_sdl_resume_audio_stream_device(handle);
	if (!stream)
		return 0;
	if (!stream->playing)
	{
		if (host_native_thread_create(audio_thread, stream, 256 * 1024) != 0)
			host_fatal("cannot start the audio thread");
		stream->playing = 1;
	}
	return audio_backend.resume(audio_backend.context, handle);
}

/* (an app stream only plays: nothing is read from it) */
int host_audio_get_stream_data(uint32_t handle, void *data, int length)
{
	if (handle && handle < HOST_AUDIO_HANDLE_BASE)
		return host_sdl_get_audio_stream_data(handle, data, length);
	return -1;
}

/* in the stream's own format */
int host_audio_get_stream_available(uint32_t handle)
{
	struct audio_stream *stream = stream_get(handle);

	if (handle && handle < HOST_AUDIO_HANDLE_BASE)
		return host_sdl_get_audio_stream_available(handle);
	if (!stream)
		return -1;
	return (int)(atomic_load(&stream->write) - atomic_load(&stream->read)) * stream->sample_size * stream->channels;
}

/* (once the engine has stopped, no render reads the stream; once its thread
has ended, no callback runs) */
void host_audio_destroy_stream(uint32_t handle)
{
	struct audio_stream *stream = stream_get(handle);

	if (handle && handle < HOST_AUDIO_HANDLE_BASE)
	{
		host_sdl_destroy_audio_stream(handle);
		return;
	}
	if (!stream || calling_back == stream)
		return;
	audio_backend.close(audio_backend.context, handle);
	if (stream->playing)
	{
		atomic_store(&stream->quit, 1);
		dispatch_semaphore_signal(stream->wake);
		pthread_mutex_lock(&stream->lock);
		while (!stream->thread_ended)
			pthread_cond_wait(&stream->ended, &stream->lock);
		pthread_mutex_unlock(&stream->lock);
	}
	pthread_mutex_lock(&streams_lock);
	stream->used = 0;
	pthread_mutex_unlock(&streams_lock);
	pthread_cond_destroy(&stream->ended);
	pthread_mutex_destroy(&stream->lock);
	dispatch_release(stream->wake);
	free(stream->ring);
	stream->ring = NULL;
}
