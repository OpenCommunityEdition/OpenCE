/* ---------- headers */

#include "cseries.h"
#include "cseries/errors.h"
#include "math/real_math.h"
#include "rasterizer/rasterizer_model_types.h"
#include "rasterizer_geometry.h"
#include "rasterizer_geometry_compression.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct environment_vertex_uncompressed
{
	real_point3d position;
	real_vector3d normal;
	real_vector3d binormal;
	real_vector3d tangent;
	real_point2d texcoord;
};

struct environment_vertex_compressed
{
	real_point3d position;
	unsigned long normal;
	unsigned long binormal;
	unsigned long tangent;
	real_point2d texcoord;
};

struct environment_lightmap_vertex_uncompressed
{
	real_vector3d incident_radiosity;
	real_point2d texcoord;
};

struct environment_lightmap_vertex_compressed
{
	unsigned long incident_radiosity;
	short lightmap_u;
	short lightmap_v;
};

/* ---------- globals */

static short const rasterizer_vertex_type_sizes[]=
{
	56,
	32,
	20,
	8,
	68,
	32,
	24,
	36,
	20,
	16,
	16,
	8,
	0,
};

/* ---------- public code */

real uncompress_int8_to_real(
	byte value)
{
	return (real)value * (1.0f / 255.0f);
}

real uncompress_int16_to_real(
	short value)
{
	return ((real)value * 2.0f + 1.0f) * (1.0f / 65535.0f);
}

long rasterizer_geometry_get_vertex_size(
	short type)
{
	match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 170, type>=0 && type<NUMBER_OF_RASTERIZER_VERTEX_TYPES);

	return rasterizer_vertex_type_sizes[type];
}

void rasterizer_geometry_byte_swap_vertices(
	short type,
	long count,
	void *vertices,
	long buffer_size)
{
	return;
}

void rasterizer_geometry_uncompress_vertices(
	short type,
	long count,
	void *uncompressed,
	long uncompressed_size,
	void *compressed,
	long compressed_size)
{
	match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 280, uncompressed);
	match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 281, compressed);

	switch (type)
	{
		case _rasterizer_vertex_type_environment_compressed:
		{
			struct environment_vertex_uncompressed *dst= (struct environment_vertex_uncompressed *)uncompressed;
			struct environment_vertex_compressed const *src= (struct environment_vertex_compressed const *)compressed;
			long index;

			match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 287, count*sizeof(struct environment_vertex_uncompressed)==uncompressed_size);
			match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 288, count*sizeof(struct environment_vertex_compressed)==compressed_size);

			for (index=0; index<count; index++, dst++, src++)
			{
				dst->position= src->position;
				dst->normal= uncompress_int32_to_real_vector3d(src->normal);
				dst->binormal= uncompress_int32_to_real_vector3d(src->binormal);
				dst->tangent= uncompress_int32_to_real_vector3d(src->tangent);
				dst->texcoord= src->texcoord;
			}

			break;
		}

		case _rasterizer_vertex_type_environment_lightmap_compressed:
		{
			struct environment_lightmap_vertex_uncompressed *dst= (struct environment_lightmap_vertex_uncompressed *)uncompressed;
			struct environment_lightmap_vertex_compressed const *src= (struct environment_lightmap_vertex_compressed const *)compressed;
			long index;

			match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 307, count*sizeof(struct environment_lightmap_vertex_uncompressed)==uncompressed_size);
			match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 308, count*sizeof(struct environment_lightmap_vertex_compressed)==compressed_size);

			for (index=0; index<count; index++, dst++, src++)
			{
				dst->incident_radiosity= uncompress_int32_to_real_vector3d(src->incident_radiosity);
				dst->texcoord.x= ((real)src->lightmap_u * 2.0f + 1.0f) * (1.0f / 65535.0f);
				dst->texcoord.y= ((real)src->lightmap_v * 2.0f + 1.0f) * (1.0f / 65535.0f);
			}

			break;
		}

		case _rasterizer_vertex_type_model_compressed:
		{
			struct model_vertex_uncompressed *dst= (struct model_vertex_uncompressed *)uncompressed;
			struct model_vertex_compressed const *src= (struct model_vertex_compressed const *)compressed;
			long index;

			match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 325, count*sizeof(struct model_vertex_uncompressed)==uncompressed_size);
			match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 326, count*sizeof(struct model_vertex_compressed)==compressed_size);

			for (index=0; index<count; index++, dst++, src++)
			{
				dst->position= src->position;
				dst->normal= uncompress_int32_to_real_vector3d(src->normal);
				dst->binormal= uncompress_int32_to_real_vector3d(src->binormal);
				dst->tangent= uncompress_int32_to_real_vector3d(src->tangent);
				dst->texcoord.x= ((real)src->texcoord.x * 2.0f + 1.0f) * (1.0f / 65535.0f);
				dst->texcoord.y= ((real)src->texcoord.y * 2.0f + 1.0f) * (1.0f / 65535.0f);
				match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 341, src->nodes[0]%3==0);
				match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 342, src->nodes[1]%3==0);
				dst->nodes[0]= (short)(src->nodes[0]/3);
				dst->nodes[1]= (short)(src->nodes[1]/3);
				dst->node_weights[0]= (real)(byte)src->node_weight * (1.0f / 255.0f);
				dst->node_weights[1]= 1.0f - dst->node_weights[0];
			}

			break;
		}

		default:
			error(_error_silent, "### ERROR can't uncompress this type of vertex buffer");
			break;
	}

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
	match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 450, vertex);
	match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 451, normal);

	*normal = uncompress_int32_to_real_vector3d(vertex->normal);

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
	match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 474, vertex);
	match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 475, normal);

	*normal = uncompress_int32_to_real_vector3d(vertex->incident_radiosity);

	return;
}

