/*
LIGHTNING.H

header included in hcex build.
*/

#ifndef __LIGHTNING_H
#define __LIGHTNING_H
#pragma once

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct lightning_globals
{
	struct data_array *lightning_data;
};

typedef char lightning_globals_size_assert[
	sizeof(struct lightning_globals) == 0x4 ? 1 : -1];

/* ---------- prototypes/LIGHTNING.C */

void lightnings_initialize(
	void);

void lightnings_dispose(
	void);
void lightnings_initialize_for_new_map(
	void);
void lightnings_dispose_from_old_map(
	void);

/* ---------- globals */

extern struct lightning_globals lightning_globals;

/* ---------- public code */

#endif // __LIGHTNING_H
