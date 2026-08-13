/*
WCSTOMBS.C

symbols in this file:
	_code_001d3099 (.text, 0x2f)
	_wcstombs (.text, 0x19e)
*/

#include <limits.h>
#include <stdlib.h>

/* ---------- constants */

#define EILSEQ 42
#define ERROR_INSUFFICIENT_BUFFER 122
#define FALSE 0
#define LC_CTYPE 2
#define _CLOCALEHANDLE 0

/* ---------- types */

typedef int BOOL;

/* ---------- external data */

extern unsigned int __lc_codepage;
extern unsigned int __lc_handle[];

/* ---------- external code */

int * __cdecl _errno(
	void);

unsigned long __stdcall GetLastError(
	void);

int __stdcall WideCharToMultiByte(
	unsigned int code_page,
	unsigned long flags,
	wchar_t const *wide_string,
	int wide_character_count,
	char *multibyte_string,
	int multibyte_character_count,
	char const *default_character,
	BOOL *used_default_character);

/* ---------- private prototypes */

static size_t __cdecl code_001d3099(
	wchar_t const *string,
	size_t count);

/* ---------- public code */

size_t __cdecl wcstombs(
	char *destination,
	wchar_t const *source,
	size_t maximum_count)
{
	size_t count = 0;
	int character_index;
	int converted_count;
	char buffer[MB_LEN_MAX];
	BOOL used_default_character = FALSE;

	if (destination && maximum_count == 0)
		return 0;

	if (destination)
	{
		if (__lc_handle[LC_CTYPE] == _CLOCALEHANDLE)
		{
			while (count < maximum_count)
			{
				if (*source > UCHAR_MAX)
				{
					*_errno() = EILSEQ;
					return (size_t)-1;
				}

				destination[count] = (char)*source;
				if (*source++ == L'\0')
					return count;

				count++;
			}

			return count;
		}

		if (MB_CUR_MAX == 1)
		{
			if (maximum_count > 0)
				maximum_count = code_001d3099(source, maximum_count);

			count = WideCharToMultiByte(
				__lc_codepage,
				0,
				source,
				(int)maximum_count,
				destination,
				(int)maximum_count,
				NULL,
				&used_default_character);

			if (count != 0 && !used_default_character)
			{
				if (destination[count - 1] == '\0')
					count--;

				return count;
			}

			*_errno() = EILSEQ;
			return (size_t)-1;
		}

		count = WideCharToMultiByte(
			__lc_codepage,
			0,
			source,
			-1,
			destination,
			(int)maximum_count,
			NULL,
			&used_default_character);

		if (count != 0 && !used_default_character)
			return count - 1;

		if (used_default_character || GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		{
			*_errno() = EILSEQ;
			return (size_t)-1;
		}

		while (count < maximum_count)
		{
			converted_count = WideCharToMultiByte(
				__lc_codepage,
				0,
				source,
				1,
				buffer,
				MB_CUR_MAX,
				NULL,
				&used_default_character);

			if (converted_count == 0 || used_default_character)
			{
				*_errno() = EILSEQ;
				return (size_t)-1;
			}

			if (count + converted_count > maximum_count)
				return count;

			for (character_index = 0;
				character_index < converted_count;
				character_index++, count++)
			{
				if ((destination[count] = buffer[character_index]) == '\0')
					return count;
			}

			source++;
		}

		return count;
	}

	if (__lc_handle[LC_CTYPE] == _CLOCALEHANDLE)
		return wcslen(source);

	count = WideCharToMultiByte(
		__lc_codepage,
		0,
		source,
		-1,
		NULL,
		0,
		NULL,
		&used_default_character);

	if (count == 0 || used_default_character)
	{
		*_errno() = EILSEQ;
		return (size_t)-1;
	}

	return count - 1;
}

/* ---------- private code */

static size_t __cdecl code_001d3099(
	wchar_t const *string,
	size_t count)
{
	size_t remaining = count + 1;
	wchar_t *current = (wchar_t *)string;

	while (--remaining && *current)
		current++;

	if (remaining && !*current)
		return current - string + 1;

	return count;
}
