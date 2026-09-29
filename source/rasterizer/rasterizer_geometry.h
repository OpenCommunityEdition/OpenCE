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
	NUMBER_OF_RASTERIZER_VERTEX_TYPES,
};

/* ---------- macros */

/* ---------- structures */

union real_vector3d;

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

/* Where a vertex or triangle buffer's data is. A map writes both of these
as addresses in the window the game was linked for, so a port that puts the
window somewhere else has to move them; where the window is where the game
expects it these are the fields themselves. A read of either has to go
through here, or the game hands the rasterizer a link-time address. Each
takes the buffer, which is a pointer. */
#ifdef HALO_ANDROID
void *vertex_buffer_base_address(struct vertex_buffer const *buffer);
void *vertex_buffer_hardware_format(struct vertex_buffer const *buffer);
void *triangle_buffer_base_address(struct triangle_buffer const *buffer);
void *triangle_buffer_hardware_format(struct triangle_buffer const *buffer);
#define VERTEX_BUFFER_BASE_ADDRESS(buffer) vertex_buffer_base_address(buffer)
#define VERTEX_BUFFER_HARDWARE_FORMAT(buffer) vertex_buffer_hardware_format(buffer)
#define TRIANGLE_BUFFER_BASE_ADDRESS(buffer) triangle_buffer_base_address(buffer)
#define TRIANGLE_BUFFER_HARDWARE_FORMAT(buffer) triangle_buffer_hardware_format(buffer)
#else
#define VERTEX_BUFFER_BASE_ADDRESS(buffer) ((buffer)->base_address)
#define VERTEX_BUFFER_HARDWARE_FORMAT(buffer) ((buffer)->hardware_format)
#define TRIANGLE_BUFFER_BASE_ADDRESS(buffer) ((buffer)->base_address)
#define TRIANGLE_BUFFER_HARDWARE_FORMAT(buffer) ((buffer)->hardware_format)
#endif

/* ---------- prototypes/RASTERIZER_GEOMETRY.C */

union real_vector3d uncompress_int32_to_real_vector3d(
	unsigned long compressed);

byte compress_real_to_int8(
	real value);

unsigned long compress_real_vector3d_to_int32_clamp(
	union real_vector3d const *vector);

long rasterizer_geometry_get_vertex_size(
	short type);

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
