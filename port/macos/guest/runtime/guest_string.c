#include <stddef.h>
#include <stdint.h>
#include <string.h>

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
