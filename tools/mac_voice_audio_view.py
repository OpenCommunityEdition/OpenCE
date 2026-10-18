#!/usr/bin/env python
"""Generate a checked Mac view of the real Opus voice mixer.

The microphone is port/mac/mac_voice_capture.c's AUHAL input; Opus encode,
decode, packet sequencing, concealment, speaker buffering and mixing remain the
shared implementation from port/linux/src/voice_audio.c.
"""
import sys

SOURCE = 'port/linux/src/voice_audio.c'

OLD_INCLUDES = '''#include "platform.h"
#include "sdl_platform.h"
#include "port_config.h"
#include "voice_audio.h"
#include "../../third_party/opus/include/opus.h"

#include <SDL3/SDL.h>
#include <math.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>'''

NATIVE_INCLUDES = '''#include "engine_xdk_prefix.h"
#include "voice_audio.h"
#include "opus.h"

#include <math.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned long long Uint64;
void platform_log(const char *format, ...);
const char *config_string(const char *name);
#include "mac_voice_capture.h"
#define PLATFORM_AUDIO_DEVICE_NAME_SIZE 128'''

NATIVE_INPUT = '''/* The microphone: an AUHAL input unit (port/mac/mac_voice_capture.c), open only
   while voice chat may send, and opened on the first request (the system asks
   for the permission then, never before). It gives mono float at 48 kHz, the
   frame the SDL builds' microphone does. */
int voice_audio_microphone(int open)
{
	if (!open)
	{
		if (microphone_open)
		{
			mac_voice_capture_close(&microphone);
			microphone_open = FALSE;
			capture_count = 0;
			microphone_failed = FALSE;
			platform_log("voice chat: microphone closed");
		}
		return FALSE;
	}
	{
		/* (audio.input_device: Settings > Audio's) */
		const char *device = config_string("audio.input_device");

		device = device && device[0] ? device : "default";
		/* (another device chosen: the one open closes, and the new opens) */
		if (microphone_open && strcmp(microphone_device, device))
		{
			mac_voice_capture_close(&microphone);
			microphone_open = FALSE;
			capture_count = 0;
			microphone_failed = FALSE;
		}
		if (!microphone_open && (!microphone_failed || GetTickCount() - microphone_failed_ms >= VOICE_RETRY_MS))
		{
			long status = 0;

			snprintf(microphone_device, sizeof(microphone_device), "%s", device);
			microphone_open = mac_voice_capture_open(&microphone, device, &status) != 0;
			microphone_failed = !microphone_open;
			microphone_failed_ms = GetTickCount();
			if (microphone_open)
				platform_log("voice chat: microphone open");
			else
				platform_log("voice chat: no microphone (%ld)", status);
		}
	}
	return microphone_open;
}

int voice_audio_read_frame(float *frame)
{
	if (!microphone_open)
		return FALSE;
	/* (a backlog dropped but for the latest frames) */
	if (mac_voice_capture_available(&microphone) > VOICE_MAXIMUM_BACKLOG)
		mac_voice_capture_trim(&microphone, VOICE_MAXIMUM_BACKLOG);
	capture_count += (int)mac_voice_capture_read(&microphone, capture + capture_count,
		(unsigned)(VOICE_FRAME_SAMPLES - capture_count));
	if (capture_count < VOICE_FRAME_SAMPLES)
		return FALSE;
	memcpy(frame, capture, sizeof(capture));
	capture_count = 0;
	return TRUE;
}

'''

VOICE_TEST_LOG = '''	/* (the tests: what each speaker sounds like, once a second) */
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
'''


def replace_once(text, old, new, label):
	count = text.count(old)
	if count != 1:
		raise ValueError('%s anchor count is %d, expected 1' % (label, count))
	return text.replace(old, new, 1)


def generate():
	text = open(SOURCE, 'r').read()
	text = replace_once(text, OLD_INCLUDES, NATIVE_INCLUDES, 'include block')
	declarations_start = text.index('/* the microphone (under voice_lock: the opening thread sets it) */')
	declarations_end = text.index('/* ---------- the microphone */', declarations_start)
	text = text[:declarations_start] + '''/* the microphone (voice chat's thread only), the device it was opened on, and
a failed open's time (it is tried again after VOICE_RETRY_MS) */
static struct mac_voice_capture microphone;
static BOOL microphone_open;
static BOOL microphone_failed;
static unsigned long microphone_failed_ms;
static char microphone_device[PLATFORM_AUDIO_DEVICE_NAME_SIZE];
static float capture[VOICE_FRAME_SAMPLES];
static int capture_count;
static OpusEncoder *encoder;
static int encoder_bitrate;

''' + text[declarations_end:]
	input_start = text.index('/* ---------- the microphone */')
	input_end = text.index('float voice_audio_level(', input_start)
	text = text[:input_start] + NATIVE_INPUT + text[input_end:]
	text = replace_once(text, VOICE_TEST_LOG, '', 'debug voice log')
	count = text.count('SDL_GetTicks()')
	if count != 2:
		raise ValueError('native clock anchors found %d times, expected 2' % count)
	text = text.replace('SDL_GetTicks()', 'GetTickCount()')
	if 'SDL_' in text or 'sdl_platform.h' in text :
		raise ValueError('native voice source still depends on SDL or port config')
	return text


if __name__ == '__main__':
	if len(sys.argv) != 2:
		sys.stderr.write('usage: mac_voice_audio_view.py output.c\n')
		sys.exit(2)
	output = open(sys.argv[1], 'w')
	output.write(generate())
	output.close()
	print('generated native view from %s' % SOURCE)
