/*
ANIMATION_BLEND_SPACE.H

A blend between two animations whose poses are in different spaces (the
object's, or the world's for a world-relative animation) not drawn
(animation_blend_space.c).
*/

#ifndef __ANIMATION_BLEND_SPACE_H
#define __ANIMATION_BLEND_SPACE_H

/* ---------- prototypes */

/* none noted: at a map's start and before a game state is loaded */
void animation_blend_space_forget(
	void);
/* as an object's pose is computed (object_compute_node_matrices): whether
in the world's space */
void animation_blend_space_computed(
	long object_index,
	boolean world_relative);
/* as an object's blend starts from the pose last computed
(object_start_interpolation) */
void animation_blend_space_started(
	long object_index);
/* whether the blend the object is in started from a pose in the other
space than an animation world relative or not */
boolean animation_blend_space_crossed(
	long object_index,
	boolean world_relative);

#endif
