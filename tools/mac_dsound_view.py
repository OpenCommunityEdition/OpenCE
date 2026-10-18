#!/usr/bin/env python
"""Make a checked Mac view of the real Linux DirectSound mixer.

Only the SDL device shell and little-endian PCM conversion are adapted. The
voice mixer, packet queue, effects, DirectSound interfaces and resampling code
are copied from port/linux/src/dsound_sdl.c unchanged.
"""
import sys

SOURCE = 'port/linux/src/dsound_sdl.c'

INCLUDES = '''#include "engine_xdk_prefix.h"
#include "port_config.h"
#include "voice_audio.h"
#include "mac_audio_output.h"
#include "mac_dsound_audio_stats.h"
#include "mac_audio_pcm.h"
#include "mac_gpu_info.h"
#include <mach/mach_time.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>'''

PCM = '''/* Diagnostic counts of voice mixes and of those at zero gain, updated under
   mixer_lock by mix_voice and read approximately by the statistics. */
static uint64_t voice_mixes;
static uint64_t voice_zero_gain_mixes;

static short *decode_pcm(const unsigned char *source, unsigned long size, unsigned long channels,
	unsigned long *frame_count)
{
	unsigned long frames;
	size_t capacity, decoded_frames = 0;
	short *samples;

	*frame_count = 0;
	if ((channels != 1 && channels != 2) || size % (2 * channels))
		return NULL;
	frames = size / (2 * channels);
	if (frames > (unsigned long)((size_t)-1) / channels / sizeof(*samples))
		return NULL;
	capacity = (size_t)frames * channels;
	samples = malloc((capacity ? capacity : channels) * sizeof(*samples));
	if (!samples)
		return NULL;
	if (!mac_audio_pcm16le_decode(source, (size_t)size, (unsigned)channels,
		(int16_t *)samples, capacity, &decoded_frames))
	{
		free(samples);
		return NULL;
	}
	*frame_count = (unsigned long)decoded_frames;
	return samples;
}'''

