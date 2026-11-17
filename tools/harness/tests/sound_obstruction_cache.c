/* A sound's wall test is made once a tick (sound_manager.c's source_audible
and its obstruction cache): the tick's other frames reuse it even though a
sound on a moving object is at a new interpolated position each frame; the
next tick, another listener or another sound in the same slot tests again;
and a sound past its range is never tested. */
#include "harness.h"
#include "types.inc"

#define REAL_MAX 3.4028235e38f
static struct sound_listener listeners[MAXIMUM_NUMBER_OF_LOCAL_PLAYERS];
static long now;
static int tests;
static long game_time_get(void) { return now; }
static struct sound_listener *listener_get(short index) { return &listeners[index]; }
static real square_root(real x) { return x; }
/* the distance is the source's x, so a source that moves stays in range */
static real source_distance_squared(short listener_index, struct sound_source const *source)
{
	(void)listener_index;
	return source->location.position.x * source->location.position.x;
}
static void compute_sound_obstruction(short listener_index, struct sound_source *source, real distance)
{
	(void)listener_index; (void)distance;
	tests++;
	source->obstruction = 0.25f * (real)tests;
	source->occlusion = 0.f;
}
#include "under_test.inc"

static struct sound_source at(real x)
{
	struct sound_source source;

	memset(&source, 0, sizeof(source));
	source.spatialization_mode = _sound_spatialization_mode_absolute;
	source.location.position.x = x;
	return source;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";
	struct sound_source source;
	long sound = (0x8001L << 16) | 5, other = (0x8002L << 16) | 5;
	int frame;

	listeners[0].valid = TRUE;
	now = 10;
	CASE("moving")
	{
		/* four frames of a tick, the source's interpolated position moving */
		for (frame = 0; frame < 4; frame++)
		{
			source = at(2.f + 0.1f * (real)frame);
			CHECK(source_audible(&source, 10.f, _sound_obstruction_cache_sound, sound) == 0, "the sound is unheard");
			CHECK(source.obstruction == 0.25f, "frame %d has obstruction %f", frame, source.obstruction);
		}
		CHECK(tests == 1, "a tick of a moving sound made %d wall tests", tests);
	}
	CASE("next-tick")
	{
		source = at(2.f);
		source_audible(&source, 10.f, _sound_obstruction_cache_looping_sound, sound);
		now++;
		source_audible(&source, 10.f, _sound_obstruction_cache_looping_sound, sound);
		CHECK(tests == 2, "two ticks made %d wall tests", tests);
	}
	CASE("listener")
	{
		source = at(2.f);
		source_audible(&source, 10.f, _sound_obstruction_cache_sound, sound);
		listeners[0].valid = FALSE;
		listeners[1].valid = TRUE;
		source_audible(&source, 10.f, _sound_obstruction_cache_sound, sound);
		CHECK(tests == 2, "another listener made %d wall tests", tests);
	}
	CASE("slot")
	{
		/* another sound in the same slot is not given the first one's result */
		source = at(2.f);
		source_audible(&source, 10.f, _sound_obstruction_cache_sound, sound);
		source_audible(&source, 10.f, _sound_obstruction_cache_sound, other);
		CHECK(tests == 2, "another sound in the slot made %d wall tests", tests);
	}
	CASE("impulse")
	{
		/* a new impulse has no datum yet: every call tests */
		source = at(2.f);
		source_audible(&source, 10.f, _sound_obstruction_cache_none, NONE);
		source_audible(&source, 10.f, _sound_obstruction_cache_none, NONE);
		CHECK(tests == 2, "two impulses made %d wall tests", tests);
	}
	CASE("out-of-range")
	{
		source = at(20.f);
		CHECK(source_audible(&source, 10.f, _sound_obstruction_cache_sound, sound) == NONE, "the sound is heard");
		CHECK(tests == 0, "a sound past its range made %d wall tests", tests);
	}
	return 0;
}