void environment_lightmap_vertex_compressed_get_texcoord(
	struct environment_lightmap_vertex_compressed const *vertex,
	real_point2d *texcoord)
{
	match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 486, vertex);
	match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 487, texcoord);

	texcoord->n[0] = ((real)vertex->lightmap_u * 2.0f + 1.0f) * (1.0f / 65535.0f);
	texcoord->n[1] = ((real)vertex->lightmap_v * 2.0f + 1.0f) * (1.0f / 65535.0f);

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

unsigned long compress_real_vector3d_to_int32(
	real_vector3d const *v)
{
	long i, j, k;
	real_vector3d v2;

	match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 69, v);
	match_vassert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 78, v->i>=-1.0f && v->i<=1.0f && v->j>=-1.0f && v->j<=1.0f && v->k>=-1.0f && v->k<=1.0f, csprintf(temporary, "invalid vector= [%f %f %f] 0x%x%x%x", v->i, v->j, v->k, *((long *)&v->i), *((long *)&v->j), *((long *)&v->k)));

	i = fast_ftol((real)floor(v->i * 1023.5f)) & 0x7ff;
	j = fast_ftol((real)floor(v->j * 1023.5f)) & 0x7ff;
	k = fast_ftol((real)floor(v->k * 511.5f)) & 0x3ff;

	v2 = uncompress_int32_to_real_vector3d(((k << 11) | j) << 11 | i);

	match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 92, fabs(v2.i - v->i)<0.01f);
	match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 93, fabs(v2.j - v->j)<0.01f);
	match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 94, fabs(v2.k - v->k)<0.01f);

	return ((k << 11) | j) << 11 | i;
}

unsigned long compress_real_vector3d_to_int32_clamp(
	real_vector3d const *v)
{
	long i, j, k;
	real_vector3d v2;

	match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 104, v);

	i = fast_ftol((real)floor(PIN(v->i, -1.0f, 1.0f) * 1023.5f)) & 0x7ff;
	j = fast_ftol((real)floor(PIN(v->j, -1.0f, 1.0f) * 1023.5f)) & 0x7ff;
	k = fast_ftol((real)floor(PIN(v->k, -1.0f, 1.0f) * 511.5f)) & 0x3ff;

	v2 = uncompress_int32_to_real_vector3d(((k << 11) | j) << 11 | i);

	match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 118, fabs(v2.i - v->i)<0.01f);
	match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 119, fabs(v2.j - v->j)<0.01f);
	match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 120, fabs(v2.k - v->k)<0.01f);

	return ((k << 11) | j) << 11 | i;
}

