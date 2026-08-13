/*
WCSUPR.C

symbols in this file:
	_wcsupr (.text, 0x2c)
	__wcsupr (.text, 0x2c; alias of _wcsupr)
*/

/* ---------- types */

typedef unsigned short wchar_t;

/* ---------- prototypes */

wchar_t * __cdecl _wcsupr(
	wchar_t *string);

/* ---------- public code */

wchar_t * __cdecl _wcsupr(
	wchar_t *string)
{
	wchar_t *character;

	for (character = string; *character; character++)
	{
		if (*character >= (wchar_t)'a' && *character <= (wchar_t)'z')
			*character += (wchar_t)('A' - 'a');
	}

	return string;
}
