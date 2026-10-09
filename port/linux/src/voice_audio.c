/*
VOICE_AUDIO.C

Voice chat's sound (port/linux/game/network_voice.c decides who hears whom):
the microphone, read in frames of 20 ms at 48 kHz, mono, and encoded with
Opus (port/third_party/opus); and the other players' voices, decoded into a
buffer each and mixed into the game's output (dsound_sdl.c's mix, at its
48 kHz, stereo) at their gain and pan.

The microphone is opened on its own thread (SDL asks Android for the
permission to record there, and waits for the answer), and only while voice
chat may send: network_voice.c closes it otherwise. A speaker plays once
VOICE_PREBUFFER_SAMPLES are buffered (the network's jitter), and waits again
when its buffer runs out; a frame lost in between is made up from the next
one's redundancy (Opus's in-band FEC), or by concealment.
*/

#include "platform.h"
#include "sdl_platform.h"
#include "port_config.h"
#include "voice_audio.h"
#include "../../third_party/opus/include/opus.h"

#include <SDL3/SDL.h>
#include <math.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define VOICE_RATE 48000
/* a speaker's buffer, and how much it holds before it plays */
#define VOICE_RING_SAMPLES (VOICE_RATE / 2)
#define VOICE_PREBUFFER_SAMPLES (3 * VOICE_FRAME_SAMPLES)
/* how many speakers are decoded at once, and how long one unheard keeps
its decoder (milliseconds) */
#define VOICE_SLOTS 16
#define VOICE_SLOT_IDLE_MS 5000
/* frames made up for when packets are lost, at most; more lost, it starts
over */
#define VOICE_MAXIMUM_CONCEALED 3
/* the microphone's frames kept before the oldest are dropped (a game that
stalled must not talk late) */
#define VOICE_MAXIMUM_BACKLOG (5 * VOICE_FRAME_SAMPLES)
/* how often a microphone that would not open is tried again (ms) */
#define VOICE_RETRY_MS 10000

struct voice_slot
{
	BOOL used;
	int speaker;
	OpusDecoder *decoder;
	/* the sequence next expected; FALSE: none yet */
	BOOL sequenced;
	unsigned short next_sequence;
	Uint64 heard_ms;
	/* the decoded samples, from read for count */
	float ring[VOICE_RING_SAMPLES];
	int read, count;
	BOOL playing;
	/* the gain and pan wanted, and those mixed last */
	float gain, pan;
	float left, right;
	/* (debug.voice_test: when it was logged) */
	Uint64 logged_ms;
};

static pthread_mutex_t voice_lock = PTHREAD_MUTEX_INITIALIZER;
static struct voice_slot voice_slots[VOICE_SLOTS];
static float voice_volume = 1.0f;

/* the microphone (under voice_lock: the opening thread sets it) */
static SDL_AudioStream *microphone;
/* the device it was opened on (audio.input_device), and the one to open */
static char microphone_device[PLATFORM_AUDIO_DEVICE_NAME_SIZE];
static char microphone_opening_device[PLATFORM_AUDIO_DEVICE_NAME_SIZE];
static BOOL microphone_wanted;
static BOOL microphone_opening;
static Uint64 microphone_failed_ms;
static BOOL microphone_failed;
static float capture[VOICE_FRAME_SAMPLES];
static int capture_count;
static OpusEncoder *encoder;
static int encoder_bitrate;
/* debug.voice_test (the automated tests): a tone instead of the microphone,
a frame each 20 ms since the first; and each speaker's level logged once a
second */
static int voice_test = -1;
static Uint64 voice_test_started_ms;
static unsigned long voice_test_frames;
static double voice_test_phase;

/* ---------- the microphone */

static void *microphone_open_thread(void *parameter)
{
	SDL_AudioSpec spec;
	SDL_AudioStream *stream;

	(void)parameter;
	spec.format = SDL_AUDIO_F32;
	spec.channels = 1;
	spec.freq = VOICE_RATE;
	stream = SDL_OpenAudioDeviceStream(platform_audio_device(TRUE, microphone_opening_device), &spec, NULL, NULL);
	if (!stream && strcmp(microphone_opening_device, "default"))
		stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_RECORDING, &spec, NULL, NULL);
	if (stream)
		SDL_ResumeAudioStreamDevice(stream);
	else
		platform_log("voice chat: no microphone (%s)", SDL_GetError());
	pthread_mutex_lock(&voice_lock);
	microphone_opening = FALSE;
	if (stream && !microphone_wanted)
	{
		/* (closed again meanwhile) */
		pthread_mutex_unlock(&voice_lock);
		SDL_DestroyAudioStream(stream);
		return NULL;
	}
	microphone = stream;
	snprintf(microphone_device, sizeof(microphone_device), "%s", microphone_opening_device);
	microphone_failed = stream == NULL;
	microphone_failed_ms = SDL_GetTicks();
	pthread_mutex_unlock(&voice_lock);
	if (stream)
		platform_log("voice chat: microphone open");
	return NULL;
}

