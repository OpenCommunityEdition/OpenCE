/*
TENPOW.C

symbols in this file:
	___ld12mul
	___multtenpow12
*/

/*
 * The CRT conversion routines use a ten-byte extended-precision value
 * followed by two bytes of additional precision.
 */
typedef struct long_double_12
{
	unsigned char bytes[12];
} long_double_12;

/* ---------- private constants */

#define LONG_DOUBLE_BIAS_MINUS_ONE 0x3FFE
#define LONG_DOUBLE_MAXIMUM_EXPONENT 0x7FFF

#define MAXIMUM_UNSIGNED_SHORT ((unsigned short)0xFFFF)
#define MOST_SIGNIFICANT_SHORT_BIT ((unsigned short)0x8000)
#define MAXIMUM_UNSIGNED_LONG ((unsigned long)0xFFFFFFFF)
#define MOST_SIGNIFICANT_LONG_BIT ((unsigned long)0x80000000)

/* ---------- private macros */

#define LONG_DOUBLE_BYTES(value) ((unsigned char *)(value)->bytes)

#define LONG_DOUBLE_EXPONENT(value) \
	((unsigned short *)(LONG_DOUBLE_BYTES(value) + 10))
#define LONG_DOUBLE_MANTISSA_HIGH(value) \
	((unsigned long *)(LONG_DOUBLE_BYTES(value) + 6))
#define LONG_DOUBLE_MANTISSA_LOW(value) \
	((unsigned long *)(LONG_DOUBLE_BYTES(value) + 2))
#define LONG_DOUBLE_EXTRA_PRECISION(value) \
	((unsigned short *)LONG_DOUBLE_BYTES(value))

#define LONG_DOUBLE_LOW(value) \
	((unsigned long *)LONG_DOUBLE_BYTES(value))
#define LONG_DOUBLE_MIDDLE(value) \
	((unsigned long *)(LONG_DOUBLE_BYTES(value) + 4))
#define LONG_DOUBLE_HIGH(value) \
	((unsigned long *)(LONG_DOUBLE_BYTES(value) + 8))

#define LONG_DOUBLE_SHORT_AT(value, offset) \
	((unsigned short *)(LONG_DOUBLE_BYTES(value) + (offset)))
#define LONG_DOUBLE_LONG_AT(value, offset) \
	((unsigned long *)(LONG_DOUBLE_BYTES(value) + (offset)))

#define LONG_DOUBLE_IS_ZERO(value) \
	(((*LONG_DOUBLE_HIGH(value) & 0x7FFFFFFF) == 0) && \
		*LONG_DOUBLE_MIDDLE(value) == 0 && \
		*LONG_DOUBLE_LOW(value) == 0)

#define LONG_DOUBLE_PUT_ZERO(value) \
	*LONG_DOUBLE_HIGH(value) = 0; \
	*LONG_DOUBLE_MIDDLE(value) = 0; \
	*LONG_DOUBLE_LOW(value) = 0

#define LONG_DOUBLE_PUT_INFINITY(value, sign) \
	*LONG_DOUBLE_HIGH(value) = (sign) ? 0xFFFF8000 : 0x7FFF8000; \
	*LONG_DOUBLE_MIDDLE(value) = 0; \
	*LONG_DOUBLE_LOW(value) = 0

/* ---------- external data */

extern long_double_12 _pow10pos[];
extern long_double_12 _pow10neg[];

/* ---------- external code */

extern int __addl(
	unsigned long first,
	unsigned long second,
	unsigned long *sum);
extern void __shl_12(long_double_12 *value);
extern void __shr_12(long_double_12 *value);

/* ---------- public code */

