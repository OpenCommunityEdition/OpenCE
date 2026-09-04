/*
RASTERIZER_FRAME_STATISTICS.H

Narrow cross-translation-unit interface owned by RASTERIZER_FRAME_STATISTICS.C.
*/

#ifndef __RASTERIZER_FRAME_STATISTICS_H
#define __RASTERIZER_FRAME_STATISTICS_H
#pragma once

struct triangle_buffer;
struct vertex_buffer;
struct rasterizer_frame_statistics_globals;

void rasterizer_frame_statistics_get_fps(
	struct rasterizer_frame_statistics_globals *frame_statistics);
void rasterizer_frame_statistics_draw(
	void);

long rasterizer_frame_statistics_count_static_vertices(
	struct triangle_buffer const *triangle_buffer,
	struct vertex_buffer const *vertex_buffer);
long rasterizer_frame_statistics_count_dynamic_vertices(
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count);

#endif /* __RASTERIZER_FRAME_STATISTICS_H */
