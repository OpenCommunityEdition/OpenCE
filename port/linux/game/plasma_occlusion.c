/* A filtered local visibility field avoids rejecting a whole model at a door.
   Queries see structure, scenery and machines, plus vehicles for turret flashes.
   The field
   is approximate within one cell of an edge; this is not a shadow renderer. */
#include "plasma_occlusion.h"
#if defined(__SSE2__)
#include <emmintrin.h>
#endif
#include "plasma_collision.h"
#include "ammo_counter_falloff.h"
#include "cseries.h"
#include "physics/collisions.h"
#include "plasma_lighting.h"
#include "authored_emitter_lighting.h"
#include "muzzle_lighting.h"
#include "shield_glow.h"
#include "covenant_emitter_lighting.h"
#include "rasterizer/rasterizer.h"
#include "rasterizer/rasterizer_lights.h"
#include "rasterizer/rasterizer_swizzle.h"
#include <math.h>
#include <string.h>
#include <xtl.h>

#include "lighting_config.h"
enum
{
	OCC_SIZE = 16,
	OCC_COUNT = 4096,
	OCC_LIGHTS = 8,
	OCC_SUBMITTED = 128
};
static struct occlusion_field
{
	long identity;
	unsigned long used, updated, window, admitted, traced;
	unsigned long scene_revision;
	struct plasma_collision_scene scene;
	int fading, texture_mask;
	real admission;
	real center[3], extent, radius, specular;
	real visibility[OCC_COUNT];
	real targets[OCC_COUNT];
	real depths[6][OCC_SIZE * OCC_SIZE], trace_center[3], trace_extent;
	IDirect3DVolumeTexture8 *texture[3];
	/* Exact last successful uploads, bounded to 384 KiB across all fields.
	   Identical texels need no memory-watch fault, decoding or GPU upload. */
	unsigned long uploaded[3][OCC_COUNT];
	int uploaded_mask;
	int directional;
	real direction[3];
	int valid;
} fields[OCC_LIGHTS];
static struct
{
	long identity, rasterizer;
} submitted[OCC_SUBMITTED];
/* Only overflow identities need admission history; no engine pointers persist. */
static struct
{
	long identity;
	unsigned long seen;
	int valid;
} overflow[OCC_SUBMITTED];
static void remember_overflow(long identity, unsigned long now)
{
	int i, oldest = 0;
	for (i = 0; i < OCC_SUBMITTED; i++)
	{
		if (overflow[i].valid && overflow[i].identity == identity)
		{
			oldest = i;
			break;
		}
		if (!overflow[i].valid)
		{
			oldest = i;
			break;
		}
		if (now - overflow[i].seen > now - overflow[oldest].seen)
			oldest = i;
	}
	overflow[oldest].identity = identity;
	overflow[oldest].seen = now;
	overflow[oldest].valid = 1;
}
static int submitted_count;
static int updates_this_window;
static int fields_dirty;
static unsigned long current_window;
static long model_slots[2] = {NONE, NONE};
static int enabled(void)
{
	return ((lighting_config_boolean("display.plasma_lighting") || authored_emitter_lighting_enabled() || covenant_emitter_lighting_enabled() || shield_glow_enabled()) &&
		lighting_config_boolean("display.plasma_occlusion")) ||
		lighting_config_boolean("display.muzzle_lighting") || lighting_config_boolean("display.vehicle_lighting");
}
struct sample_axis
{
	int a, b, inside;
	real t;
};
static void sample_axis_initialize(struct sample_axis *axis, real coordinate)
{
	real p = MAX(0.0f, MIN((real)(OCC_SIZE - 1), coordinate * OCC_SIZE - 0.5f));
	axis->a = (int)p;
	axis->b = MIN(axis->a + 1, OCC_SIZE - 1);
	axis->t = p - axis->a;
	axis->inside = coordinate >= 0 && coordinate <= 1;
}
static real sample_axes(const real *values, const struct sample_axis *x,
	const struct sample_axis *y, const struct sample_axis *z)
{
	int row0 = (z->a * OCC_SIZE + y->a) * OCC_SIZE;
	int row1 = (z->a * OCC_SIZE + y->b) * OCC_SIZE;
	int row2 = (z->b * OCC_SIZE + y->a) * OCC_SIZE;
	int row3 = (z->b * OCC_SIZE + y->b) * OCC_SIZE;
	real a = values[row0+x->a], b = values[row1+x->a];
	real c = values[row2+x->a], d = values[row3+x->a];
	/* Seven linear interpolations replace eight three-factor weights. This
	   render-only field tolerates sub-texel rounding, before 8-bit upload. */
	a += (values[row0+x->b] - a) * x->t;
	b += (values[row1+x->b] - b) * x->t;
	c += (values[row2+x->b] - c) * x->t;
	d += (values[row3+x->b] - d) * x->t;
	a += (b - a) * y->t;
	c += (d - c) * y->t;
	return a + (c - a) * z->t;
}
static real sample_field(const real *values, real x, real y, real z)
{
	struct sample_axis axes[3];
	sample_axis_initialize(&axes[0], x);
	sample_axis_initialize(&axes[1], y);
	sample_axis_initialize(&axes[2], z);
	return sample_axes(values, &axes[0], &axes[1], &axes[2]);
}

