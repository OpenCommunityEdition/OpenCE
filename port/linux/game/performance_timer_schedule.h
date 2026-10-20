/* NHE common timer cues in authoritative 30 Hz match ticks. */
#ifndef PERFORMANCE_TIMER_SCHEDULE_H
#define PERFORMANCE_TIMER_SCHEDULE_H

enum performance_timer_cue_group
{
	_performance_timer_countdown = 1,
	_performance_timer_beeps = 2,
	_performance_timer_minutes = 4,
};

static char const *performance_timer_cue_masked(long previous, long current, unsigned groups)
{
	static char const *const minutes[] = {
		"1_minute", "2_minutes", "3_minutes", "4_minutes", "5_minutes",
		"6_minutes", "7_minutes", "8_minutes", "9_minutes", "10_minutes",
		"11_minutes", "12_minutes", "13_minutes", "14_minutes", "15_minutes",
		"16_minutes", "17_minutes", "18_minutes", "19_minutes", "20_minutes",
		"21_minutes", "22_minutes", "23_minutes", "24_minutes", "25_minutes",
		"26_minutes", "27_minutes", "28_minutes", "29_minutes", "30_minutes"};
	static char const *const countdown[] = {"10", "9", "8", "7", "6", "5", "4", "3", "2", "1"};
	long tick;
	/* Never replay old calls after joining, enabling, time reversal or a long
	 * gap. For a short host-time jump, choose only the most recent due cue. */
	if (previous < 0 || current <= previous || current - previous > 30)
		return 0;
	for (tick = current; tick > previous; --tick)
	{
		long phase = tick % 1800;
		if ((groups & _performance_timer_minutes) && phase == 15 && tick >= 1800)
			return minutes[(tick / 1800 - 1) % 30];
		if ((groups & _performance_timer_countdown) && phase == 915) return "30_seconds_left";
		if ((groups & _performance_timer_countdown) && phase == 1215) return "20_seconds";
		if ((groups & _performance_timer_countdown) && phase >= 1500 && phase <= 1770 && phase % 30 == 0)
			return countdown[(phase - 1500) / 30];
		if ((groups & _performance_timer_beeps) &&
			((phase == 0 && tick > 0) || phase == 600 || phase == 900 || phase == 1200))
			return "timerbeep";
	}
	return 0;
}

static inline char const *performance_timer_cue(long previous, long current)
{
	return performance_timer_cue_masked(previous, current, 7);
}

#endif
