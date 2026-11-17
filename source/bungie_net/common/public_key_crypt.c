/* ---------- headers */

#include "cseries.h"
#include "cseries/errors.h"
#include "bungie_net/common/64bit_math.h"
#include "bungie_net/common/public_key_crypt.h"
#include "bungie_net/common/prime_numbers.h"
#include "bungie_net/common/random_numbers.h"
#include "memory/byte_swapping.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

static unsigned long x_exp_y_mod_n(
	unsigned long base,
	unsigned long exponent,
	unsigned long modulus);
static unsigned long generate_diffie_hellman_public_key(
	unsigned long p,
	unsigned long x,
	unsigned long g);
static unsigned long generate_diffie_hellman_private_key(
	unsigned long public_key,
	unsigned long p,
	unsigned long x);

/* ---------- globals */

/* ---------- public code */

void generate_key_parameters(
	struct public_key *p,
	struct public_key *x,
	struct public_key *g)
{
	long i = 0;
	long count = 2;

	while (count)
	{
		unsigned long prime0;
		unsigned long prime1;

		do
		{
			prime0 = randomprime(0xFFFF);
			prime1 = randomprime(0xFFFF);
			p->dwords[i] = prime0 * prime1 + 2;
		}
		while (p->dwords[i] < 0xFFFFFF);

		x->dwords[i] = randomrange(0xFF, p->dwords[i] - 2);
		g->dwords[i] = randomrange(0xFF, p->dwords[i] - 1);
		match_assert(
			"c:\\halo\\SOURCE\\bungie_net\\common\\public_key_crypt.c",
			162,
			x->dwords[i] < (p->dwords[i] - 2));
		match_assert(
			"c:\\halo\\SOURCE\\bungie_net\\common\\public_key_crypt.c",
			163,
			g->dwords[i] < (p->dwords[i] - 1));

		i++;
		count--;
	}

	return;
}

void generate_public_key(
	struct public_key const *p,
	struct public_key const *x,
	struct public_key const *g,
	struct public_key *public_key)
{
	long i;

	for (i = 0; i < 2; i++)
		public_key->dwords[i] = generate_diffie_hellman_public_key(p->dwords[i], x->dwords[i], g->dwords[i]);

	error(
		2,
		"p= %8lX%8lX\nx= %8lX%8lX\ng= %8lX%8lX\npublic key= %8lX%8lX\n\n",
		p->dwords[0], p->dwords[1],
		x->dwords[0], x->dwords[1],
		g->dwords[0], g->dwords[1],
		public_key->dwords[0], public_key->dwords[1]);

	return;
}

void generate_private_key(
	struct public_key const *public_key,
	struct public_key const *p,
	struct public_key const *x,
	struct public_key *private_key)
{
	long i;

	for (i = 0; i < 2; i++)
	{
		private_key->dwords[i] = generate_diffie_hellman_private_key(
			public_key->dwords[i],
			p->dwords[i],
			x->dwords[i]);

		private_key->dwords[i] = SWAP4(private_key->dwords[i]);
	}

	error(
		2,
		"public_key= %8lX%8lX\np= %8lX%8lX\nx= %8lX%8lX\nprivate key= %8lX%8lX\n\n",
		public_key->dwords[0], public_key->dwords[1],
		p->dwords[0], p->dwords[1],
		x->dwords[0], x->dwords[1],
		private_key->dwords[0], private_key->dwords[1]);

	return;
}

/* ---------- private code */

static unsigned long x_exp_y_mod_n(
	unsigned long base,
	unsigned long exponent,
	unsigned long modulus)
{
	struct qword_value s;
	struct qword_value base_value;
	struct qword_value modulus_value;
	struct qword_value product;

	s.qword = 1;
	base_value.qword = base;
	modulus_value.qword = modulus;

	while (exponent)
	{
		if (exponent & 1)
		{
			multiply64(&s, &base_value, &product);
			divide64(&product, &modulus_value, NULL, &s);
		}

		exponent >>= 1;
		multiply64(&base_value, &base_value, &product);
		divide64(&product, &modulus_value, NULL, &base_value);
	}

	match_assert(
		"c:\\halo\\SOURCE\\bungie_net\\common\\public_key_crypt.c",
		95,
		s.qword <= 0xFFFFFFFF);

	return (unsigned long)s.qword;
}

static unsigned long generate_diffie_hellman_public_key(
	unsigned long p,
	unsigned long x,
	unsigned long g)
{
	match_assert("c:\\halo\\SOURCE\\bungie_net\\common\\public_key_crypt.c", 112, p>2);
	match_assert("c:\\halo\\SOURCE\\bungie_net\\common\\public_key_crypt.c", 113, x<(p-1));
	match_assert("c:\\halo\\SOURCE\\bungie_net\\common\\public_key_crypt.c", 114, g<p);

	return x_exp_y_mod_n(g, x, p);
}

static unsigned long generate_diffie_hellman_private_key(
	unsigned long public_key,
	unsigned long p,
	unsigned long x)
{
	match_assert("c:\\halo\\SOURCE\\bungie_net\\common\\public_key_crypt.c", 133, p>2);
	match_assert("c:\\halo\\SOURCE\\bungie_net\\common\\public_key_crypt.c", 134, x<(p-1));

	return x_exp_y_mod_n(public_key, x, p);
}
