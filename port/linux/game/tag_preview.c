/*
 * A software preview of the tag highlighted in the Forge Tag Browser, drawn
 * into a bitmap_data shown on the screen's tag_preview widget. The tag's first
 * LOD triangles are centred and scaled to fit, then drawn filled with diffuse
 * and rim lighting as they turn.
 */

#ifndef HALO_RELEASE
#define HALO_RELEASE
#endif
#include "cseries.h"
#include "math/real_math.h"
#include <stdlib.h>
#include <string.h>

#include "bitmaps/bitmaps.h"
#include "bitmaps/bitmap_group.h"
#include "cache/texture_cache.h"
#include "tag_preview.h"

/* Mesh buffers are allocated with standard CRT calloc/malloc in forge_runtime_tags.c */
#undef malloc
#undef free
#undef realloc

enum
{
	BITMAP_DATA_SIGNATURE = 'bitm',
	_bitmap_type_2d = 0,
	_bitmap_format_a8 = 0,
	_bitmap_format_y8,
	_bitmap_format_ay8,
	_bitmap_format_a8y8,
	_bitmap_format_unused1,
	_bitmap_format_unused2,
	_bitmap_format_r5g6b5,
	_bitmap_format_unused3,
	_bitmap_format_a1r5g5b5,
	_bitmap_format_a4r4g4b4,
	_bitmap_format_x8r8g8b8,
	_bitmap_format_a8r8g8b8,
	_bitmap_format_unused4,
	_bitmap_format_unused5,
	_bitmap_format_dxt1,
	_bitmap_format_dxt3,
	_bitmap_format_dxt5,
	_bitmap_format_p8_bump,
	_bitmap_linear_bit = 4
};

extern int forge_ui_build_tag_preview_mesh(
	int is_installed_mode,
	int category_tab,
	int list_index,
	struct tag_injector_mesh *mesh);

static struct bitmap_data preview_bitmap;
static unsigned int preview_pixels[TAG_PREVIEW_WIDTH * TAG_PREVIEW_HEIGHT];
static float preview_depth[TAG_PREVIEW_WIDTH * TAG_PREVIEW_HEIGHT];
static boolean preview_bitmap_ready;

static int selected_mode = -1;
static int selected_cat = -1;
static int selected_index = -1;
static int selected_revision = -1;
static int loaded_mode = -1;
static int loaded_cat = -1;
static int loaded_index = -1;
static int loaded_revision = -1;
static struct tag_injector_mesh mesh;
static float mesh_centre[3];
static float mesh_radius;
static unsigned int turn_Step;

void tag_preview_mesh_free(struct tag_injector_mesh *m)
{
	long i;

	if (!m)
		return;
	if (m->positions)
		free(m->positions);
	if (m->normals)
		free(m->normals);
	if (m->texcoords)
		free(m->texcoords);
	if (m->triangles)
		free(m->triangles);
	if (m->triangle_materials)
		free(m->triangle_materials);
	if (m->materials)
		free(m->materials);
	if (m->textures)
	{
		for (i = 0; i < m->texture_count; i++)
		{
			if (m->textures[i].pixels)
				free(m->textures[i].pixels);
		}
		free(m->textures);
	}
	memset(m, 0, sizeof(*m));
}

static void drop_mesh(void)
{
	if (preview_bitmap_ready)
	{
		bitmap_delete(&preview_bitmap);
		memset(&preview_bitmap, 0, sizeof(preview_bitmap));
		preview_bitmap_ready = FALSE;
	}
	tag_preview_mesh_free(&mesh);
	loaded_mode = -1;
	loaded_cat = -1;
	loaded_index = -1;
	loaded_revision = -1;
	mesh_radius = 0.0f;
}

void tag_preview_select(int mode, int category_tab, int list_index, int revision)
{
	selected_mode = mode;
	selected_cat = category_tab;
	selected_index = list_index;
	selected_revision = revision;
}

void tag_preview_clear(void)
{
	selected_mode = -1;
	selected_cat = -1;
	selected_index = -1;
	selected_revision = -1;
	drop_mesh();
}

