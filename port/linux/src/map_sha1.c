/*
MAP_SHA1.C

SHA-1 (map_sha1.h), the straightforward way: a block at a time, with
the words of each block read big-endian, as the standard writes them.
*/

#include "map_sha1.h"

#include <string.h>

#define ROTATE_LEFT(value, bits) ((((value) << (bits)) | ((value) >> (32 - (bits)))) & 0xFFFFFFFFUL)

static void sha1_block(struct sha1_state *state, const unsigned char *block)
{
	unsigned long words[80];
	unsigned long a = state->state[0], b = state->state[1], c = state->state[2], d = state->state[3],
		e = state->state[4];
	int index;

	for (index = 0; index < 16; index++)
	{
		words[index] = (unsigned long)block[index * 4] << 24 | (unsigned long)block[index * 4 + 1] << 16 |
			(unsigned long)block[index * 4 + 2] << 8 | (unsigned long)block[index * 4 + 3];
	}
	for (; index < 80; index++)
	{
		words[index] = ROTATE_LEFT(words[index - 3] ^ words[index - 8] ^ words[index - 14] ^ words[index - 16], 1);
	}
	for (index = 0; index < 80; index++)
	{
		unsigned long f, k, temporary;

		if (index < 20)
		{
			f = (b & c) | (~b & d);
			k = 0x5A827999UL;
		}
		else if (index < 40)
		{
			f = b ^ c ^ d;
			k = 0x6ED9EBA1UL;
		}
		else if (index < 60)
		{
			f = (b & c) | (b & d) | (c & d);
			k = 0x8F1BBCDCUL;
		}
		else
		{
			f = b ^ c ^ d;
			k = 0xCA62C1D6UL;
		}
		temporary = (ROTATE_LEFT(a, 5) + (f & 0xFFFFFFFFUL) + e + k + words[index]) & 0xFFFFFFFFUL;
		e = d;
		d = c;
		c = ROTATE_LEFT(b, 30);
		b = a;
		a = temporary;
	}
	state->state[0] = (state->state[0] + a) & 0xFFFFFFFFUL;
	state->state[1] = (state->state[1] + b) & 0xFFFFFFFFUL;
	state->state[2] = (state->state[2] + c) & 0xFFFFFFFFUL;
	state->state[3] = (state->state[3] + d) & 0xFFFFFFFFUL;
	state->state[4] = (state->state[4] + e) & 0xFFFFFFFFUL;
}

void sha1_start(struct sha1_state *state)
{
	state->state[0] = 0x67452301UL;
	state->state[1] = 0xEFCDAB89UL;
	state->state[2] = 0x98BADCFEUL;
	state->state[3] = 0x10325476UL;
	state->state[4] = 0xC3D2E1F0UL;
	state->length = 0;
	state->buffered = 0;
}

void sha1_update(struct sha1_state *state, const void *bytes, unsigned long size)
{
	const unsigned char *input = bytes;

	state->length += size;
	if (state->buffered)
	{
		unsigned long take = 64 - (unsigned long)state->buffered;

		if (take > size)
			take = size;
		memcpy(state->buffer + state->buffered, input, take);
		state->buffered += (int)take;
		input += take;
		size -= take;
		if (state->buffered < 64)
			return;
		sha1_block(state, state->buffer);
		state->buffered = 0;
	}
	while (size >= 64)
	{
		sha1_block(state, input);
		input += 64;
		size -= 64;
	}
	if (size)
	{
		memcpy(state->buffer, input, size);
		state->buffered = (int)size;
	}
}

void sha1_finish(struct sha1_state *state, unsigned char digest[SHA1_DIGEST_SIZE])
{
	unsigned long long bits = state->length * 8;
	unsigned char padding[72];
	int padding_size = (state->buffered < 56 ? 56 : 120) - state->buffered;
	int index;

	memset(padding, 0, sizeof(padding));
	padding[0] = 0x80;
	for (index = 0; index < 8; index++)
		padding[padding_size + index] = (unsigned char)(bits >> (56 - index * 8));
	sha1_update(state, padding, (unsigned long)(padding_size + 8));
	for (index = 0; index < 5; index++)
	{
		digest[index * 4] = (unsigned char)(state->state[index] >> 24);
		digest[index * 4 + 1] = (unsigned char)(state->state[index] >> 16);
		digest[index * 4 + 2] = (unsigned char)(state->state[index] >> 8);
		digest[index * 4 + 3] = (unsigned char)state->state[index];
	}
}

void sha1_bytes(const void *bytes, unsigned long size, unsigned char digest[SHA1_DIGEST_SIZE])
{
	struct sha1_state state;

	sha1_start(&state);
	sha1_update(&state, bytes, size);
	sha1_finish(&state, digest);
}