static void half_sample_axes(struct sample_axis axes[3][OCC_SIZE])
{
	int axis, i;
	/* With a half-size receiver volume, coordinate * 16 - 0.5 is exactly
	   3.75 + index / 2. All samples are interior, at quarter-texel weights.
	   Keep the same interpolation order; other scale ratios use sample_field. */
	for (axis = 0; axis < 3; axis++)
		for (i = 0; i < OCC_SIZE; i++)
		{
			axes[axis][i].a = (i + 7) / 2;
			axes[axis][i].b = axes[axis][i].a + 1;
			axes[axis][i].t = i & 1 ? 0.25f : 0.75f;
			axes[axis][i].inside = TRUE;
		}
}

/* Reuse the x/y interpolation shared by neighbouring output cells. The final
   z interpolation remains in the update loop. This is the same x-then-y-then-z
   arithmetic as sample_axes, with three interpolations per cell instead of
   seven. Scratch lives only during this synchronous render update. */
static void reproject_rows(const real *values, struct sample_axis const axes[3][OCC_SIZE], real *rows)
{
	real horizontal[OCC_COUNT];
	int x, y, z, low[3] = {0, OCC_SIZE - 1, OCC_SIZE - 1}, high[3] = {0, 0, 0};
	/* Only source rows reached by the requested coordinates are needed.
	   In the half-size volume, ten source rows feed sixteen output rows. */
	for (x = 0; x < OCC_SIZE; x++)
		for (y = 1; y < 3; y++)
		{
			low[y] = MIN(low[y], MIN(axes[y][x].a, axes[y][x].b));
			high[y] = MAX(high[y], MAX(axes[y][x].a, axes[y][x].b));
		}
	for (z = low[2]; z <= high[2]; z++)
		for (y = low[1]; y <= high[1]; y++)
			for (x = 0; x < OCC_SIZE; x++)
			{
				int row = (z * OCC_SIZE + y) * OCC_SIZE;
				real a = values[row + axes[0][x].a];
				horizontal[row + x] = a + (values[row + axes[0][x].b] - a) * axes[0][x].t;
			}
	for (z = low[2]; z <= high[2]; z++)
		for (y = 0; y < OCC_SIZE; y++)
			for (x = 0; x < OCC_SIZE; x++)
			{
				int row0 = (z * OCC_SIZE + axes[1][y].a) * OCC_SIZE;
				int row1 = (z * OCC_SIZE + axes[1][y].b) * OCC_SIZE;
				real a = horizontal[row0 + x];
				rows[(z * OCC_SIZE + y) * OCC_SIZE + x] = a + (horizontal[row1 + x] - a) * axes[1][y].t;
			}
}
static real sample_history_rows(const real *rows, int x, int y, struct sample_axis const *z)
{
	real a = rows[(z->a * OCC_SIZE + y) * OCC_SIZE + x];
	return a + (rows[(z->b * OCC_SIZE + y) * OCC_SIZE + x] - a) * z->t;
}

/* One depth ray per cubemap texel serves every radial cell on that ray.
   Interpolate visibility comparisons, never distances across a doorway. */
