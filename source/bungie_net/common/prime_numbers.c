/*
PRIME_NUMBERS.C

symbols in this file:
0006F3A0 0020:
	_compare_prime_numbers_descending (0000)
0006F3C0 0160:
	_generate_prime_numbers (0000)
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
#include "bungie_net/common/64bit_math.h"
#include "bungie_net/common/random_numbers.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

unsigned long *generate_prime_numbers(
	unsigned long maximum,
	unsigned long *num_primes);

void probable_prime64(
	struct qword_value *result);

/* ---------- globals */

/* ---------- public code */

unsigned long randomprime(
	unsigned long maximum)
{
	unsigned long result = 0;
	unsigned long prime_count;
	unsigned long *primes = generate_prime_numbers(maximum, &prime_count);

	if (primes)
	{
		result = primes[randomrange(0, prime_count - 1)];
		match_free("c:\\halo\\SOURCE\\bungie_net\\common\\prime_numbers.c", 137, primes);
	}

	return result;
}

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
/* NonMatching foundation: this source preserves the January count model,
 * including an in-bounds trailing slot for prime 2. */
unsigned long *generate_prime_numbers(
	unsigned long maximum,
	unsigned long *num_primes)
{
	unsigned long *primes;
	unsigned long odd_count;
	unsigned long total_count;
	unsigned long sqrt_max;
	unsigned long i;
	unsigned long scan_count;
	unsigned long sieve_count;
	unsigned long j;
	unsigned long k;
	unsigned long m;

	odd_count = maximum >> 1;
	if (!(maximum & 1))
		odd_count--;
	i = 0;
	scan_count = 0;

	match_assert("c:\\halo\\SOURCE\\bungie_net\\common\\prime_numbers.c", 61, num_primes);

	if (maximum < 2)
	{
		*num_primes = 0;

		return NULL;
	}

	total_count = odd_count + 1;
	*num_primes = total_count;
	primes = match_malloc(
		"c:\\halo\\SOURCE\\bungie_net\\common\\prime_numbers.c",
		71,
		odd_count * sizeof(*primes) + sizeof(*primes));

	if (primes)
	{
		sqrt_max = (unsigned long)sqrt((double)maximum);
		k = 3;

		while (i < odd_count)
		{
			primes[i] = k;
			k += 2;
			i++;
		}

		while (scan_count < odd_count && primes[scan_count] <= sqrt_max)
			scan_count++;

		if (scan_count)
		{
			m = 1;
			j = 0;
			sieve_count = scan_count;

			while (sieve_count)
			{
				if (primes[j])
				{
					for (k = m; k < odd_count; k++)
					{
						if (primes[k] && !(primes[k] % primes[j]))
						{
							primes[k] = 0;
							(*num_primes)--;
						}
					}
				}

				m++;
				j++;
				sieve_count--;
			}
		}

		primes[odd_count] = 2;
		qsort(primes, total_count, sizeof(*primes), compare_prime_numbers_descending);

		if (*num_primes < total_count)
		{
			primes = match_realloc(
				"c:\\halo\\SOURCE\\bungie_net\\common\\prime_numbers.c",
				117,
				primes,
				*num_primes * sizeof(*primes));
		}
	}

	return primes;
}

void probable_prime64(
	struct qword_value *result)
{
	unsigned long prime_count;
	unsigned long *primes;
	unsigned long prime;
	unsigned long iteration_count;
	struct qword_value two;
	struct qword_value prime_qword;

	match_assert("c:\\halo\\SOURCE\\bungie_net\\common\\prime_numbers.c", 150, result);

	result->qword = 1;
	two.qword = 2;
	iteration_count = 4;

	do
	{
		prime = 0;
		primes = generate_prime_numbers(0xFFFF, &prime_count);

		if (primes)
		{
			prime = primes[randomrange(0, prime_count - 1)];
			match_free("c:\\halo\\SOURCE\\bungie_net\\common\\prime_numbers.c", 137, primes);
		}

		prime_qword.qword = prime;
		multiply64(result, &prime_qword, result);
	}
	while (--iteration_count);

	add64(result, &two, result);

	return;
}
