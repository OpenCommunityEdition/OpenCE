/* ---------- headers */

#include <wchar.h>
#include <wctype.h>
#include <stdarg.h>

#include "cseries/cseries.h"
#include "cseries/errors.h" /* port: ustring_format_checked */
#include "unicode.h"

/* ---------- constants */

enum
{
	MAXIMUM_MEMCMP_SIZE = 0x10000000,
	MAXIMUM_MEMCPY_MEMMOVE_SIZE = 0x10000000,
	MAXIMUM_MEMSET_SIZE = 0x10000000,
	MAXIMUM_MODE_STRING_SIZE = 4,
	MAXIMUM_STRING_SIZE = 0x8000,
};

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

#ifndef HALO_ARM64_GUEST /* Mach-O section names differ; the default is .bss anyway */
#pragma bss_seg(".bss")
#endif
static wchar_t bss_004c1a08[0x100];
#ifndef HALO_ARM64_GUEST
#pragma bss_seg()
#endif

/* ---------- public code */

int uisalpha(
	wchar_t character)
{
	return iswalpha(character);
}

int uisupper(
	wchar_t character)
{
	return iswupper(character);
}

int uislower(
	wchar_t character)
{
	return iswlower(character);
}

int uisdigit(
	wchar_t character)
{
	return iswdigit(character);
}

int uisxdigit(
	wchar_t character)
{
	return iswxdigit(character);
}

int uisspace(
	wchar_t character)
{
	return iswspace(character);
}

int uispunct(
	wchar_t character)
{
	return iswpunct(character);
}

int uisalnum(
	wchar_t character)
{
	return iswalnum(character);
}

int uisprint(
	wchar_t character)
{
	return iswprint(character);
}

int uisgraph(
	wchar_t character)
{
	return iswgraph(character);
}

int uiscntrl(
	wchar_t character)
{
	return iswcntrl(character);
}

int utoupper(
	wchar_t character)
{
	return towupper(character);
}

int utolower(
	wchar_t character)
{
	return towlower(character);
}

void *umemchr(
	void const *buffer,
	int value,
	unsigned long count)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		84,
		buffer);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		85,
		count < MAXIMUM_MEMCMP_SIZE);

	return memchr(buffer, value, count);
}

long umemcmp(
	void const *buffer1,
	void const *buffer2,
	unsigned long count)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		109,
		buffer1 && buffer2);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		110,
		(count >= 0) && (count <= MAXIMUM_MEMCMP_SIZE));

	return csmemcmp(buffer1, buffer2, count);
}

void *umemmove(
	void *dest,
	void const *src,
	unsigned long count)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		121,
		dest && src);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		122,
		(count >= 0) && (count <= MAXIMUM_MEMCPY_MEMMOVE_SIZE));

	return csmemmove(dest, src, count);
}

wint_t ufgetc(
	FILE *stream)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		589,
		stream);

	return fgetwc(stream);
}

wint_t ufputc(
	wchar_t character,
	FILE *stream)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		599,
		stream);

	return fputwc(character, stream);
}

wint_t uungetc(
	wchar_t character,
	FILE *stream)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		609,
		stream);

	return ungetwc(character, stream);
}

int ufclose(
	FILE *stream)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		900,
		stream);

	return fclose(stream);
}

wchar_t *uctime(
	time_t const *timer)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		1014,
		timer);

	return _wctime(timer);
}

wchar_t *uasctime(
	struct tm const *timeptr)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		1023,
		timeptr);

	return _wasctime(timeptr);
}

wchar_t *utmpnam(
	wchar_t *string)
{
	return _wtmpnam(string);
}

unsigned long ustrlen(
	wchar_t const *string)
{
	unsigned long size;

	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		194,
		string);

	size = wcslen(string);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		196,
		size < MAXIMUM_STRING_SIZE);

	return size;
}

wchar_t *ustrchr(
	wchar_t const *string,
	wchar_t character)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		224,
		string);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		225,
		wcslen(string) < MAXIMUM_STRING_SIZE);

	return wcschr(string, character);
}

wchar_t *ustrrchr(
	wchar_t const *string,
	wchar_t character)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		333,
		string);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		334,
		wcslen(string) < MAXIMUM_STRING_SIZE);

	return wcsrchr(string, character);
}

