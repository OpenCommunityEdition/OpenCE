/* ---------- headers */

#define BUILDING_CSERIES

#include <wchar.h>

#include "cseries.h"
#include "cseries_windows.h"
#include "profile.h"
#include "errors.h"
#include "real_math.h"
#include "byte_swapping.h"
#include "crc.h"

/* ---------- constants */

enum
{
	MAXIMUM_MEMCPY_MEMMOVE_SIZE = 0x10000000,
	MAXIMUM_MEMSET_SIZE = 0x10000000,
	MAXIMUM_MEMCMP_SIZE = 0x10000000,
	MAXIMUM_STRING_SIZE = 0x2000,
};

/* ---------- macros */

#ifdef HALO_RELEASE
#define cseries_match_assert(file, line, expr) if (!(expr)) { release_assert_failed(STRINGIFY(expr), MATCH_FILE(file), MATCH_LINE(line), TRUE); }
#else
#define cseries_match_assert(file, line, expr) if (!(expr)) { stack_walk(0); error(_error_silent, "EXCEPTION %s in %s,#%d: %s", "halt", MATCH_FILE(file), MATCH_LINE(line), STRINGIFY(expr)); system_exit(-1); }
#endif
#define cseries_assert(expr) cseries_match_assert(__FILE__, __LINE__, expr)

/* ---------- structures */

/* ---------- globals */

char temporary[256];

static const real_argb_color global_real_argb_color_table[17] =
{
	{ 1.f, 1.f,		1.f,	1.f  },
	{ 1.f, .5f,		.5f,	.5f  },
	{ 1.f, .0f,		.0f,	.0f  },
	{ 1.f, 1.f,		.0f,	.0f  },
	{ 1.f, .0f,		1.f,	.0f  },
	{ 1.f, .0f,		.0f,	1.f  },
	{ 1.f, .0f,		1.f,	1.f  },
	{ 1.f, 1.f,		1.f,	.0f  },
	{ 1.f, 1.f,		.0f,	1.f  },
	{ 1.f, 1.f,		.41f,	.7f  },
	{ 1.f, .39f,	.58f,	.93f },
	{ 1.f, 1.f,		.5f,	.0f },
	{ 1.f, .44f,	.05f,	.43f },
	{ 1.f, .5f,		1.f,	.83f },
	{ 1.f, .0f,		.39f,	.0f },
	{ 1.f, 1.f,		.63f,	.48f },
	{ 1.f, .81f,	.13f,	.56f }
};

const real_argb_color *global_real_argb_white = &global_real_argb_color_table[0];
const real_argb_color *global_real_argb_grey = &global_real_argb_color_table[1];
const real_argb_color *global_real_argb_black = &global_real_argb_color_table[2];
const real_argb_color *global_real_argb_red = &global_real_argb_color_table[3];
const real_argb_color *global_real_argb_green = &global_real_argb_color_table[4];
const real_argb_color *global_real_argb_blue = &global_real_argb_color_table[5];
const real_argb_color *global_real_argb_cyan = &global_real_argb_color_table[6];
const real_argb_color *global_real_argb_yellow = &global_real_argb_color_table[7];
const real_argb_color *global_real_argb_magenta = &global_real_argb_color_table[8];
const real_argb_color *global_real_argb_pink = &global_real_argb_color_table[9];
const real_argb_color *global_real_argb_lightblue = &global_real_argb_color_table[10];
const real_argb_color *global_real_argb_orange = &global_real_argb_color_table[11];
const real_argb_color *global_real_argb_purple = &global_real_argb_color_table[12];
const real_argb_color *global_real_argb_aqua = &global_real_argb_color_table[13];
const real_argb_color *global_real_argb_darkgreen = &global_real_argb_color_table[14];
const real_argb_color *global_real_argb_salmon = &global_real_argb_color_table[15];
const real_argb_color *global_real_argb_violet = &global_real_argb_color_table[16];

