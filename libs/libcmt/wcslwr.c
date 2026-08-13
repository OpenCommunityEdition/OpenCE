/*
WCSLWR.C

symbols in this file:
	_wcslwr (.text, 0x2c)
	__wcslwr (.text, 0x2c; linked alias of _wcslwr)
*/

/* ---------- types */

typedef unsigned short wchar_t;

/* ---------- prototypes */

wchar_t * __cdecl _wcslwr(
	wchar_t *string);

/* ---------- public code */

wchar_t * __cdecl _wcslwr(
	wchar_t *string)
{
	wchar_t *character;

	for (character = string; *character; character++)
	{
		if (*character >= (wchar_t)'A' && *character <= (wchar_t)'Z')
			*character += (wchar_t)('a' - 'A');
	}

	return string;
}
