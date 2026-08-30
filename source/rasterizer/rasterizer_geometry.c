/*
RASTERIZER_GEOMETRY.C

symbols in this file:
0016F7D0 0020:
	_uncompress_int8_to_real (0000)
0016F7F0 0020:
	_uncompress_int16_to_real (0000)
0016F810 0090:
	_uncompress_int32_to_real_vector3d (0000)
0016F8A0 0050:
	_rasterizer_geometry_get_vertex_size (0000)
0016F8F0 0010:
	_rasterizer_geometry_byte_swap_vertices (0000)
0016F900 0440:
	_rasterizer_geometry_uncompress_vertices (0000)
0016FD40 0010:
	_rasterizer_geometry_stripify (0000)
0016FD50 0070:
	_environment_vertex_compressed_get_point (0000)
0016FDC0 0080:
	_environment_vertex_compressed_get_normal (0000)
0016FE40 0070:
	_environment_vertex_compressed_get_texcoord (0000)
0016FEB0 0080:
	_environment_lightmap_vertex_compressed_get_incident_radiosity (0000)
0016FF30 0090:
	_environment_lightmap_vertex_compressed_get_texcoord (0000)
0016FFC0 0060:
	_compress_real_to_int8 (0000)
00170020 0050:
	_compress_real_to_int8_clamp (0000)
00170070 0070:
	_compress_real_to_int16 (0000)
001700E0 0060:
	_compress_real_to_int16_clamp (0000)
00170140 0220:
	_compress_real_vector3d_to_int32 (0000)
00170360 0200:
	_compress_real_vector3d_to_int32_clamp (0000)
00170560 02e0:
	_rasterizer_geometry_compress_vertices (0000)
0029E344 001a:
	_rdata_0029e344 (0000)
0029E360 0004:
	__real@35000000 (0000)
0029E364 0004:
	__real@3a001002 (0000)
0029E368 0030:
	??_C@_0DA@GFMENBNG@c?3?2halo?2SOURCE?2rasterizer?2raster@ (0000)
0029E398 0044:
	??_C@_0EE@PNJAPPJM@count?$CKsizeof?$CIstruct?5environment_@ (0000)
0029E3E0 0048:
	??_C@_0EI@DONGBPFL@count?$CKsizeof?$CIstruct?5environment_@ (0000)
0029E428 004d:
	??_C@_0EN@HJINING@count?$CKsizeof?$CIstruct?5environment_@ (0000)
0029E478 0051:
	??_C@_0FB@GCMMGGGN@count?$CKsizeof?$CIstruct?5environment_@ (0000)
0029E4CC 0013:
	??_C@_0BD@OPFHNGDK@src?9?$DOnodes?$FL1?$FN?$CF3?$DN?$DN0?$AA@ (0000)
0029E4E0 0013:
	??_C@_0BD@CDPNNGKE@src?9?$DOnodes?$FL0?$FN?$CF3?$DN?$DN0?$AA@ (0000)
0029E4F4 003e:
	??_C@_0DO@MIHGPCNE@count?$CKsizeof?$CIstruct?5model_vertex@ (0000)
0029E538 0042:
	??_C@_0EC@DGDJGMJD@count?$CKsizeof?$CIstruct?5model_vertex@ (0000)
0029E57C 0036:
	??_C@_0DG@CBADPEAN@?$CD?$CD?$CD?5ERROR?5can?8t?5uncompress?5this?5@ (0000)
0029E5B4 000d:
	??_C@_0N@NFCOPMLF@uncompressed?$AA@ (0000)
0029E5C4 0009:
	??_C@_08FDJGCBLL@texcoord?$AA@ (0000)
0029E5D0 0013:
	??_C@_0BD@GHLLAJAG@z?$DO?$DN0?40f?5?$CG?$CG?5z?$DM?$DN1?40f?$AA@ (0000)
0029E5E4 0004:
	__real@46ffff00 (0000)
0029E5E8 0014:
	??_C@_0BE@NNGOJEKG@z?$DO?$DN?91?40f?5?$CG?$CG?5z?$DM?$DN1?40f?$AA@ (0000)
0029E5FC 0018:
	??_C@_0BI@PFBMHOJF@fabs?$CIv2?4k?5?9?5v?9?$DOk?$CJ?$DM0?401f?$AA@ (0000)
0029E614 0018:
	??_C@_0BI@EMAPPLEH@fabs?$CIv2?4j?5?9?5v?9?$DOj?$CJ?$DM0?401f?$AA@ (0000)
0029E62C 0018:
	??_C@_0BI@FMEKHDHA@fabs?$CIv2?4i?5?9?5v?9?$DOi?$CJ?$DM0?401f?$AA@ (0000)
0029E644 0004:
	__real@43ffc000 (0000)
0029E648 0004:
	__real@447fe000 (0000)
0029E64C 0024:
	??_C@_0CE@BAFANFAL@invalid?5vector?$DN?5?$FL?$CFf?5?$CFf?5?$CFf?$FN?50x?$CFx?$CF@ (0000)
0029E670 0034:
	??_C@_0DE@MCFDJBBN@?$CD?$CD?$CD?5ERROR?5can?8t?5compress?5this?5ty@ (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "math/real_math.h"
#include "rasterizer_geometry.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct environment_vertex_compressed
{
	real_point3d position;
	unsigned long normal;
	unsigned long binormal;
	unsigned long tangent;
	real_point2d texcoord;
};

struct environment_lightmap_vertex_compressed
{
	unsigned long incident_radiosity;
	short lightmap_u;
	short lightmap_v;
};

/* ---------- prototypes */

