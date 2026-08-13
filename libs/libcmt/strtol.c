/*
STRTOL.C

symbols in this file:
	_strtoxl (.text, 0x1a9)
	_strtol (.text, 0x17)
	_strtoul (.text, 0x17)
*/

/* ---------- constants */

enum
{
	_strtox_unsigned = 1,
	_strtox_negative = 2,
	_strtox_overflow = 4,
	_strtox_read_digit = 8,
	_character_digit = 4,
	_character_space = 8,
	_character_alpha = 0x103,
	_error_range = 34
};

#define ASCII_IS_DIGIT(character) (_pctype[(unsigned char)(character)] & _character_digit)
#define ASCII_IS_ALPHA(character) (_pctype[(unsigned char)(character)] & _character_alpha)
#define ASCII_TO_UPPER(character) \
	((character) >= 'a' && (character) <= 'z' ? (character) - ('a' - 'A') : (character))
#define MAXIMUM_UNSIGNED_LONG 0xFFFFFFFFUL
#define MAXIMUM_SIGNED_LONG 0x7FFFFFFFUL
#define MINIMUM_SIGNED_LONG_MAGNITUDE 0x80000000UL

/* ---------- external data */

extern int __mb_cur_max;
extern unsigned short *_pctype;

/* ---------- external code */

int * __cdecl _errno(
	void);

int __cdecl _isctype(
	int character,
	int character_type);

/* ---------- private code */

static unsigned long __cdecl strtoxl(
	char const *nptr,
	char const **endptr,
	int base,
	int flags)
{
	char const *position;
	char character;
	unsigned long number;
	unsigned int digit;
	unsigned long maximum_value;

	position = nptr;
	number = 0;

	character = *position++;
	while (__mb_cur_max > 1 ?
		_isctype((unsigned char)character, _character_space) :
		(_pctype[(unsigned char)character] & _character_space))
	{
		character = *position++;
	}

	if (character == '-')
	{
		flags |= _strtox_negative;
		character = *position++;
	}
	else if (character == '+')
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
		if (character != '0')
			base = 10;
		else if (*position == 'x' || *position == 'X')
			base = 16;
		else
			base = 8;
	}

	if (base == 16)
	{
		if (character == '0' && (*position == 'x' || *position == 'X'))
		{
			++position;
			character = *position++;
		}
	}

	maximum_value = MAXIMUM_UNSIGNED_LONG / base;

	for (;;)
	{
		if (ASCII_IS_DIGIT(character))
			digit = character - '0';
		else if (ASCII_IS_ALPHA(character))
			digit = ASCII_TO_UPPER(character) - 'A' + 10;
		else
			break;

		if (digit >= (unsigned int)base)
			break;

		flags |= _strtox_read_digit;

		if (number < maximum_value ||
			(number == maximum_value && digit <= MAXIMUM_UNSIGNED_LONG % base))
		{
			number = number * base + digit;
		}
		else
		{
			flags |= _strtox_overflow;
		}

		character = *position++;
	}

	--position;

	if (!(flags & _strtox_read_digit))
	{
		if (endptr)
			position = nptr;

		number = 0L;
	}
	else if ((flags & _strtox_overflow) ||
		(!(flags & _strtox_unsigned) &&
			(((flags & _strtox_negative) && number > MINIMUM_SIGNED_LONG_MAGNITUDE) ||
				(!(flags & _strtox_negative) && number > MAXIMUM_SIGNED_LONG))))
	{
		*_errno() = _error_range;

		if (flags & _strtox_unsigned)
			number = MAXIMUM_UNSIGNED_LONG;
		else if (flags & _strtox_negative)
			number = MINIMUM_SIGNED_LONG_MAGNITUDE;
		else
			number = MAXIMUM_SIGNED_LONG;
	}

	if (endptr != 0)
		*endptr = position;

	if (flags & _strtox_negative)
		number = (unsigned long)(-(long)number);

	return number;
}

/* ---------- public code */

long __cdecl strtol(
	char const *nptr,
	char **endptr,
	int base)
{
	return (long)strtoxl(nptr, endptr, base, 0);
}

unsigned long __cdecl strtoul(
	char const *nptr,
	char **endptr,
	int base)
{
	return strtoxl(nptr, endptr, base, _strtox_unsigned);
}
