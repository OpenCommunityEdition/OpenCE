/*
PHYSICAL_MEMORY_MAP.H

header included in hcex build.
*/

#ifndef __PHYSICAL_MEMORY_MAP_H
#define __PHYSICAL_MEMORY_MAP_H
#pragma once

/* ---------- constants */

/* port: the tag cache's size, which the loader checks a map's tag data and
structure bsps against (cache_files.c) */
#define TAG_CACHE_SIZE 0x1600000
/* The desktop port can retain larger, higher-rate sound permutations without
   being constrained by the Xbox's 4 MiB sound cache. Keep all three users of
   this size (allocation, LRU pages, and packet bounds) in sync. */
#if defined(HALO_NATIVE_AUDIO) && !defined(HALO_ANDROID)
#define PHYSICAL_SOUND_CACHE_SIZE 0x1000000
#else
#define PHYSICAL_SOUND_CACHE_SIZE 0x400000
#endif

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/PHYSICAL_MEMORY_MAP.C */

void physical_memory_allocate(void);
void physical_memory_verify(void);
void physical_memory_free(void);

void *physical_memory_get_game_state_base_address(void);
void *physical_memory_get_tag_cache_base_address(void);
void *physical_memory_get_texture_cache_base_address(void);
void *physical_memory_get_sound_cache_base_address(void);

/* ---------- globals */

/* ---------- public code */

#endif // __PHYSICAL_MEMORY_MAP_H