wchar_t *ustrlwr(
	wchar_t *string)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		392,
		string);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		393,
		wcslen(string) < MAXIMUM_STRING_SIZE);

	return _wcslwr(string);
}

wchar_t *ustrupr(
	wchar_t *string)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		402,
		string);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		403,
		wcslen(string) < MAXIMUM_STRING_SIZE);

	return _wcsupr(string);
}

wchar_t *ugets(
	wchar_t *string)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		643,
		string);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		644,
		wcslen(string) < MAXIMUM_STRING_SIZE);

	return _getws(string);
}

int uputs(
	wchar_t const *string)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		665,
		string);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		666,
		wcslen(string) < MAXIMUM_STRING_SIZE);

	return _putws(string);
}

int uremove(
	wchar_t const *path)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		946,
		path);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		947,
		wcslen(path) < MAXIMUM_STRING_SIZE);

	return _wremove(path);
}

long ustrtol(
	wchar_t const *nptr,
	wchar_t **endptr,
	int base)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		967,
		nptr);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		968,
		wcslen(nptr) < MAXIMUM_STRING_SIZE);

	return wcstol(nptr, endptr, base);
}

int uatoi(
	wchar_t const *string)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		1002,
		string);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		1003,
		wcslen(string) < MAXIMUM_STRING_SIZE);

	return _wtoi(string);
}

void *umemset(
	void *buffer,
	int value,
	unsigned long count)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		133,
		buffer);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		134,
		(count >= 0) && (count <= MAXIMUM_MEMSET_SIZE));

	return csmemset(buffer, value, count);
}

long ustrncmp(
	wchar_t const *string1,
	wchar_t const *string2,
	unsigned long count)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		298,
		string1 && string2);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		299,
		(count >= 0) && (count < MAXIMUM_STRING_SIZE));

	return wcsncmp(string1, string2, count);
}

wchar_t *ustrncpy(
	wchar_t *dest,
	wchar_t const *src,
	unsigned long count)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		310,
		dest && src);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		311,
		(count >= 0) && (count < MAXIMUM_STRING_SIZE));

	return wcsncpy(dest, src, count);
}

wchar_t *ustrtok(
	wchar_t *string,
	wchar_t const *delimiters)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		368,
		delimiters);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		369,
		wcslen(delimiters) < MAXIMUM_STRING_SIZE);

	return wcstok(string, delimiters);
}

int uprintf(
	wchar_t const *format,
	...)
{
	int result;
	va_list arglist;

	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		699,
		format);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		700,
		wcslen(format) < MAXIMUM_STRING_SIZE);

	va_start(arglist, format);
	result = vwprintf(format, arglist);
	va_end(arglist);

	return result;
}

int uvprintf(
	wchar_t const *format,
	va_list arglist)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		803,
		format);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		804,
		wcslen(format) < MAXIMUM_STRING_SIZE);

	return vwprintf(format, arglist);
}

void uperror(
	wchar_t const *string)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		922,
		string);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		923,
		wcslen(string) < MAXIMUM_STRING_SIZE);

	_wperror(string);

	return;
}

unsigned long ustrtoul(
	wchar_t const *nptr,
	wchar_t **endptr,
	int base)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		979,
		nptr);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		980,
		wcslen(nptr) < MAXIMUM_STRING_SIZE);

	return wcstoul(nptr, endptr, base);
}

double ustrtod(
	wchar_t const *nptr,
	wchar_t **endptr)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		990,
		nptr);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		991,
		wcslen(nptr) < MAXIMUM_STRING_SIZE);

	return wcstod(nptr, endptr);
}

unsigned long ustrnlen(
	wchar_t const *string,
	unsigned long maximum_length)
{
	unsigned long size;

	size = 0;
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		208,
		string);

	while ((size < maximum_length) && *string++)
		size++;

	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		214,
		size < MAXIMUM_STRING_SIZE);

	return size;
}

wchar_t *ustrcpy(
	wchar_t *dest,
	wchar_t const *src)
{
	unsigned long source_size;

	source_size = wcslen(src);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		146,
		(source_size >= 0) && (source_size < MAXIMUM_STRING_SIZE));
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		147,
		((src+source_size) < dest) || ((dest + source_size) < src));

	return wcscpy(dest, src);
}

