/* GCC 4's system ctype macro expands the legacy isspace declaration in hs.c.
   Include this after the probe's common forced headers. */
#include <ctype.h>
#undef isspace
#define isspace mac_game_tick_isspace
