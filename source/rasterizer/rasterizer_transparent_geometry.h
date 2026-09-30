/*
RASTERIZER_TRANSPARENT_GEOMETRY.H

Narrow cross-translation-unit interface owned by RASTERIZER_TRANSPARENT_GEOMETRY.C.
*/

#ifndef __RASTERIZER_TRANSPARENT_GEOMETRY_H
#define __RASTERIZER_TRANSPARENT_GEOMETRY_H
#pragma once

#include "cseries.h"

struct transparent_geometry_group;

void rasterizer_transparent_geometry_groups_begin(
	void);
void rasterizer_transparent_geometry_groups_end(
	void);
void rasterizer_transparent_geometry_group_draw(
	struct transparent_geometry_group *group,
	boolean dirty);
void rasterizer_transparent_geometry_group_draw__internal(
	struct transparent_geometry_group const *group,
	boolean has_lightmap);

#endif /* __RASTERIZER_TRANSPARENT_GEOMETRY_H */
