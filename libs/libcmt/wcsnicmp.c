/*
WCSNICMP.C

symbols in this file:
	__wcsnicmp (.text, 0x9e)
*/

/* ---------- types */

typedef unsigned short wchar_t;

enum
{
	_lc_ctype_index = 2,
	_c_locale_handle = 0
};

/* ---------- externals */

extern unsigned long __lc_handle[];

wchar_t __cdecl towlower(
	wchar_t character);

/* ---------- public code */

int __cdecl _wcsnicmp(
	wchar_t const *left,
	wchar_t const *right,
	unsigned long count)
{
	wchar_t left_character;
	wchar_t right_character;
	int result = 0;

	if (count)
	{
		if (__lc_handle[_lc_ctype_index] == _c_locale_handle)
		{
			do
			{
				left_character = *left >= L'A' && *left <= L'Z'
					? *left + (L'a' - L'A')
					: *left;
				right_character = *right >= L'A' && *right <= L'Z'
					? *right + (L'a' - L'A')
					: *right;
				left++;
				right++;
			}
			while (--count && left_character && left_character == right_character);
		}
		else
		{
			do
			{
				left_character = towlower(*left++);
				right_character = towlower(*right++);
			}
			while (--count && left_character && left_character == right_character);
		}

		result = left_character - right_character;
	}

	return result;
}
