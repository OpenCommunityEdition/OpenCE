/*
WCSSTR.C

symbols in this file:
	_wcsstr (.text, 0x5E)
*/

/* ---------- types */

typedef unsigned short wchar_t;

/* ---------- public code */

wchar_t * __cdecl wcsstr(
	wchar_t const *string,
	wchar_t const *substring)
{
	wchar_t const *candidate = string;

	if (!*substring)
		return (wchar_t *)string;

	while (*candidate)
	{
		wchar_t const *left = candidate;
		wchar_t const *right = substring;

		while (*left && *right && !(*left - *right))
		{
			left++;
			right++;
		}

		if (!*right)
			return (wchar_t *)candidate;

		candidate++;
	}

	return 0;
}
