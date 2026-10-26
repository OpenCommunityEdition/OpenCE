/*
TALKING_TIMER.C (test)
*/

#include "harness.h"
#include "lines.inc"

#include "under_test.inc"

static int is(long second, long line)
{
	return talking_timer_line_at(second) == line;
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";
	long minute, second;

	CASE("seconds")
	{
		CHECK(is(20, NONE), "0:20 is silent");
		CHECK(is(30, TALKING_TIMER_30_SECONDS), "0:30");
		CHECK(is(40, TALKING_TIMER_20_SECONDS), "0:40");
		CHECK(is(4 * 60 + 30, TALKING_TIMER_30_SECONDS), "4:30 as 0:30");
	}
	CASE("countdown")
	{
		for (second = 50; second < 60; second++)
			CHECK(is(second, TALKING_TIMER_COUNTDOWN_10 + second - 50), "0:%ld", second);
	}
	CASE("minutes")
	{
		CHECK(is(60, TALKING_TIMER_MINUTE_1), "1:00");
		CHECK(is(120, TALKING_TIMER_MINUTE_1 + 1), "2:00");
		CHECK(is(30 * 60, TALKING_TIMER_MINUTE_1 + 29), "30:00");
		CHECK(is(31 * 60, NONE), "31:00 is silent");
		CHECK(is(61 * 60, NONE), "61:00 is silent");
		CHECK(is(200 * 60, NONE), "200:00 is silent");
		CHECK(is(31 * 60 + 30, TALKING_TIMER_30_SECONDS), "31:30 still calls 30 seconds");
		CHECK(is(31 * 60 + 50, TALKING_TIMER_COUNTDOWN_10), "31:50 still counts down");
	}
	CASE("silent")
	{
		CHECK(is(0, NONE), "0:00 has no callout");
		CHECK(is(-1, NONE), "no clock");
		for (minute = 0; minute < 3; minute++)
			for (second = 1; second < 60; second++)
				if (second != 30 && second != 40 && second < 50)
					CHECK(is(minute * 60 + second, NONE), "%ld:%02ld is silent", minute, second);
	}
	return 0;
}
