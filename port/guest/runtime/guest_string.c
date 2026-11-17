/*
GUEST_STRING.C

String functions faster than musl's generic C ones, which the guest takes
in their place (tools/android_build.py MUSL_EXCLUDE). The renderer's state
caches compare their keys with memcmp on every draw (the pixel shaders'
keys, the vertex constants, the uniforms), and musl's compares a byte at a
time.
*/

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* eight bytes at a time while they match (AArch64 loads them unaligned),
then the bytes that differ, or the tail. no_builtin: clang would turn the
loops back into a call to memcmp */
__attribute__((no_builtin)) int memcmp(const void *left, const void *right, size_t size)
{
	const unsigned char *l = left, *r = right;

	for (; size >= 8; size -= 8, l += 8, r += 8)
	{
		uint64_t a, b;

		__builtin_memcpy(&a, l, 8);
		__builtin_memcpy(&b, r, 8);
		if (a != b)
			break;
	}
	for (; size; size--, l++, r++)
	{
		if (*l != *r)
			return *l - *r;
	}
	return 0;
}
