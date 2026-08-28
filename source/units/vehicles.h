/*
VEHICLES.H

header included in hcex build.
*/

#ifndef __VEHICLES_H
#define __VEHICLES_H
#pragma once

/* ---------- headers */

#include "units.h"

/* ---------- constants */

/* ---------- macros */

#define vehicle_get(index) ((struct unit_datum *)object_get_and_verify_type((index), _object_mask_vehicle))
#define vehicle_try_and_get(index) ((struct unit_datum *)object_try_and_get_and_verify_type((index), _object_mask_vehicle))
#define vehicle_runtime_get(index) ((struct vehicle_runtime_datum *)object_get_and_verify_type((index), _object_mask_vehicle))

/* ---------- structures */

struct vehicle_runtime_datum;

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
