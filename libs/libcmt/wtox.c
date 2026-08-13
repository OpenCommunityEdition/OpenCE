/*
WTOX.C

symbols in this file:
	__wtol (.text, 0x5f)
	__wtoi (.text, 0x5)
	__wtoi64 (.text, 0x8e)
*/

#include <wchar.h>

/* ---------- external code */

int __cdecl _wchartodigit(
	wchar_t character);

/* ---------- public code */

long __cdecl _wtol(
	wchar_t const *nptr)
{
	int character;
	long total;
	int sign;

	while (iswspace((wint_t)*nptr))
		++nptr;

	character = (wint_t)*nptr++;
	sign = character;
	if (character == L'-' || character == L'+')
		character = (wint_t)*nptr++;

	total = 0;

	while ((character = _wchartodigit((wchar_t)character)) != -1)
	{
		total = 10 * total + character;
		character = (wint_t)*nptr++;
	}

	if (sign == L'-')
		return -total;
	else
		return total;
}

int __cdecl _wtoi(
	wchar_t const *nptr)
{
	return (int)_wtol(nptr);
}

__int64 __cdecl _wtoi64(
	wchar_t const *nptr)
{
	int character;
	__int64 total;
	int sign;

	while (iswspace((wint_t)*nptr))
		++nptr;

	character = (wint_t)*nptr++;
	sign = character;
	if (character == L'-' || character == L'+')
		character = (wint_t)*nptr++;

	total = 0;

	while ((character = _wchartodigit((wchar_t)character)) != -1)
	{
		total = 10 * total + character;
		character = (wint_t)*nptr++;
	}

	if (sign == L'-')
		return -total;
	else
		return total;
}