static void measure_mesh(void)
{
	float min_v[3], max_v[3];
	long i, k;
	float r2 = 0.0f;

	if (mesh.vertex_count <= 0)
	{
		mesh_radius = 0.0f;
		return;
	}
	for (k = 0; k < 3; k++)
		min_v[k] = max_v[k] = mesh.positions[0][k];
	for (i = 1; i < mesh.vertex_count; i++)
	{
		for (k = 0; k < 3; k++)
		{
			if (mesh.positions[i][k] < min_v[k])
				min_v[k] = mesh.positions[i][k];
			if (mesh.positions[i][k] > max_v[k])
				max_v[k] = mesh.positions[i][k];
		}
	}
	for (k = 0; k < 3; k++)
		mesh_centre[k] = 0.5f * (min_v[k] + max_v[k]);
	for (i = 0; i < mesh.vertex_count; i++)
	{
		float dx = mesh.positions[i][0] - mesh_centre[0];
		float dy = mesh.positions[i][1] - mesh_centre[1];
		float dz = mesh.positions[i][2] - mesh_centre[2];
		float d2 = dx * dx + dy * dy + dz * dz;

		if (d2 > r2)
			r2 = d2;
	}
	mesh_radius = (float)sqrt(r2);
}

static void load_selected(void)
{
	if (loaded_mode == selected_mode &&
		loaded_cat == selected_cat &&
		loaded_index == selected_index &&
		loaded_revision == selected_revision)
		return;
	drop_mesh();
	loaded_mode = selected_mode;
	loaded_cat = selected_cat;
	loaded_index = selected_index;
	loaded_revision = selected_revision;
	turn_Step = 0;
	if (selected_mode < 0 || selected_cat < 0 || selected_index < 0)
		return;
	if (forge_ui_build_tag_preview_mesh(selected_mode, selected_cat, selected_index, &mesh))
		measure_mesh();
}

static void project_vertex(
	const float v[3],
	float ca,
	float sa,
	float ct,
	float st,
	float scale,
	float cx,
	float cy,
	float out[3])
{
	float x = (v[0] - mesh_centre[0]) * scale;
	float y = (v[1] - mesh_centre[1]) * scale;
	float z = (v[2] - mesh_centre[2]) * scale;
	float rx = x * ca - y * sa;
	float ry = x * sa + y * ca;
	float rz = z;
	float ty = ry * ct - rz * st;
	float tz = ry * st + rz * ct;

	out[0] = cx + rx;
	out[1] = cy - tz;
	out[2] = ty;
}

static void unpack_rgb565(unsigned int c, unsigned int rgb[3])
{
	rgb[0] = ((c >> 11) & 31u) * 255u / 31u;
	rgb[1] = ((c >> 5) & 63u) * 255u / 63u;
	rgb[2] = (c & 31u) * 255u / 31u;
}

static void sample_dxt_color(const unsigned char *block, int x, int y, unsigned int rgb[3], unsigned int *alpha_out)
{
	unsigned int c0 = (unsigned int)block[0] | ((unsigned int)block[1] << 8);
	unsigned int c1 = (unsigned int)block[2] | ((unsigned int)block[3] << 8);
	unsigned int bits = (unsigned int)block[4] |
		((unsigned int)block[5] << 8) |
		((unsigned int)block[6] << 16) |
		((unsigned int)block[7] << 24);
	unsigned int code = (bits >> (2 * (y * 4 + x))) & 3u;
	unsigned int a[3], b[3], k;

	unpack_rgb565(c0, a);
	unpack_rgb565(c1, b);
	*alpha_out = 255u;
	if (code == 0)
	{
		for (k = 0; k < 3; k++)
			rgb[k] = a[k];
	}
	else if (code == 1)
	{
		for (k = 0; k < 3; k++)
			rgb[k] = b[k];
	}
	else if (c0 > c1)
	{
		for (k = 0; k < 3; k++)
			rgb[k] = (code == 2) ? (2u * a[k] + b[k]) / 3u : (a[k] + 2u * b[k]) / 3u;
	}
	else if (code == 2)
	{
		for (k = 0; k < 3; k++)
			rgb[k] = (a[k] + b[k]) / 2u;
	}
	else
	{
		rgb[0] = rgb[1] = rgb[2] = 0;
		*alpha_out = 0;
	}
}

