/*
TIME.C

symbols in this file:
	_time (.text, 0x38)
*/

/* ---------- types */

typedef long time_t;

struct file_time
{
	unsigned long low;
	unsigned long high;
};

union file_time_value
{
	struct file_time parts;
	unsigned __int64 ticks;
};

/* ---------- external code */

void __stdcall GetSystemTimeAsFileTime(
	struct file_time *file_time);

/* ---------- public code */

time_t __cdecl time(
	time_t *destination)
{
	union file_time_value file_time;
	time_t result;

	GetSystemTimeAsFileTime(&file_time.parts);
	result = (time_t)((file_time.ticks - 116444736000000000ui64) / 10000000ui64);

	if (destination)
		*destination = result;

	return result;
}
