/*
LIGHT_VOLUMES.H

header included in hcex build.
*/

#ifndef __LIGHT_VOLUMES_H
#define __LIGHT_VOLUMES_H
#pragma once

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/EXAMPLE.C */

void light_volumes_initialize(
	void);
void light_volumes_dispose(
	void);
void light_volumes_initialize_for_new_map(
	void);
void light_volumes_dispose_from_old_map(
	void);

/* ---------- globals */

/* ---------- public code */

#endif // __LIGHT_VOLUMES_H
