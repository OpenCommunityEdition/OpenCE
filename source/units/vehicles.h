/*
VEHICLES.H

header included in hcex build.
*/

#ifndef __VEHICLES_H
#define __VEHICLES_H
#pragma once

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/VEHICLES.C */

void vehicles_initialize(
	void);
void vehicles_initialize_for_new_map(
	void);
void vehicles_dispose_from_old_map(
	void);
void vehicles_dispose(
	void);
void vehicle_delete(
	long vehicle_index);

/* ---------- globals */

/* ---------- public code */

#endif // __VEHICLES_H
