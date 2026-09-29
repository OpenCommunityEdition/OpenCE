/* Thread-safe quick-play control; the engine consumes requests on its own thread. */
#include <emscripten.h>
#include <stdio.h>
#include <string.h>

void web_js_post(int kind, const char *text);
static int cancel_requested;
static int background_active;
/* Give deferred network/menu teardown a few frames after a terminal event. */
static double background_drain_until;

EMSCRIPTEN_KEEPALIVE void web_quick_play_cancel(void)
{
	__atomic_store_n(&cancel_requested, 1, __ATOMIC_RELEASE);
}

int web_quick_play_take_cancel(void)
{
	return __atomic_exchange_n(&cancel_requested, 0, __ATOMIC_ACQ_REL);
}

int web_quick_play_background_active(void)
{
	/* A cancel must also wake a game that was waiting for a hidden page. */
	return __atomic_load_n(&background_active, __ATOMIC_ACQUIRE) ||
		__atomic_load_n(&cancel_requested, __ATOMIC_ACQUIRE) ||
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
