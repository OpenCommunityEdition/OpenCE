#ifndef HALO_MAC_ENGINE_PREFIX_H
#define HALO_MAC_ENGINE_PREFIX_H
/* Initial native engine-unit boundary, not a complete platform implementation.
   No x86 architecture impersonation, global TLS erasure or system math remap. */
#if !defined(__ppc__) && !defined(__i386__)
#error Native engine integration requires a 32-bit PPC or i386 target
#endif
#include "engine_capacity.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <float.h>
#include <wchar.h>
/* Engine strings are UTF-16 under -fshort-wchar; Darwin libc uses wchar32.
   Only the function needed by the current linked slice is implemented. */
wchar_t *halo_mac_utf16_wcsncpy(wchar_t *, const wchar_t *, size_t);
#define wcsncpy halo_mac_utf16_wcsncpy
/* MS CRT signatures in currently discarded unicode.c functions. These have
   no implementation: linking either path must fail rather than call wchar32. */
wchar_t *halo_mac_utf16_wcstok_unavailable(wchar_t *, const wchar_t *);
int halo_mac_utf16_vswprintf_unavailable(wchar_t *, const wchar_t *, va_list);
#define wcstok halo_mac_utf16_wcstok_unavailable
#define vswprintf halo_mac_utf16_vswprintf_unavailable
#define random halo_mac_game_random
#define __int64 long long
/* MS control-word declarations only; no FP-environment implementation yet. */
#include "port/linux/include/float.h"
#endif
