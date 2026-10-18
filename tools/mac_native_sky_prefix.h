/* The standalone layout probe needs the game's random(long) declaration while
   keeping Darwin's unrelated random(void) libc declaration out of its name. */
#include <stdlib.h>
#define random mac_native_sky_random
