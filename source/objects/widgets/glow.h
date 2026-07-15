/*
GLOW.H

header included in hcex build.
*/

#ifndef __GLOW_H
#define __GLOW_H
#pragma once

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct glow_globals
{
	struct data_array *glow_data;
	struct data_array *glow_particle_data;
};

/* ---------- prototypes/EXAMPLE.C */

void glow_initialize(
	void);
void glow_initialize_for_new_map(
	void);
void glow_dispose_from_old_map(
	void);
void glow_dispose(
	void);

/* ---------- globals */

extern struct glow_globals glow_globals;

/* ---------- public code */

#endif // __GLOW_H
