/*
EFFECT_AUDIO.C

Plays a WAV file once, mixed into the game's output next to the players'
voices (voice_audio_mix calls effect_audio_mix). A file is read the first
time it is played, turned into 48 kHz stereo and kept; one that cannot be
read is not tried again for a few seconds, so a missing file costs nothing
while the game goes on.
*/

#include "platform.h"
#include "effect_audio.h"

#include <SDL3/SDL.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- constants */

enum
{
	EFFECT_RATE = 48000,
	EFFECT_VOICES = 8,
	EFFECT_SAMPLES = 4,
	/* the longest sound kept, and the biggest file read */
	EFFECT_MAXIMUM_SECONDS = 10,
	EFFECT_MAXIMUM_FILE = 16 * 1024 * 1024,
	/* a file that failed is not tried again for this long (milliseconds) */
	EFFECT_RETRY_MILLISECONDS = 10000,
};

/* ---------- structures */

struct effect_sample
{
	char path[512];
	float *frames;
	unsigned long count;
	Uint64 failed_at;
	int failed;
};

struct effect_voice
{
	struct effect_sample *sample;
	unsigned long position;
	float gain;
};

/* ---------- globals */

static pthread_mutex_t effect_lock = PTHREAD_MUTEX_INITIALIZER;
static struct effect_sample effect_samples[EFFECT_SAMPLES];
static struct effect_voice effect_voices[EFFECT_VOICES];

/* ---------- private code */

static unsigned long effect_u16(const unsigned char *data)
{
	return (unsigned long)data[0] | ((unsigned long)data[1] << 8);
}

static unsigned long effect_u32(const unsigned char *data)
{
	return effect_u16(data) | (effect_u16(data + 2) << 16);
}

/* one sample, -1 to 1 */
static float effect_decode(const unsigned char *data, int bits, int is_float)
{
	if (is_float && bits == 32)
	{
		union { unsigned int u; float f; } value;

		value.u = (unsigned int)effect_u32(data);
		return value.f;
	}
	switch (bits)
	{
	case 8:
		return ((float)data[0] - 128.0f) / 128.0f;
	case 16:
		return (float)(short)effect_u16(data) / 32768.0f;
	case 24:
	{
		unsigned int value = ((unsigned int)data[0] << 8) | ((unsigned int)data[1] << 16) |
			((unsigned int)data[2] << 24);

		return (float)((int)value / 256) / 8388608.0f;
	}
	case 32:
		return (float)((double)(int)(unsigned int)effect_u32(data) / 2147483648.0);
	default:
		return 0.0f;
	}
}

/* a WAV file as 48 kHz stereo; 1 if it was read */
static int effect_load(
	struct effect_sample *sample)
{
	FILE *file = fopen(sample->path, "rb");
	unsigned char *data;
	long size;
	unsigned long offset = 12;
	unsigned long format = 0, channels = 0, rate = 0, bits = 0, data_offset = 0, data_size = 0;
	unsigned long frames, count, index;
	float *result;

