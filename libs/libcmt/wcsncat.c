/*
WCSNCAT.C

symbols in this file:
	_wcsncat (.text, 0x3d)
*/

/* ---------- types */

typedef unsigned short wchar_t;

/* ---------- prototypes */

wchar_t * __cdecl wcsncat(
	wchar_t *front,
	wchar_t const *back,
	unsigned long count);

/* ---------- public code */

wchar_t * __cdecl wcsncat(
	wchar_t *front,
	wchar_t const *back,
	unsigned long count)
{
	wchar_t *start = front;

	while (*front++)
		;
	front--;

	while (count--)
	{
		if (!(*front++ = *back++))
			return start;
	}
	*front = 0;

	return start;
}
