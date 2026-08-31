/*
PROJECTILES.H

header included in hcex build.
*/

#ifndef __PROJECTILES_H
#define __PROJECTILES_H
#pragma once

#include "items/items.h"

/* ---------- constants */

/* ---------- macros */

#define projectile_get(index) \
	((struct projectile_datum *)object_get_and_verify_type( \
		(index), \
		_object_mask_projectile))

/* ---------- structures */

struct _projectile_datum
{
	unsigned long flags;
	short action;
	short hit_material_type;
	long ignore_object_index;
	long target_object_index;
	long tracer_attachment_index;
	real detonation_timer;
	real detonation_timer_delta;
	real arming_time;
};

struct projectile_datum
{
	long definition_index;
	struct _object_datum object;
	struct _item_datum item;
	struct _projectile_datum projectile;
};

typedef char projectile_target_object_index_offset_assert[
	offsetof(struct projectile_datum, projectile.target_object_index) == 0x1E8
		? 1
		: -1];
typedef char projectile_detonation_timer_offset_assert[
	offsetof(struct projectile_datum, projectile.detonation_timer) == 0x1F0
		? 1
		: -1];
typedef char projectile_arming_time_offset_assert[
	offsetof(struct projectile_datum, projectile.arming_time) == 0x1F8
		? 1
		: -1];

/* ---------- prototypes/PROJECTILES.C */

void projectiles_initialize(
	void);
void projectiles_initialize_for_new_map(
	void);
void projectiles_dispose_from_old_map(
	void);
void projectiles_dispose(
	void);
void projectiles_delete_all(
	void);
void projectile_delete(
	long projectile_index);
boolean dangerous_projectiles_near_player(
	void);
void projectile_set_target_object_index(
	long projectile_index,
	long target_object_index);
void projectile_accelerate(long projectile_index, union real_vector3d const *acceleration);

/* ---------- globals */

/* ---------- public code */

#endif // __PROJECTILES_H
