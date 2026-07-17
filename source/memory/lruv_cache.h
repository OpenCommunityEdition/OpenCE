/*
LRUV_CACHE.H

header included in hcex build.
*/

#ifndef __LRUV_CACHE_H
#define __LRUV_CACHE_H
#pragma once

/* ---------- headers */

#include "cseries.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct lruv_cache;

/* ---------- prototypes/LRUV_CACHE.C */

long lruv_allocation_size(long maximum_block_count);
void lruv_initialize(
	struct lruv_cache *cache,
	const char *name,
	long page_count,
	long page_size_bits,
	long maximum_block_count,
	void (*delete_block_proc)(long),
	boolean (*locked_block_proc)(long));

/* ---------- globals */

/* ---------- public code */

#endif // __LRUV_CACHE_H
