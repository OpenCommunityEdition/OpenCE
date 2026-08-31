/*
RASTERIZER_GEOMETRY.H

header included in hcex build.
*/

#ifndef __RASTERIZER_GEOMETRY_H
#define __RASTERIZER_GEOMETRY_H
#pragma once

/* ---------- constants */

enum
{
	_rasterizer_vertex_type_environment_uncompressed = 0,
	_rasterizer_vertex_type_environment_compressed,
	_rasterizer_vertex_type_environment_lightmap_uncompressed,
	_rasterizer_vertex_type_environment_lightmap_compressed,
	_rasterizer_vertex_type_model_uncompressed,
	_rasterizer_vertex_type_model_compressed,
	_rasterizer_vertex_type_dynamic_unlit,
	_rasterizer_vertex_type_dynamic_lit,
	_rasterizer_vertex_type_dynamic_screen,
	_rasterizer_vertex_type_debug,
	_rasterizer_vertex_type_decal,
	_rasterizer_vertex_type_detail_object,
	_rasterizer_vertex_type_environment_uncompressed_ff,
	_rasterizer_vertex_type_environment_lightmap_uncompressed_ff,
	_rasterizer_vertex_type_model_uncompressed_ff,
	_rasterizer_vertex_type_model_processed,
	_rasterizer_vertex_type_unlit_zsprite,
	_rasterizer_vertex_type_widget,
	NUMBER_OF_RASTERIZER_VERTEX_TYPES,
};

/* ---------- macros */

/* ---------- structures */

struct vertex_buffer
{
	short type;
	word pad;
	long count;
	long offset;
	void *base_address;
	void *hardware_format;
};

enum
{
	_triangle_buffer_type_triangles,
	_triangle_buffer_type_precompiled_strip,
	NUMBER_OF_TRIANGLE_BUFFER_TYPES,
};

struct triangle_buffer
{
	short type;
	word pad;
	long count;
	void *base_address;
	void *hardware_format;
};

/* ---------- prototypes/EXAMPLE.C */

void rasterizer_geometry_uncompress_vertices(
	short type,
	long count,
	void *uncompressed,
	long uncompressed_size,
	void *compressed,
	long compressed_size);

void rasterizer_geometry_compress_vertices(
	short type,
	long count,
	void *compressed,
	long compressed_size,
	void *uncompressed,
	long uncompressed_size);

/* ---------- globals */

/* ---------- public code */

#endif // __RASTERIZER_GEOMETRY_H