static void trace_depths(real const extent, real_point3d const *from, long ignore, int vehicles, int resolution, real depths[6][OCC_SIZE * OCC_SIZE])
{
	real_vector3d vectors[PLASMA_COLLISION_RAYS];
	real hits[PLASMA_COLLISION_RAYS];
	int face, x, y, axis, count = 0;
	unsigned long flags =
		FLAG(_collision_test_structure_bit) | FLAG(_collision_test_front_facing_surfaces_bit) |
		FLAG(_collision_test_back_facing_surfaces_bit) | FLAG(_collision_test_objects_bit) |
		FLAG(_collision_test_objects_scenery_bit) | FLAG(_collision_test_objects_machines_bit);
	if (vehicles) flags |= FLAG(_collision_test_objects_vehicles_bit);
	for (face = 0; face < 6; face++)
		for (y = 0; y < resolution; y++)
			for (x = 0; x < resolution; x++)
			{
				real component[3];
				real_vector3d ray;
				axis = face / 2;
				component[axis] = (face & 1) ? extent : -extent;
				component[(axis + 1) % 3] = (2.0f * (x + 0.5f) / resolution - 1) * extent;
				component[(axis + 2) % 3] = (2.0f * (y + 0.5f) / resolution - 1) * extent;
				ray.i = component[0];
				ray.j = component[1];
				ray.k = component[2];
				vectors[count++] = ray;
			}
	plasma_collision_depths(flags, from, vectors, count, ignore, hits);
	count = 0;
	for (face = 0; face < 6; face++)
		for (y = 0; y < resolution; y++)
			for (x = 0; x < resolution; x++)
				depths[face][y * OCC_SIZE + x] = hits[count++];
}
static real depth_visibility(real depths[6][OCC_SIZE * OCC_SIZE], real_vector3d const *ray, real extent, int resolution)
{
	real p[3], magnitude, u, v, tx, ty, result = 0, length, fraction, hits[2][2];
	int axis = 0, face, x, y, i, j, clear = TRUE;
	p[0] = ray->i;
	p[1] = ray->j;
	p[2] = ray->k;
	if (fabsf(p[1]) > fabsf(p[axis]))
		axis = 1;
	if (fabsf(p[2]) > fabsf(p[axis]))
		axis = 2;
	magnitude = fabsf(p[axis]);
	if (magnitude < 0.0001f)
		return 1;
	face = axis * 2 + (p[axis] > 0);
	u = MAX(0.0f, MIN((real)(resolution - 1), (p[(axis + 1) % 3] / magnitude + 1) * 0.5f * resolution - 0.5f));
	v = MAX(0.0f, MIN((real)(resolution - 1), (p[(axis + 2) % 3] / magnitude + 1) * 0.5f * resolution - 0.5f));
	x = (int)u;
	y = (int)v;
	tx = u - x;
	ty = v - y;
	fraction = magnitude / extent;
	for (j = 0; j < 2; j++)
		for (i = 0; i < 2; i++)
		{
			hits[j][i] = depths[face][MIN(y + j, resolution - 1) * OCC_SIZE + MIN(x + i, resolution - 1)];
			if (!(hits[j][i] >= fraction)) clear = FALSE;
		}
	/* All four comparisons saturate to one before any distance feathering.
	   Retain the original sum/order of weights, even its rounding at seams,
	   but avoid the square root and four redundant visibility calculations. */
	if (clear)
		return (1 - tx) * (1 - ty) + tx * (1 - ty) + (1 - tx) * ty + tx * ty;
	length = sqrtf(p[0] * p[0] + p[1] * p[1] + p[2] * p[2]) * extent / magnitude;
	for (j = 0; j < 2; j++)
		for (i = 0; i < 2; i++)
		{
			real visible = MAX(0.0f, MIN(1.0f, 1.0f - (fraction - hits[j][i]) * length / 0.025f));
			result += visible * (i ? tx : 1 - tx) * (j ? ty : 1 - ty);
		}
	return result;
}
/* Four receiver cells share face selection and visibility arithmetic. Keep
   exact divides, square roots and weight order: no approximate reciprocals or
   reduced grids. Other CPU targets retain the scalar implementation. */