int uvfprintf(
	FILE *stream,
	wchar_t const *format,
	va_list arglist)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		792,
		stream && format);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		793,
		wcslen(format) < MAXIMUM_STRING_SIZE);

	return vfwprintf(stream, format, arglist);
}

void *umemcpy(
	void *dest,
	void const *src,
	unsigned long count)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		96,
		dest && src);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		97,
		(count >= 0) && (count < MAXIMUM_MEMCPY_MEMMOVE_SIZE));
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		98,
		(((char *)src+count) <= (char *)dest) || (((char *)dest+count) <= (char *)src));

	return csmemcpy(dest, src, count);
}

wchar_t *ustrncat(
	wchar_t *dest,
	wchar_t const *src,
	unsigned long count)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		273,
		dest && src);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		274,
		wcslen(dest) < MAXIMUM_STRING_SIZE);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		275,
		(count >= 0) && (count < MAXIMUM_STRING_SIZE));

	return wcsncat(dest, src, count);
}

wchar_t *ufgets(
	wchar_t *string,
	int size,
	FILE *stream)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		620,
		string);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		621,
		wcslen(string) < MAXIMUM_STRING_SIZE);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		622,
		size < MAXIMUM_STRING_SIZE);

	return fgetws(string, size, stream);
}

int ufputs(
	wchar_t const *string,
	FILE *stream)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		632,
		string);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		633,
		wcslen(string) < MAXIMUM_STRING_SIZE);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		634,
		stream);

	return fputws(string, stream);
}

int ufprintf(
	FILE *stream,
	wchar_t const *format,
	...)
{
	int result;
	va_list arglist;

	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		680,
		stream);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		681,
		format);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		682,
		wcslen(format) < MAXIMUM_STRING_SIZE);

	va_start(arglist, format);
	result = vfwprintf(stream, format, arglist);
	va_end(arglist);

	return result;
}

FILE *ufdopen(
	int fd,
	wchar_t const *path)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		877,
		path);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		878,
		wcslen(path) < MAXIMUM_STRING_SIZE);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		879,
		fd > 0);

	return _wfdopen(fd, path);
}

FILE *ufopen(
	wchar_t const *path,
	wchar_t const *mode)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		889,
		path && mode);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		890,
		wcslen(path) < MAXIMUM_STRING_SIZE);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		891,
		wcslen(mode) < MAXIMUM_MODE_STRING_SIZE);

	return _wfopen(path, mode);
}

FILE *upopen(
	wchar_t const *command,
	wchar_t const *mode)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		935,
		command && mode);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		936,
		wcslen(command) < MAXIMUM_STRING_SIZE);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		937,
		wcslen(mode) < MAXIMUM_MODE_STRING_SIZE);

	return NULL;
}

wchar_t *ascii_to_wide(
	char const *ascii,
	wchar_t *unicode,
	unsigned long size)
{
	unsigned long length;
	long i;

	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		1087,
		ascii && unicode);

	length = csstrlen(ascii);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		1089,
		length < MAXIMUM_STRING_SIZE);

	if (size >= 2 * length + 2)
	{
		unicode[length] = 0;
		for (i = length - 1; i >= 0; i--)
			unicode[i] = (short)ascii[i];

		return unicode;
	}

	return NULL;
}

wchar_t *ustrcat(
	wchar_t *dest,
	wchar_t const *src)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		157,
		dest && src);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		158,
		wcslen(dest) < MAXIMUM_STRING_SIZE);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		159,
		wcslen(src) < MAXIMUM_STRING_SIZE);

	return wcscat(dest, src);
}

long ustrcmp(
	wchar_t const *string1,
	wchar_t const *string2)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		181,
		string1 && string2);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		182,
		wcslen(string1) < MAXIMUM_STRING_SIZE);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		183,
		wcslen(string2) < MAXIMUM_STRING_SIZE);

	return wcscmp(string1, string2);
}

long ustrcoll(
	wchar_t const *string1,
	wchar_t const *string2)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		235,
		string1 && string2);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		236,
		wcslen(string1) < MAXIMUM_STRING_SIZE);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		237,
		wcslen(string2) < MAXIMUM_STRING_SIZE);

	return wcscoll(string1, string2);
}

unsigned long ustrcspn(
	wchar_t const *string,
	wchar_t const *character_set)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		247,
		string && character_set);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		248,
		wcslen(string) < MAXIMUM_STRING_SIZE);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		249,
		wcslen(character_set) < MAXIMUM_STRING_SIZE);

	return wcscspn(string, character_set);
}

