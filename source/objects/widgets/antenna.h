/*
ANTENNA.H

header included in hcex build.
*/

#ifndef __ANTENNA_H
#define __ANTENNA_H
#pragma once

/* ---------- headers */

#include "objects/widgets/widget_types.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/ANTENNA.C */

void antennas_initialize(
	void);
void antennas_initialize_for_new_map(
	void);
void antennas_dispose_from_old_map(
	void);
void antennas_dispose(
	void);

long antenna_new(
	long definition_index);
void antenna_delete(
	long antenna_index);
void antennas_update(
	real delta);
void antenna_render(
	long object_index,
	long antenna_index,
	struct render_lighting const *lighting,
	struct render_animation const *animation);

/* ---------- globals */

extern struct data_array *antenna_data;

/* ---------- public code */

#endif // __ANTENNA_H
