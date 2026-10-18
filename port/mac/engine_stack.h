#ifndef HALO_MAC_ENGINE_STACK_H
#define HALO_MAC_ENGINE_STACK_H
#include <stdio.h>
/* Best-effort native unwind, bounded to 128 frames, safe for concurrent callers.
   Not signal-handler-safe and not an interpreter for Windows CPU contexts. */
unsigned mac_engine_stack_trace(FILE *,unsigned skip);
void stack_walk(short levels_to_ignore);
#endif
