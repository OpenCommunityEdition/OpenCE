/*
LOCALTIM.C

symbols in this file:
	_localtime (.text, 0x180)
*/

/* ---------- constants */

#define SECOND_SECONDS 1L
#define MINUTE_SECONDS (60L * SECOND_SECONDS)
#define HOUR_SECONDS (60L * MINUTE_SECONDS)
#define DAY_SECONDS (24L * HOUR_SECONDS)
#define WEEK_DAYS 7
#define YEAR_DAYS 365
#define DECEMBER 11
#define DECEMBER_DAYS 31
#define MAXIMUM_TIME 0x7FFFFFFFL

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

/* ---------- external data */

extern int _daylight;
extern long _dstbias;
extern long _timezone;

/* ---------- external code */

void __cdecl __tzset(
	void);

struct tm * __cdecl gmtime(
	time_t const *timer);

int __cdecl _isindst(
	struct tm *time);

/* ---------- public code */

struct tm * __cdecl localtime(
	time_t const *timer)
{
	time_t calendar_time;
	struct tm *result;

	if (*timer < 0L)
	{
		return 0;
	}

	__tzset();

	if (*timer > 3L * DAY_SECONDS &&
		*timer < MAXIMUM_TIME - 3L * DAY_SECONDS)
	{
		calendar_time = *timer - _timezone;
		result = gmtime(&calendar_time);

		if (_daylight && _isindst(result))
		{
			calendar_time -= _dstbias;
			result = gmtime(&calendar_time);
			result->tm_isdst = 1;
		}

		return result;
	}

	result = gmtime(timer);

	if (_daylight && _isindst(result))
	{
		calendar_time = result->tm_sec - _dstbias - _timezone;
		result->tm_isdst = 1;
	}
	else
	{
		calendar_time = result->tm_sec - _timezone;
	}

	result->tm_sec = (int)(calendar_time % MINUTE_SECONDS);
	if (result->tm_sec < 0)
	{
		result->tm_sec += (int)MINUTE_SECONDS;
		calendar_time -= MINUTE_SECONDS;
	}

	calendar_time = calendar_time / MINUTE_SECONDS + result->tm_min;
	result->tm_min = (int)(calendar_time % MINUTE_SECONDS);
	if (result->tm_min < 0)
	{
		result->tm_min += (int)MINUTE_SECONDS;
		calendar_time -= MINUTE_SECONDS;
	}

	calendar_time = calendar_time / MINUTE_SECONDS + result->tm_hour;
	result->tm_hour = (int)(calendar_time % 24L);
	if (result->tm_hour < 0)
	{
		result->tm_hour += 24;
		calendar_time -= 24L;
	}

	calendar_time /= 24L;
	if (calendar_time > 0L)
	{
		result->tm_wday = (result->tm_wday + (int)calendar_time) % WEEK_DAYS;
		result->tm_mday += (int)calendar_time;
		result->tm_yday += (int)calendar_time;
	}
	else if (calendar_time < 0L)
	{
		result->tm_wday =
			(result->tm_wday + (int)calendar_time + WEEK_DAYS) % WEEK_DAYS;
		result->tm_mday += (int)calendar_time;

		if (result->tm_mday <= 0)
		{
			result->tm_mday += DECEMBER_DAYS;
			result->tm_year--;
			result->tm_yday = YEAR_DAYS - 1;
			result->tm_mon = DECEMBER;
		}
		else
		{
			result->tm_yday += (int)calendar_time;
		}
	}

	return result;
}
