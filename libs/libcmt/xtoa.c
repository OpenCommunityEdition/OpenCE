/*
XTOA.C

symbols in this file:
	_xtoa (.text, 0x3e)
	__itoa (.text, 0x2a)
	__ltoa (.text, 0x27)
	__ultoa (.text, 0x1a)
	_x64toa@20 (.text, 0x6d)
	__i64toa (.text, 0x31)
	__ui64toa (.text, 0x1b)
*/

/* ---------- private code */

static void __cdecl xtoa(
	unsigned long value,
	char *buffer,
	unsigned int radix,
	int is_negative)
{
	char *position;
	char *first_digit;
	char temporary;
	unsigned int digit;

	position = buffer;

	if (is_negative)
	{
		*position++ = '-';
		/* BUG (preserved for exact matching): negating LONG_MIN overflows signed C. */
		value = (unsigned long)(-(long)value);
	}

	first_digit = position;

	do
	{
		digit = (unsigned int)(value % radix);
		value /= radix;

		if (digit > 9)
			*position++ = (char)(digit - 10 + 'a');
		else
			*position++ = (char)(digit + '0');
	}
	while (value > 0);

	*position-- = '\0';

	do
	{
		temporary = *position;
		*position = *first_digit;
		*first_digit = temporary;
		--position;
		++first_digit;
	}
	while (first_digit < position);
}

static void __stdcall x64toa(
	unsigned __int64 value,
	char *buffer,
	unsigned int radix,
	int is_negative)
{
	char *position;
	char *first_digit;
	char temporary;
	unsigned int digit;

	position = buffer;

	if (is_negative)
	{
		*position++ = '-';
		/* BUG (preserved for exact matching): negating INT64_MIN overflows signed C. */
		value = (unsigned __int64)(-(__int64)value);
	}

	first_digit = position;

	do
	{
		digit = (unsigned int)(value % radix);
		value /= radix;

		if (digit > 9)
			*position++ = (char)(digit - 10 + 'a');
		else
			*position++ = (char)(digit + '0');
	}
	while (value > 0);

	*position-- = '\0';

	do
	{
		temporary = *position;
		*position = *first_digit;
		*first_digit = temporary;
		--position;
		++first_digit;
	}
	while (first_digit < position);
}

/* ---------- public code */

char * __cdecl _itoa(
	int value,
	char *buffer,
	int radix)
{
	if (radix == 10 && value < 0)
		xtoa((unsigned long)value, buffer, radix, 1);
	else
		xtoa((unsigned long)(unsigned int)value, buffer, radix, 0);

	return buffer;
}

char * __cdecl _ltoa(
	long value,
	char *buffer,
	int radix)
{
	xtoa((unsigned long)value, buffer, radix, radix == 10 && value < 0);

	return buffer;
}

char * __cdecl _ultoa(
	unsigned long value,
	char *buffer,
	int radix)
{
	xtoa(value, buffer, radix, 0);

	return buffer;
}

char * __cdecl _i64toa(
	__int64 value,
	char *buffer,
	int radix)
{
	x64toa((unsigned __int64)value, buffer, radix, radix == 10 && value < 0);

	return buffer;
}

char * __cdecl _ui64toa(
	unsigned __int64 value,
	char *buffer,
	int radix)
{
	x64toa(value, buffer, radix, 0);

	return buffer;
}