static void sample_texture(
	const struct tag_injector_texture *t,
	float u,
	float v,
	unsigned int rgba[4])
{
	int x, y;
	unsigned int p;

	u -= (float)floor(u);
	v -= (float)floor(v);
	x = (int)(u * (float)t->width);
	y = (int)(v * (float)t->height);
	if (x < 0)
		x = 0;
	else if (x >= t->width)
		x = t->width - 1;
	if (y < 0)
		y = 0;
	else if (y >= t->height)
		y = t->height - 1;

	rgba[3] = 255u;
	switch (t->format)
	{
	case _bitmap_format_a8:
		p = t->pixels[y * t->width + x];
		rgba[0] = rgba[1] = rgba[2] = 255u;
		rgba[3] = p;
		break;
	case _bitmap_format_y8:
	case _bitmap_format_ay8:
	case _bitmap_format_p8_bump:
		p = t->pixels[y * t->width + x];
		rgba[0] = rgba[1] = rgba[2] = p;
		if (t->format == _bitmap_format_ay8)
			rgba[3] = p;
		break;
	case _bitmap_format_a8y8:
		p = (unsigned int)((y * t->width + x) * 2);
		rgba[0] = rgba[1] = rgba[2] = t->pixels[p];
		rgba[3] = t->pixels[p + 1];
		break;
	case _bitmap_format_r5g6b5:
		p = (unsigned int)((y * t->width + x) * 2);
		unpack_rgb565((unsigned int)t->pixels[p] | ((unsigned int)t->pixels[p + 1] << 8), rgba);
		break;
	case _bitmap_format_a1r5g5b5:
		p = (unsigned int)((y * t->width + x) * 2);
		p = (unsigned int)t->pixels[p] | ((unsigned int)t->pixels[p + 1] << 8);
		rgba[0] = ((p >> 10) & 31u) * 255u / 31u;
		rgba[1] = ((p >> 5) & 31u) * 255u / 31u;
		rgba[2] = (p & 31u) * 255u / 31u;
		rgba[3] = (p & 0x8000u) ? 255u : 0u;
		break;
	case _bitmap_format_a4r4g4b4:
		p = (unsigned int)((y * t->width + x) * 2);
		p = (unsigned int)t->pixels[p] | ((unsigned int)t->pixels[p + 1] << 8);
		rgba[0] = ((p >> 8) & 15u) * 17u;
		rgba[1] = ((p >> 4) & 15u) * 17u;
		rgba[2] = (p & 15u) * 17u;
		rgba[3] = ((p >> 12) & 15u) * 17u;
		break;
	case _bitmap_format_x8r8g8b8:
	case _bitmap_format_a8r8g8b8:
		p = (unsigned int)((y * t->width + x) * 4);
		rgba[2] = t->pixels[p];
		rgba[1] = t->pixels[p + 1];
		rgba[0] = t->pixels[p + 2];
		if (t->format == _bitmap_format_a8r8g8b8)
			rgba[3] = t->pixels[p + 3];
		break;
	case _bitmap_format_dxt1:
	{
		int bw = (t->width + 3) / 4;
		const unsigned char *block = t->pixels + ((y / 4) * bw + (x / 4)) * 8;

		sample_dxt_color(block, x & 3, y & 3, rgba, &rgba[3]);
		break;
	}
	case _bitmap_format_dxt3:
	{
		int bw = (t->width + 3) / 4;
		const unsigned char *block = t->pixels + ((y / 4) * bw + (x / 4)) * 16;
		unsigned int unused;
		int bit = ((y & 3) * 4 + (x & 3)) * 4;
		unsigned int a4 = (block[bit >> 3] >> (bit & 4)) & 15u;

		sample_dxt_color(block + 8, x & 3, y & 3, rgba, &unused);
		rgba[3] = a4 * 17u;
		break;
	}
	case _bitmap_format_dxt5:
	{
		int bw = (t->width + 3) / 4;
		const unsigned char *block = t->pixels + ((y / 4) * bw + (x / 4)) * 16;
		unsigned int unused;
		unsigned int a0 = block[0], a1 = block[1];
		int idx = (y & 3) * 4 + (x & 3);
		int bit = idx * 3;
		unsigned int bytes = (unsigned int)block[2 + (bit >> 3)] |
			((unsigned int)block[3 + (bit >> 3)] << 8);
		unsigned int code = (bytes >> (bit & 7)) & 7u;

		sample_dxt_color(block + 8, x & 3, y & 3, rgba, &unused);
		if (code == 0)
			rgba[3] = a0;
		else if (code == 1)
			rgba[3] = a1;
		else if (a0 > a1)
			rgba[3] = ((8u - code) * a0 + (code - 1u) * a1) / 7u;
		else if (code < 6)
			rgba[3] = ((6u - code) * a0 + (code - 1u) * a1) / 5u;
		else
			rgba[3] = (code == 6) ? 0u : 255u;
		break;
	}
	default:
		rgba[0] = 150u;
		rgba[1] = 195u;
		rgba[2] = 235u;
		break;
	}
}