#if defined(__SSE2__)
static __m128 visibility_choose(__m128 mask, __m128 a, __m128 b)
{
	return _mm_or_ps(_mm_and_ps(mask, a), _mm_andnot_ps(mask, b));
}
static void depth_visibility_row(real depths[6][OCC_SIZE * OCC_SIZE], const real xs[OCC_SIZE], real y, real z,
								 real extent, int resolution, real *output)
{
	__m128 zero = _mm_setzero_ps(), one = _mm_set1_ps(1), half = _mm_set1_ps(.5f);
	__m128 py = _mm_set1_ps(y), pz = _mm_set1_ps(z), ey = _mm_set1_ps(fabsf(y)), ez = _mm_set1_ps(fabsf(z));
	__m128 ex = _mm_set1_ps(extent), size = _mm_set1_ps((real)resolution), limit = _mm_set1_ps((real)(resolution - 1));
	__m128 facey = _mm_set1_ps((real)(2 + (y > 0))), facez = _mm_set1_ps((real)(4 + (z > 0)));
	__m128 abs_mask = _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff));
	int x, i, j, lane;
	for (x = 0; x < OCC_SIZE; x += 4)
	{
		__m128 px = _mm_loadu_ps(xs + x), magnitude = _mm_and_ps(px, abs_mask);
		__m128 mask = _mm_cmpgt_ps(ey, magnitude), u = visibility_choose(mask, pz, py),
			   v = visibility_choose(mask, px, pz);
		__m128 face = visibility_choose(mask, facey, _mm_and_ps(_mm_cmpgt_ps(px, zero), one));
		__m128 near_origin, tx, ty, fraction, hits[4], length;
		__m128 clear = _mm_castsi128_ps(_mm_set1_epi32(-1)), result = zero;
		int faces[4], columns[4], rows[4];
		real gather[4];
		magnitude = visibility_choose(mask, ey, magnitude);
		mask = _mm_cmpgt_ps(ez, magnitude);
		magnitude = visibility_choose(mask, ez, magnitude);
		face = visibility_choose(mask, facez, face);
		u = visibility_choose(mask, px, u);
		v = visibility_choose(mask, py, v);
		near_origin = _mm_cmplt_ps(magnitude, _mm_set1_ps(.0001f));
		magnitude = visibility_choose(near_origin, one, magnitude);
		u = _mm_sub_ps(_mm_mul_ps(_mm_mul_ps(_mm_add_ps(_mm_div_ps(u, magnitude), one), half), size), half);
		v = _mm_sub_ps(_mm_mul_ps(_mm_mul_ps(_mm_add_ps(_mm_div_ps(v, magnitude), one), half), size), half);
		u = _mm_max_ps(zero, _mm_min_ps(limit, u));
		v = _mm_max_ps(zero, _mm_min_ps(limit, v));
		_mm_storeu_si128((__m128i *)faces, _mm_cvttps_epi32(face));
		_mm_storeu_si128((__m128i *)columns, _mm_cvttps_epi32(u));
		_mm_storeu_si128((__m128i *)rows, _mm_cvttps_epi32(v));
		tx = _mm_sub_ps(u, _mm_cvtepi32_ps(_mm_cvttps_epi32(u)));
		ty = _mm_sub_ps(v, _mm_cvtepi32_ps(_mm_cvttps_epi32(v)));
		fraction = _mm_div_ps(magnitude, ex);
		for (j = 0; j < 2; j++)
			for (i = 0; i < 2; i++)
			{
				for (lane = 0; lane < 4; lane++)
					gather[lane] = depths[faces[lane]][MIN(rows[lane] + j, resolution - 1) * OCC_SIZE +
													   MIN(columns[lane] + i, resolution - 1)];
				hits[j * 2 + i] = _mm_loadu_ps(gather);
				clear = _mm_and_ps(clear, _mm_cmpge_ps(hits[j * 2 + i], fraction));
			}
		if (_mm_movemask_ps(clear) == 15)
		{
			result =
				_mm_add_ps(_mm_mul_ps(_mm_sub_ps(one, tx), _mm_sub_ps(one, ty)), _mm_mul_ps(tx, _mm_sub_ps(one, ty)));
			result = _mm_add_ps(result, _mm_mul_ps(_mm_sub_ps(one, tx), ty));
			result = _mm_add_ps(result, _mm_mul_ps(tx, ty));
		}
		else
		{
			length = _mm_div_ps(_mm_mul_ps(_mm_sqrt_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(px, px), _mm_mul_ps(py, py)),
																  _mm_mul_ps(pz, pz))),
										   ex),
								magnitude);
			for (j = 0; j < 2; j++)
				for (i = 0; i < 2; i++)
				{
					__m128 visible = _mm_sub_ps(
						one, _mm_div_ps(_mm_mul_ps(_mm_sub_ps(fraction, hits[j * 2 + i]), length), _mm_set1_ps(.025f)));
					visible = _mm_max_ps(zero, _mm_min_ps(one, visible));
					result = _mm_add_ps(result, _mm_mul_ps(_mm_mul_ps(visible, i ? tx : _mm_sub_ps(one, tx)),
														   j ? ty : _mm_sub_ps(one, ty)));
				}
		}
		_mm_storeu_ps(output + x, visibility_choose(near_origin, one, result));
	}
}
#else
static void depth_visibility_row(real depths[6][OCC_SIZE * OCC_SIZE], const real xs[OCC_SIZE], real y, real z,
								 real extent, int resolution, real *output)
{
	int x;
	for (x = 0; x < OCC_SIZE; x++)
	{
		real_vector3d ray;
		ray.i = xs[x];
		ray.j = y;
		ray.k = z;
		output[x] = depth_visibility(depths, &ray, extent, resolution);
	}
}
#endif

static void reconstruct_visibility(real depths[6][OCC_SIZE * OCC_SIZE], real extent, int resolution, real dx, real dy,
								   real dz, real *output)
{
	real offsets[OCC_SIZE], xs[OCC_SIZE];
	int x, y, z;
	for (x = 0; x < OCC_SIZE; x++)
	{
		offsets[x] = (2.0f * (x + 0.5f) / OCC_SIZE - 1) * extent;
		xs[x] = offsets[x] + dx;
	}
	for (z = 0; z < OCC_SIZE; z++)
		for (y = 0; y < OCC_SIZE; y++)
			depth_visibility_row(depths, xs, offsets[y] + dy, offsets[z] + dz, extent, resolution,
								 output + (z * OCC_SIZE + y) * OCC_SIZE);
}