const real_rgb_color *global_real_rgb_white = &global_real_argb_color_table[0].rgb;
const real_rgb_color *global_real_rgb_grey = &global_real_argb_color_table[1].rgb;
const real_rgb_color *global_real_rgb_black = &global_real_argb_color_table[2].rgb;
const real_rgb_color *global_real_rgb_red = &global_real_argb_color_table[3].rgb;
const real_rgb_color *global_real_rgb_green = &global_real_argb_color_table[4].rgb;
const real_rgb_color *global_real_rgb_blue = &global_real_argb_color_table[5].rgb;
const real_rgb_color *global_real_rgb_cyan = &global_real_argb_color_table[6].rgb;
const real_rgb_color *global_real_rgb_yellow = &global_real_argb_color_table[7].rgb;
const real_rgb_color *global_real_rgb_magenta = &global_real_argb_color_table[8].rgb;
const real_rgb_color *global_real_rgb_pink = &global_real_argb_color_table[9].rgb;
const real_rgb_color *global_real_rgb_lightblue = &global_real_argb_color_table[10].rgb;
const real_rgb_color *global_real_rgb_orange = &global_real_argb_color_table[11].rgb;
const real_rgb_color *global_real_rgb_purple = &global_real_argb_color_table[12].rgb;
const real_rgb_color *global_real_rgb_aqua = &global_real_argb_color_table[13].rgb;
const real_rgb_color *global_real_rgb_darkgreen = &global_real_argb_color_table[14].rgb;
const real_rgb_color *global_real_rgb_salmon = &global_real_argb_color_table[15].rgb;
const real_rgb_color *global_real_rgb_violet = &global_real_argb_color_table[16].rgb;

/* ---------- public code */

void cseries_initialize(
	void)
{
	debug_memory_manager_initialize();
	profile_initialize();
	profile_global_enable = FALSE;

	return;
};

void cseries_dispose(
	void)
{
	debug_dump_memory();

	return;
};

tag string_to_tag(
	const char *s)
{
	tag t = *(tag*)s;
	
	return SWAP4(t);
}

char *tag_to_string(
	tag t,
	char *s)
{
	*(unsigned long *)s = SWAP4(t);
	s[4] = '\0';

	return s;
}

long strnlen(
	const char *string,
	long n)
{
	long length;
	const char *p;

	length = 0;
	if (n > 0)
	{
		p = string;
		do
		{
			if (!*p++)
				break;
			length++;
		} while (length < n);
	}

	return length;
}


char *strnupr(
	char *string,
	long n)
{
	unsigned char *p;
	
	for (p = (unsigned char *)string; *p && n-->0; p++)
	{
		*p = toupper(*p);
	}
	
	return string;
}

char *strnlwr(
	char *string,
	long n)
{
	unsigned char *p;
	
	for (p = (unsigned char *)string; *p && n-->0; p++)
	{
		*p = tolower(*p);
	}
	
	return string;
}

char *strupr(
	char *string)
{
	unsigned char *p;
	
	for (p = (unsigned char *)string; *p; p++)
	{
		*p = toupper(*p);
	}
	
	return string;
}

char *strlwr(
	char *string)
{
	unsigned char *p;
	
	for (p = (unsigned char *)string; *p; p++)
	{
		*p = tolower(*p);
	}
	
	return string;
}

char *csprintf(
	char *buffer,
	char *format,
	...)
{
	va_list arglist;
	
	va_start(arglist, format);
	/* port: no longer than the longest a string is (MAXIMUM_STRING_SIZE,
	as csstrlen asserts): the caller's buffer's size isn't passed. But
	temporary, which most callers format into (often with a map's tag
	names), is only its own size */
	vsnprintf(buffer, buffer == temporary ? sizeof(temporary) : MAXIMUM_STRING_SIZE, format, arglist);
	va_end(arglist);
	
	return buffer;
}

#ifdef HALO_RELEASE
__thread boolean display_assert_skipped = FALSE;

/* port: a release build carries on past a failed assertion (cseries.h),
but notes it in debug.txt, as a debug build does before it stops: each
place's first failure, then its 10th, 100th, 1000th and so on with the
count, so that one failing every frame does not flood the file */
void release_assert_failed(
	char const *information,
	char const *file,
	long line,
	boolean fatal)
{
	static struct
	{
		char const *file;
		long line;
		unsigned long count;
	} places[512];
	static volatile long places_lock;
	unsigned long index = ((unsigned long)(size_t)file + (unsigned long)line * 2654435761UL) % NUMBEROF(places);
	unsigned long count = 0;
	unsigned long probe;

	while (__sync_lock_test_and_set(&places_lock, 1))
		;
	for (probe = 0; probe < NUMBEROF(places); probe++)
	{
		if (!places[index].file)
		{
			places[index].file = file;
			places[index].line = line;
		}
		if (places[index].file == file && places[index].line == line)
		{
			count = ++places[index].count;
			break;
		}
		index = (index + 1) % NUMBEROF(places);
	}
	__sync_lock_release(&places_lock);

	/* (a place the full table has no room for: every time) */
	if (count > 1)
	{
		unsigned long power = 10;

		while (power < count && power < 1000000000UL)
			power *= 10;
		if (power != count)
			return;
	}
	if (count > 1)
	{
		error(_error_log, "EXCEPTION %s in %s,#%ld: %s (release build, failed %lu times)", fatal ? "assert" : "warn",
			file, line, information ? information : "<no reason given>", count);
	}
	else
	{
		error(_error_log, "EXCEPTION %s in %s,#%ld: %s (release build)", fatal ? "assert" : "warn",
			file, line, information ? information : "<no reason given>");
	}
}
#endif

