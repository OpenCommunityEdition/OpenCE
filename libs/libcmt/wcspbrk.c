/*
WCSPBRK.C

symbols in this file:
	_wcspbrk (.text, 0x40)
*/

/* ---------- types */

typedef unsigned short wchar_t;

/* ---------- public code */

wchar_t * __cdecl wcspbrk(
	wchar_t const *string,
	wchar_t const *control)
{
	while (*string)
	{
		wchar_t const *candidate = control;

		while (*candidate)
		{
			if (*candidate == *string)
				return (wchar_t *)string;

			candidate++;
		}

		string++;
	}

	return 0;
}
