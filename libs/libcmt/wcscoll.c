/*
WCSCOLL.C

symbols in this file:
	_wcscoll (.text, 0x5)
*/

/* ---------- types */

typedef unsigned short wchar_t;

/* ---------- prototypes */

int __cdecl wcscmp(
	wchar_t const *left,
	wchar_t const *right);

/* ---------- public code */

int __cdecl wcscoll(
	wchar_t const *left,
	wchar_t const *right)
{
	return wcscmp(left, right);
}