	if (!file)
		return 0;
	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fseek(file, 0, SEEK_SET);
	if (size < 44 || size > EFFECT_MAXIMUM_FILE)
	{
		fclose(file);
		return 0;
	}
	data = (unsigned char *)malloc((size_t)size);
	if (!data || fread(data, 1, (size_t)size, file) != (size_t)size)
	{
		free(data);
		fclose(file);
		return 0;
	}
	fclose(file);
	if (memcmp(data, "RIFF", 4) || memcmp(data + 8, "WAVE", 4))
	{
		free(data);
		return 0;
	}
	while (offset + 8 <= (unsigned long)size)
	{
		unsigned long length = effect_u32(data + offset + 4);

		if (!memcmp(data + offset, "fmt ", 4) && length >= 16 && offset + 8 + length <= (unsigned long)size)
		{
			format = effect_u16(data + offset + 8);
			channels = effect_u16(data + offset + 10);
			rate = effect_u32(data + offset + 12);
			bits = effect_u16(data + offset + 22);
			/* (extensible: the real format is the sub-format's first word) */
			if (format == 0xFFFE && length >= 26)
				format = effect_u16(data + offset + 32);
		}
		else if (!memcmp(data + offset, "data", 4))
		{
			data_offset = offset + 8;
			data_size = length;
			if (data_offset + data_size > (unsigned long)size)
				data_size = (unsigned long)size - data_offset;
			break;
		}
		offset += 8 + length + (length & 1);
	}
	if ((format != 1 && format != 3) || channels < 1 || rate < 1000 || rate > 400000 ||
		(bits != 8 && bits != 16 && bits != 24 && bits != 32) || !data_offset ||
		(format == 3 && bits != 32))
	{
		free(data);
		return 0;
	}
	frames = data_size / (channels * (bits / 8));
	if (frames == 0)
	{
		free(data);
		return 0;
	}
	count = (unsigned long)((double)frames * EFFECT_RATE / (double)rate);
	if (count > (unsigned long)EFFECT_RATE * EFFECT_MAXIMUM_SECONDS)
		count = (unsigned long)EFFECT_RATE * EFFECT_MAXIMUM_SECONDS;
	if (count == 0)
	{
		free(data);
		return 0;
	}
	result = (float *)malloc(count * 2 * sizeof(float));
	if (!result)
	{
		free(data);
		return 0;
	}
	for (index = 0; index < count; index++)
	{
		double source = (double)index * (double)rate / EFFECT_RATE;
		unsigned long first = (unsigned long)source;
		unsigned long second = first + 1 < frames ? first + 1 : first;
		float blend = (float)(source - (double)first);
		int side;

		for (side = 0; side < 2; side++)
		{
			unsigned long channel = (unsigned long)side < channels ? (unsigned long)side : 0;
			float a = effect_decode(data + data_offset + (first * channels + channel) * (bits / 8), (int)bits, format == 3);
			float b = effect_decode(data + data_offset + (second * channels + channel) * (bits / 8), (int)bits, format == 3);

			result[index * 2 + side] = a + (b - a) * blend;
		}
	}
	free(data);
	sample->frames = result;
	sample->count = count;
	return 1;
}

/* ---------- public code */

int effect_audio_play_file(
	const char *path,
	float gain)
{
	struct effect_sample *sample = NULL;
	struct effect_sample *free_sample = NULL;
	int index;
	Uint64 now = SDL_GetTicks();

	if (!path || !path[0] || strlen(path) >= sizeof(effect_samples[0].path))
		return 0;
	pthread_mutex_lock(&effect_lock);
	for (index = 0; index < EFFECT_SAMPLES; index++)
	{
		if (effect_samples[index].path[0] && !strcmp(effect_samples[index].path, path))
		{
			sample = &effect_samples[index];
			break;
		}
		if (!effect_samples[index].path[0] && !free_sample)
			free_sample = &effect_samples[index];
	}
	if (!sample)
	{
		if (!free_sample)
		{
			pthread_mutex_unlock(&effect_lock);
			return 0;
		}
		sample = free_sample;
		strcpy(sample->path, path);
	}
	if (!sample->frames)
	{
		if (sample->failed && now - sample->failed_at < EFFECT_RETRY_MILLISECONDS)
		{
			pthread_mutex_unlock(&effect_lock);
			return 0;
		}
		/* (read with the lock held: the mix waits a moment, once) */
		if (!effect_load(sample))
		{
			if (!sample->failed)
				platform_log("effects: could not read %s (a WAV file: 8, 16, 24 or 32 bit PCM)", path);
			sample->failed = 1;
			sample->failed_at = now;
			pthread_mutex_unlock(&effect_lock);
			return 0;
		}
		sample->failed = 0;
	}
	for (index = 0; index < EFFECT_VOICES; index++)
	{
		if (!effect_voices[index].sample)
			break;
	}
	if (index == EFFECT_VOICES)
		index = 0;
	effect_voices[index].sample = sample;
	effect_voices[index].position = 0;
	effect_voices[index].gain = gain < 0.0f ? 0.0f : gain > 4.0f ? 4.0f : gain;
	pthread_mutex_unlock(&effect_lock);

	return 1;
}

void effect_audio_mix(
	float *output,
	unsigned long frames)
{
	int index;

	pthread_mutex_lock(&effect_lock);
	for (index = 0; index < EFFECT_VOICES; index++)
	{
		struct effect_voice *voice = &effect_voices[index];
		unsigned long frame, count;

		if (!voice->sample)
			continue;
		count = voice->sample->count - voice->position;
		if (count > frames)
			count = frames;
		for (frame = 0; frame < count; frame++)
		{
			output[frame * 2] += voice->sample->frames[(voice->position + frame) * 2] * voice->gain;
			output[frame * 2 + 1] += voice->sample->frames[(voice->position + frame) * 2 + 1] * voice->gain;
		}
		voice->position += count;
		if (voice->position >= voice->sample->count)
			voice->sample = NULL;
	}
	pthread_mutex_unlock(&effect_lock);
}
