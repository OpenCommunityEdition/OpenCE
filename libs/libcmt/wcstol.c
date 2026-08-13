/*
WCSTOL.C

symbols in this file:
	_wcstoxl (.text, 0x1c5)
	_wcstol (.text, 0x17)
	_wcstoul (.text, 0x17)
*/

#include <stddef.h>

/* ---------- constants */

enum
{
	_wcstox_unsigned = 1,
	_wcstox_negative = 2,
	_wcstox_overflow = 4,
	_wcstox_read_digit = 8,
	_wide_character_space = 8,
	_error_range = 34
};

#define ASCII_IS_WIDE_ALPHA(character) \
	(((character) >= L'A' && (character) <= L'Z') || \
		((character) >= L'a' && (character) <= L'z'))
#define ASCII_WIDE_TO_UPPER(character) \
	((character) >= L'a' && (character) <= L'z' ? \
		(character) - (L'a' - L'A') : (character))
#define MAXIMUM_UNSIGNED_LONG 0xFFFFFFFFUL
#define MAXIMUM_SIGNED_LONG 0x7FFFFFFFUL
#define MINIMUM_SIGNED_LONG_MAGNITUDE 0x80000000UL

/* ---------- external code */

int * __cdecl _errno(
	void);

int __cdecl iswctype(
	wchar_t character,
	wchar_t character_type);

int __cdecl _wchartodigit(
	wchar_t character);

/* ---------- private code */

static unsigned long __cdecl wcstoxl(
	wchar_t const *nptr,
	wchar_t const **endptr,
	int base,
	int flags)
{
	wchar_t const *position;
	wchar_t character;
	unsigned long number;
	unsigned int digit;
	unsigned long maximum_value;

	position = nptr;
	number = 0;

	character = *position++;
	while (iswctype(character, _wide_character_space))
		character = *position++;

	if (character == L'-')
	{
		flags |= _wcstox_negative;
		character = *position++;
	}
	else if (character == L'+')
	{
		character = *position++;
	}

	if (base < 0 || base == 1 || base > 36)
	{
		if (endptr)
			*endptr = nptr;

		return 0L;
	}
	else if (base == 0)
	{
		if (_wchartodigit(character) != 0)
			base = 10;
		else if (*position == L'x' || *position == L'X')
			base = 16;
		else
			base = 8;
	}

	if (base == 16)
	{
		if (_wchartodigit(character) == 0 &&
			(*position == L'x' || *position == L'X'))
		{
			++position;
			character = *position++;
		}
	}

	maximum_value = MAXIMUM_UNSIGNED_LONG / base;

	for (;;)
	{
		if ((digit = _wchartodigit(character)) != -1)
		{
		}
		else if (ASCII_IS_WIDE_ALPHA(character))
		{
			digit = ASCII_WIDE_TO_UPPER(character) - L'A' + 10;
		}
		else
		{
			break;
		}

		if (digit >= (unsigned int)base)
			break;

		flags |= _wcstox_read_digit;

		if (number < maximum_value ||
			(number == maximum_value && digit <= MAXIMUM_UNSIGNED_LONG % base))
		{
			number = number * base + digit;
		}
		else
		{
			flags |= _wcstox_overflow;
		}

		character = *position++;
	}

	--position;

	if (!(flags & _wcstox_read_digit))
	{
		if (endptr)
			position = nptr;

		number = 0L;
	}
	else if ((flags & _wcstox_overflow) ||
		(!(flags & _wcstox_unsigned) &&
			(((flags & _wcstox_negative) && number > MINIMUM_SIGNED_LONG_MAGNITUDE) ||
				(!(flags & _wcstox_negative) && number > MAXIMUM_SIGNED_LONG))))
	{
		*_errno() = _error_range;

		if (flags & _wcstox_unsigned)
			number = MAXIMUM_UNSIGNED_LONG;
		else if (flags & _wcstox_negative)
			number = MINIMUM_SIGNED_LONG_MAGNITUDE;
		else
			number = MAXIMUM_SIGNED_LONG;
	}

	if (endptr != NULL)
		*endptr = position;

	if (flags & _wcstox_negative)
		number = (unsigned long)(-(long)number);

	return number;
}

/* ---------- public code */

long __cdecl wcstol(
	wchar_t const *nptr,
	wchar_t **endptr,
	int base)
{
	return (long)wcstoxl(nptr, endptr, base, 0);
}

unsigned long __cdecl wcstoul(
	wchar_t const *nptr,
	wchar_t **endptr,
	int base)
{
	return wcstoxl(nptr, endptr, base, _wcstox_unsigned);
}
