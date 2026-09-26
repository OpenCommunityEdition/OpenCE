/*
LIMITS.H

Windows <limits.h>. The game's cseries.h declares LONG_MAX, LONG_MIN,
CHAR_MAX and CHAR_MIN as enumerators, which works with the Xbox's C runtime
because nothing includes <limits.h> behind the game's back. Windows'
<stdlib.h> does, so crt/stdlib.h removes those four macros again when it was
the one to define them; this header (which has no include guard of its own)
brings them back for a unit that includes <limits.h> itself.
*/

#include_next <limits.h>

#ifndef LONG_MAX
#define LONG_MAX 2147483647L
#endif
#ifndef LONG_MIN
#define LONG_MIN (-2147483647L - 1)
#endif
#ifndef CHAR_MAX
#define CHAR_MAX 127
#endif
#ifndef CHAR_MIN
#define CHAR_MIN (-128)
#endif
