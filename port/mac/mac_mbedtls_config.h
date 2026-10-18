/*
MAC_MBEDTLS_CONFIG.H

mbedtls as the Mac's compiler can build it. The vendored copy stays
unchanged (AGENTS.md: third-party code is copied unchanged), so what the
Mac cannot do is turned off here instead, through mbedtls's own
MBEDTLS_CONFIG_FILE.

- MBEDTLS_HAVE_ASM: constant_time_impl.h's x86 inline assembly is not
  something Apple's GCC 4.0.1 can parse, and the PowerPC build has no
  assembly of its own to lose. Without it mbedtls uses its C versions,
  which are the same constant-time code written portably.
- MBEDTLS_AESNI_C and MBEDTLS_PADLOCK_C: GCC 4.0.1 has neither intrinsic,
  and no Mac this port runs on has either instruction set (the G4s are
  PowerPC, and AES-NI postdates the Core 2 Duo).
- MBEDTLS_PLATFORM_MS_TIME_ALT: platform_util.c's own mbedtls_ms_time
  wants clock_gettime(CLOCK_MONOTONIC), which 10.4 does not have at all;
  its other branches are Windows and #error. port/mac/mac_mbedtls_time.c
  gives it one instead. This is set here rather than on the command line
  so that every file of the library is built against the same
  configuration: mbedtls options change structure layouts, and a library
  half built one way and half the other is the kind of fault that shows
  up as something else entirely.
*/

#ifndef HALO_MAC_MBEDTLS_CONFIG_H
#define HALO_MAC_MBEDTLS_CONFIG_H

#include "mbedtls/mbedtls_config.h"

#undef MBEDTLS_HAVE_ASM
#undef MBEDTLS_AESNI_C
#undef MBEDTLS_PADLOCK_C

#ifndef MBEDTLS_PLATFORM_MS_TIME_ALT
#define MBEDTLS_PLATFORM_MS_TIME_ALT
#endif

#endif
