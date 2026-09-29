/* Thread-safe quick-play control; the engine consumes requests on its own thread. */
#ifdef HALO_QUICK_PLAY_TEST
#define EMSCRIPTEN_KEEPALIVE
double emscripten_get_now(void);
#else
#include <emscripten.h>
#endif
#include <stdio.h>
#include <stdint.h>
#include <string.h>

void web_js_post(int kind, const char *text);
int web_multiplayer_active(void);
static int cancel_requested;
static uint64_t restart_requested;
static int background_active;
/* Give deferred network/menu teardown a few frames after a terminal event. */
static double background_drain_until;

EMSCRIPTEN_KEEPALIVE void web_quick_play_cancel(void)
{
	__atomic_store_n(&restart_requested, 0, __ATOMIC_RELEASE);
	__atomic_store_n(&cancel_requested, 1, __ATOMIC_RELEASE);
}

/* The page elects; only the game thread tears down or starts a session.
   target is the browser socket word, converted to engine address order here. */
EMSCRIPTEN_KEEPALIVE void web_quick_play_restart(int mode, unsigned int target)
{
	if (mode != 1 && mode != 2)
		return;
	__atomic_store_n(&restart_requested, ((uint64_t)mode << 32) | target, __ATOMIC_RELEASE);
}

int web_quick_play_take_restart(int *host, unsigned long *target)
{
	uint64_t request = __atomic_exchange_n(&restart_requested, 0, __ATOMIC_ACQ_REL);
	unsigned int address = (unsigned int)request;
	int mode = (int)(request >> 32);
	if (!request)
		return 0;
	*host = mode == 1;
	*target = ((address & 0xffU) << 24) | ((address & 0xff00U) << 8) |
		((address >> 8) & 0xff00U) | ((address >> 24) & 0xffU);
	return 1;
}

int web_quick_play_take_cancel(void)
{
	return __atomic_exchange_n(&cancel_requested, 0, __ATOMIC_ACQ_REL);
}

int web_quick_play_background_active(void)
{
	/* A cancel must also wake a game that was waiting for a hidden page. */
	return web_multiplayer_active() ||
		__atomic_load_n(&background_active, __ATOMIC_ACQUIRE) ||
		__atomic_load_n(&cancel_requested, __ATOMIC_ACQUIRE) ||
		__atomic_load_n(&restart_requested, __ATOMIC_ACQUIRE) ||
		emscripten_get_now() < background_drain_until;
}

void web_quick_play_report(const char *phase, const char *message)
{
	char json[512];
	int active = strcmp(phase, "menu") && strcmp(phase, "error");
	if (!active)
		background_drain_until = emscripten_get_now() + 1000.0;
	__atomic_store_n(&background_active, active, __ATOMIC_RELEASE);
	/* Both strings are fixed engine literals, with no user text or quotes. */
	snprintf(json, sizeof(json), "{\"phase\":\"%s\",\"message\":\"%s\"}", phase, message);
	web_js_post(6, json);
}
