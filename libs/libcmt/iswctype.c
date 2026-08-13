/*
ISWCTYPE.C

symbols in this file:
	_iswctype (.text, 0x30)
	_is_wctype (.text, 0x5)
*/

/* ---------- types */

typedef unsigned short wint_t;
typedef unsigned short wctype_t;

/* ---------- constants */

enum
{
	_wide_character_count = 256,
	_wide_end_of_file = 0xFFFF
};

/* ---------- external data */

extern wctype_t const *_pwctype;

/* ---------- public code */

int __cdecl iswctype(
	wint_t character,
	wctype_t mask)
{
	wctype_t character_type;

	if (character == (wint_t)_wide_end_of_file || character >= _wide_character_count)
		character_type = 0;
	else
		character_type = _pwctype[character];

	return character_type & mask;
}

int __cdecl is_wctype(
	wint_t character,
	wctype_t mask)
{
	return iswctype(character, mask);
}
