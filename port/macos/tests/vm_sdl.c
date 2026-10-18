/*
VM_SDL.C

A guest program for tools/test_macos_vm.py: SDL through the guest's bridge
(port/android/guest/runtime/guest_sdl.c) and the macOS host's
(port/android/host/host_sdl.c), headless (SDL's dummy drivers). It runs
the case named by argv[1], prints "ok" and returns 0 if it passes.
*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* (after the C library's: SDL defines alloca, which stdlib.h declares) */
#include <SDL3/SDL.h>

#define CHECK(condition) do { if (!(condition)) { printf("failed: %s (line %d)\n", #condition, __LINE__); exit(1); } } while (0)

/* ---------- audio: the stream's callback runs as guest code, on a host
thread of its own (host_sdl.c's audio thread, with a vCPU of its own) */

struct audio_state
{
	volatile int calls;
	volatile int other_thread;
	pthread_t main_thread;
	SDL_AudioStream *stream;
};

static void SDLCALL feed(void *userdata, SDL_AudioStream *stream, int additional, int total)
{
	struct audio_state *state = userdata;
	static float silence[4096];
	int length = additional < (int)sizeof(silence) ? additional : (int)sizeof(silence);

	(void)total;
	if (!pthread_equal(pthread_self(), state->main_thread))
		state->other_thread = 1;
	if (length > 0)
		SDL_PutAudioStreamData(stream, silence, length);
	__atomic_add_fetch(&state->calls, 1, __ATOMIC_SEQ_CST);
}

static void case_audio(void)
{
	SDL_AudioSpec spec;
	struct audio_state state;
	int waited;

	memset(&state, 0, sizeof(state));
	state.main_thread = pthread_self();
	spec.format = SDL_AUDIO_F32;
	spec.channels = 2;
	spec.freq = 48000;
	CHECK(SDL_Init(SDL_INIT_AUDIO));
	state.stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, feed, &state);
	CHECK(state.stream != NULL);
	CHECK(SDL_ResumeAudioStreamDevice(state.stream));
	for (waited = 0; waited < 300 && state.calls < 3; waited++)
		SDL_Delay(10);
	CHECK(state.calls >= 3);
	CHECK(state.other_thread);
	SDL_DestroyAudioStream(state.stream);
}

/* ---------- events, time and gamepads, with no window system */

static void case_events(void)
{
	SDL_Event event;
	Uint64 before;
	int count = -1, polled = 0;
	SDL_JoystickID *gamepads;

	CHECK(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS));
	before = SDL_GetTicks();
	SDL_Delay(30);
	CHECK(SDL_GetTicks() >= before + 25);
	while (SDL_PollEvent(&event) && polled < 1000)
		polled++;
	CHECK(polled < 1000);
	gamepads = SDL_GetGamepads(&count);
	CHECK(count == 0);
	SDL_free(gamepads);
}

/* ---------- an OpenGL ES import while headless: refused, and said once */

void hostgl_glClear(unsigned int mask);

static void case_gl(void)
{
	hostgl_glClear(0x4000);
	hostgl_glClear(0x4000);
}

int main(int argc, char **argv)
{
	const char *name = argc > 1 ? argv[1] : "";

	if (!strcmp(name, "audio"))
		case_audio();
	else if (!strcmp(name, "audio_then_wait"))
	{
		/* (the audio thread ends well before the program does) */
		case_audio();
		SDL_Delay(300);
	}
	else if (!strcmp(name, "events"))
		case_events();
	else if (!strcmp(name, "gl"))
		case_gl();
	else
	{
		printf("no case %s\n", name);
		return 2;
	}
	printf("ok\n");
	return 0;
}