void display_assert(
	char *information,
	char *file,
	long line,
	boolean fatal)
{
#ifdef HALO_RELEASE
	/* release builds carry on past assertions (cseries.h), including the
	ones written out as display_assert followed by system_exit(-1), which
	then returns (cseries_windows.c); noted in debug.txt all the same (a
	halt, which does stop, too) */
	release_assert_failed(information, file, line, fatal);
	display_assert_skipped = fatal;
#else
	if (fatal)
	{
		stack_walk(0);
	}
	
	error(_error_silent, "EXCEPTION %s in %s,#%d: %s", fatal ? "halt" : "warn", file, line, information ? information : "<no reason given>");
#endif
}

long csmemcmp(
	const void *p1,
	const void *p2,
	unsigned long size)
{
	cseries_match_assert("c:\\halo\\SOURCE\\cseries\\cseries.c", 255, p1 && p2);
	cseries_match_assert("c:\\halo\\SOURCE\\cseries\\cseries.c", 256, size>=0 && size<=MAXIMUM_MEMCMP_SIZE);

	return memcmp(p1, p2, size);
}

void *csmemmove(
	void *destination,
	const void *source,
	unsigned long size)
{
	cseries_match_assert("c:\\halo\\SOURCE\\cseries\\cseries.c", 267, destination && source);
	cseries_match_assert("c:\\halo\\SOURCE\\cseries\\cseries.c", 268, size>=0 && size<=MAXIMUM_MEMCPY_MEMMOVE_SIZE);

	return memmove(destination, source, size);
}

void *csmemset(
	void *buffer,
	long c,
	unsigned long size)
{
	cseries_match_assert("c:\\halo\\SOURCE\\cseries\\cseries.c", 279, buffer);
	cseries_match_assert("c:\\halo\\SOURCE\\cseries\\cseries.c", 280, size>=0 && size<=MAXIMUM_MEMSET_SIZE);

	return memset(buffer, c, size);
}

char *csstrcat(
	char *s1,
	const char *s2)
{
	unsigned long length;
	unsigned long append_length;

	cseries_match_assert("c:\\halo\\SOURCE\\cseries\\cseries.c", 290, s1 && s2);

	/* port: the result is no longer than the longest a string is
	(MAXIMUM_STRING_SIZE, as csstrlen asserts): the rest of s2 is left
	off. The caller's buffer's size isn't passed */
	length = strlen(s1);
	if (length >= MAXIMUM_STRING_SIZE-1)
		return s1;
	append_length = strlen(s2);
	if (append_length > MAXIMUM_STRING_SIZE-1-length)
		append_length = MAXIMUM_STRING_SIZE-1-length;
	memmove(s1+length, s2, append_length);
	s1[length+append_length] = 0;

	return s1;
}

long csstrcmp(
	const char *s1,
	const char *s2)
{
	cseries_match_assert("c:\\halo\\SOURCE\\cseries\\cseries.c", 300, s1 && s2);

	return strcmp(s1, s2);
}

char *csstrncat(
	char *s1,
	const char *s2,
	unsigned long size)
{
	cseries_match_assert("c:\\halo\\SOURCE\\cseries\\cseries.c", 311, s1 && s2);
	cseries_match_assert("c:\\halo\\SOURCE\\cseries\\cseries.c", 312, size>=0 && size<MAXIMUM_STRING_SIZE);

	return strncat(s1, s2, size);
}

