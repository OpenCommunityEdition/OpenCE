#include "guest_host.h"

#include <math.h>
#include <string.h>

struct guest_sincos_result { double sine, cosine; };
struct guest_sincosf_result { float sine, cosine; };

__attribute__((no_builtin)) struct guest_sincos_result __sincos_stret(double x)
{
	struct guest_sincos_result result;

	sincos(x, &result.sine, &result.cosine);
	return result;
}

__attribute__((no_builtin)) struct guest_sincosf_result __sincosf_stret(float x)
{
	struct guest_sincosf_result result;

	sincosf(x, &result.sine, &result.cosine);
	return result;
}

__attribute__((no_builtin)) float __exp10f(float x)
{
	return exp10f(x);
}

__attribute__((no_builtin)) double __exp10(double x)
{
	return exp10(x);
}

__attribute__((no_builtin)) void __bzero(void *buffer, size_t size)
{
	memset(buffer, 0, size);
}

__attribute__((no_builtin)) void memset_pattern4(void *buffer, const void *pattern, size_t size)
{
	unsigned char *out = buffer;
	size_t index;

	for (index = 0; index < size; index++)
		out[index] = ((const unsigned char *)pattern)[index & 3];
}

__attribute__((no_builtin)) void memset_pattern8(void *buffer, const void *pattern, size_t size)
{
	unsigned char *out = buffer;
	size_t index;

	for (index = 0; index < size; index++)
		out[index] = ((const unsigned char *)pattern)[index & 7];
}

__attribute__((no_builtin)) void memset_pattern16(void *buffer, const void *pattern, size_t size)
{
	unsigned char *out = buffer;
	size_t index;

	for (index = 0; index < size; index++)
		out[index] = ((const unsigned char *)pattern)[index & 15];
}

void __stack_chk_fail(void)
{
	host_abort("stack smashing detected");
}

int backtrace(void **frames, int size)
{
	(void)frames;
	(void)size;
	return 0;
}

void backtrace_symbols_fd(void *const *frames, int size, int fd)
{
	(void)frames;
	(void)size;
	(void)fd;
}
