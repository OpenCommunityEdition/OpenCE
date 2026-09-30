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
lights): the occlusion darkens only the lightmaps' share; 2: the objects
drawn, before the level (their pixels take the sun's traced shadows) */
void halo_ray_traced_light_stage(int stage);
/* whether the level's lightmaps are left out of the light buffer this
frame (the traced light is added in their place, after the light decals):
the lightmap pass draws the self-illumination alone, and the game's
unshadowed dynamic lights on the level are not drawn (the rays trace them) */
int halo_ray_traced_lightmaps_hidden(void);
/* an object's light, as the game samples it from the lightmap under it
(object_lights.c): the colour, the way it comes from and how much from that
way, replaced by the traced light's probe near the point, if there is one
(and a probe asked for there, for the next frame) */
void halo_ray_traced_object_lighting(const float *position, float *color, float *normal, float *accuracy);
/* after the dynamic lights, before the textures (render.c): the traced light
in place of the lightmaps' (display.ray_tracing_gi); the camera as for
halo_ray_traced_lighting */
void halo_ray_traced_light_buffer(float z_near, float z_far, float vertical_field_of_view, const float *position,
	const float *forward, const float *up);
/* F9 (sdl_platform.c) */
/* returns what it is now, as text */
const char *halo_ray_tracing_toggle(void);
/* F6: the next view (the lighting, the ray view, split, the occlusion);
returns its name */
const char *halo_ray_tracing_next_view(void);
/* F4: the objects' shapes in the rays next (the drawn models, the collision
models, ellipsoids); returns their name */
const char *halo_ray_tracing_shapes_next(void);
/* F5: the ray probe off, live (the crosshair's rays drawn), frozen; returns
what it is now */
const char *halo_ray_tracing_probe_next(void);
/* what it shows: 1 the lighting, 2 the occlusion, 3 the depth, 4 the ray
view, 5 split (tests) */
void halo_ray_tracing_debug_mode(int mode);

#endif