void rasterizer_geometry_compress_vertices(
	short type,
	long count,
	void *compressed,
	long compressed_size,
	void *uncompressed,
	long uncompressed_size)
{
	match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 194, uncompressed);
	match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 195, compressed);

	switch (type)
	{
		case _rasterizer_vertex_type_environment_uncompressed:
		{
			struct environment_vertex_compressed *dst= (struct environment_vertex_compressed *)compressed;
			struct environment_vertex_uncompressed const *src= (struct environment_vertex_uncompressed const *)uncompressed;
			long index;

			match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 201, count*sizeof(struct environment_vertex_uncompressed)==uncompressed_size);
			match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 202, count*sizeof(struct environment_vertex_compressed)==compressed_size);

			for (index=0; index<count; index++, src++, dst++)
			{
				dst->position= src->position;
				dst->normal= compress_real_vector3d_to_int32_clamp(&src->normal);
				dst->binormal= compress_real_vector3d_to_int32_clamp(&src->binormal);
				dst->tangent= compress_real_vector3d_to_int32_clamp(&src->tangent);
				dst->texcoord= src->texcoord;
			}

			break;
		}

		case _rasterizer_vertex_type_environment_lightmap_uncompressed:
		{
			struct environment_lightmap_vertex_compressed *dst= (struct environment_lightmap_vertex_compressed *)compressed;
			struct environment_lightmap_vertex_uncompressed const *src= (struct environment_lightmap_vertex_uncompressed const *)uncompressed;
			long index;

			match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 221, count*sizeof(struct environment_lightmap_vertex_uncompressed)==uncompressed_size);
			match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 222, count*sizeof(struct environment_lightmap_vertex_compressed)==compressed_size);

			for (index=0; index<count; index++, src++, dst++)
			{
				dst->incident_radiosity= compress_real_vector3d_to_int32_clamp(&src->incident_radiosity);
				dst->lightmap_u= compress_real_to_int16_clamp(src->texcoord.x);
				dst->lightmap_v= compress_real_to_int16_clamp(src->texcoord.y);
			}

			break;
		}

		case _rasterizer_vertex_type_model_uncompressed:
		{
			struct model_vertex_compressed *dst= (struct model_vertex_compressed *)compressed;
			struct model_vertex_uncompressed const *src= (struct model_vertex_uncompressed const *)uncompressed;
			long index;

			match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 239, count*sizeof(struct model_vertex_uncompressed)==uncompressed_size);
			match_assert("c:\\halo\\SOURCE\\rasterizer\\rasterizer_geometry.c", 240, count*sizeof(struct model_vertex_compressed)==compressed_size);

			for (index=0; index<count; index++, src++, dst++)
			{
				dst->position= src->position;
				dst->normal= compress_real_vector3d_to_int32_clamp(&src->normal);
				dst->binormal= compress_real_vector3d_to_int32_clamp(&src->binormal);
				dst->tangent= compress_real_vector3d_to_int32_clamp(&src->tangent);
				dst->texcoord.x= compress_real_to_int16_clamp(src->texcoord.x);
				dst->texcoord.y= compress_real_to_int16_clamp(src->texcoord.y);
				dst->nodes[0]= (byte)(src->nodes[0]*3);
				dst->nodes[1]= (byte)(src->nodes[1]*3);
				dst->node_weight= compress_real_to_int16_clamp(src->node_weights[0]);
			}

			break;
		}

		default:
			error(_error_silent, "### ERROR can't compress this type of vertex buffer");
			break;
	}

	return;
}

/* ---------- private code */

real_vector3d uncompress_int32_to_real_vector3d(
	unsigned long compressed)
{
	real_vector3d v;
	real value = (real)(long)(compressed<<21);

	v.i = (value * (1.0f/1048576.0f) + 1.0f) * (1.0f/2047.0f);
	compressed >>= 11;
	value = (real)(long)(compressed<<21);
	v.j = (value * (1.0f/1048576.0f) + 1.0f) * (1.0f/2047.0f);
	compressed >>= 11;
	value = (real)(long)(compressed<<22);
	v.k = (value * (1.0f/2097152.0f) + 1.0f) * (1.0f/1023.0f);

	return v;
}