OUTPUT = '''/* ---------- OpenAL voices (HALO_MAC_AUDIO_OPENAL=1) */

#include "mac_dsound_openal.inc"

/* ---------- native output */

static struct mac_audio_output native_audio_output = { NULL };
static BOOL audio_started = FALSE;
static pthread_mutex_t silent_thread_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_t silent_thread;
static BOOL silent_thread_started;
static BOOL silent_thread_stop;
static uint64_t silent_callbacks;
static uint64_t silent_frames;
static uint64_t silent_nonzero_frames;
static struct mac_audio_output_stats closed_output_stats;
static BOOL output_started;
static uint64_t mix_calls;
static uint64_t mix_frames;
static uint64_t mix_nanoseconds;
static uint64_t mix_max_nanoseconds;
static mach_timebase_info_data_t mix_timebase;

/* Mix and record the time spent, on the AudioUnit or silent clock thread. */
static void timed_mix(float *output, unsigned long frames)
{
	uint64_t start, elapsed;

	start = mach_absolute_time();
	mix(output, frames);
	elapsed = mach_absolute_time() - start;
	if (mix_timebase.denom)
		elapsed = elapsed / mix_timebase.denom * mix_timebase.numer +
			elapsed % mix_timebase.denom * mix_timebase.numer / mix_timebase.denom;
	pthread_mutex_lock(&silent_thread_lock);
	++mix_calls;
	mix_frames += frames;
	mix_nanoseconds += elapsed;
	if (elapsed > mix_max_nanoseconds)
		mix_max_nanoseconds = elapsed;
	pthread_mutex_unlock(&silent_thread_lock);
}

static int native_audio_fill(void *context, float *output, uint32_t frames)
{
	(void)context;
	timed_mix(output, (unsigned long)frames);
	return 1;
}

/* If the default AudioUnit cannot open, keep the actual mixer advancing at
   its sample rate. mach_wait_until is available in the Tiger SDK; the thread
   is owned and joined when the final DirectSound reference is released. */
static void *silent_clock_thread(void *parameter)
{
	float buffer[480 * OUTPUT_CHANNELS];
	mach_timebase_info_data_t timebase;
	uint64_t deadline, interval;

	(void)parameter;
	if (mach_timebase_info(&timebase) != KERN_SUCCESS || !timebase.numer)
		return NULL;
	interval = (uint64_t)10000000 * timebase.denom / timebase.numer;
	if (!interval)
		interval = 1;
	deadline = mach_absolute_time();
	for (;;)
	{
		BOOL stop;
		uint64_t nonzero_frames = 0;
		unsigned long frame;
		kern_return_t wait_status;

		pthread_mutex_lock(&silent_thread_lock);
		stop = silent_thread_stop;
		pthread_mutex_unlock(&silent_thread_lock);
		if (stop)
			break;
		timed_mix(buffer, 480);
		for (frame = 0; frame < 480; frame++)
			if (buffer[frame * 2] != 0.0f || buffer[frame * 2 + 1] != 0.0f)
				++nonzero_frames;
		pthread_mutex_lock(&silent_thread_lock);
		++silent_callbacks;
		silent_frames += 480;
		silent_nonzero_frames += nonzero_frames;
		pthread_mutex_unlock(&silent_thread_lock);
		deadline += interval;
		wait_status = mach_wait_until(deadline);
		if (wait_status != KERN_SUCCESS)
		{
			platform_log("audio clock stopped after mach_wait_until error %ld", (long)wait_status);
			break;
		}
	}
	return NULL;
}

static void silent_clock_start(void)
{
	pthread_mutex_lock(&silent_thread_lock);
	silent_thread_stop = FALSE;
	if (!silent_thread_started)
	{
		if (pthread_create(&silent_thread, NULL, silent_clock_thread, NULL) == 0)
			silent_thread_started = TRUE;
		else
			platform_log("audio clock thread could not start");
	}
	pthread_mutex_unlock(&silent_thread_lock);
}

static void silent_clock_stop(void)
{
	pthread_t thread;
	BOOL join;

	pthread_mutex_lock(&silent_thread_lock);
	join = silent_thread_started;
	thread = silent_thread;
	silent_thread_stop = TRUE;
	pthread_mutex_unlock(&silent_thread_lock);
	if (join)
	{
		pthread_join(thread, NULL);
		pthread_mutex_lock(&silent_thread_lock);
		silent_thread_started = FALSE;
		pthread_mutex_unlock(&silent_thread_lock);
	}
}

/* port: audio.output_device, the device's name ("default": the system's) */
static char audio_device_name[128];
static const char *audio_device_setting(void)
{
	const char *name = config_string("audio.output_device");

	return name && name[0] ? name : "default";
}

static void audio_start(void)
{
	long status = 0;

	if (audio_started)
		return;
	audio_started = TRUE;
	pthread_mutex_lock(&silent_thread_lock);
	silent_callbacks = silent_frames = silent_nonzero_frames = 0;
	mix_calls = mix_frames = mix_nanoseconds = mix_max_nanoseconds = 0;
	if (mach_timebase_info(&mix_timebase) != KERN_SUCCESS)
		mix_timebase.numer = mix_timebase.denom = 0;
	memset(&closed_output_stats, 0, sizeof(closed_output_stats));
	output_started = FALSE;
	pthread_mutex_unlock(&silent_thread_lock);
	master_volume = (float)config_real("audio.volume");
	reverb_enabled = audio_reverb_setting();
	resampler_initialize();
	resampler_phases_initialize();
	reverb_initialize();
	if (config_boolean("audio.enabled"))
		openal_start();
	/* port: audio.output_device (mac_audio_output_open_device) */
	snprintf(audio_device_name, sizeof(audio_device_name), "%s", audio_device_setting());
	if (config_boolean("audio.enabled") &&
		mac_audio_output_open_device(&native_audio_output, native_audio_fill, NULL, &status,
			audio_device_name) &&
		mac_audio_output_start(&native_audio_output, &status))
	{
		pthread_mutex_lock(&silent_thread_lock);
		output_started = TRUE;
		pthread_mutex_unlock(&silent_thread_lock);
		return;
	}
	if (native_audio_output.state &&
		!mac_audio_output_close(&native_audio_output, &status))
		platform_log("audio output could not close after error %ld", status);
	platform_log("audio output unavailable (%ld); draining sound silently", status);
	silent_clock_start();
}

static void audio_stop(void)
{
	long status = 0;
	struct mac_audio_output_stats output_stats;

	openal_stop();
	silent_clock_stop();
	if (native_audio_output.state && mac_audio_output_stop(&native_audio_output, &status) &&
		mac_audio_output_statistics(&native_audio_output, &output_stats))
	{
		pthread_mutex_lock(&silent_thread_lock);
		closed_output_stats = output_stats;
		output_started = FALSE;
		pthread_mutex_unlock(&silent_thread_lock);
	}
	if (!mac_audio_output_close(&native_audio_output, &status))
	{
		platform_log("audio output could not close (%ld)", status);
		return;
	}
	audio_started = FALSE;
}

/* port: audio.output_device changed (Settings > Audio, OK): the sound goes on
   on the new device (platform_display_apply, port/mac/engine_display.c, each
   time the settings are saved) */
void mac_dsound_output_device_apply(void)
{
	long status = 0;

	if (!audio_started || !native_audio_output.state || !output_started ||
		!strcmp(audio_device_name, audio_device_setting()))
		return;
	snprintf(audio_device_name, sizeof(audio_device_name), "%s", audio_device_setting());
	if (!mac_audio_output_close(&native_audio_output, &status))
	{
		platform_log("audio output could not close (%ld)", status);
		return;
	}
	if (!mac_audio_output_open_device(&native_audio_output, native_audio_fill, NULL, &status, audio_device_name) ||
		!mac_audio_output_start(&native_audio_output, &status))
	{
		if (native_audio_output.state)
			mac_audio_output_close(&native_audio_output, &status);
		platform_log("audio output unavailable (%ld); draining sound silently", status);
		pthread_mutex_lock(&silent_thread_lock);
		output_started = FALSE;
		pthread_mutex_unlock(&silent_thread_lock);
		silent_clock_start();
	}
}

int mac_dsound_audio_statistics(struct mac_dsound_audio_stats *stats)
{
	struct mac_audio_output_stats output_stats;
	if (!stats)
		return 0;
	memset(stats, 0, sizeof(*stats));
	if (native_audio_output.state &&
		mac_audio_output_statistics(&native_audio_output, &output_stats))
		stats->output = output_stats;
	pthread_mutex_lock(&silent_thread_lock);
	if (!native_audio_output.state)
		stats->output = closed_output_stats;
	stats->silent_callbacks = silent_callbacks;
	stats->silent_frames = silent_frames;
	stats->silent_nonzero_frames = silent_nonzero_frames;
	stats->sound_started = audio_started;
	stats->silent_clock_started = silent_thread_started;
	stats->output_started = output_started;
	stats->mix_calls = mix_calls;
	stats->mix_frames = mix_frames;
	stats->mix_nanoseconds = mix_nanoseconds;
	stats->mix_max_nanoseconds = mix_max_nanoseconds;
	stats->voice_mixes = voice_mixes;
	stats->voice_zero_gain_mixes = voice_zero_gain_mixes;
	stats->openal_active = openal.active;
	stats->openal_reverb = openal.reverb_on;
	stats->openal_sends_set = openal.sends_set;
	stats->openal_obstructions_set = openal.obstructions_set;
	stats->openal_room_type = openal.room_type;
	stats->openal_sources = openal.sources;
	stats->openal_fallbacks = openal.fallbacks;
	stats->openal_buffers_queued = openal.buffers_queued;
	stats->openal_buffer_failures = openal.buffer_failures;
	pthread_mutex_unlock(&silent_thread_lock);
	return 1;
}
'''


