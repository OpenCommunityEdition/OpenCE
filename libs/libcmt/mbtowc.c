/*
MBTOWC.C

symbols in this file:
	_mbtowc (.text, 0x37)
*/

/* ---------- types */

typedef unsigned short wchar_t;

/* ---------- public code */

int __cdecl mbtowc(
	wchar_t *wide_character,
	char const *character,
	unsigned long count)
{
	if (!character || !count)
		return 0;

	if (!*character)
	{
		if (wide_character)
			*wide_character = 0;

		return 0;
	}

	if (wide_character)
		*wide_character = (unsigned char)*character;

	return 1;
}
