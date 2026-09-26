/*
MSVC_COMDAT.C

External definitions of the game's header inline functions.

MSVC emits a C __inline function that has external linkage as a COMDAT, so
a unit that sees only an ordinary prototype (bitmap_color_conversion.h
declares real_rgb_color_to_pixel32, for example) links against a copy
emitted by any other unit. The Linux build gives header inlines internal
linkage instead (halo_linux_prefix.h), so no unit exports them. This unit
includes the headers that hold such functions with __inline meaning plain
gnu89 inline, which emits an external definition of each; the generated
`#pragma weak` list makes those definitions pick-any, like a COMDAT.

tools/linux_link_check.py fails the link if a header inline that some unit
calls through a prototype is still missing: add its header here.

On Windows (port/windows) the functions keep MSVC's COMDAT linkage in every
other unit, and this unit adds one weak definition of each (a COFF weak
external, which a COMDAT copy or an outright definition elsewhere overrides;
COFF allows only one weak definition of a name, hence only here). The C
runtime's headers come first, so that its own inline functions are left
alone.
*/

#ifdef _WIN32
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#endif

#undef __inline
#undef __forceinline
#ifdef _WIN32
#define __inline __attribute__((weak))
#define __forceinline __attribute__((weak))
#else
#define __inline __inline__
#define __forceinline __inline__
#endif

#include "cseries.h"
#include "math/real_math.h"
#include "bitmaps/bitmaps_inlines.h"
#include "physics/collisions.h"