static void draw_triangle(
	const float a[3],
	const float b[3],
	const float c[3],
	const float ua[2],
	const float ub[2],
	const float uc[2],
	float light,
	float rim,
	const struct tag_injector_texture *tex,
	const float tint[3],
	unsigned char alpha_replicates)
{
	float area = (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0]);
	int x0, y0, x1, y1, x, y;
	float inv_area;
	unsigned int flat_color = 0;

	if (area > -0.001f && area < 0.001f)
		return;
	inv_area = 1.0f / area;

	if (!tex)
	{
		unsigned int r = (unsigned int)(46.0f + 132.0f * light + 58.0f * rim);
		unsigned int g = (unsigned int)(72.0f + 150.0f * light + 33.0f * rim);
		unsigned int bl = (unsigned int)(104.0f + 140.0f * light + 11.0f * rim);

		if (r > 255)
			r = 255;
		if (g > 255)
			g = 255;
		if (bl > 255)
			bl = 255;
		flat_color = 0xFF000000u | (r << 16) | (g << 8) | bl;
	}

	x0 = (int)floor(a[0]);
	if ((int)floor(b[0]) < x0)
		x0 = (int)floor(b[0]);
	if ((int)floor(c[0]) < x0)
		x0 = (int)floor(c[0]);
	x1 = (int)ceil(a[0]);
	if ((int)ceil(b[0]) > x1)
		x1 = (int)ceil(b[0]);
	if ((int)ceil(c[0]) > x1)
		x1 = (int)ceil(c[0]);
	y0 = (int)floor(a[1]);
	if ((int)floor(b[1]) < y0)
		y0 = (int)floor(b[1]);
	if ((int)floor(c[1]) < y0)
		y0 = (int)floor(c[1]);
	y1 = (int)ceil(a[1]);
	if ((int)ceil(b[1]) > y1)
		y1 = (int)ceil(b[1]);
	if ((int)ceil(c[1]) > y1)
		y1 = (int)ceil(c[1]);

	if (x0 < 1)
		x0 = 1;
	if (y0 < 1)
		y0 = 1;
	if (x1 >= TAG_PREVIEW_WIDTH - 1)
		x1 = TAG_PREVIEW_WIDTH - 2;
	if (y1 >= TAG_PREVIEW_HEIGHT - 1)
		y1 = TAG_PREVIEW_HEIGHT - 2;

	for (y = y0; y <= y1; y++)
	{
		float py = (float)y + 0.5f;

		for (x = x0; x <= x1; x++)
		{
			float px = (float)x + 0.5f;
			float w0 = ((b[0] - px) * (c[1] - py) - (b[1] - py) * (c[0] - px)) * inv_area;
			float w1 = ((c[0] - px) * (a[1] - py) - (c[1] - py) * (a[0] - px)) * inv_area;
			float w2 = 1.0f - w0 - w1;
			float z;
			int idx;

			if (w0 < 0.0f || w1 < 0.0f || w2 < 0.0f)
				continue;
			z = w0 * a[2] + w1 * b[2] + w2 * c[2];
			idx = y * TAG_PREVIEW_WIDTH + x;
			if (z >= preview_depth[idx])
				continue;
			if (tex)
			{
				unsigned int rgba[4];
				float shade = 0.28f + 0.82f * light + 0.18f * rim;
				unsigned int r, g, bl;

				sample_texture(
					tex,
					w0 * ua[0] + w1 * ub[0] + w2 * uc[0],
					w0 * ua[1] + w1 * ub[1] + w2 * uc[1],
					rgba);
				if (tint)
				{
					/*
					 * shader_model color-change: the primary/secondary color
					 * tints the base map where its alpha mask allows it (or
					 * across the whole diffuse when the shader has no mask
					 * and alpha is 255). When base_map_alpha_Replicates is set
					 * (Cryo glass), alpha is a luminance/opacity mask instead.
					 */
					if (alpha_replicates)
					{
						if (rgba[3] < 24u)
							continue;
					}
					else
					{
						float m = (float)rgba[3] * (1.0f / 255.0f);

						rgba[0] = (unsigned int)((float)rgba[0] * (1.0f - m + m * tint[0]));
						rgba[1] = (unsigned int)((float)rgba[1] * (1.0f - m + m * tint[1]));
						rgba[2] = (unsigned int)((float)rgba[2] * (1.0f - m + m * tint[2]));
					}
				}
				else if (rgba[3] < 32u)
				{
					continue;
				}
				r = (unsigned int)((float)rgba[0] * shade);
				g = (unsigned int)((float)rgba[1] * shade);
				bl = (unsigned int)((float)rgba[2] * shade);
				if (r > 255)
					r = 255;
				if (g > 255)
					g = 255;
				if (bl > 255)
					bl = 255;
				preview_depth[idx] = z;
				preview_pixels[idx] = 0xFF000000u | (r << 16) | (g << 8) | bl;
			}
			else
			{
				preview_depth[idx] = z;
				preview_pixels[idx] = flat_color;
			}
		}
	}
}

