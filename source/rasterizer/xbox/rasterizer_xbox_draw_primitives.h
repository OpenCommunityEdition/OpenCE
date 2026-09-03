/*
RASTERIZER_XBOX_DRAW_PRIMITIVES.H
*/

#ifndef __RASTERIZER_XBOX_DRAW_PRIMITIVES_H
#define __RASTERIZER_XBOX_DRAW_PRIMITIVES_H
#pragma once

/* ---------- prototypes/RASTERIZER_XBOX_DRAW_PRIMITIVES.C */

void rasterizer_draw_dynamic_vertices(
	long first_primitive_index,
	long primitive_count,
	long dynamic_vertex_buffer_index,
	short vertices_per_primitive);

#endif // __RASTERIZER_XBOX_DRAW_PRIMITIVES_H