void __ld12mul(
	long_double_12 *first,
	long_double_12 *second)
{
	unsigned short sign = 0;
	long_double_12 temporary_mantissa;
	int i;
	unsigned short first_exponent;
	unsigned short second_exponent;
	unsigned short exponent_sum;
	int result_offset;
	int first_offset;
	int second_offset;
	int sticky = 0;

	*LONG_DOUBLE_LOW(&temporary_mantissa) = 0;
	*LONG_DOUBLE_MIDDLE(&temporary_mantissa) = 0;
	*LONG_DOUBLE_HIGH(&temporary_mantissa) = 0;

	first_exponent = *LONG_DOUBLE_EXPONENT(first);
	second_exponent = *LONG_DOUBLE_EXPONENT(second);

	sign = (first_exponent ^ second_exponent) & MOST_SIGNIFICANT_SHORT_BIT;
	first_exponent &= 0x7FFF;
	second_exponent &= 0x7FFF;
	exponent_sum = first_exponent + second_exponent;

	if (first_exponent >= LONG_DOUBLE_MAXIMUM_EXPONENT ||
		second_exponent >= LONG_DOUBLE_MAXIMUM_EXPONENT ||
		exponent_sum > LONG_DOUBLE_MAXIMUM_EXPONENT + LONG_DOUBLE_BIAS_MINUS_ONE)
	{
		LONG_DOUBLE_PUT_INFINITY(first, sign);
		return;
	}

	if (exponent_sum <= LONG_DOUBLE_BIAS_MINUS_ONE - 63)
	{
		LONG_DOUBLE_PUT_ZERO(first);
		return;
	}

	if (first_exponent == 0)
	{
		exponent_sum++;
		if (LONG_DOUBLE_IS_ZERO(first))
		{
			*LONG_DOUBLE_EXPONENT(first) = 0;
			return;
		}
	}

	if (second_exponent == 0)
	{
		exponent_sum++;
		if (LONG_DOUBLE_IS_ZERO(second))
		{
			LONG_DOUBLE_PUT_ZERO(first);
			return;
		}
	}

	result_offset = 0;
	for (i = 0; i < 5; i++)
	{
		int j;
		first_offset = i << 1;
		second_offset = 8;

		for (j = 5 - i; j > 0; j--)
		{
			unsigned long product;
			int carry;
			unsigned short *first_part;
			unsigned short *second_part;
			unsigned long *result_part;

			first_part = LONG_DOUBLE_SHORT_AT(first, first_offset);
			second_part = LONG_DOUBLE_SHORT_AT(second, second_offset);
			result_part = LONG_DOUBLE_LONG_AT(&temporary_mantissa, result_offset);
			product = (unsigned long)*first_part * (unsigned long)*second_part;
			carry = __addl(*result_part, product, result_part);
			if (carry)
				(*LONG_DOUBLE_SHORT_AT(&temporary_mantissa, result_offset + 4))++;

			first_offset += 2;
			second_offset -= 2;
		}

		result_offset += 2;
	}

	exponent_sum -= LONG_DOUBLE_BIAS_MINUS_ONE;

	while ((short)exponent_sum > 0 &&
		(*LONG_DOUBLE_HIGH(&temporary_mantissa) & MOST_SIGNIFICANT_LONG_BIT) == 0)
	{
		__shl_12(&temporary_mantissa);
		exponent_sum--;
	}

	if ((short)exponent_sum <= 0)
	{
		exponent_sum--;
		while ((short)exponent_sum < 0)
		{
			if (*LONG_DOUBLE_EXTRA_PRECISION(&temporary_mantissa) & 1)
				sticky++;
			__shr_12(&temporary_mantissa);
			exponent_sum++;
		}

		if (sticky)
			*LONG_DOUBLE_EXTRA_PRECISION(&temporary_mantissa) |= 1;
	}

	if (*LONG_DOUBLE_EXTRA_PRECISION(&temporary_mantissa) > 0x8000 ||
		(*LONG_DOUBLE_LOW(&temporary_mantissa) & 0x1FFFF) == 0x18000)
	{
		if (*LONG_DOUBLE_MANTISSA_LOW(&temporary_mantissa) == MAXIMUM_UNSIGNED_LONG)
		{
			*LONG_DOUBLE_MANTISSA_LOW(&temporary_mantissa) = 0;
			if (*LONG_DOUBLE_MANTISSA_HIGH(&temporary_mantissa) == MAXIMUM_UNSIGNED_LONG)
			{
				*LONG_DOUBLE_MANTISSA_HIGH(&temporary_mantissa) = 0;
				if (*LONG_DOUBLE_EXPONENT(&temporary_mantissa) == MAXIMUM_UNSIGNED_SHORT)
				{
					*LONG_DOUBLE_EXPONENT(&temporary_mantissa) = MOST_SIGNIFICANT_SHORT_BIT;
					exponent_sum++;
				}
				else
				{
					(*LONG_DOUBLE_EXPONENT(&temporary_mantissa))++;
				}
			}
			else
			{
				(*LONG_DOUBLE_MANTISSA_HIGH(&temporary_mantissa))++;
			}
		}
		else
		{
			(*LONG_DOUBLE_MANTISSA_LOW(&temporary_mantissa))++;
		}
	}

	if (exponent_sum >= 0x7FFF)
	{
		LONG_DOUBLE_PUT_INFINITY(first, sign);
		return;
	}

	*LONG_DOUBLE_EXTRA_PRECISION(first) =
		*LONG_DOUBLE_SHORT_AT(&temporary_mantissa, 2);
	*LONG_DOUBLE_MANTISSA_LOW(first) =
		*LONG_DOUBLE_MIDDLE(&temporary_mantissa);
	*LONG_DOUBLE_MANTISSA_HIGH(first) =
		*LONG_DOUBLE_HIGH(&temporary_mantissa);
	*LONG_DOUBLE_EXPONENT(first) = exponent_sum | sign;
}

void __multtenpow12(
	long_double_12 *value,
	int power,
	unsigned exact_multiplication)
{
	long_double_12 *power_of_ten = _pow10pos - 8;

	if (power == 0)
		return;

	if (power < 0)
	{
		power = -power;
		power_of_ten = _pow10neg - 8;
	}

	if (!exact_multiplication)
		*LONG_DOUBLE_EXTRA_PRECISION(value) = 0;

	while (power)
	{
		int low_three_bits;
		long_double_12 unrounded;
		long_double_12 *multiplier;

		power_of_ten += 7;
		low_three_bits = power & 7;
		power >>= 3;
		if (low_three_bits == 0)
			continue;

		multiplier = power_of_ten + low_three_bits;
		if (*LONG_DOUBLE_EXTRA_PRECISION(multiplier) >= 0x8000)
		{
			unrounded = *multiplier;
			(*LONG_DOUBLE_MANTISSA_LOW(&unrounded))--;
			multiplier = &unrounded;
		}

		__ld12mul(value, multiplier);
	}
}
