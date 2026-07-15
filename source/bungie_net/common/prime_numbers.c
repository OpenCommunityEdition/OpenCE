/*
PRIME_NUMBERS.C

symbols in this file:
0006F3A0 0020:
	_compare_prime_numbers_descending (0000)
0006F3C0 0160:
	_code_0006f3c0 (0000)
0006F520 0050:
	_randomprime (0000)
0006F570 00c0:
	_probable_prime64 (0000)
002558F0 0008:
	__real@41f0000000000000 (0000)
002558F8 000b:
	??_C@_0L@BKHCFPJF@num_primes?$AA@ (0000)
00255904 0031:
	??_C@_0DB@FDHHNPGN@c?3?2halo?2SOURCE?2bungie_net?2common@ (0000)
*/

/* ---------- headers */

#include "cseries.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

/* ---------- private code */

int compare_prime_numbers_descending(
	void const *left,
	void const *right)
{
	unsigned long left_value = *(unsigned long const *)left;
	unsigned long right_value = *(unsigned long const *)right;

	if (left_value < right_value)
		return 1;

	return left_value > right_value ? -1 : 0;
}
