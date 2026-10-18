#!/usr/bin/env python
"""Generate a checked Mac view of the real Opus voice mixer.

Only microphone capture is unavailable on this native boundary. Opus encode,
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
void platform_log(const char *format, ...);'''

NATIVE_INPUT = '''/* Tiger's native output is available, but this port has no microphone
   input device provider yet. Sending voice stays disabled until one exists. */
int voice_audio_microphone(int open)
{
	(void)open;
	return FALSE;
}

int voice_audio_read_frame(float *frame)
{
	(void)frame;
	return FALSE;
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
	text = text[:declarations_start] + '''static OpusEncoder *encoder;
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
	if 'SDL_' in text or 'sdl_platform.h' in text or 'port_config.h' in text:
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