wchar_t *ustrpbrk(
	wchar_t const *string,
	wchar_t const *character_set)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		321,
		string && character_set);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		322,
		wcslen(string) < MAXIMUM_STRING_SIZE);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		323,
		wcslen(character_set) < MAXIMUM_STRING_SIZE);

	return wcspbrk(string, character_set);
}

unsigned long ustrspn(
	wchar_t const *string,
	wchar_t const *character_set)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		344,
		string && character_set);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		345,
		wcslen(string) < MAXIMUM_STRING_SIZE);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		346,
		wcslen(character_set) < MAXIMUM_STRING_SIZE);

	return wcsspn(string, character_set);
}

wchar_t *ustrstr(
	wchar_t const *string,
	wchar_t const *character_set)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		356,
		string && character_set);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		357,
		wcslen(string) < MAXIMUM_STRING_SIZE);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		358,
		wcslen(character_set) < MAXIMUM_STRING_SIZE);

	return wcsstr(string, character_set);
}

wchar_t *ustrnlwr(
	wchar_t *string,
	unsigned long count)
{
	wchar_t *position;

	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		415,
		string);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		416,
		wcslen(string) < MAXIMUM_STRING_SIZE);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		417,
		count < MAXIMUM_STRING_SIZE);

	for (position = string; *position; position++)
		*position = towupper(*position);

	return string;
}

wchar_t *ustrnupr(
	wchar_t *string,
	unsigned long count)
{
	wchar_t *position;

	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		435,
		string);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		436,
		wcslen(string) < MAXIMUM_STRING_SIZE);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		437,
		count < MAXIMUM_STRING_SIZE);

	for (position = string; *position; position++)
		*position = towlower(*position);

	return string;
}

unsigned long ustrxfrm(
	wchar_t *dest,
	wchar_t const *src,
	unsigned long count)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		380,
		dest && src);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		381,
		wcslen(dest) < MAXIMUM_STRING_SIZE);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		382,
		wcslen(src) < MAXIMUM_STRING_SIZE);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		383,
		count < MAXIMUM_STRING_SIZE);

	return wcsxfrm(dest, src, count);
}

long ustrcasecmp(
	wchar_t const *string1,
	wchar_t const *string2)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		455,
		string1 && string2);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		456,
		wcslen(string1) < MAXIMUM_STRING_SIZE);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		457,
		wcslen(string2) < MAXIMUM_STRING_SIZE);

	return _wcsicmp(string1, string2);
}

long ustrncasecmp(
	wchar_t const *string1,
	wchar_t const *string2,
	unsigned long count)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		472,
		string1 && string2);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		473,
		wcslen(string1) < MAXIMUM_STRING_SIZE);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		474,
		wcslen(string2) < MAXIMUM_STRING_SIZE);

	return _wcsnicmp(string1, string2, count);
}

int usnprintf(
	wchar_t *string,
	unsigned long size,
	wchar_t const *format,
	...)
{
	int result;
	va_list arglist;

	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		719,
		string);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		720,
		(size > 0) && (size <= MAXIMUM_STRING_SIZE));
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		721,
		wcslen(format) < MAXIMUM_STRING_SIZE);

	va_start(arglist, format);
	result = _vsnwprintf(string, size, format, arglist);
	va_end(arglist);

	return result;
}

