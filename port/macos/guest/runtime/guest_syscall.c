#include "guest_host.h"

long __guest_syscall(long long number, long long a, long long b, long long c,
	long long d, long long e, long long f)
{
	return (long)host_syscall(number, a, b, c, d, e, f);
}