static real filter_visibility(real history, real target, real clear_alpha, real block_alpha)
{
	real alpha = target < history ? block_alpha : clear_alpha;
	real value = history + (target - history) * alpha;
	/* Render-only fade history can otherwise stick at subnormal values.
	   This cutoff is far below one 8-bit texture step and leaves the
	   simulation's floating-point mode untouched. */
	if (value < 1.0e-20f) value = 0;
	return value;
}
static int upload(IDirect3DVolumeTexture8 **texture, unsigned long const *pixels)
{
	D3DLOCKED_BOX box;
	if (!*texture && D3DDevice_CreateVolumeTexture(OCC_SIZE, OCC_SIZE, OCC_SIZE, 1, 0, D3DFMT_A8R8G8B8,
												   D3DPOOL_MANAGED, texture) < 0)
		return 0;
	if (IDirect3DVolumeTexture8_LockBox(*texture, 0, &box, NULL, 0) < 0 || !box.pBits)
		return 0;
	rasterizer_xbox_bitmap_swizzle3d_long(box.pBits, (void *)pixels, OCC_SIZE, OCC_SIZE, OCC_SIZE);
	return IDirect3DVolumeTexture8_UnlockBox(*texture, 0) >= 0;
}
/* Keep the large reconstruction scratch off the repeated per-draw lookup
   path. Clang otherwise inlines this back into field_for, reserving/probing
   its large stack frame even on a cache hit. All ports build with Clang.
   This runs at most once per admitted field in a view. */
