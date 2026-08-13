/*
WCSSPN.C

symbols in this file:
	_wcsspn (.text, 0x46)
*/

/* ---------- types */

typedef unsigned short wchar_t;

/* ---------- public code */

unsigned long __cdecl wcsspn(
	wchar_t const *string,
	wchar_t const *control)
{
	wchar_t const *str = string;
	wchar_t const *ctl;

	while (*str)
	{
		for (ctl = control; *ctl != *str; ctl++)
		{
			if (*ctl == 0)
				return str - string;
		}
		str++;
	}

	return str - string;
}
