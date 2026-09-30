/*
RAYTRACE_GL.H

Screen-space ray-traced lighting (raytrace_gl.c, display.ray_tracing).
*/

#ifndef __HALO_RAYTRACE_GL_H
#define __HALO_RAYTRACE_GL_H

/* after a window's opaque world is drawn (source/render/render.c): its
camera's clip planes, vertical field of view (radians), and position,
forward and up vectors in the world (3 floats each) */
void halo_ray_traced_lighting(float z_near, float z_far, float vertical_field_of_view, const float *position,
	const float *forward, const float *up);
/* the light in the window before and after the dynamic lights (0: the
lightmaps, 1: with the flashlight's, the plasma's and the other dynamic
lights): the occlusion darkens only the lightmaps' share */
void halo_ray_traced_light_stage(int stage);
/* F9 (sdl_platform.c) */
/* returns what it is now, as text */
const char *halo_ray_tracing_toggle(void);
/* what it shows: 1 the lighting, 2 the occlusion, 3 the depth (tests) */
void halo_ray_tracing_debug_mode(int mode);

#endif
