/* What the Linux build's headers route to its platform layer, for
   tools/mac_sim_math_probe.c built with the Linux build's flags (see
   tools/mac_sim_math_checks.sh); built without the Linux prefix header. */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
void *csmemcpy(void *d, const void *s, unsigned long n) { return memcpy(d, s, n); }
unsigned int _control87(unsigned int value, unsigned int mask) { return 0; }
int halo_linux_printf(const char *format, ...)
{
	va_list args; int n;
	va_start(args, format); n = vprintf(format, args); va_end(args);
	return n;
}
void release_assert_failed(const char *expression, const char *file, int line)
{
	fprintf(stderr, "assert %s %s:%d\n", expression, file, line); abort();
}
