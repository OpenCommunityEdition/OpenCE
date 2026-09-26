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
*/

#undef __inline
#undef __forceinline
#define __inline __inline__
#define __forceinline __inline__

#include "cseries.h"
#include "math/real_math.h"
#include "bitmaps/bitmaps_inlines.h"
#include "physics/collisions.h"
