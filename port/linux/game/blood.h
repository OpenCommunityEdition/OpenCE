/*
BLOOD.H

Dynamic blood (port/linux/game/blood.c): a hit throws blood onto the wall
behind, and blood on a wall runs down it.
*/

#ifndef __BLOOD_H
#define __BLOOD_H
#pragma once

/* built into every port build; on or off by Video Settings' Dynamic Blood
(config.toml's game.dynamic_blood, off by default) */
#ifndef BLOOD_EFFECTS
#define BLOOD_EFFECTS 1
#endif

#if BLOOD_EFFECTS

struct damage_data;

struct blood_parameters
{
	real enabled; /* 1: as game.dynamic_blood says; 0: off */
	real amount; /* times as much (and as large) */
	real drips; /* 1: blood on walls runs */
	real drip_length; /* times as long */
};

extern struct blood_parameters blood_parameters;

/* from decals_initialize_for_new_map */
void blood_initialize_for_new_map(
	void);

/* from decals_update, each tick: drips running */
void blood_update(
	void);

/* from render_window, each frame before the decals are drawn: running
drips drawn out to where they are between ticks */
void blood_render_update(
	void);

/* from decal_new_from_collision: a decal landed on the level (blood on a
wall starts running) */
void blood_decal_landed(
	long decal_definition_index,
	real_point3d const *point,
	real_vector3d const *normal,
	real radius,
	short cluster_index);

/* from decals.c: blood lands on the level's two-sided surfaces too (a
tree's trunk and leaves, a fence), as on any other */
boolean blood_decal_on_two_sided(
	long decal_definition_index);

/* drops at runs' heads and falling (with Dynamic Blood on) */
boolean blood_drops_animated(
	void);

/* blood_drops.c: the drops to draw this frame (a run's head, a drop falling):
where, which way (a surface's normal, or the way it falls), how big, and the
blood's splat (its species, its colour) */
short blood_drops_for_drawing(
	real_point3d *positions,
	real_vector3d *directions,
	real *radii,
	long *decals,
	short maximum);

/* render_particles.c: the drops drawn, after the game's particles
(blood_drops.c) */
void blood_drops_render(
	void);

/* from object_damage_aftermath: a biped hit (blood on the wall or slope behind it) */
void blood_biped_damaged(
	long biped_index,
	struct damage_data const *damage,
	real damage_amount);

#endif // BLOOD_EFFECTS

#endif // __BLOOD_H
