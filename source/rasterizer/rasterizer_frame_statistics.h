/*
RASTERIZER_FRAME_STATISTICS.H

Narrow cross-translation-unit interface owned by RASTERIZER_FRAME_STATISTICS.C.
*/

#ifndef __RASTERIZER_FRAME_STATISTICS_H
#define __RASTERIZER_FRAME_STATISTICS_H
#pragma once

long rasterizer_frame_statistics_count_dynamic_vertices(
	long dynamic_triangle_buffer_index,
	long first_triangle_index,
	long triangle_count);

#endif /* __RASTERIZER_FRAME_STATISTICS_H */
