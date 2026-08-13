/*
STRICMP.C

symbols in this file:
	__stricmp (.text, 0x43)
*/

enum
{
	_lc_ctype_index = 2,
	_c_locale_handle = 0
};

/* ---------- externals */

extern unsigned long __lc_handle[];

int __cdecl __ascii_stricmp(
	char const *left,
	char const *right);

int __cdecl tolower(
	int character);

/* ---------- public code */

int __cdecl _stricmp(
	char const *left,
	char const *right)
{
	int left_character;
	int right_character;

	if (__lc_handle[_lc_ctype_index] == _c_locale_handle)
		return __ascii_stricmp(left, right);

	do
	{
		left_character = tolower((unsigned char)*left++);
		right_character = tolower((unsigned char)*right++);
	}
	while (left_character && left_character == right_character);

	return left_character - right_character;
}
