/*
GMTIME.C

symbols in this file:
	_gmtime (.text, 0x107)
	_bss_004e4150 (.bss, 0x24)
*/

/* ---------- constants */

#define DAY_SECONDS (24L * 60L * 60L)
#define YEAR_SECONDS (365L * DAY_SECONDS)
#define FOUR_YEAR_SECONDS (1461L * DAY_SECONDS)
#define BASE_YEAR 70
#define BASE_WEEKDAY 4

/* ---------- types */

typedef long time_t;

struct tm
{
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
};

struct thread_data
{
	unsigned char reserved[0x40];
	unsigned short *wasctime_buffer;
	struct tm *gmtime_buffer;
};

/* ---------- external data */

extern int _lpdays[13];
extern int _days[13];

/* ---------- external code */

struct thread_data * __cdecl _getptd(
	void);

void * __cdecl malloc(
	unsigned long size);

/* ---------- public data */

struct tm bss_004e4150 = { 0 };

/* ---------- public code */

struct tm * __cdecl gmtime(
	time_t const *timer)
{
	time_t calendar_time = *timer;
	int is_leap_year = 0;
	int temporary_time;
	int *month_days;
	struct tm *result;
	struct thread_data *thread_data = _getptd();

	if (calendar_time < 0L)
	{
		return 0;
	}

	if (!thread_data->gmtime_buffer &&
		!(thread_data->gmtime_buffer = (struct tm *)malloc(sizeof(struct tm))))
	{
		result = &bss_004e4150;
	}
	else
	{
		result = thread_data->gmtime_buffer;
	}

	temporary_time = (int)(calendar_time / FOUR_YEAR_SECONDS);
	calendar_time -= (time_t)temporary_time * FOUR_YEAR_SECONDS;
	temporary_time = temporary_time * 4 + BASE_YEAR;

	if (calendar_time >= YEAR_SECONDS)
	{
		temporary_time++;
		calendar_time -= YEAR_SECONDS;

		if (calendar_time >= YEAR_SECONDS)
		{
			temporary_time++;
			calendar_time -= YEAR_SECONDS;

			if (calendar_time >= YEAR_SECONDS + DAY_SECONDS)
			{
				temporary_time++;
				calendar_time -= YEAR_SECONDS + DAY_SECONDS;
			}
			else
			{
				is_leap_year++;
			}
		}
	}

	result->tm_year = temporary_time;
	result->tm_yday = (int)(calendar_time / DAY_SECONDS);
	calendar_time -= (time_t)result->tm_yday * DAY_SECONDS;

	if (is_leap_year)
	{
		month_days = _lpdays;
	}
	else
	{
		month_days = _days;
	}

	for (temporary_time = 1;
		month_days[temporary_time] < result->tm_yday;
		temporary_time++)
	{
	}

	result->tm_mon = --temporary_time;
	result->tm_mday = result->tm_yday - month_days[temporary_time];
	result->tm_wday = (int)(*timer / DAY_SECONDS + BASE_WEEKDAY) % 7;

	result->tm_hour = (int)(calendar_time / (60L * 60L));
	calendar_time -= (time_t)result->tm_hour * (60L * 60L);
	result->tm_min = (int)(calendar_time / 60L);
	result->tm_sec = (int)(calendar_time - (time_t)result->tm_min * 60L);
	result->tm_isdst = 0;

	return result;
}
