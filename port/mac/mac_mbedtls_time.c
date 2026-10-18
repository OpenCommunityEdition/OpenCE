/*
MAC_MBEDTLS_TIME.C

mbedtls's millisecond clock for Mac OS X 10.4.

mac_mbedtls_config.h sets MBEDTLS_PLATFORM_MS_TIME_ALT, which tells the
library that the application provides mbedtls_ms_time. It has to: the
version in platform_util.c wants clock_gettime(CLOCK_MONOTONIC), which
10.4 does not have, and its only other branches are Windows and an
#error. Seven of the TLS sources call this, so the library does not link
without it.

UpTime is the system's monotonic count since boot, and
AbsoluteToNanoseconds applies the machine's own timebase to it. Doing the
scaling that way rather than by hand matters on PowerPC, where the
timebase is a ratio near 1e9/33e6: multiplying the raw count by the
numerator overflows 64 bits after a few minutes of uptime. Both calls are
in CoreServices, which the Mac build already links through Carbon, and
both work on PowerPC and Intel.

Monotonic is what mbedtls wants here. It measures session ticket ages and
handshake timeouts with it, so a wall clock that a user or NTP can move
backwards would be wrong.
*/

#include <CoreServices/CoreServices.h>

#include "mbedtls/platform_time.h"

mbedtls_ms_time_t mbedtls_ms_time(void)
{
	Nanoseconds elapsed = AbsoluteToNanoseconds(UpTime());
	/* (a UnsignedWide is the count in two halves) */
	unsigned long long nanoseconds =
		((unsigned long long)elapsed.hi << 32) | (unsigned long long)elapsed.lo;

	return (mbedtls_ms_time_t)(nanoseconds / 1000000ULL);
}