static __attribute__((noinline)) struct occlusion_field *update_field(struct occlusion_field *field, long rasterizer_index, unsigned long now)
{
	struct rasterizer_light_submit_parameters *light;
	int i, x, y, z, stationary, vehicles, resolution, retraced = FALSE;
	unsigned long scene_revision;
	real elapsed, extent, specular, clear_alpha, block_alpha, admission;
	real previous[OCC_COUNT];
	real projected[OCC_COUNT];
	struct sample_axis history_axes[3][OCC_SIZE];
	real trace_dx, trace_dy, trace_dz;
	real axis[3], origin[3], radius;
	fields_dirty = 1;
	field->used = now;
	light = &rasterizer_lights.lights[rasterizer_index];
	specular = plasma_occlusion_specular_scale(rasterizer_index);
	extent = light->radius * MAX(1.0f, specular);
	if (!(extent > 0.001f && extent < 1000.0f))
		return NULL;
	updates_this_window++;
	elapsed = field->valid ? MIN((real)(now - field->updated) * 0.001f, 0.1f) : 0.1f;
	trace_dx = light->position.x - field->trace_center[0];
	trace_dy = light->position.y - field->trace_center[1];
	trace_dz = light->position.z - field->trace_center[2];
	vehicles = muzzle_lighting_vehicle_occlusion(field->identity) || plasma_lighting_vehicle_occlusion(field->identity) ||
		covenant_emitter_lighting_vehicle_occlusion(field->identity);
	scene_revision = plasma_collision_scene_revision(&field->scene, &light->position, extent, vehicles);
	/* Only the deliberately short, soft prong glow uses a smaller ray grid.
	   Keep the established shield/projectile/vehicle grids and 16^3 filtered
	   output unchanged. A prong's 0.6-unit extent needs 96 rays, not 1536. */
	resolution = plasma_lighting_prong_cone(rasterizer_index, axis, origin, &radius) ? 4 : OCC_SIZE;
	/* Cache collision depths briefly, but project and fade at every rendered pose.
	   Exact stationary poses can retain depths while all collision inputs are
	   unchanged. Geometry changes follow the existing 12 ms trace cadence;
	   they cannot be mistaken for a permanently unchanged stationary scene. */
	if (!field->valid ||
		(now - field->traced >= 12 && (field->scene_revision != scene_revision ||
		 trace_dx != 0 || trace_dy != 0 || trace_dz != 0)) || field->trace_extent != extent ||
		trace_dx * trace_dx + trace_dy * trace_dy + trace_dz * trace_dz > extent * extent * 0.015625f)
	{
		trace_depths(extent, &light->position, covenant_emitter_lighting_occlusion_ignore(field->identity),
			vehicles, resolution, field->depths);
		retraced = TRUE;
		field->scene_revision = scene_revision;
		field->traced = now;
		field->trace_extent = extent;
		field->trace_center[0] = light->position.x;
		field->trace_center[1] = light->position.y;
		field->trace_center[2] = light->position.z;
		trace_dx = trace_dy = trace_dz = 0;
	}
	memcpy(previous, field->visibility, sizeof(previous));
	stationary = field->valid && field->extent == extent &&
		field->center[0] == light->position.x && field->center[1] == light->position.y &&
		field->center[2] == light->position.z;
	admission = field->fading ? MIN(1.0f, (real)(now - field->admitted) / 80.0f) : 1.0f;
	admission = admission * admission * (3.0f - 2.0f * admission);
	clear_alpha = 1.0f - powf(0.5f, elapsed / 0.025f);
	block_alpha = 1.0f - powf(0.5f, elapsed / 0.008f);
	/* Translation and uniform scale are separable: each axis has sixteen
	   coordinates, shared by all 4096 cells. Keep the original arithmetic
	   and interpolation order, including the outside-history fallback. */
	if (!stationary && field->valid && field->extent > 0)
	{
		for (i = 0; i < OCC_SIZE; i++)
		{
			real offset = (2.0f * (i + 0.5f) / OCC_SIZE - 1) * extent;
			sample_axis_initialize(&history_axes[0][i], 0.5f + (light->position.x + offset - field->center[0]) / (2 * field->extent));
			sample_axis_initialize(&history_axes[1][i], 0.5f + (light->position.y + offset - field->center[1]) / (2 * field->extent));
			sample_axis_initialize(&history_axes[2][i], 0.5f + (light->position.z + offset - field->center[2]) / (2 * field->extent));
		}
		reproject_rows(previous, history_axes, projected);
	}
	/* Unchanged poses only advance fades. This flat loop can vectorize without
	   ray construction or history-reprojection branches. */
	if (stationary && !retraced)
	{
		for (i = 0; i < OCC_COUNT; i++)
			field->visibility[i] = filter_visibility(previous[i], field->targets[i], clear_alpha, block_alpha);
	}
	else
	{
		reconstruct_visibility(field->depths, extent, resolution, trace_dx, trace_dy, trace_dz, field->targets);
		for (z = 0; z < OCC_SIZE; z++)
			for (y = 0; y < OCC_SIZE; y++)
				for (x = 0; x < OCC_SIZE; x++)
				{
					real target, history;
					i = (z * OCC_SIZE + y) * OCC_SIZE + x;
					target = field->targets[i];
					/* Reproject history at the receiver, not at a moving light-local cell. */
					history = target;
					if (stationary)
						history = previous[i];
					else if (field->valid && field->extent > 0)
					{
						if (history_axes[0][x].inside && history_axes[1][y].inside && history_axes[2][z].inside)
							history = sample_history_rows(projected, x, y, &history_axes[2][z]);
					}
					field->visibility[i] = filter_visibility(history, target, clear_alpha, block_alpha);
				}
	}

	/* Texture coordinates are light-local. A changed world position alone
	   does not change texels if the projected visibility stayed identical. */
	if (!field->valid || field->radius != light->radius || field->specular != specular ||
		field->extent != extent || field->admission != admission ||
		memcmp(previous, field->visibility, sizeof(previous)))
		field->texture_mask = 0;
	else
		field->texture_mask &= 7; /* failed uploads retry next view */
	field->center[0] = light->position.x;
	field->center[1] = light->position.y;
	field->center[2] = light->position.z;
	field->radius = light->radius;
	field->specular = specular;
	field->extent = extent;
	field->admission = admission;
	field->updated = now;
	field->window = current_window;
	field->valid = 1;
	return field;
}
static struct occlusion_field *field_for(long rasterizer_index)
{
	struct occlusion_field *field = NULL;
	unsigned long now = GetTickCount();
	long identity = NONE;
	int i;
	if (!enabled() || rasterizer_index < 0 || rasterizer_index >= rasterizer_lights.light_count)
		return NULL;
	for (i = 0; i < submitted_count; i++)
		if (submitted[i].rasterizer == rasterizer_index)
			identity = submitted[i].identity;
	if (identity == NONE)
		return NULL;
	for (i = 0; i < OCC_LIGHTS; i++)
		if (fields[i].valid && fields[i].identity == identity)
			field = &fields[i];
	if (field && field->window == current_window)
		return field;
	if (updates_this_window >= OCC_LIGHTS)
	{
		remember_overflow(identity, now);
		return NULL;
	}
	if (!field)
	{
		for (i = 0; i < OCC_LIGHTS; i++)
			if (!fields[i].valid || now - fields[i].used > 100)
			{
				field = &fields[i];
				break;
			}
		if (!field)
		{
			remember_overflow(identity, now);
			return NULL; /* active lights are never evicted */
		}
		field->valid = 0;
		field->identity = identity;
		field->admitted = now;
		field->fading = 0;
		for (i = 0; i < OCC_SUBMITTED; i++)
			if (overflow[i].valid && overflow[i].identity == identity && now - overflow[i].seen < 200)
				field->fading = 1;
		for (i = 0; i < OCC_COUNT; i++)
			field->visibility[i] = 1;
	}
	return update_field(field, rasterizer_index, now);
}
/* Build only a texture actually consumed by this view. CPU visibility history
   stays independent of pass order, and each pass uploads at most once. */
