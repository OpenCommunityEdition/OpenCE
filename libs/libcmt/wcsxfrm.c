/*
WCSXFRM.C

symbols in this file:
	_wcsxfrm (.text, 0x25)
*/

/* ---------- types */

typedef unsigned int size_t;
typedef unsigned short wchar_t;

/* ---------- external code */

size_t __cdecl wcslen(
	wchar_t const *string);

wchar_t * __cdecl wcsncpy(
	wchar_t *destination,
	wchar_t const *source,
	size_t count);

/* ---------- public code */

size_t __cdecl wcsxfrm(
	wchar_t *destination,
	wchar_t const *source,
	size_t count)
{
	if (destination)
		wcsncpy(destination, source, count);

	return wcslen(source);
}
