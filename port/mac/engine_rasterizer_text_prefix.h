#ifndef HALO_MAC_RASTERIZER_TEXT_PREFIX_H
#define HALO_MAC_RASTERIZER_TEXT_PREFIX_H
/* GCC 4 rejects the repeated compatible typedef in rasterizer_text.c.
   Keep the public declaration and give that TU's local spelling a unique name;
   both signatures retain every argument and the original 32-bit pixel type. */
#include "math/real_math.h"
#include "math/integer_math.h"
#include "text/draw_string.h"
#define draw_character_proc mac_rasterizer_text_draw_character_proc
#endif
