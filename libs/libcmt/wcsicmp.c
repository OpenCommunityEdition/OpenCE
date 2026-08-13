/*
WCSICMP.C

symbols in this file:
	__wcsicmp (.text, 0x88)
*/

/* ---------- constants */

enum
{
	C_LOCALE_HANDLE = 0,
	LC_CTYPE_INDEX = 2
};

/* ---------- types */

typedef unsigned short wchar_t;
typedef unsigned short wint_t;

/* ---------- globals */

extern unsigned long __lc_handle[];

/* ---------- prototypes */

wint_t __cdecl towlower(
	wint_t character);

int __cdecl _wcsicmp(
	wchar_t const *string1,
	wchar_t const *string2);

/* ---------- public code */

int __cdecl _wcsicmp(
	wchar_t const *string1,
	wchar_t const *string2)
{
	wchar_t character1, character2;

	if (__lc_handle[LC_CTYPE_INDEX] == C_LOCALE_HANDLE)
	{
		do
		{
			character1 = (*string1 >= (wchar_t)'A' && *string1 <= (wchar_t)'Z')
				? *string1 + (wchar_t)('a' - 'A')
				: *string1;
			character2 = (*string2 >= (wchar_t)'A' && *string2 <= (wchar_t)'Z')
				? *string2 + (wchar_t)('a' - 'A')
				: *string2;

			string1++;
			string2++;
		}
		while (character1 && character1 == character2);
	}
	else
	{
		do
		{
			character1 = towlower(*string1++);
			character2 = towlower(*string2++);
		}
		while (character1 && character1 == character2);
	}

	return (int)character1 - (int)character2;
}
