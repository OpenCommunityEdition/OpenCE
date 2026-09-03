/*
RENDER_CAMERAS_INTERNAL.H

Narrow cross-translation-unit interface owned by RENDER_CAMERAS.C.
*/

#ifndef __RENDER_CAMERAS_INTERNAL_H
#define __RENDER_CAMERAS_INTERNAL_H
#pragma once

#include "render_cameras.h"

struct render_mirror;

real render_frustum_sphere_diameter_in_pixels(
	struct render_frustum const *frustum,
	real_point3d const *point,
	real radius);
void render_frustum_get_projection_bounds(
	struct render_frustum const *frustum,
	real_rectangle2d *bounds);
short render_frustum_sphere_visible(
	struct render_frustum const *frustum,
	real_point3d const *point,
	real radius);
short render_frustum_cube_visible(
	struct render_frustum const *frustum,
	real_rectangle3d const *bounds,
	boolean test_frustum_against_cube);
boolean render_frustum_triangle_visible(
	struct render_frustum const *frustum,
	real_point3d const *point0,
	real_point3d const *point1,
	real_point3d const *point2);
boolean render_camera_build_clipped_frustum_bounds(
	struct render_camera const *camera,
	real_rectangle2d const *clip,
	real_rectangle2d *frustum_bounds);
void render_camera_build_frustum_bounds(
	struct render_camera const *camera,
	real_rectangle2d *frustum_bounds);
void render_camera_mirror(
	struct render_camera const *camera,
	struct render_mirror const *mirror,
	struct render_camera *result);
boolean render_camera_view_to_screen(
	struct render_camera const *camera,
	struct render_frustum const *frustum,
	real_point3d const *view_point,
	real_point2d *screen_point);

#endif /* __RENDER_CAMERAS_INTERNAL_H */
