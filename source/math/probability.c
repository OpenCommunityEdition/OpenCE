/*
PROBABILITY.C

symbols in this file:
000FA5F0 0030:
	_factorial (0000)
000FA620 0030:
	_permutations (0000)
000FA650 0050:
	_combinations (0000)
000FA6A0 00f0:
	_permute (0000)
000FA790 0120:
	_combine (0000)
0027AE48 0008:
	??_C@_07OKCKIPNN@indices?$AA@ (0000)
0027AE50 0007:
	??_C@_06POCLMCPF@base?$DO0?$AA@ (0000)
0027AE58 0022:
	??_C@_0CC@JNAEFKFC@c?3?2halo?2SOURCE?2math?2probability?4@ (0000)
0027AE7C 000c:
	??_C@_0M@EDJJGKEM@base?$DO?$DNcount?$AA@ (0000)
*/

/* ---------- headers */

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

long factorial(short value)
{
	long result = 0;
	short factor;

	if (value >= 0)
	{
		result = 1;
		for (factor = value; factor > 1; factor--)
		{
			result *= factor;
		}
	}

	return result;
}

long permutations(
	short count,
	short selection_count)
{
	long result = 0;

	if (count >= selection_count && selection_count >= 0)
	{
		result = 1;
		while (selection_count != 0)
		{
			result *= count;
			count--;
			selection_count--;
		}
	}

	return result;
}

long combinations(
	short count,
	short selection_count)
{
	long result = 0;
	short divisor;

	if (count >= selection_count && selection_count >= 0)
	{
		if (selection_count > count - selection_count)
		{
			selection_count = count - selection_count;
		}

		result = permutations(count, selection_count);
		for (divisor = selection_count; divisor > 1; divisor--)
		{
			result /= divisor;
		}
	}

	return result;
}

/* ---------- private code */
