/*
WCSCMP.C

symbols in this file:
	_wcscmp (.text, 0x32)
*/

/* ---------- types */

typedef unsigned short wchar_t;

/* ---------- public code */

int __cdecl wcscmp(
	wchar_t const *left,
	wchar_t const *right)
{
	int result;

	while (!(result = *left - *right) && *right)
	{
		left++;
		right++;
	}

	if (result < 0)
		result = -1;
	else if (result > 0)
		result = 1;

	return result;
}
