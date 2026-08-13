/*
TOLOWER.C

symbols in this file:
	__tolower (.text, 0x8)
	_tolower (.text, 0x31)
*/

/* ---------- constants */

enum
{
	_upper_case = 1,
	_case_difference = 'a' - 'A'
};

/* ---------- external data */

extern int __mb_cur_max;
extern unsigned short *_pctype;

/* ---------- external code */

int __cdecl _isctype(
	int character,
	int character_type);

/* ---------- public code */

int __cdecl _tolower(
	int character)
{
	return character + _case_difference;
}

int __cdecl tolower(
	int character)
{
	int is_upper_case;

	if (__mb_cur_max > 1)
		is_upper_case = _isctype(character, _upper_case);
	else
		is_upper_case = _pctype[character] & _upper_case;

	return is_upper_case ? character + _case_difference : character;
}
