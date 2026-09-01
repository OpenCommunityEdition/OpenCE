/*
RENDER_CAMERAS_INTERNAL.H

Narrow cross-translation-unit interface owned by RENDER_CAMERAS.C.
*/

#ifndef __RENDER_CAMERAS_INTERNAL_H
#define __RENDER_CAMERAS_INTERNAL_H
#pragma once

#include "render_cameras.h"

real render_frustum_sphere_diameter_in_pixels(
	struct render_frustum const *frustum,
	real_point3d const *point,
	real radius);

#endif /* __RENDER_CAMERAS_INTERNAL_H */
