/* The shared generator's exact text builder, with native Mac GL declarations. */
#include "mac_nv2a_shader_api.h"
/* Generated source text is short-lived compiler state outside engine heaps. */
#undef malloc
#undef realloc
#undef free
#undef memcpy
#undef memset
#undef strlen
#include "../linux/src/xgpu_text.c"