static BOOL voice_testing(void)
{
	if (voice_test < 0)
		voice_test = config_boolean("debug.voice_test") ? 1 : 0;
	return voice_test != 0;
}

int voice_audio_microphone(int open)
{
	SDL_AudioStream *closing = NULL;
	BOOL start = FALSE;
	BOOL ready;

	if (voice_testing())
	{
		if (open && !voice_test_started_ms)
		{
			voice_test_started_ms = SDL_GetTicks();
			voice_test_frames = 0;
		}
		if (!open)
			voice_test_started_ms = 0;
		return open;
	}
	if (!platform_sdl_initialize())
		return FALSE;
	{
		/* (audio.input_device: Settings > Audio's, none on Android) */
		const char *device = config_string("audio.input_device");

		device = device && device[0] ? device : "default";
		pthread_mutex_lock(&voice_lock);
		/* (another device chosen: the one open closed, and the new opened) */
		if (microphone && strcmp(microphone_device, device))
		{
			closing = microphone;
			microphone = NULL;
			microphone_failed = FALSE;
		}
		if (!microphone_opening)
			snprintf(microphone_opening_device, sizeof(microphone_opening_device), "%s", device);
		pthread_mutex_unlock(&voice_lock);
		if (closing)
		{
			SDL_DestroyAudioStream(closing);
			closing = NULL;
			capture_count = 0;
		}
	}
	pthread_mutex_lock(&voice_lock);
	microphone_wanted = open;
	if (open && !microphone && !microphone_opening &&
		(!microphone_failed || SDL_GetTicks() - microphone_failed_ms >= VOICE_RETRY_MS))
	{
		microphone_opening = TRUE;
		start = TRUE;
	}
	if (!open && microphone)
	{
		closing = microphone;
		microphone = NULL;
		microphone_failed = FALSE;
	}
	ready = microphone != NULL;
	pthread_mutex_unlock(&voice_lock);
	if (closing)
	{
		SDL_DestroyAudioStream(closing);
		capture_count = 0;
		platform_log("voice chat: microphone closed");
	}
	if (start)
	{
		pthread_t thread;

		if (pthread_create(&thread, NULL, microphone_open_thread, NULL) == 0)
			pthread_detach(thread);
		else
		{
			pthread_mutex_lock(&voice_lock);
			microphone_opening = FALSE;
			pthread_mutex_unlock(&voice_lock);
		}
	}
	return ready;
}

int voice_audio_read_frame(float *frame)
{
	SDL_AudioStream *stream;

	if (voice_testing())
	{
		int index;

		if (!voice_test_started_ms ||
			(SDL_GetTicks() - voice_test_started_ms) / 20 <= voice_test_frames)
		{
			return FALSE;
		}
		voice_test_frames++;
		for (index = 0; index < VOICE_FRAME_SAMPLES; index++)
		{
			frame[index] = 0.3f * (float)sin(voice_test_phase);
			voice_test_phase += 2.0 * 3.14159265358979 * 440.0 / VOICE_RATE;
		}
		voice_test_phase = fmod(voice_test_phase, 2.0 * 3.14159265358979);
		return TRUE;
	}

	pthread_mutex_lock(&voice_lock);
	stream = microphone;
	pthread_mutex_unlock(&voice_lock);
	if (!stream)
		return FALSE;
	/* (a backlog dropped but for the latest frames) */
	{
		int available = SDL_GetAudioStreamAvailable(stream) / (int)sizeof(float);

		while (available > VOICE_MAXIMUM_BACKLOG)
		{
			float discard[VOICE_FRAME_SAMPLES];
			int take = available - VOICE_MAXIMUM_BACKLOG;

			if (take > VOICE_FRAME_SAMPLES)
				take = VOICE_FRAME_SAMPLES;
			if (SDL_GetAudioStreamData(stream, discard, take * (int)sizeof(float)) <= 0)
				break;
			available -= take;
		}
	}
	while (capture_count < VOICE_FRAME_SAMPLES)
	{
		int got = SDL_GetAudioStreamData(stream, capture + capture_count,
			(VOICE_FRAME_SAMPLES - capture_count) * (int)sizeof(float));

		if (got <= 0)
			return FALSE;
		capture_count += got / (int)sizeof(float);
	}
	memcpy(frame, capture, sizeof(capture));
	capture_count = 0;
	return TRUE;
}

