/*
WCTIME.C

symbols in this file:
	__wctime (.text, 0x19)
*/

/* ---------- types */

typedef long time_t;
typedef unsigned short wchar_t;

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

/* ---------- external code */

struct tm * __cdecl localtime(
	time_t const *timer);

wchar_t * __cdecl _wasctime(
	struct tm const *time);

/* ---------- public code */

wchar_t * __cdecl _wctime(
	time_t const *timer)
{
	struct tm *time = localtime(timer);
	wchar_t *result;

	if (time)
	{
		result = _wasctime(time);
	}
	else
	{
		result = 0;
	}

	return result;
}
