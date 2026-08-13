/*
STRNICMP.C

symbols in this file:
	__strnicmp (.text, 0x55)
*/

enum
{
	_lc_ctype_index = 2,
	_c_locale_handle = 0
};

/* ---------- externals */

extern unsigned long __lc_handle[];

int __cdecl __ascii_strnicmp(
	char const *left,
	char const *right,
	unsigned long count);

int __cdecl tolower(
	int character);

/* ---------- public code */

int __cdecl _strnicmp(
	char const *left,
	char const *right,
	unsigned long count)
{
	int left_character;
	int right_character;

	if (count)
	{
		if (__lc_handle[_lc_ctype_index] == _c_locale_handle)
			return __ascii_strnicmp(left, right, count);

		do
		{
			left_character = tolower((unsigned char)*left++);
			right_character = tolower((unsigned char)*right++);
		}
		while (--count && left_character && left_character == right_character);

		return left_character - right_character;
	}

	return 0;
}
