/* the game includes <StdDef.h>; Linux file names are case sensitive.
(include_next: the real <stddef.h> is musl's, later in the search path;
on the Linux build glibc's) */
#pragma once
#include_next <stddef.h>
