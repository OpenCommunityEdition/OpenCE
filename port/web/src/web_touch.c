/*
WEB_TOUCH.C

The on-screen touch controls between the page's overlay
(port/web/site/touch.js) and the game, as port/android/host/host_touch.c
has them on Android: the services touch_input.c asks of the host. The page
writes its stick, buttons and swipes into the shared words (web_shared.h);
the game reads them when it reads port 0's controller, and writes back
whether a game is being played (the scene), the profile's button mapping and
port 0's rumble.
*/

#include <stdint.h>
#include <string.h>
#include <time.h>

#include "web_shared.h"

/* a press shorter than this still reaches the game, which reads the
controller once a frame (host_touch.c's TOUCH_TAP_NS) */
#define TOUCH_TAP_NS 60000000LL

static int32_t presses_seen[WEB_TOUCH_INPUTS];
static int64_t pressed_ns[WEB_TOUCH_INPUTS];

static int64_t now_ns(void)
{
	struct timespec now;

	clock_gettime(CLOCK_MONOTONIC, &now);
	return (int64_t)now.tv_sec * 1000000000LL + now.tv_nsec;
}

/* the screen's edges the system keeps for its gestures: none a page knows */
void host_gesture_insets(int *insets)
{
	memset(insets, 0, 4 * sizeof(*insets));
}

/* the overlay's controller state, with each press held for at least
TOUCH_TAP_NS (a tap can go down and up between two reads) */
void host_touch_read(int32_t *state)
{
	int64_t now = now_ns();
	int input;

	for (input = 0; input < 7; input++)
		state[input] = __atomic_load_n(&web_shared[_web_touch_state + input], __ATOMIC_RELAXED);
	for (input = 0; input < WEB_TOUCH_INPUTS; input++)
	{
		int32_t presses = __atomic_load_n(&web_shared[_web_touch_presses + input], __ATOMIC_RELAXED);

		if (presses != presses_seen[input])
		{
			presses_seen[input] = presses;
			pressed_ns[input] = now;
		}
		if (pressed_ns[input] && now - pressed_ns[input] < TOUCH_TAP_NS)
		{
			if (input < 15)
				state[6] |= 1 << input;
			else if (!state[4 + input - 15])
				state[4 + input - 15] = 32767;
		}
	}
}

/* the swipe, then the gyroscope's turn, since the last read, in the
overlay's logical pixels with its sensitivity applied */
void host_touch_look_read(float *delta)
{
	int index;

	for (index = 0; index < 4; index++)
		delta[index] = (float)__atomic_exchange_n(&web_shared[_web_touch_look + index], 0, __ATOMIC_RELAXED) / 256.0f;
}

/* port 0's motors, each 0..65535, as the phone's vibration (the page's
navigator.vibrate) */
void host_touch_rumble(unsigned int low, unsigned int high)
{
	unsigned int strength = low > high ? low : high;

	__atomic_store_n(&web_shared[_web_touch_rumble], strength ? 64 + (int)(strength * 191u / 65535u) : 0,
		__ATOMIC_RELAXED);
}

void host_touch_scene(int scene)
{
	__atomic_store_n(&web_shared[_web_touch_scene], scene, __ATOMIC_RELAXED);
}

/* the game control on each controller button (port/linux/game/touch_game.c),
which name the overlay's buttons */
void host_touch_bindings(const int32_t *controls)
{
	int index;

	for (index = 0; index < 16; index++)
		__atomic_store_n(&web_shared[_web_touch_bindings + index], controls[index], __ATOMIC_RELAXED);
	__atomic_add_fetch(&web_shared[_web_touch_bindings_serial], 1, __ATOMIC_RELEASE);
}