/* port: whether a format (a map's text: its string lists' are the game's
formats) takes no other arguments than conversions names, in order, if
fewer: 'd' an integer (%d, %i, %u, %x, %X, %o, %c), 'f' a real (%f, %e,
%g), 's' a string (%s, %S). %% and the flags, widths and precisions are
allowed; '*' (which takes an argument of its own) and %n are not. A format
that takes others would have the game read an argument as what it is not */
int ustring_format_takes(
	wchar_t const *format,
	char const *conversions)
{
	if (!format || !conversions)
		return FALSE;
	while (*format)
	{
		char kind;
		short digits;

		if (*format++ != L'%')
			continue;
		if (*format == L'%')
		{
			format++;
			continue;
		}
		while (*format == L'-' || *format == L'+' || *format == L' ' || *format == L'#' || *format == L'0')
			format++;
		/* (a width, then a precision, of a few digits: the game's are, and
		Windows's printf refuses what is not a conversion) */
		for (digits = 0; *format >= L'0' && *format <= L'9'; digits++)
			format++;
		if (digits > 3)
			return FALSE;
		if (*format == L'.')
		{
			format++;
			for (digits = 0; *format >= L'0' && *format <= L'9'; digits++)
				format++;
			if (digits > 3)
				return FALSE;
		}
		/* (the size prefixes the game's own formats use) */
		if (*format == L'h' || *format == L'l' || *format == L'w')
			format++;
		switch (*format)
		{
		case L'd': case L'i': case L'u': case L'x': case L'X': case L'o': case L'c':
			kind = 'd';
			break;
		case L'f': case L'e': case L'E': case L'g': case L'G':
			kind = 'f';
			break;
		case L's': case L'S':
			kind = 's';
			break;
		default:
			return FALSE;
		}
		format++;
		if (*conversions++ != kind)
			return FALSE;
	}

	return TRUE;
}

/* port: a map's format, if it takes the arguments a caller gives it
(ustring_format_takes); else an empty one, whose message is then empty, and
which is logged once */
wchar_t const *ustring_format_checked(
	wchar_t const *format,
	char const *conversions)
{
	static boolean logged = FALSE;

	if (ustring_format_takes(format, conversions))
		return format;
	if (!logged)
	{
		logged = TRUE;
		error(_error_silent, "a map's text is a format of other arguments than the game gives it (%s): it is not shown",
			conversions);
	}

	return L"";
}

/* port: at most size - 1 characters of src, always terminated */
wchar_t *ustrncpy_terminated(
	wchar_t *dest,
	wchar_t const *src,
	unsigned long size)
{
	if (size > 0)
	{
		wcsncpy(dest, src ? src : L"", size - 1);
		dest[size - 1] = 0;
	}

	return dest;
}

int usprintf(
	wchar_t *string,
	wchar_t const *format,
	...)
{
	int result;
	va_list arglist;

	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		751,
		string && format);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		752,
		wcslen(string) < MAXIMUM_STRING_SIZE);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		753,
		wcslen(format) < MAXIMUM_STRING_SIZE);

	va_start(arglist, format);
	result = vswprintf(string, format, arglist);
	va_end(arglist);

	return result;
}

int uvsnprintf(
	wchar_t *string,
	unsigned long size,
	wchar_t const *format,
	va_list arglist)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		816,
		string && format);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		817,
		wcslen(string) < MAXIMUM_STRING_SIZE);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		818,
		wcslen(format) < MAXIMUM_STRING_SIZE);

	return _vsnwprintf(string, size, format, arglist);
}

int uvsprintf(
	wchar_t *string,
	wchar_t const *format,
	va_list arglist)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		841,
		string && format);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		842,
		wcslen(string) < MAXIMUM_STRING_SIZE);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		843,
		wcslen(format) < MAXIMUM_STRING_SIZE);

	return vswprintf(string, format, arglist);
}

FILE *ufreopen(
	wchar_t const *path,
	wchar_t const *mode,
	FILE *stream)
{
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		911,
		path && mode);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		912,
		wcslen(path) < MAXIMUM_STRING_SIZE);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		913,
		wcslen(mode) < MAXIMUM_MODE_STRING_SIZE);

	return _wfreopen(path, mode, stream);
}

char *wide_to_ascii(
	wchar_t const *unicode,
	char *ascii,
	unsigned long size)
{
	unsigned long length;
	unsigned long i;

	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		1040,
		unicode && ascii);

	length = wcslen(unicode);
	match_assert(
		"c:\\halo\\SOURCE\\text\\unicode.c",
		1042,
		length < MAXIMUM_STRING_SIZE);

	if (length > size - 1)
		return NULL;

	for (i = 0; i < length; i++)
	{
		if (unicode[i] & 0xFF80)
			return NULL;
	}

	for (i = 0; i < length; i++)
		ascii[i] = (char)unicode[i];

	ascii[i] = 0;

	return ascii;
}

wchar_t *ustrerror(
	int error_number)
{
	bss_004c1a08[0] = 0;
	usnprintf(bss_004c1a08, 0x100, L"%hs", strerror(error_number));

	return bss_004c1a08;
}

/* ---------- private code */