real_vector3d *uncompress_int32_to_real_vector3d(
	real_vector3d *result,
	unsigned long compressed);

/* Bungie's own float-to-long conversion helper, recovered verbatim from the
   historical cseries.h.  January inlines it at every conversion below;
   ordinary C conversions lower through a 64-bit fistp under this compiler
   and cannot reproduce those chains.  Admitted by owner ruling 2026-08-30,
   kept unit-local so a shared-header __inline cannot perturb other units. */
static __inline long fast_ftol(
	float d)
{
	long result;

	__asm
	{
		fld d
		fistp result
	}

	return result;
}

/* ---------- globals */

/* ---------- public code */

float uncompress_int8_to_real(
	byte value)
{
	return (float)value * (1.0f / 255.0f);
}

float uncompress_int16_to_real(
	short value)
{
	return ((float)value * 2.0f + 1.0f) * (1.0f / 65535.0f);
}

void rasterizer_geometry_byte_swap_vertices(
	short type,
	long count,
	void *vertices,
	long buffer_size)
{
	return;
}

boolean rasterizer_geometry_stripify(
	struct triangle_buffer *triangle_buffer,
	struct vertex_buffer *vertex_buffer)
{
	return TRUE;
}

void environment_vertex_compressed_get_point(
	struct environment_vertex_compressed const *vertex,
	real_point3d *point)
{
	match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 438, vertex);
	match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 439, point);

	*point = vertex->position;

	return;
}

void environment_vertex_compressed_get_normal(
	struct environment_vertex_compressed const *vertex,
	real_vector3d *normal)
{
	real_vector3d decompressed_normal;

	match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 450, vertex);
	match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 451, normal);

	*normal = *uncompress_int32_to_real_vector3d(&decompressed_normal, vertex->normal);

	return;
}

void environment_vertex_compressed_get_texcoord(
	struct environment_vertex_compressed const *vertex,
	real_point2d *texcoord)
{
	match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 462, vertex);
	match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 463, texcoord);

	*texcoord = vertex->texcoord;

	return;
}

void environment_lightmap_vertex_compressed_get_incident_radiosity(
	struct environment_lightmap_vertex_compressed const *vertex,
	real_vector3d *normal)
{
	real_vector3d decompressed_normal;

	match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 474, vertex);
	match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 475, normal);

	*normal = *uncompress_int32_to_real_vector3d(&decompressed_normal, vertex->incident_radiosity);

	return;
}

void environment_lightmap_vertex_compressed_get_texcoord(
	struct environment_lightmap_vertex_compressed const *vertex,
	real_point2d *texcoord)
{
	match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 486, vertex);
	match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 487, texcoord);

	texcoord->n[0] = ((float)vertex->lightmap_u * 2.0f + 1.0f) * (1.0f / 65535.0f);
	texcoord->n[1] = ((float)vertex->lightmap_v * 2.0f + 1.0f) * (1.0f / 65535.0f);

	return;
}


byte compress_real_to_int8(
	real z)
{
	match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 42, z>=0.0f && z<=1.0f);

	z = z * 255.0f;

	return (byte)fast_ftol(z);
}

byte compress_real_to_int8_clamp(
	real z)
{
	z = PIN(z, 0.0f, 1.0f) * 255.0f;

	return (byte)fast_ftol(z);
}

short compress_real_to_int16(
	real z)
{
	match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 55, z>=-1.0f && z<=1.0f);

	z = (real)floor(z * 32767.5f);

	return (short)fast_ftol(z);
}

short compress_real_to_int16_clamp(
	real z)
{
	z = (real)floor(PIN(z, -1.0f, 1.0f) * 32767.5f);

	return (short)fast_ftol(z);
}

/* ---------- private code */
