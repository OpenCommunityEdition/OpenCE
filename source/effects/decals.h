/*
DECALS.H

header included in hcex build.
*/

#ifndef __DECALS_H
#define __DECALS_H
#pragma once

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/EXAMPLE.C */

/* ---------- globals */

/* ---------- public code */

void decals_dispose(
	void);
void decals_dispose_from_old_map(
	void);
void decals_update(
	void);
void decals_delete_permanent_from_cluster(
	short cluster_index);

#endif // __DECALS_H
