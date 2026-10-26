/*
TALKING_TIMER_UPDATE.C (test)
*/

#include "harness.h"
#include "lines.inc"

#define TICKS_PER_SECOND 30

static long now_ticks, lines_played, last_line = NONE;
static boolean running = TRUE, in_play = TRUE, setting_on = TRUE;
static unsigned long changes = 1;
static float last_volume, volume_setting = 1.0f, timer_setting = 1.0f, effects_setting = 1.0f;

#define game_time_get() (now_ticks)
#define game_engine_running() (running)
#define game_engine_in_play() (in_play)
#define config_changes() (changes)
#define config_boolean(name) (setting_on)
#define config_real(name) (!strcmp(name, "audio.volume") ? volume_setting \
	: !strcmp(name, "audio.timer_volume") ? timer_setting : effects_setting)
#define talking_timer_audio_play(line, volume) (last_line = (line), last_volume = (volume), lines_played++)
#define PIN(value, low, high) ((value) < (low) ? (low) : (value) > (high) ? (high) : (value))

#include "under_test.inc"

static void frames_until(long ticks)
{
	for (; now_ticks <= ticks; now_ticks += 2)
		talking_timer_update();
	now_ticks = ticks;
}

static int near(float value, float expected)
{
	return value > expected - 0.001f && value < expected + 0.001f;
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";

	/* (read here so a negative control that drops their other read still builds with -Werror) */
	(void)running;
	(void)in_play;

	CASE("game")
	{
		now_ticks = 0;
		frames_until(61 * TICKS_PER_SECOND);
		CHECK(lines_played == 13, "0:30, 0:40, ten counts, 1:00: %ld", lines_played);
		CHECK(last_line == TALKING_TIMER_MINUTE_1, "1:00 said last");
	}
	CASE("turned_on")
	{
		frames_until(10 * TICKS_PER_SECOND);
		setting_on = FALSE;
		changes++;
		frames_until(30 * TICKS_PER_SECOND + 15);
		CHECK(lines_played == 0, "off: silent");
		setting_on = TRUE;
		changes++;
		frames_until(31 * TICKS_PER_SECOND);
		CHECK(lines_played == 0, "nothing for the second it is turned on in");
		frames_until(40 * TICKS_PER_SECOND);
		CHECK(lines_played == 1 && last_line == TALKING_TIMER_20_SECONDS, "0:40 plays");
	}
	CASE("postgame")
	{
		frames_until(45 * TICKS_PER_SECOND);
		in_play = FALSE;
		lines_played = 0;
		frames_until(55 * TICKS_PER_SECOND);
		CHECK(lines_played == 0, "the postgame is silent");
		in_play = TRUE;
		now_ticks = 0;
		frames_until(30 * TICKS_PER_SECOND);
		CHECK(lines_played == 1 && last_line == TALKING_TIMER_30_SECONDS, "the new game calls from its 0:00");
	}
	CASE("not_multiplayer")
	{
		running = FALSE;
		frames_until(61 * TICKS_PER_SECOND);
		CHECK(lines_played == 0, "silent");
	}
	CASE("skip")
	{
		frames_until(30 * TICKS_PER_SECOND - 3);
		lines_played = 0;
		now_ticks = 31 * TICKS_PER_SECOND + 6;
		talking_timer_update();
		CHECK(lines_played == 0, "0:30 not caught up");
	}
	CASE("once")
	{
		frames_until(30 * TICKS_PER_SECOND - 1);
		lines_played = 0;
		for (now_ticks = 30 * TICKS_PER_SECOND; now_ticks < 31 * TICKS_PER_SECOND; now_ticks++)
		{
			talking_timer_update();
			talking_timer_update();
		}
		CHECK(lines_played == 1, "once: %ld lines", lines_played);
	}
	CASE("on_the_second")
	{
		static const long seconds[] = { 30, 40, 60 };
		static const long lines[] = { TALKING_TIMER_30_SECONDS, TALKING_TIMER_20_SECONDS, TALKING_TIMER_MINUTE_1 };
		int index;

		for (index = 0; index < 3; index++)
		{
			frames_until(seconds[index] * TICKS_PER_SECOND - 1);
			lines_played = 0;
			now_ticks = seconds[index] * TICKS_PER_SECOND;
			talking_timer_update();
			CHECK(lines_played == 1 && last_line == lines[index], "0:%ld said on its first tick", seconds[index]);
		}
	}
	CASE("countdown")
	{
		long second;

		frames_until(50 * TICKS_PER_SECOND - 1);
		lines_played = 0;
		for (second = 50; second < 60; second++)
		{
			now_ticks = second * TICKS_PER_SECOND;
			talking_timer_update();
			CHECK(lines_played == second - 49 && last_line == TALKING_TIMER_COUNTDOWN_10 + second - 50,
				"0:%ld said on its second", second);
			frames_until((second + 1) * TICKS_PER_SECOND - 1);
		}
		CHECK(lines_played == 10, "%ld lines", lines_played);
	}
	CASE("volume")
	{
		volume_setting = 0.5f;
		timer_setting = 0.5f;
		effects_setting = 0.1f;
		changes++;
		frames_until(30 * TICKS_PER_SECOND);
		CHECK(near(last_volume, 0.5f * 0.5f * 1.25f), "0.5 × 0.5 × 1.25: %f", last_volume);
		volume_setting = 1.0f;
		timer_setting = 1.0f;
		changes++;
		frames_until(40 * TICKS_PER_SECOND);
		CHECK(near(last_volume, 1.25f), "timer volume 1.0 is 25%% louder: %f", last_volume);
		timer_setting = 1.5f;
		changes++;
		frames_until(50 * TICKS_PER_SECOND);
		CHECK(near(last_volume, 1.25f), "timer volume pinned to 1.0: %f", last_volume);
	}
	CASE("default")
	{
		timer_setting = TIMER_VOLUME_DEFAULT;
		changes++;
		frames_until(30 * TICKS_PER_SECOND);
		CHECK(near(last_volume, 1.0f), "the default plays the clips at their level: %f", last_volume);
	}
	CASE("silent_at_zero")
	{
		timer_setting = 0.0f;
		changes++;
		frames_until(31 * TICKS_PER_SECOND);
		CHECK(lines_played == 0, "no line at timer volume 0: %ld lines", lines_played);
		timer_setting = 0.4f;
		volume_setting = 0.0f;
		changes++;
		frames_until(41 * TICKS_PER_SECOND);
		CHECK(lines_played == 0, "no line at master volume 0: %ld lines", lines_played);
	}
	return 0;
}
