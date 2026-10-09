/*
RASTERIZER_TRANSPARENT_GEOMETRY.H

Narrow cross-translation-unit interface owned by RASTERIZER_TRANSPARENT_GEOMETRY.C.
*/

#ifndef __RASTERIZER_TRANSPARENT_GEOMETRY_H
#define __RASTERIZER_TRANSPARENT_GEOMETRY_H
#pragma once

#include "cseries.h"

struct transparent_geometry_group;

/* port: static enclosure recognition, used when model tags load. */
struct shader;
struct vertex_buffer;
struct triangle_buffer;
boolean rasterizer_transparent_geometry_is_enclosure(
	struct shader const *glass, struct vertex_buffer const *outer,
	struct triangle_buffer const *triangles,
	struct shader const *energy, struct vertex_buffer const *inner);

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

/* port: refine centroid sorting using planar BSP glass and model bounds. */
void rasterizer_transparent_geometry_order_models(short *order, long count);

#endif /* __RASTERIZER_TRANSPARENT_GEOMETRY_H */