/* Keep the pixel/row scratch out of cache hits, including under LTO. */
static __attribute__((noinline)) void *prepare_field_texture(struct occlusion_field *field, int pass, int directional, real const axis[3])
{
	unsigned long pixels[OCC_COUNT];
	real rows[OCC_COUNT];
	struct sample_axis half_axes[3][OCC_SIZE];
	int x, y, z, i;
	real scale;
	scale = (pass == 2 ? field->specular : 1.0f) * field->radius / field->extent;
	if (pass && scale == 0.5f)
	{
		half_sample_axes(half_axes);
		reproject_rows(field->visibility, half_axes, rows);
	}
	/* The ordinary model mask needs no radial/directional calculation or
	   resampling. A flat conversion keeps its common path vectorizable. */
	if (!pass && !directional)
	{
		for (i = 0; i < OCC_COUNT; i++)
			pixels[i] = (unsigned long)(255 * (1 - field->admission + field->admission * field->visibility[i]) + 0.5f) * 0x01010101UL;
	}
	else if (pass && !directional && (scale == 0.5f || scale == 1.0f))
	{
		/* Ordinary radial masks use half-size or unit-size sampling. Resolve
		   the source slices once per output slice, leaving contiguous
		   rows and no profile/resampling branches in the conversion loop. */
		for (z = 0; z < OCC_SIZE; z++)
		{
			real const *a = scale == 1.0f ? field->visibility + z * OCC_SIZE * OCC_SIZE : rows + half_axes[2][z].a * OCC_SIZE * OCC_SIZE;
			real const *b = scale == 1.0f ? a : rows + half_axes[2][z].b * OCC_SIZE * OCC_SIZE;
			real blend = scale == 1.0f ? 0 : half_axes[2][z].t;
			real dz = 2.0f * (z + 0.5f) / OCC_SIZE - 1;
			for (y = 0; y < OCC_SIZE; y++)
			{
				real dy = 2.0f * (y + 0.5f) / OCC_SIZE - 1;
				for (x = 0; x < OCC_SIZE; x++)
				{
					int row = y * OCC_SIZE + x;
					real dx = 2.0f * (x + 0.5f) / OCC_SIZE - 1;
					real remaining = MAX(0.0f, 1 - dx * dx - dy * dy - dz * dz);
					real value = a[row] + (b[row] - a[row]) * blend;
					value = 1 - field->admission + field->admission * value;
					value *= remaining * remaining;
					pixels[z * OCC_SIZE * OCC_SIZE + row] = (unsigned long)(255 * value + 0.5f) * 0x01010101UL;
				}
			}
		}
	}
	else
	{
		for (z = 0; z < OCC_SIZE; z++)
			for (y = 0; y < OCC_SIZE; y++)
				for (x = 0; x < OCC_SIZE; x++)
				{
					real dx = 2.0f * (x + 0.5f) / OCC_SIZE - 1;
					real dy = 2.0f * (y + 0.5f) / OCC_SIZE - 1;
					real dz = 2.0f * (z + 0.5f) / OCC_SIZE - 1;
					real remaining = MAX(0.0f, 1 - dx * dx - dy * dy - dz * dz);
					real value;
					i = (z * OCC_SIZE + y) * OCC_SIZE + x;
					/* Unit-scale samples land exactly at texel centres. Outside the
					   attenuation sphere the result is zero regardless of visibility. */
					value = field->visibility[i];
					if (pass && remaining > 0 && scale != 1.0f)
						value = scale == 0.5f ? sample_history_rows(rows, x, y, &half_axes[2][z]) :
							sample_field(field->visibility, 0.5f + dx * scale * 0.5f,
								0.5f + dy * scale * 0.5f, 0.5f + dz * scale * 0.5f);
					value = 1 - field->admission + field->admission * value;
					if (directional)
						value *= directional == 3 ? plasma_prong_falloff_factor(axis, dx, dy, dz) : directional == 2 ? needler_falloff_factor(dz *
							(pass == 0 ? field->extent / field->radius : pass == 2 ? field->specular : 1.0f)) :
							ammo_counter_falloff_factor(axis, dx, dy, dz);
					if (pass)
						value *= remaining * remaining;
					pixels[i] = (unsigned long)(255 * value + 0.5f) * 0x01010101UL;
				}
	}
	if (!(field->uploaded_mask & (1 << pass)) || memcmp(field->uploaded[pass], pixels, sizeof(pixels)))
	{
		if (!upload(&field->texture[pass], pixels))
		{
			/* A failed unlock may have changed backing storage. */
			field->uploaded_mask &= ~(1 << pass);
			field->texture_mask |= 8 << pass;
			return NULL;
		}
		memcpy(field->uploaded[pass], pixels, sizeof(pixels));
		field->uploaded_mask |= 1 << pass;
	}
	field->texture_mask |= 1 << pass;
	return field->texture[pass];
}
static void *field_texture(struct occlusion_field *field, int pass, long rasterizer)
{
	real axis[3], origin[3], radius;
	int directional = covenant_emitter_lighting_cone(rasterizer, axis, origin, &radius);
	if (!directional) directional = plasma_lighting_prong_cone(rasterizer, axis, origin, &radius);
	if (!field)
		return NULL;
	/* Directional profiles can rotate without changing collision visibility.
	   Needler's lower-height fade is independent of the weapon's aim. */
	if (directional != field->directional ||
		(directional && directional != 2 && memcmp(axis, field->direction, sizeof(field->direction))))
	{
		field->texture_mask = 0;
		field->directional = directional;
		if (directional && directional != 2) memcpy(field->direction, axis, sizeof(field->direction));
	}
	/* Equal radii produce identical diffuse/specular texels, including the
	   directional mask. Share that upload; different extents stay independent. */
	if (pass == 2 && field->specular == 1.0f)
		pass = 1;
	/* A failed pass falls back for the rest of this view, then retries next view. */
	if (field->texture_mask & (8 << pass))
		return NULL;
	if (field->texture_mask & (1 << pass))
		return field->texture[pass];
	return prepare_field_texture(field, pass, directional, axis);
}
void plasma_occlusion_reset(void)
{
	int i, j;
	ammo_counter_falloff_dispose();
	/* The disabled frame-end path may request reset repeatedly. Clear the
	   bounded upload/visibility storage only after it has actually been used. */
	if (fields_dirty)
	{
		for (i = 0; i < OCC_LIGHTS; i++)
			for (j = 0; j < 3; j++)
				if (fields[i].texture[j])
					D3DResource_Release((D3DResource *)fields[i].texture[j]);
		memset(fields, 0, sizeof(fields));
		fields_dirty = 0;
	}
	memset(overflow, 0, sizeof(overflow));
	plasma_occlusion_window();
}


