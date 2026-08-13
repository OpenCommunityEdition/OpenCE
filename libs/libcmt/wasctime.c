/*
WASCTIME.C

symbols in this file:
	_code_001ccf70 (.text, 0x1a)
	__wasctime (.text, 0xdc)
	_bss_004e3e7c (.bss, 0x34)
*/

/* ---------- types */

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

struct thread_data
{
	unsigned char reserved[0x40];
	wchar_t *wasctime_buffer;
};

/* ---------- external data */

extern char const __dnames[];
extern char const __mnames[];

/* ---------- external code */

struct thread_data * __cdecl _getptd(
	void);

void * __cdecl malloc(
	unsigned long size);

/* ---------- private data */

wchar_t bss_004e3e7c[26] = { 0 };

/* ---------- private prototypes */

static wchar_t *code_001ccf70(
	wchar_t *destination,
	int value);

/* ---------- public code */

wchar_t * __cdecl _wasctime(
	struct tm const *time)
{
	struct thread_data *thread_data = _getptd();
	wchar_t *buffer;
	wchar_t *output;
	int character_index;
	int day_name_offset;
	int month_name_offset;

	if (!thread_data->wasctime_buffer &&
		!(thread_data->wasctime_buffer = (wchar_t *)malloc(26 * sizeof(wchar_t))))
	{
		output = bss_004e3e7c;
	}
	else
	{
		output = thread_data->wasctime_buffer;
	}

	buffer = output;

	day_name_offset = time->tm_wday * 3;
	month_name_offset = time->tm_mon * 3;

	for (character_index = 0; character_index < 3;)
	{
		*output = (wchar_t)__dnames[day_name_offset + character_index];
		output[4] = (wchar_t)__mnames[month_name_offset + character_index];
		character_index++;
		output++;
	}

	*output = ' ';
	output += 4;
	*output++ = ' ';
	output = code_001ccf70(output, time->tm_mday);
	*output++ = ' ';
	output = code_001ccf70(output, time->tm_hour);
	*output++ = ':';
	output = code_001ccf70(output, time->tm_min);
	*output++ = ':';
	output = code_001ccf70(output, time->tm_sec);
	*output++ = ' ';
	output = code_001ccf70(output, time->tm_year / 100 + 19);
	output = code_001ccf70(output, time->tm_year % 100);
	*output++ = '\n';
	*output = 0;

	return buffer;
}

/* ---------- private code */

static wchar_t *code_001ccf70(
	wchar_t *destination,
	int value)
{
	*destination++ = (wchar_t)(value / 10 + '0');
	*destination++ = (wchar_t)(value % 10 + '0');

	return destination;
}