float voice_audio_level(const float *frame)
{
	double sum = 0.0;
	int index;

	for (index = 0; index < VOICE_FRAME_SAMPLES; index++)
		sum += (double)frame[index] * frame[index];
	return (float)sqrt(sum / VOICE_FRAME_SAMPLES);
}

int voice_audio_encode(const float *frame, int bitrate, unsigned char *packet, int maximum)
{
	int length;

	if (!encoder)
	{
		int error = 0;

		encoder = opus_encoder_create(VOICE_RATE, 1, OPUS_APPLICATION_VOIP, &error);
		if (!encoder)
		{
			platform_log("voice chat: no Opus encoder (%d)", error);
			return 0;
		}
		/* (a lost frame made up from the next's redundancy; modest
		complexity: Android runs the game emulated) */
		opus_encoder_ctl(encoder, OPUS_SET_INBAND_FEC(1));
		opus_encoder_ctl(encoder, OPUS_SET_PACKET_LOSS_PERC(10));
		opus_encoder_ctl(encoder, OPUS_SET_COMPLEXITY(5));
		opus_encoder_ctl(encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
		encoder_bitrate = 0;
	}
	if (bitrate != encoder_bitrate)
	{
		opus_encoder_ctl(encoder, OPUS_SET_BITRATE(bitrate));
		encoder_bitrate = bitrate;
	}
	length = opus_encode_float(encoder, frame, VOICE_FRAME_SAMPLES, packet, maximum);
	return length > 0 ? length : 0;
}

/* ---------- the voices */

/* (under voice_lock) the slot of a speaker, or a free one (or the longest
unheard) taken for it */
static struct voice_slot *voice_slot_for(int speaker, BOOL create)
{
	Uint64 now = SDL_GetTicks();
	struct voice_slot *oldest = NULL;
	int index;

	for (index = 0; index < VOICE_SLOTS; index++)
	{
		if (voice_slots[index].used && voice_slots[index].speaker == speaker)
			return &voice_slots[index];
	}
	if (!create)
		return NULL;
	for (index = 0; index < VOICE_SLOTS; index++)
	{
		struct voice_slot *slot = &voice_slots[index];

		if (!slot->used)
		{
			oldest = slot;
			break;
		}
		if (now - slot->heard_ms >= VOICE_SLOT_IDLE_MS && (!oldest || slot->heard_ms < oldest->heard_ms))
			oldest = slot;
	}
	if (!oldest)
		return NULL;
	if (!oldest->decoder)
	{
		int error = 0;

		oldest->decoder = opus_decoder_create(VOICE_RATE, 1, &error);
		if (!oldest->decoder)
			return NULL;
	}
	else
	{
		opus_decoder_ctl(oldest->decoder, OPUS_RESET_STATE);
	}
	oldest->used = TRUE;
	oldest->speaker = speaker;
	oldest->sequenced = FALSE;
	oldest->read = oldest->count = 0;
	oldest->playing = FALSE;
	oldest->left = oldest->right = 0.0f;
	return oldest;
}

/* (under voice_lock) samples into a slot's buffer, the oldest dropped when
it is full */
static void voice_slot_put(struct voice_slot *slot, const float *samples, int count)
{
	int index;

	for (index = 0; index < count; index++)
	{
		if (slot->count == VOICE_RING_SAMPLES)
		{
			slot->read = (slot->read + 1) % VOICE_RING_SAMPLES;
			slot->count--;
		}
		slot->ring[(slot->read + slot->count) % VOICE_RING_SAMPLES] = samples[index];
		slot->count++;
	}
}

void voice_audio_play(int speaker, unsigned short sequence, const unsigned char *packet, int length, float gain,
	float pan)
{
	float samples[(VOICE_MAXIMUM_CONCEALED + 1) * VOICE_FRAME_SAMPLES];
	struct voice_slot *slot;
	OpusDecoder *decoder;
	int lost = 0;
	int total = 0;
	int decoded;

	if (length <= 0 || length > VOICE_MAXIMUM_PACKET)
		return;
	pthread_mutex_lock(&voice_lock);
	slot = voice_slot_for(speaker, TRUE);
	if (!slot)
	{
		pthread_mutex_unlock(&voice_lock);
		return;
	}
	slot->heard_ms = SDL_GetTicks();
	slot->gain = gain < 0.0f ? 0.0f : gain > 2.0f ? 2.0f : gain;
	slot->pan = pan < -1.0f ? -1.0f : pan > 1.0f ? 1.0f : pan;
	if (slot->sequenced)
	{
		unsigned short ahead = (unsigned short)(sequence - slot->next_sequence);

		/* (late, or repeated: dropped) */
		if (ahead >= 0x8000)
		{
			pthread_mutex_unlock(&voice_lock);
			return;
		}
		/* (a few lost are made up for; more, it starts over) */
		if (ahead <= VOICE_MAXIMUM_CONCEALED)
			lost = ahead;
	}
	slot->sequenced = TRUE;
	slot->next_sequence = (unsigned short)(sequence + 1);
	decoder = slot->decoder;
	pthread_mutex_unlock(&voice_lock);
	/* (decoded outside the lock, which the mixer waits on: only this
	thread uses a slot's decoder, and only it gives slots to speakers) */
	while (lost > 0)
	{
		/* (the last lost from this packet's redundancy, those before it
		concealed) */
		BOOL redundancy = lost == 1;

		decoded = opus_decode_float(decoder, redundancy ? packet : NULL, redundancy ? length : 0,
			samples + total, VOICE_FRAME_SAMPLES, redundancy ? 1 : 0);
		if (decoded > 0)
			total += decoded;
		lost--;
	}
	decoded = opus_decode_float(decoder, packet, length, samples + total, VOICE_FRAME_SAMPLES, 0);
	if (decoded > 0)
		total += decoded;
	/* (the tests: what each speaker sounds like, once a second) */
	if (voice_testing() && total >= VOICE_FRAME_SAMPLES)
	{
		Uint64 now = SDL_GetTicks();

		if (now - slot->logged_ms >= 1000)
		{
			slot->logged_ms = now;
			platform_log("voice test: speaker %d level %.3f gain %.2f pan %.2f bytes %d", speaker,
				voice_audio_level(samples + total - VOICE_FRAME_SAMPLES), gain, pan, length);
		}
	}
	pthread_mutex_lock(&voice_lock);
	if (slot->used && slot->speaker == speaker)
		voice_slot_put(slot, samples, total);
	pthread_mutex_unlock(&voice_lock);
}

void voice_audio_forget(int speaker)
{
	struct voice_slot *slot;

	pthread_mutex_lock(&voice_lock);
	slot = voice_slot_for(speaker, FALSE);
	if (slot)
	{
		slot->used = FALSE;
		slot->count = 0;
		slot->playing = FALSE;
	}
	pthread_mutex_unlock(&voice_lock);
}

void voice_audio_forget_all(void)
{
	int index;

	pthread_mutex_lock(&voice_lock);
	for (index = 0; index < VOICE_SLOTS; index++)
	{
		voice_slots[index].used = FALSE;
		voice_slots[index].count = 0;
		voice_slots[index].playing = FALSE;
	}
	pthread_mutex_unlock(&voice_lock);
}

int voice_audio_speaking(int speaker)
{
	struct voice_slot *slot;
	BOOL speaking = FALSE;

	pthread_mutex_lock(&voice_lock);
	slot = voice_slot_for(speaker, FALSE);
	if (slot)
		speaking = slot->playing || slot->count >= VOICE_FRAME_SAMPLES;
	pthread_mutex_unlock(&voice_lock);
	return speaking;
}

void voice_audio_set_volume(float volume)
{
	voice_volume = volume < 0.0f ? 0.0f : volume > 2.0f ? 2.0f : volume;
}

void voice_audio_mix(float *output, unsigned long frames)
{
	int index;

	pthread_mutex_lock(&voice_lock);
	for (index = 0; index < VOICE_SLOTS; index++)
	{
		struct voice_slot *slot = &voice_slots[index];
		float angle, left, right, step_left, step_right;
		unsigned long frame, count;

		if (!slot->used)
			continue;
		if (!slot->playing && slot->count >= VOICE_PREBUFFER_SAMPLES)
			slot->playing = TRUE;
		if (!slot->playing)
			continue;
		/* (equal power: both sides at the gain when centred) */
		angle = (slot->pan + 1.0f) * 0.25f * 3.14159265f;
		left = slot->gain * voice_volume * cosf(angle) * 1.41421356f;
		right = slot->gain * voice_volume * sinf(angle) * 1.41421356f;
		count = (unsigned long)slot->count < frames ? (unsigned long)slot->count : frames;
		/* (the gains glide to their new values over the chunk) */
		step_left = count ? (left - slot->left) / (float)count : 0.0f;
		step_right = count ? (right - slot->right) / (float)count : 0.0f;
		for (frame = 0; frame < count; frame++)
		{
			float sample = slot->ring[slot->read];

			slot->left += step_left;
			slot->right += step_right;
			output[frame * 2] += sample * slot->left;
			output[frame * 2 + 1] += sample * slot->right;
			slot->read = (slot->read + 1) % VOICE_RING_SAMPLES;
		}
		slot->count -= (int)count;
		/* (run out: it waits to buffer again) */
		if (!slot->count)
			slot->playing = FALSE;
	}
	pthread_mutex_unlock(&voice_lock);
}