static void render_frame(void)
{
	int x, y;
	long i;
	float angle = (float)turn_Step * 0.025f;
	float ca = (float)cos(angle);
	float sa = (float)sin(angle);
	float tilt = 0.32f;
	float ct = (float)cos(tilt);
	float st = (float)sin(tilt);
	float cx = TAG_PREVIEW_WIDTH * 0.5f;
	float cy = TAG_PREVIEW_HEIGHT * 0.52f;
	float fit = (TAG_PREVIEW_HEIGHT < TAG_PREVIEW_WIDTH ? TAG_PREVIEW_HEIGHT : TAG_PREVIEW_WIDTH) * 0.44f;
	float scale = mesh_radius > 0.0001f ? fit / mesh_radius : 1.0f;

	for (y = 0; y < TAG_PREVIEW_HEIGHT; y++)
	{
		unsigned int bg = 0xD8060E18u;

		for (x = 0; x < TAG_PREVIEW_WIDTH; x++)
		{
			preview_pixels[y * TAG_PREVIEW_WIDTH + x] = bg;
			preview_depth[y * TAG_PREVIEW_WIDTH + x] = 1e30f;
		}
	}
	for (x = 0; x < TAG_PREVIEW_WIDTH; x++)
	{
		preview_pixels[x] = 0xFF285888u;
		preview_pixels[(TAG_PREVIEW_HEIGHT - 1) * TAG_PREVIEW_WIDTH + x] = 0xFF285888u;
	}
	for (y = 0; y < TAG_PREVIEW_HEIGHT; y++)
	{
		preview_pixels[y * TAG_PREVIEW_WIDTH] = 0xFF285888u;
		preview_pixels[y * TAG_PREVIEW_WIDTH + TAG_PREVIEW_WIDTH - 1] = 0xFF285888u;
	}

	if (mesh.vertex_count <= 0 || mesh.triangle_count <= 0 || mesh_radius <= 0.0001f)
		return;

	for (i = 0; i < mesh.triangle_count; i++)
	{
		long i0 = mesh.triangles[i][0];
		long i1 = mesh.triangles[i][1];
		long i2 = mesh.triangles[i][2];
		float p0[3], p1[3], p2[3];
		float ux, uy, uz, vx, vy, vz, nx, ny, nz, len;
		float light, rim;
		const struct tag_injector_texture *tex = NULL;
		const float *tint = NULL;
		unsigned char alpha_replicates = 0;
		float ua[2] = { 0.0f, 0.0f };
		float ub[2] = { 0.0f, 0.0f };
		float uc[2] = { 0.0f, 0.0f };

		if (i0 < 0 || i0 >= mesh.vertex_count ||
			i1 < 0 || i1 >= mesh.vertex_count ||
			i2 < 0 || i2 >= mesh.vertex_count)
			continue;
		project_vertex(mesh.positions[i0], ca, sa, ct, st, scale, cx, cy, p0);
		project_vertex(mesh.positions[i1], ca, sa, ct, st, scale, cx, cy, p1);
		project_vertex(mesh.positions[i2], ca, sa, ct, st, scale, cx, cy, p2);

		if (mesh.normals)
		{
			float wx = mesh.normals[i0][0] + mesh.normals[i1][0] + mesh.normals[i2][0];
			float wy = mesh.normals[i0][1] + mesh.normals[i1][1] + mesh.normals[i2][1];
			float wz = mesh.normals[i0][2] + mesh.normals[i1][2] + mesh.normals[i2][2];
			float rx = wx * ca - wy * sa;
			float ry = wx * sa + wy * ca;

			nx = rx;
			ny = -(ry * st + wz * ct);
			nz = ry * ct - wz * st;
		}
		else
		{
			ux = p1[0] - p0[0];
			uy = p1[1] - p0[1];
			uz = p1[2] - p0[2];
			vx = p2[0] - p0[0];
			vy = p2[1] - p0[1];
			vz = p2[2] - p0[2];
			nx = uy * vz - uz * vy;
			ny = uz * vx - ux * vz;
			nz = ux * vy - uy * vx;
		}
		len = (float)sqrt(nx * nx + ny * ny + nz * nz);
		if (len <= 0.0001f)
			continue;
		nx /= len;
		ny /= len;
		nz /= len;
		if (nz > 0.0f)
		{
			nx = -nx;
			ny = -ny;
			nz = -nz;
		}

		light = -0.40f * nx - 0.50f * ny - 0.77f * nz;
		if (light < 0.0f)
			light = 0.0f;
		rim = 1.0f + nz;
		if (rim < 0.0f)
			rim = 0.0f;

		if (mesh.texcoords && mesh.triangle_materials && mesh.materials)
		{
			short m = mesh.triangle_materials[i];

			if (m >= 0 && m < mesh.material_count)
			{
				short ti = mesh.materials[m].texture_index;
				float us = mesh.materials[m].u_scale;
				float vs = mesh.materials[m].v_scale;

				if (ti >= 0 && ti < mesh.texture_count && mesh.textures[ti].pixels)
				{
					tex = &mesh.textures[ti];
					ua[0] = mesh.texcoords[i0][0] * us;
					ua[1] = mesh.texcoords[i0][1] * vs;
					ub[0] = mesh.texcoords[i1][0] * us;
					ub[1] = mesh.texcoords[i1][1] * vs;
					uc[0] = mesh.texcoords[i2][0] * us;
					uc[1] = mesh.texcoords[i2][1] * vs;
					alpha_replicates = mesh.materials[m].base_map_alpha_Replicates;
					if (mesh.materials[m].color_source == 1)
						tint = mesh.color_a;
					else if (mesh.materials[m].color_source >= 2)
						tint = mesh.color_b;
				}
			}
		}

		draw_triangle(p0, p1, p2, ua, ub, uc, light, rim, tex, tint, alpha_replicates);
	}
}

