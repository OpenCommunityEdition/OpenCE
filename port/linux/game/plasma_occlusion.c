/* A filtered local visibility field avoids rejecting a whole model at a door.
   Queries see structure, scenery and machines, plus vehicles for turret flashes.
   The field
   is approximate within one cell of an edge; this is not a shadow renderer. */
#include "plasma_occlusion.h"
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
	int fading, texture_mask;
	real admission;
	real center[3], extent, radius, specular;
	real visibility[OCC_COUNT];
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
	int i, j, k;
	real result = 0;
	for (k = 0; k < 2; k++)
		for (j = 0; j < 2; j++)
			for (i = 0; i < 2; i++)
				result += values[((k ? z->b : z->a) * OCC_SIZE + (j ? y->b : y->a)) * OCC_SIZE +
								 (i ? x->b : x->a)] *
						  (i ? x->t : 1 - x->t) * (j ? y->t : 1 - y->t) * (k ? z->t : 1 - z->t);
	return result;
}
static real sample_field(const real *values, real x, real y, real z)
{
	struct sample_axis axes[3];
	sample_axis_initialize(&axes[0], x);
	sample_axis_initialize(&axes[1], y);
	sample_axis_initialize(&axes[2], z);
	return sample_axes(values, &axes[0], &axes[1], &axes[2]);
}
/* One depth ray per cubemap texel serves every radial cell on that ray.
   Interpolate visibility comparisons, never distances across a doorway. */
