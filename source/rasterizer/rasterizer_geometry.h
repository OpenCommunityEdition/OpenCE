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

#ifdef HALO_LINUX
/* ---------- prototypes/RASTERIZER_XBOX_HARDWARE_GEOMETRY.C */

/* The native builds make buffers for the geometry of Halo Custom Edition
maps, which Xbox caches carry ready made (port/linux/game/
custom_edition_geometry.c). Nothing in January calls these, and this header
is part of the byte-matched build, so they are declared for the native
builds only. */
boolean rasterizer_vertex_buffer_new(
	struct vertex_buffer *vertex_buffer,
	long vertex_type,
	long count,
	void const *vertices,
	long buffer_size);
void rasterizer_vertex_buffer_delete(
	struct vertex_buffer *vertex_buffer);
boolean rasterizer_triangle_buffer_new(
	struct triangle_buffer *triangle_buffer,
	short triangle_type,
	long count,
	void const *triangles);
void rasterizer_triangle_buffer_delete(
	struct triangle_buffer *triangle_buffer);
#endif

/* ---------- globals */

/* ---------- public code */

#endif // __RASTERIZER_GEOMETRY_H
