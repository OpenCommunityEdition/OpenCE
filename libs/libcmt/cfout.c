/*
CFOUT.C

symbols in this file:
	___dtold (.text, 0xBA)
	__fltout2 (.text, 0x66)
*/

#include <string.h>

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef signed short s_short;
typedef unsigned int u_long;

typedef struct double_bit_container
{
	double value;
} DOUBLE;

typedef struct long_double_bit_container
{
	u_char value[10];
} _LDOUBLE;

typedef struct floating_output_state
{
	short exponent;
	char sign;
	char mantissa_length;
	char mantissa[22];
} FOS;

typedef struct floating_output_result
{
	int sign;
	int decimal_point;
	int flag;
	char *mantissa;
} *STRFLT;

enum
{
	_maximum_output_digits = 17
};

enum
{
	_long_double_bias = 0x3FFF,
	_long_double_maximum_exponent = 0x7FFF,
	_double_bias = 0x3FF,
	_double_maximum_exponent = 0x7FF
};

#define MOST_SIGNIFICANT_ULONG ((u_long)0x80000000)
#define LONG_DOUBLE_EXPONENT(p) ((u_short *)((p)->value + 8))
#define LONG_DOUBLE_MANTISSA_HIGH(p) ((u_long *)((p)->value + 4))
#define LONG_DOUBLE_MANTISSA_LOW(p) ((u_long *)(p)->value)
#define DOUBLE_EXPONENT(p) ((u_short *)(p) + 3)
#define DOUBLE_MANTISSA_HIGH(p) ((u_long *)(p) + 1)
#define DOUBLE_MANTISSA_LOW(p) ((u_long *)(p))

int __cdecl $I10_OUTPUT(
	_LDOUBLE value,
	int digit_count,
	unsigned output_flags,
	FOS *output);

void __cdecl __dtold(
	_LDOUBLE *long_double,
	double *value)
{
	u_short exponent;
	u_short sign;
	u_long mantissa_high;
	u_long mantissa_low;
	u_long most_significant_bit = MOST_SIGNIFICANT_ULONG;
	u_short long_double_exponent = 0;

	exponent = (*DOUBLE_EXPONENT(value) & (u_short)0x7FF0) >> 4;
	sign = *DOUBLE_EXPONENT(value) & (u_short)0x8000;
	mantissa_high = *DOUBLE_MANTISSA_HIGH(value) & 0xFFFFF;
	mantissa_low = *DOUBLE_MANTISSA_LOW(value);

	switch (exponent)
	{
	case _double_maximum_exponent:
		long_double_exponent = _long_double_maximum_exponent;
		break;

	case 0:
		if (mantissa_high == 0 && mantissa_low == 0)
		{
			*LONG_DOUBLE_MANTISSA_HIGH(long_double) = 0;
			*LONG_DOUBLE_MANTISSA_LOW(long_double) = 0;
			*LONG_DOUBLE_EXPONENT(long_double) = 0;
			return;
		}

		long_double_exponent =
			(u_short)((s_short)exponent - _double_bias + _long_double_bias + 1);
		most_significant_bit = 0;
		break;

	default:
		exponent -= _double_bias;
		long_double_exponent =
			(u_short)((s_short)exponent + _long_double_bias);
		break;
	}

	*LONG_DOUBLE_MANTISSA_HIGH(long_double) =
		most_significant_bit | mantissa_high << 11 | mantissa_low >> 21;
	*LONG_DOUBLE_MANTISSA_LOW(long_double) = mantissa_low << 11;

	while ((*LONG_DOUBLE_MANTISSA_HIGH(long_double) &
		MOST_SIGNIFICANT_ULONG) == 0)
	{
		*LONG_DOUBLE_MANTISSA_HIGH(long_double) =
			*LONG_DOUBLE_MANTISSA_HIGH(long_double) << 1 |
			(MOST_SIGNIFICANT_ULONG &
				*LONG_DOUBLE_MANTISSA_LOW(long_double) ? 1 : 0);
		(*LONG_DOUBLE_MANTISSA_LOW(long_double)) <<= 1;
		long_double_exponent--;
	}

	*LONG_DOUBLE_EXPONENT(long_double) = sign | long_double_exponent;
}

STRFLT __cdecl _fltout2(
	DOUBLE value,
	STRFLT output,
	char *result)
{
	_LDOUBLE long_double;
	FOS output_state;
	char *source;
	char *destination;

	__dtold(&long_double, (double *)&value);
	output->flag = $I10_OUTPUT(
		long_double,
		_maximum_output_digits,
		0,
		&output_state);
	output->sign = output_state.sign;
	output->decimal_point = output_state.exponent;
	source = output_state.mantissa;
	destination = result;
	while ((*destination++ = *source++) != '\0')
	{
	}
	output->mantissa = result;

	return output;
}
