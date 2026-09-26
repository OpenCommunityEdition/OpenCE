/*
STDLIB.H

Windows <stdlib.h>, which includes <limits.h>: if that defined the limits
the game's cseries.h declares itself, they are removed again (see
crt/limits.h).
*/

#ifndef LONG_MAX
#define HALO_WINDOWS_STDLIB_DEFINES_LIMITS
#endif

#include_next <stdlib.h>

#ifdef HALO_WINDOWS_STDLIB_DEFINES_LIMITS
#undef HALO_WINDOWS_STDLIB_DEFINES_LIMITS
#undef LONG_MAX
#undef LONG_MIN
#undef CHAR_MAX
#undef CHAR_MIN
#endif