static void trace_depths(real const extent, real_point3d const *from, long ignore, int vehicles, real depths[6][OCC_SIZE * OCC_SIZE])
{
	int face, x, y, axis;
	unsigned long flags =
		FLAG(_collision_test_structure_bit) | FLAG(_collision_test_front_facing_surfaces_bit) |
		FLAG(_collision_test_back_facing_surfaces_bit) | FLAG(_collision_test_objects_bit) |
		FLAG(_collision_test_objects_scenery_bit) | FLAG(_collision_test_objects_machines_bit);
	if (vehicles) flags |= FLAG(_collision_test_objects_vehicles_bit);
	plasma_collision_begin(flags, from, ignore);
	for (face = 0; face < 6; face++)
		for (y = 0; y < OCC_SIZE; y++)
			for (x = 0; x < OCC_SIZE; x++)
			{
				real component[3];
				real_vector3d ray;
				axis = face / 2;
				component[axis] = (face & 1) ? extent : -extent;
				component[(axis + 1) % 3] = (2.0f * (x + 0.5f) / OCC_SIZE - 1) * extent;
				component[(axis + 2) % 3] = (2.0f * (y + 0.5f) / OCC_SIZE - 1) * extent;
				ray.i = component[0];
				ray.j = component[1];
				ray.k = component[2];
				/* A clear ray must not become a blocker when reprojection crosses its endpoint. */
				depths[face][y * OCC_SIZE + x] =
					plasma_collision_depth(flags, from, &ray, ignore);
			}
	plasma_collision_end();
}
static real depth_visibility(real depths[6][OCC_SIZE * OCC_SIZE], real_vector3d const *ray, real extent)
{
	real p[3], magnitude, u, v, tx, ty, result = 0, length;
	int axis = 0, face, x, y, i, j;
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
	u = MAX(0.0f, MIN((real)(OCC_SIZE - 1), (p[(axis + 1) % 3] / magnitude + 1) * 0.5f * OCC_SIZE - 0.5f));
	v = MAX(0.0f, MIN((real)(OCC_SIZE - 1), (p[(axis + 2) % 3] / magnitude + 1) * 0.5f * OCC_SIZE - 0.5f));
	x = (int)u;
	y = (int)v;
	tx = u - x;
	ty = v - y;
	length = sqrtf(p[0] * p[0] + p[1] * p[1] + p[2] * p[2]) * extent / magnitude;
	for (j = 0; j < 2; j++)
		for (i = 0; i < 2; i++)
		{
			real hit = depths[face][MIN(y + j, OCC_SIZE - 1) * OCC_SIZE + MIN(x + i, OCC_SIZE - 1)];
			real visible = MAX(0.0f, MIN(1.0f, 1.0f - (magnitude / extent - hit) * length / 0.025f));
			result += visible * (i ? tx : 1 - tx) * (j ? ty : 1 - ty);
		}
	return result;
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
static struct occlusion_field *field_for(long rasterizer_index)
{
	struct occlusion_field *field = NULL;
	struct rasterizer_light_submit_parameters *light;
	unsigned long now = GetTickCount();
	long identity = NONE;
	int i, x, y, z, stationary;
	real elapsed, extent, specular, clear_alpha, block_alpha, admission;
	real previous[OCC_COUNT];
	struct sample_axis history_axes[3][OCC_SIZE];
	real trace_dx, trace_dy, trace_dz;
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
	/* Cache collision depths briefly, but project and fade at every rendered pose.
	   Fast motion and radius changes invalidate the cache immediately. */
	if (!field->valid || now - field->traced >= 12 || field->trace_extent != extent ||
		trace_dx * trace_dx + trace_dy * trace_dy + trace_dz * trace_dz > extent * extent * 0.015625f)
	{
		trace_depths(extent, &light->position, covenant_emitter_lighting_occlusion_ignore(field->identity),
			muzzle_lighting_vehicle_occlusion(field->identity) || plasma_lighting_vehicle_occlusion(field->identity) ||
			covenant_emitter_lighting_vehicle_occlusion(field->identity), field->depths);
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
		for (i = 0; i < OCC_SIZE; i++)
		{
			real offset = (2.0f * (i + 0.5f) / OCC_SIZE - 1) * extent;
			sample_axis_initialize(&history_axes[0][i], 0.5f + (light->position.x + offset - field->center[0]) / (2 * field->extent));
			sample_axis_initialize(&history_axes[1][i], 0.5f + (light->position.y + offset - field->center[1]) / (2 * field->extent));
			sample_axis_initialize(&history_axes[2][i], 0.5f + (light->position.z + offset - field->center[2]) / (2 * field->extent));
		}
	for (z = 0; z < OCC_SIZE; z++)
		for (y = 0; y < OCC_SIZE; y++)
			for (x = 0; x < OCC_SIZE; x++)
			{
				real_vector3d ray, visibility_ray;
				real target, alpha, history;
				i = (z * OCC_SIZE + y) * OCC_SIZE + x;
				ray.i = (2.0f * (x + 0.5f) / OCC_SIZE - 1) * extent;
				ray.j = (2.0f * (y + 0.5f) / OCC_SIZE - 1) * extent;
				ray.k = (2.0f * (z + 0.5f) / OCC_SIZE - 1) * extent;
				visibility_ray.i = ray.i + trace_dx;
				visibility_ray.j = ray.j + trace_dy;
				visibility_ray.k = ray.k + trace_dz;
				target = depth_visibility(field->depths, &visibility_ray, extent);
				/* Reproject history at the receiver, not at a moving light-local cell. */
				history = target;
				if (stationary)
					history = previous[i];
				else if (field->valid && field->extent > 0)
				{
					if (history_axes[0][x].inside && history_axes[1][y].inside && history_axes[2][z].inside)
						history = sample_axes(previous, &history_axes[0][x], &history_axes[1][y], &history_axes[2][z]);
				}
				alpha = target < history ? block_alpha : clear_alpha;
				field->visibility[i] = history + (target - history) * alpha;
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
/* Build only a texture actually consumed by this view. CPU visibility history
   stays independent of pass order, and each pass uploads at most once. */
static void *field_texture(struct occlusion_field *field, int pass, long rasterizer)
{
	unsigned long pixels[OCC_COUNT];
	int x, y, z, i;
	real scale, axis[3], origin[3], radius;
	int directional = covenant_emitter_lighting_cone(rasterizer, axis, origin, &radius);
	if (!field)
		return NULL;
	/* Directional profiles can rotate without changing collision visibility.
	   Needler's lower-height fade is independent of the weapon's aim. */
	if (directional != field->directional ||
		(directional == 1 && memcmp(axis, field->direction, sizeof(field->direction))))
	{
		field->texture_mask = 0;
		field->directional = directional;
		if (directional == 1) memcpy(field->direction, axis, sizeof(field->direction));
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
	scale = (pass == 2 ? field->specular : 1.0f) * field->radius / field->extent;
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
					value = sample_field(field->visibility, 0.5f + dx * scale * 0.5f,
						0.5f + dy * scale * 0.5f, 0.5f + dz * scale * 0.5f);
				value = 1 - field->admission + field->admission * value;
				if (directional)
					value *= directional == 2 ? needler_falloff_factor(dz *
						(pass == 0 ? field->extent / field->radius : pass == 2 ? field->specular : 1.0f)) :
						ammo_counter_falloff_factor(axis, dx, dy, dz);
				if (pass)
					value *= remaining * remaining;
				pixels[i] = (unsigned long)(255 * value + 0.5f) * 0x01010101UL;
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