struct bitmap_data *tag_preview_picture(void)
{
	load_selected();
	if (mesh.vertex_count <= 0 || mesh.triangle_count <= 0)
		return NULL;

	if (!preview_bitmap_ready)
	{
		memset(&preview_bitmap, 0, sizeof(preview_bitmap));
		preview_bitmap.signature = BITMAP_DATA_SIGNATURE;
		preview_bitmap.width = TAG_PREVIEW_WIDTH;
		preview_bitmap.height = TAG_PREVIEW_HEIGHT;
		preview_bitmap.depth = 1;
		preview_bitmap.type = _bitmap_type_2d;
		preview_bitmap.format = _bitmap_format_a8r8g8b8;
		preview_bitmap.flags = FLAG(0); /* _bitmap_has_power_of_two_dimensions_bit */
		preview_bitmap.mipmap_count = 0;
		preview_bitmap.pixels_offset = 0;
		preview_bitmap.pixels_size = sizeof(preview_pixels);
		preview_bitmap.base_address = preview_pixels;
		preview_bitmap.hardware_format = NULL;
		preview_bitmap_ready = TRUE;
	}

	render_frame();
	bitmap_rebuild(&preview_bitmap);
	turn_Step++;
	return preview_bitmap.hardware_format ? &preview_bitmap : NULL;
}