long csstrncmp(
	const char *s1,
	const char *s2,
	unsigned long size)
{
	cseries_match_assert("c:\\halo\\SOURCE\\cseries\\cseries.c", 323, s1 && s2);
	cseries_match_assert("c:\\halo\\SOURCE\\cseries\\cseries.c", 324, size>=0 && size<MAXIMUM_STRING_SIZE);

	return strncmp(s1, s2, size);
}

char *csstrncpy(
	char *s1,
	const char *s2,
	unsigned long size)
{
	cseries_match_assert("c:\\halo\\SOURCE\\cseries\\cseries.c", 335, s1 && s2);
	cseries_match_assert("c:\\halo\\SOURCE\\cseries\\cseries.c", 336, size>=0 && size<MAXIMUM_STRING_SIZE);

	return strncpy(s1, s2, size);
}

char *csstrtok(
	char *s1,
	const char *s2)
{
	cseries_match_assert("c:\\halo\\SOURCE\\cseries\\cseries.c", 346, s2);

	return strtok(s1, s2);
}

unsigned long csstrlen(
	const char *s1)
{
	long size;
	
	cseries_match_assert("c:\\halo\\SOURCE\\cseries\\cseries.c", 357, s1);
	size = strlen(s1);
	cseries_match_assert("c:\\halo\\SOURCE\\cseries\\cseries.c", 359, size>=0 && size<MAXIMUM_STRING_SIZE);
	
	return size;
}

char *csstrcpy(
	char *destination,
	const char *source)
{
	long source_size = strlen(source);
	
	cseries_match_assert("c:\\halo\\SOURCE\\cseries\\cseries.c", 371, source_size>=0 && source_size<MAXIMUM_STRING_SIZE);
	cseries_match_assert("c:\\halo\\SOURCE\\cseries\\cseries.c", 372, source+source_size<destination || destination+source_size<source);
	
	/* port: no more is copied than the longest a string is
	(MAXIMUM_STRING_SIZE, the assert above, which a release build only
	logs), and copying over itself is a memmove. The destination isn't
	measured first (it was, unused): it is often not yet a string */
	if (source_size >= MAXIMUM_STRING_SIZE)
		source_size = MAXIMUM_STRING_SIZE-1;
	memmove(destination, source, source_size);
	destination[source_size] = 0;

	return destination;
}

void *csmemcpy(
	void *destination,
	const void *source,
	unsigned long size)
{
	cseries_match_assert("c:\\halo\\SOURCE\\cseries\\cseries.c", 383, destination && source);
	cseries_match_assert("c:\\halo\\SOURCE\\cseries\\cseries.c", 384, size>=0 && size<MAXIMUM_MEMCPY_MEMMOVE_SIZE);
	cseries_match_assert("c:\\halo\\SOURCE\\cseries\\cseries.c", 385, (byte *)source+size<=(byte *)destination || (byte *)destination+size<=(byte *)source);
	
	return memcpy(destination, source, size);
}

long csstrcasecmp(
	const char *s1,
	const char *s2)
{
	int c1;
	int c2;
	const char *first;
	const char *second;

	cseries_match_assert("c:\\halo\\SOURCE\\cseries\\cseries.c", 397, s1 && s2);

	c1 = towlower(*s1);
	c2 = towlower(*s2);
	if (c1 == 0)
		goto c1_zero;

	second = s2;
	first = s1;
loop:
	if (c2 == 0)
		goto c2_zero;
	if (c1 != c2)
		goto not_equal;
	second++;
	first++;
	c1 = towlower(*first);
	c2 = towlower(*second);
	if (c1 != 0)
		goto loop;

c1_zero:
	if (c2 != 0)
		return -1;
	return 0;

c2_zero:
	return c1 != 0;

not_equal:
	return c1 > c2 ? 1 : -1;
}

char *stristr(
	const char *haystack,
	const char *needle)
{
	/* as the assembly below: the first character matches exactly, the
	rest without regard to case */
	char first = *needle++;
	unsigned long length;

	if (!first)
		return (char *)haystack;
	length = csstrlen(needle);
	for (; *haystack; haystack++)
	{
		if (*haystack == first && !_strnicmp(haystack + 1, needle, length))
			return (char *)haystack;
	}
	return NULL;
}

unsigned long string_hash(
	const char *string)
{
	unsigned long hash;
	
	crc_new(&hash);
	crc_checksum_buffer(&hash, string, csstrlen(string));
	
	return hash;
}

/* ---------- private code */
