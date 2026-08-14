/*
MANTOLD.C

symbols in this file:
	__addl (.text, 0x21)
	__add_12 (.text, 0x5E)
	__shl_12 (.text, 0x2E)
	__shr_12 (.text, 0x2D)
	__mtold12 (.text, 0xCE)
*/

#include <stddef.h>

struct extended_float
{
	unsigned long low_word;
	unsigned long middle_word;
	union
	{
		unsigned long word;
		struct
		{
			unsigned short high_mantissa;
			unsigned short exponent;
		} parts;
	} high_word;
};

typedef char assert_extended_float_size[
	sizeof(struct extended_float) == 12 ? 1 : -1];
typedef char assert_extended_float_exponent_offset[
	offsetof(struct extended_float, high_word.parts.exponent) == 10 ? 1 : -1];

int __cdecl __addl(
	unsigned long x,
	unsigned long y,
	unsigned long *sum)
{
	unsigned long result;
	int carry = 0;

	result = x + y;
	if (result < x || result < y)
		carry++;
	*sum = result;
	return carry;
}

void __cdecl __add_12(
	struct extended_float *x,
	struct extended_float *y)
{
	int low_carry;
	int middle_increment_carry;
	int middle_carry;

	low_carry = __addl(x->low_word, y->low_word, &x->low_word);
	if (low_carry)
	{
		middle_increment_carry = __addl(
			x->middle_word,
			1,
			&x->middle_word);
		if (middle_increment_carry)
			x->high_word.word++;
	}
	middle_carry = __addl(
		x->middle_word,
		y->middle_word,
		&x->middle_word);
	if (middle_carry)
		x->high_word.word++;

	/* The conversion domain guarantees that the final carry is unused. */
	(void)__addl(
		x->high_word.word,
		y->high_word.word,
		&x->high_word.word);
}

void __cdecl __shl_12(struct extended_float *value)
{
	unsigned long low_carry;
	unsigned long middle_carry;

	low_carry = value->low_word & 0x80000000 ? 1 : 0;
	middle_carry = value->middle_word & 0x80000000 ? 1 : 0;
	value->low_word <<= 1;
	value->middle_word = value->middle_word << 1 | low_carry;
	value->high_word.word = value->high_word.word << 1 | middle_carry;
}

void __cdecl __shr_12(struct extended_float *value)
{
	unsigned long high_carry;
	unsigned long middle_carry;

	high_carry = value->high_word.word & 1 ? 0x80000000 : 0;
	middle_carry = value->middle_word & 1 ? 0x80000000 : 0;
	value->high_word.word >>= 1;
	value->middle_word = value->middle_word >> 1 | high_carry;
	value->low_word = value->low_word >> 1 | middle_carry;
}

void __cdecl __mtold12(
	char *mantissa,
	unsigned int mantissa_length,
	struct extended_float *value)
{
	struct extended_float digit;
	unsigned short exponent = 0x3FFE + 80;

	value->low_word = 0;
	value->middle_word = 0;
	value->high_word.word = 0;
	for (;
		mantissa_length > 0;
		mantissa_length--, mantissa++)
	{
		digit = *value;
		__shl_12(value);
		__shl_12(value);
		__add_12(value, &digit);
		__shl_12(value);
		digit.low_word = (unsigned long)*mantissa;
		digit.middle_word = 0;
		digit.high_word.word = 0;
		__add_12(value, &digit);
	}

	while (value->high_word.word == 0)
	{
		value->high_word.word = value->middle_word >> 16;
		value->middle_word =
			value->middle_word << 16 | value->low_word >> 16;
		value->low_word <<= 16;
		exponent -= 16;
	}
	while ((value->high_word.word & 0x8000) == 0)
	{
		__shl_12(value);
		exponent--;
	}
	value->high_word.parts.exponent = exponent;
}
