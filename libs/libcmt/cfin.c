/*
CFIN.C

symbols in this file:
	__fltin2 (.text, 0x85)
*/

#include <stddef.h>

enum
{
	string_to_float_underflow = 1,
	string_to_float_overflow = 2,
	string_to_float_no_digits = 4,
	conversion_no_digits = 512,
	conversion_overflow = 128,
	conversion_underflow = 256
};

enum intrinsic_conversion_status
{
	intrinsic_conversion_ok,
	intrinsic_conversion_overflow,
	intrinsic_conversion_underflow
};

struct extended_float
{
	unsigned char bytes[12];
};

/* Copying this wrapper transfers a double without executing x87 code. */
struct encoded_double
{
	unsigned long words[2];
};

struct float_input_result
{
	int flags;
	int byte_count;
	long integer_value;
	long double_alignment;
	struct encoded_double double_value;
};

typedef char assert_float_input_double_offset[
	offsetof(struct float_input_result, double_value) == 0x10 ? 1 : -1];

unsigned int __cdecl __strgtold12(
	struct extended_float *result,
	char const **end,
	char const *string,
	int use_twelve_bytes,
	int scale,
	int decimal_point,
	int implicit_exponent);

enum intrinsic_conversion_status __cdecl _ld12tod(
	struct extended_float *value,
	struct encoded_double *result);

struct float_input_result * __cdecl _fltin2(
	struct float_input_result *result,
	char const *string,
	int ignored_length,
	int ignored_scale,
	int ignored_radix)
{
	struct extended_float extended_value;
	struct encoded_double double_value;
	char const *end;
	unsigned int flags;
	int result_flags = 0;

	flags = __strgtold12(
		&extended_value,
		&end,
		string,
		0,
		0,
		0,
		0);
	if (flags & string_to_float_no_digits)
	{
		result_flags |= conversion_no_digits;
		double_value.words[0] = 0;
		double_value.words[1] = 0;
	}
	else
	{
		enum intrinsic_conversion_status status =
			_ld12tod(&extended_value, &double_value);

		if ((flags & string_to_float_overflow) ||
			status == intrinsic_conversion_overflow)
		{
			result_flags |= conversion_overflow;
		}
		if ((flags & string_to_float_underflow) ||
			status == intrinsic_conversion_underflow)
		{
			result_flags |= conversion_underflow;
		}
	}

	result->flags = result_flags;
	result->byte_count = end - string;
	result->double_value = double_value;
	return result;
}
