#ifndef HALO_MAC_ENGINE_ARENA_H
#define HALO_MAC_ENGINE_ARENA_H
/* Whole game_state.c allocator and registration tables, with an owned native
   arena. Start before model buffers. This is engine-thread-only bootstrap;
   it does not initialize gameplay, open saves or run map lifecycle callbacks.
   Release all engine users before stop; stop refuses live data/pool entries. */
int mac_engine_arena_start(void);
int mac_engine_arena_stop(void);
int mac_engine_arena_contains(const void *pointer);
unsigned long mac_engine_arena_used(void);
unsigned long mac_engine_arena_registrations(void);
#endif
