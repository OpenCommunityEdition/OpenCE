/*
_CTYPE.C

symbols in this file:
	_isalpha (.text, 0x2E)
	_isupper (.text, 0x29)
	_islower (.text, 0x29)
	_isdigit (.text, 0x29)
	_isxdigit (.text, 0x2E)
	_isspace (.text, 0x29)
	_ispunct (.text, 0x29)
	_isalnum (.text, 0x2E)
	_isprint (.text, 0x2E)
	_isgraph (.text, 0x2E)
	_iscntrl (.text, 0x29)
	___isascii (.text, 0xD)
	___toascii (.text, 0x8)
	___iscsymf (.text, 0x3D)
	___iscsym (.text, 0x3D)
*/

/* ---------- constants */

enum
{
	character_type_upper = 0x0001,
	character_type_lower = 0x0002,
	character_type_digit = 0x0004,
	character_type_space = 0x0008,
	character_type_punctuation = 0x0010,
	character_type_control = 0x0020,
	character_type_blank = 0x0040,
	character_type_hexadecimal = 0x0080,
	character_type_alpha = 0x0100 | character_type_upper | character_type_lower
};

/* ---------- external data */

extern int __mb_cur_max;
extern unsigned short *_pctype;

/* ---------- external code */

int __cdecl _isctype(
	int character,
	int character_type);

/* ---------- public code */

int __cdecl isalpha(
	int character)
{
	return __mb_cur_max > 1
		? _isctype(character, character_type_alpha)
		: _pctype[character] & character_type_alpha;
}

int __cdecl isupper(
	int character)
{
	return __mb_cur_max > 1
		? _isctype(character, character_type_upper)
		: _pctype[character] & character_type_upper;
}

int __cdecl islower(
	int character)
{
	return __mb_cur_max > 1
		? _isctype(character, character_type_lower)
		: _pctype[character] & character_type_lower;
}

int __cdecl isdigit(
	int character)
{
	return __mb_cur_max > 1
		? _isctype(character, character_type_digit)
		: _pctype[character] & character_type_digit;
}

int __cdecl isxdigit(
	int character)
{
	return __mb_cur_max > 1
		? _isctype(character, character_type_hexadecimal)
		: _pctype[character] & character_type_hexadecimal;
}

int __cdecl isspace(
	int character)
{
	return __mb_cur_max > 1
		? _isctype(character, character_type_space)
		: _pctype[character] & character_type_space;
}

int __cdecl ispunct(
	int character)
{
	return __mb_cur_max > 1
		? _isctype(character, character_type_punctuation)
		: _pctype[character] & character_type_punctuation;
}

int __cdecl isalnum(
	int character)
{
	return __mb_cur_max > 1
		? _isctype(character, character_type_alpha | character_type_digit)
		: _pctype[character] & (character_type_alpha | character_type_digit);
}

int __cdecl isprint(
	int character)
{
	return __mb_cur_max > 1
		? _isctype(character, character_type_blank | character_type_punctuation | character_type_alpha | character_type_digit)
		: _pctype[character] & (character_type_blank | character_type_punctuation | character_type_alpha | character_type_digit);
}

int __cdecl isgraph(
	int character)
{
	return __mb_cur_max > 1
		? _isctype(character, character_type_punctuation | character_type_alpha | character_type_digit)
		: _pctype[character] & (character_type_punctuation | character_type_alpha | character_type_digit);
}

int __cdecl iscntrl(
	int character)
{
	return __mb_cur_max > 1
		? _isctype(character, character_type_control)
		: _pctype[character] & character_type_control;
}

int __cdecl __isascii(
	int character)
{
	return (unsigned int)character < 0x80;
}

int __cdecl __toascii(
	int character)
{
	return character & 0x7F;
}

int __cdecl __iscsymf(
	int character)
{
	return (__mb_cur_max > 1
		? _isctype(character, character_type_alpha)
		: _pctype[character] & character_type_alpha)
		|| character == '_';
}

int __cdecl __iscsym(
	int character)
{
	return (__mb_cur_max > 1
		? _isctype(character, character_type_alpha | character_type_digit)
		: _pctype[character] & (character_type_alpha | character_type_digit))
		|| character == '_';
}
