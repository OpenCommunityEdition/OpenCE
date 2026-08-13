/*
WCSNCPY.C

symbols in this file:
	_wcsncpy (.text, 0x3D)
*/

/* ---------- types */

typedef unsigned int size_t;
typedef unsigned short wchar_t;

/* ---------- public code */

wchar_t * __cdecl wcsncpy(
	wchar_t *destination,
	wchar_t const *source,
	size_t count)
{
	wchar_t *result = destination;

	while (count && (*destination++ = *source++))
		count--;

	if (count)
	{
		while (--count)
			*destination++ = 0;
	}

	return result;
}
