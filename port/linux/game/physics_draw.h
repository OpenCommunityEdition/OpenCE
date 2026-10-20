/*
PHYSICS_DRAW.H

The host's Jolt bodies drawn as wireframes over the game
(debug.physics_draw, physics_draw.c).
*/

#ifndef __PHYSICS_DRAW_H
#define __PHYSICS_DRAW_H

struct ragdoll_pose;

/* whether the bodies are drawn (debug.physics_draw, with the host's
physics on) */
boolean physics_draw_enabled(void);
/* a shape (HOST_PHYSICS_SHAPE_*) of a size, centred at a point in a pose's
frame, in debug lines */
void physics_draw_shape(struct ragdoll_pose const *pose, short shape, real const *size,
	real_point3d const *center, real_argb_color const *color);
/* (render_debug.c, each frame) every ragdoll's parts and piece of debris */
void physics_draw_render(void);

#endif
