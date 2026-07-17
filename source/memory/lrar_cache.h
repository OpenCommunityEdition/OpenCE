/*
LRAR_CACHE.H

header included in hcex build.
*/

#ifndef __LRAR_CACHE_H
#define __LRAR_CACHE_H
#pragma once

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct lrar_cache;

typedef void (*lrar_lock_proc)(
	short *reference,
	short value);
typedef void (*lrar_unlock_proc)(
	void *reference);

/* ---------- prototypes/EXAMPLE.C */

void lrar_dispose(
	struct lrar_cache *cache);
unsigned long lrar_block_address(
	struct lrar_cache *cache,
	short block_index);
void lrar_deallocate(
	struct lrar_cache *cache,
	short block_index);

/* ---------- globals */

/* ---------- public code */

#endif // __LRAR_CACHE_H