void plasma_occlusion_window(void)
{
	ammo_counter_falloff_window();
	current_window++;
	submitted_count = 0;
	updates_this_window = 0;
	model_slots[0] = model_slots[1] = NONE;
}
unsigned long plasma_occlusion_view_serial(void)
{
	return current_window;
}
void plasma_occlusion_submit(long light, long rasterizer_index)
{
	if ((((lighting_config_boolean("display.plasma_lighting") || authored_emitter_lighting_enabled()) && lighting_config_boolean("display.plasma_occlusion")) ||
		plasma_lighting_vehicle_occlusion(light) || covenant_emitter_lighting_vehicle_occlusion(light) ||
		muzzle_lighting_selected(rasterizer_index) ||
		((covenant_emitter_lighting_selected(rasterizer_index) || shield_glow_selected(rasterizer_index)) && lighting_config_boolean("display.plasma_occlusion"))) &&
		submitted_count < OCC_SUBMITTED)
	{
		submitted[submitted_count].identity = light;
		submitted[submitted_count++].rasterizer = rasterizer_index;
	}
}
void plasma_occlusion_model_slot(long rasterizer_index, short slot)
{
	if (slot >= 0 && slot < 2)
		model_slots[slot] = rasterizer_index;
}
void *plasma_occlusion_texture(long rasterizer_index, int specular)
{
	struct occlusion_field *field = field_for(rasterizer_index);
	void *texture = field_texture(field, specular ? 2 : 1, rasterizer_index);
	return texture ? texture : ammo_counter_falloff_texture(rasterizer_index, specular ? 2 : 1, NULL);
}
void *plasma_occlusion_model_texture(int slot, float transform[4])
{
	struct occlusion_field *field;
	if (slot < 0 || slot >= 2)
		return NULL;
	field = field_for(model_slots[slot]);
	if (!field)
		return ammo_counter_falloff_texture(model_slots[slot], 0, transform);
	transform[0] = field->center[0];
	transform[1] = field->center[1];
	transform[2] = field->center[2];
	transform[3] = 0.5f / field->extent;
	{
		void *texture = field_texture(field, 0, model_slots[slot]);
		return texture ? texture : ammo_counter_falloff_texture(model_slots[slot], 0, transform);
	}
}
