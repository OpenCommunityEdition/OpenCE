/*
RAYTRACE_GL.H

Screen-space ray-traced lighting (raytrace_gl.c, display.ray_tracing).
*/

#ifndef __HALO_RAYTRACE_GL_H
#define __HALO_RAYTRACE_GL_H

/* after a window's opaque world is drawn (source/render/render.c): its
camera's clip planes and vertical field of view (radians) */
void halo_ray_traced_lighting(float z_near, float z_far, float vertical_field_of_view);
/* F9 (sdl_platform.c) */
void halo_ray_tracing_toggle(void);
/* what it shows: 1 the lighting, 2 the occlusion, 3 the depth (tests) */
void halo_ray_tracing_debug_mode(int mode);

#endif