def replace_once(text, old, new, label):
    count = text.count(old)
    if count != 1:
        raise ValueError('%s anchor count is %d, expected 1' % (label, count))
    return text.replace(old, new, 1)


def generate():
    text = open(SOURCE, 'rb').read().decode('utf-8')
    old_includes = '''#include "platform.h"
#include "sdl_platform.h"
#include "port_config.h"
#include "voice_audio.h"

#include <SDL3/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>'''
    text = replace_once(text, old_includes, INCLUDES, 'SDL include block')
    pcm_start = text.index('static short *decode_pcm(')
    pcm_end = text.index('/* ---------- 3D */', pcm_start)
    text = text[:pcm_start] + PCM + '\n\n' + text[pcm_end:]
    out_start = text.index('/* ---------- output */')
    out_end = text.index('/* ---------- completion */', out_start)
    text = text[:out_start] + OUTPUT + '\n\n' + text[out_end:]
    text = replace_once(text,
        '''	float direct_lowpass[2];
	float room_lowpass;
};''',
        '''	float direct_lowpass[2];
	float room_lowpass;
	/* its OpenAL source and each packet's buffer (mac_dsound_openal.inc) */
	unsigned int openal_source;
	unsigned int openal_buffers[MAXIMUM_STREAM_PACKETS];
	BOOL openal_failed;
	/* the reverb send and obstruction last given to the source */
	float openal_send, openal_obstruction;
};''', 'OpenAL stream fields')
    text = replace_once(text,
        '''	if (stream->paused || !stream->packet_count || !stream->sample_rate)
		return;
	step =''',
        '''	if (stream->paused || !stream->packet_count || !stream->sample_rate)
		return;
	/* (OpenAL plays it) */
	if (stream->openal_source)
		return;
	step =''', 'OpenAL mix skip')
    text = replace_once(text,
        '''	stream->packet_count++;
	pthread_mutex_unlock(&mixer_lock);
	return S_OK;''',
        '''	openal_stream_queue(stream, entry);
	stream->packet_count++;
	pthread_mutex_unlock(&mixer_lock);
	return S_OK;''', 'OpenAL queue')
    text = replace_once(text,
        '''	pthread_mutex_lock(&mixer_lock);
	while (stream->packet_count)
	{''',
        '''	pthread_mutex_lock(&mixer_lock);
	openal_stream_stop(stream);
	while (stream->packet_count)
	{''', 'OpenAL flush')
    text = replace_once(text,
        '''	pthread_mutex_unlock(&mixer_lock);
	free(stream);
	return 0;''',
        '''	pthread_mutex_unlock(&mixer_lock);
	openal_stream_delete(stream);
	free(stream);
	return 0;''', 'OpenAL release')
    text = replace_once(text,
        '''		resampler_mode = resampler_mode_read();
	}
	streams_complete_finished();''',
        '''		resampler_mode = resampler_mode_read();
	}
	openal_update();
	streams_complete_finished();''', 'OpenAL update')
    text = replace_once(text,
        '''	voice_gains(stream, &target_left, &target_right, &target_room, &target_direct_lowpass, &target_room_lowpass);
''',
        '''	voice_gains(stream, &target_left, &target_right, &target_room, &target_direct_lowpass, &target_room_lowpass);
	++voice_mixes;
	if (target_left == 0.0f && target_right == 0.0f && target_room == 0.0f)
		++voice_zero_gain_mixes;
''', 'voice gain diagnostic')
    text = replace_once(text,
        '''	(void)sound;
	return direct_sound.reference_count ? --direct_sound.reference_count : 0;''',
        '''	ULONG count;
	(void)sound;
	count = direct_sound.reference_count ? --direct_sound.reference_count : 0;
	if (!count)
		audio_stop();
	return count;''', 'DirectSound release')
    return text


if __name__ == '__main__':
    if len(sys.argv) != 2:
        sys.stderr.write('usage: mac_dsound_view.py output.c\n')
        sys.exit(2)
    output = generate()
    target = open(sys.argv[1], 'wb')
    target.write(output.encode('utf-8'))
    target.close()
    print('generated native view from %s' % SOURCE)
