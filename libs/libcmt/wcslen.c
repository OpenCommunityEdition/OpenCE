/*
WCSLEN.C

symbols in this file:
	_wcslen (.text, 0x16)
*/

/* ---------- types */

typedef unsigned short wchar_t;

/* ---------- public code */

unsigned long __cdecl wcslen(
	wchar_t const *string)
{
	wchar_t const *end = string;

	while (*end++)
		;

	return end - string - 1;
}
