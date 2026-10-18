/* Platform heap backing. These are native C allocations, not game-state arena
   addresses or debug_memory.c allocations with an Xbox debug header. */
#include <stdlib.h>
#undef malloc
#undef free
void *system_malloc(long bytes)
{ return bytes>0?malloc((size_t)bytes):NULL; }
void system_free(void *pointer)
{ free(pointer); }
