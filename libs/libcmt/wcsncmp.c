/*
WCSNCMP.C

symbols in this file:
	_wcsncmp (.text, 0x35)
*/

/* ---------- types */

typedef unsigned short wchar_t;

/* ---------- public code */

int __cdecl wcsncmp(
	wchar_t const *left,
	wchar_t const *right,
	unsigned long count)
{
	if (!count)
		return 0;

	while (--count && *left && *left == *right)
	{
		left++;
		right++;
	}

	return *left - *right;
}
