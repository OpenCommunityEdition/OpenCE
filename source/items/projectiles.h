/*
PROJECTILES.H

header included in hcex build.
*/

#ifndef __PROJECTILES_H
#define __PROJECTILES_H
#pragma once

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

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
void projectile_accelerate(long projectile_index, union real_vector3d const *acceleration);

/* ---------- globals */

/* ---------- public code */

#endif // __PROJECTILES_H
