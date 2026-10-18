/* Compile-only experiment, not a production platform prefix.
 * Include libc first so the engine's random name does not collide with it. */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <float.h>
#define random halo_probe_game_random
#define __int64 long long
/* This probe never uses the extern TLS flag. A real TLS backend is still needed. */
#define __thread
#include "port/linux/include/float.h"
