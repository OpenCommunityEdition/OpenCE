/*
WCSCHR.C

symbols in this file:
	_wcschr (.text, 0x22)
*/

/* ---------- types */

typedef unsigned short wchar_t;

/* ---------- public code */

wchar_t * __cdecl wcschr(
	wchar_t const *string,
	wchar_t character)
{
	while (*string && *string != character)
		string++;

	if (*string == character)
		return (wchar_t *)string;

	return 0;
}
