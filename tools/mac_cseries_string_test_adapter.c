/* Standalone asset-probe link adapter. It supplies only the ordinary C string
   operations used by this probe; production builds use the engine's cseries.c. */
#include "cseries.h"
#include <string.h>
#undef strcmp
#undef strlen
long csstrcmp(const char *a,const char *b) { return strcmp(a,b); }
unsigned long csstrlen(const char *s) { return (unsigned long)strlen(s); }
