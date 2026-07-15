/*
CONTRAILS.H

header included in hcex build.
*/

#ifndef __CONTRAILS_H
#define __CONTRAILS_H
#pragma once

/* ---------- headers */

#include "cseries/cseries.h"
#include "math/real_math.h"
#include "memory/data.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/CONTRAILS.C */

real contrail_scale_value(
	real value,
	real scale,
	unsigned long flags,
	char flag_bit);
void contrails_initialize(
	void);
void contrails_initialize_for_new_map(
	void);
void contrails_dispose_from_old_map(
	void);
void contrails_dispose(
	void);
void contrails_disconnect_from_structure_bsp(
	void);
long contrail_new(long definition_index, long object_index, short attachment_index);
void contrail_owner_collision(long contrail_index, unsigned char object_dying, real dt);

/* ---------- globals */

extern struct data_array *contrail_data;
extern struct data_array *contrail_point_data;

/* ---------- public code */

#endif // __CONTRAILS_H
