/*
TOUPPER.C

symbols in this file:
	__toupper (.text, 0x8)
	_toupper (.text, 0x31)
*/

/* ---------- constants */

enum
{
	_lower_case = 2,
	_case_difference = 'A' - 'a'
};

/* ---------- external data */

extern int __mb_cur_max;
extern unsigned short *_pctype;

/* ---------- external code */

int __cdecl _isctype(
	int character,
	int character_type);

/* ---------- public code */

int __cdecl _toupper(
	int character)
{
	return character + _case_difference;
}

int __cdecl toupper(
	int character)
{
	int is_lower_case;

	if (__mb_cur_max > 1)
		is_lower_case = _isctype(character, _lower_case);
	else
		is_lower_case = _pctype[character] & _lower_case;

	return is_lower_case ? character + _case_difference : character;
}
