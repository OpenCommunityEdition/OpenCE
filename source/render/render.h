/*
RENDER.H

header included in hcex build.
*/

#ifndef __RENDER_H
#define __RENDER_H
#pragma once

/* ---------- headers */

#include "render_cameras.h"
#include "structures/structures.h"
#include "structures/structure_render.h"

/* ---------- constants */

enum
{
	MAXIMUM_RENDERED_DISTANT_LIGHTS = 2,
	MAXIMUM_RENDERED_POINT_LIGHTS = 2,
	/* port: twice the Xbox's 16384 structure triangles a frame */
	MAXIMUM_RENDERED_ENVIRONMENT_SURFACES = 32768,
	MAXIMUM_RENDERED_CLUSTERS = 128,
	MAXIMUM_SURFACES_PER_STRUCTURE = 0x20000,
	MAXIMUM_RENDERED_LIGHTS = 128,
	/* the native builds' larger light pool (halo_port_capacity.h) */
	MAXIMUM_LIGHTS_PER_MAP = HALO_PORT_MAXIMUM_LIGHTS_PER_MAP,
	MAXIMUM_LENS_FLARES_PER_LIGHT = 8,
	MAXIMUM_QUEUED_LENS_FLARES = 8,
};

/* ---------- macros */

/* ---------- structures */

struct render_distant_light
{
	real_rgb_color color;
	real_vector3d direction;
};

struct render_lighting
{
	real_rgb_color ambient_color;
	short distant_light_count;
	word pad;
	struct render_distant_light distant_lights[MAXIMUM_RENDERED_DISTANT_LIGHTS];
	short point_light_count;
	word pad1;
	long point_light_indices[MAXIMUM_RENDERED_POINT_LIGHTS];
	real_argb_color reflection_tint_color;
	real_vector3d shadow_vector;
	real_rgb_color shadow_color;
};

/* HCEX and the PC-demo PDB type node_matrices as a non-const `real_matrix4x3 *`.
   It is declared const here because its users store const node-matrix pointers
   into it (transparent_geometry_group.node_matrices is const in HCEX as well);
   the qualifier changes no emitted byte, and the non-const spelling would add
   eleven C4090 const-qualifier warnings in five translation units. */
struct render_skinning
{
	real_matrix4x3 const *node_matrices;
	short node_matrix_count;
};


struct rendered_cluster
{
	short cluster_index;
	real_rectangle2d clip_bounds;
	struct render_frustum frustum;
};

struct render_globals
{
	long frame_index;
	long scene_index;
	short local_player_index;
	short window_index;
	real time_delta_since_tick_sec;
	struct render_camera camera;
	struct render_frustum frustum;
	struct render_fog fog;
	long leaf_index;
	long cluster_index;
	boolean under_water;
	boolean visible_sky_model;
	short visible_sky_index;

	// Bitvector of all clusters where bit set means the cluster is visible
	unsigned long visible_cluster_flags[MAXIMUM_CLUSTERS_PER_STRUCTURE / LONG_BITS];
	struct rendered_cluster rendered_clusters[MAXIMUM_RENDERED_CLUSTERS];
	short rendered_cluster_count;
	unsigned long environment_surface_flags[MAXIMUM_SURFACES_PER_STRUCTURE];
	/* port: a long, as a short would wrap at the raised count */
	long environment_surface_count;
	long environment_surface_indices[MAXIMUM_RENDERED_ENVIRONMENT_SURFACES];
};

/* ---------- prototypes/RENDER.C */

void render_effects(boolean enable);
void render_initialize(void);
void render_initialize_for_new_map(void);
void render_dispose_from_old_map(void);
void render_dispose(void);

void render_frame(
	struct render_window *windoze,
	short window_count,
	point2d const *screenshot_page_index,
	point2d const *screenshot_index,
	struct bitmap_data *screenshot_bitmap,
	real time_delta_since_tick_sec);
void render_frame_pregame(
	struct render_window const *window,
	struct bitmap_data *bitmap);
void render_frame_present(
	const point2d *screenshot_index,
	struct bitmap_data *bitmap);
boolean render_location_visible(
	struct location *location);
struct rendered_cluster *rendered_cluster_get(
	short rendered_cluster_index);

/* ---------- prototypes/RENDER_CONTRAILS.C */

void render_contrails_ground_mapped(
	void);
void render_contrails_media_mapped(
	void);
void render_contrails_normal(
	void);

/* ---------- prototypes/RENDER_OBJECTS.C */

void render_objects_initialize(
	void);
void render_objects_initialize_for_new_map(
	void);
void render_objects_dispose_from_old_map(
	void);
void render_objects_dispose(
	void);
struct render_lighting *object_get_cached_render_lighting(
	long object_index,
	real level_of_detail_pixels);
void render_objects(
	void);
void render_object_shadows(
	void);

/* ---------- globals */

extern struct render_globals render;
extern boolean render_particle_systems_enabled;

/* ---------- public code */

#endif // __RENDER_H
