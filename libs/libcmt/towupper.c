/*
TOWUPPER.C

symbols in this file:
	_towupper (.text, 0x47)
*/

/* ---------- types */

typedef unsigned short wint_t;
typedef unsigned short wctype_t;

/* ---------- constants */

enum
{
	_lower_case = 2,
	_case_difference = 'a' - 'A',
	_c_locale_handle = 0,
	_lc_ctype_index = 2,
	_wide_character_count = 256,
	_wide_end_of_file = 0xFFFF
};

/* ---------- external data */

extern unsigned long __lc_handle[];

/* ---------- external code */

int __cdecl iswctype(
	wint_t character,
	wctype_t mask);

/* ---------- public code */

wint_t __cdecl towupper(
	wint_t character)
{
	if (character == (wint_t)_wide_end_of_file)
		return character;

	if (__lc_handle[_lc_ctype_index] == _c_locale_handle)
		return (character >= (wint_t)'a' && character <= (wint_t)'z')
			? character - _case_difference
			: character;

	if (character < _wide_character_count)
		iswctype(character, _lower_case);

	return character;
}
