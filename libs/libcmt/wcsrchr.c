/*
WCSRCHR.C

symbols in this file:
	_wcsrchr (.text, 0x30)
*/

/* ---------- types */

typedef unsigned short wchar_t;

/* ---------- public code */

wchar_t * __cdecl wcsrchr(
	wchar_t const *string,
	wchar_t character)
{
	wchar_t const *start = string;

	while (*string++)
		;

	while (--string != start && *string != character)
		;

	return *string == character ? (wchar_t *)string : 0;
}
