/* Streaming SHA-256 for MCC content identity. Adapted from this repository's
 * CC0 p2p_crypto.c (FIPS 180-4); no changes to the transport crypto. */
#include "mcc_sha256.h"
#include <string.h>

static const unsigned int mcc_sha256_constants[64] =
{
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};

#define ROTATE_RIGHT(value, count) (((value) >> (count)) | ((value) << (32 - (count))))

static void mcc_sha256_block(struct mcc_sha256 *context, const unsigned char *block)
{
	unsigned int words[64];
	unsigned int a, b, c, d, e, f, g, h;
	int index;

	for (index = 0; index < 16; index++)
	{
		words[index] = (unsigned int)block[index * 4] << 24 | (unsigned int)block[index * 4 + 1] << 16 |
			(unsigned int)block[index * 4 + 2] << 8 | (unsigned int)block[index * 4 + 3];
	}
	for (index = 16; index < 64; index++)
	{
		unsigned int s0 = ROTATE_RIGHT(words[index - 15], 7) ^ ROTATE_RIGHT(words[index - 15], 18) ^
			(words[index - 15] >> 3);
		unsigned int s1 = ROTATE_RIGHT(words[index - 2], 17) ^ ROTATE_RIGHT(words[index - 2], 19) ^
			(words[index - 2] >> 10);

		words[index] = words[index - 16] + s0 + words[index - 7] + s1;
	}
	a = context->state[0]; b = context->state[1]; c = context->state[2]; d = context->state[3];
	e = context->state[4]; f = context->state[5]; g = context->state[6]; h = context->state[7];
	for (index = 0; index < 64; index++)
	{
		unsigned int s1 = ROTATE_RIGHT(e, 6) ^ ROTATE_RIGHT(e, 11) ^ ROTATE_RIGHT(e, 25);
		unsigned int choice = (e & f) ^ (~e & g);
		unsigned int first = h + s1 + choice + mcc_sha256_constants[index] + words[index];
		unsigned int s0 = ROTATE_RIGHT(a, 2) ^ ROTATE_RIGHT(a, 13) ^ ROTATE_RIGHT(a, 22);
		unsigned int majority = (a & b) ^ (a & c) ^ (b & c);
		unsigned int second = s0 + majority;

		h = g; g = f; f = e; e = d + first;
		d = c; c = b; b = a; a = first + second;
	}
	context->state[0] += a; context->state[1] += b; context->state[2] += c; context->state[3] += d;
	context->state[4] += e; context->state[5] += f; context->state[6] += g; context->state[7] += h;
}

void mcc_sha256_begin(struct mcc_sha256 *context)
{
	static const unsigned int initial[8] =
	{
		0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
	};

	memcpy(context->state, initial, sizeof(initial));
	context->length = 0;
	context->used = 0;
}

void mcc_sha256_add(struct mcc_sha256 *context, const void *data, int size)
{
	const unsigned char *bytes = data;

	context->length += (unsigned long long)size;
	while (size > 0)
	{
		int count = 64 - context->used < size ? 64 - context->used : size;

		memcpy(context->block + context->used, bytes, (size_t)count);
		context->used += count;
		bytes += count;
		size -= count;
		if (context->used == 64)
		{
			mcc_sha256_block(context, context->block);
			context->used = 0;
		}
	}
}

void mcc_sha256_end(struct mcc_sha256 *context, unsigned char *digest)
{
	unsigned long long bits = context->length * 8;
	unsigned char length[8];
	int index;

	for (index = 0; index < 8; index++)
		length[index] = (unsigned char)(bits >> (56 - index * 8));
	mcc_sha256_add(context, "\x80", 1);
	while (context->used != 56)
		mcc_sha256_add(context, "", 1);
	mcc_sha256_add(context, length, 8);
	for (index = 0; index < 8; index++)
	{
		digest[index * 4] = (unsigned char)(context->state[index] >> 24);
		digest[index * 4 + 1] = (unsigned char)(context->state[index] >> 16);
		digest[index * 4 + 2] = (unsigned char)(context->state[index] >> 8);
		digest[index * 4 + 3] = (unsigned char)context->state[index];
	}
}

