/*
PHYSICS_DRAW.C

The host's Jolt bodies drawn as wireframes over the game, for tuning them
(debug.physics_draw): each ragdoll's parts and each piece of debris, at the
poses the host last gave, with the game's own debug lines
(render_debug.c). A ragdoll's part is green while its ragdoll is awake and
grey asleep, red on a limb torn from it; a piece of debris yellow awake and
grey asleep. Called from render_debug() each frame; nothing is drawn with
the setting off.
*/

#include "cseries/cseries.h"
#include "math/real_math.h"
#include "render/render_debug.h"
#include "physics_debris.h"
#include "physics_draw.h"
#include "physics_jolt.h"
#include "ragdoll_bodies.h"
#include "ragdolls.h"

#if defined(HALO_GUEST_VM) && defined(HALO_METAL)
#include "host_physics.h"
#else
#define HOST_PHYSICS_SHAPE_SPHERE 0
#define HOST_PHYSICS_SHAPE_CAPSULE 1
#define HOST_PHYSICS_SHAPE_BOX 2
#endif

/* port/linux/src/port_config.c */
int config_boolean(const char *name);
unsigned long config_changes(void);

static unsigned long physics_draw_changes = (unsigned long)-1;
static boolean physics_draw_on = FALSE;

static real_argb_color const physics_draw_awake = { { 1.0f, 0.2f, 1.0f, 0.3f } };
static real_argb_color const physics_draw_asleep = { { 1.0f, 0.55f, 0.55f, 0.55f } };
static real_argb_color const physics_draw_torn = { { 1.0f, 1.0f, 0.2f, 0.2f } };
static real_argb_color const physics_draw_debris_awake = { { 1.0f, 1.0f, 0.85f, 0.2f } };

boolean physics_draw_enabled(void)
{
	if (config_changes() != physics_draw_changes)
	{
		physics_draw_changes = config_changes();
		physics_draw_on = config_boolean("debug.physics_draw");
	}
	return physics_draw_on && physics_jolt_enabled();
}

/* a point in a pose's frame, in the world's */
static void physics_draw_point(
	struct ragdoll_pose const *pose,
	real_point3d const *point,
	real_point3d *result)
{
	real_quaternion const *q = &pose->rotation;
	real_vector3d t;

	/* (p + 2w(v x p) + v x 2(v x p)) */
	t.i = 2.0f * (q->v.j * point->z - q->v.k * point->y);
	t.j = 2.0f * (q->v.k * point->x - q->v.i * point->z);
	t.k = 2.0f * (q->v.i * point->y - q->v.j * point->x);
	result->x = pose->position.x + point->x + q->w * t.i + (q->v.j * t.k - q->v.k * t.j);
	result->y = pose->position.y + point->y + q->w * t.j + (q->v.k * t.i - q->v.i * t.k);
	result->z = pose->position.z + point->z + q->w * t.k + (q->v.i * t.j - q->v.j * t.i);
}

/* A shape (HOST_PHYSICS_SHAPE_*) of a size (as the host has them: a
sphere's radius; a capsule's radius and half its cylinder's length, along
its frame's x axis; a box's half extents), centred at `center` in a pose's
frame. */
void physics_draw_shape(
	struct ragdoll_pose const *pose,
	short shape,
	real const *size,
	real_point3d const *center,
	real_argb_color const *color)
{
	real_point3d middle;

	physics_draw_point(pose, center, &middle);
	if (shape == HOST_PHYSICS_SHAPE_SPHERE)
	{
		render_debug_sphere(TRUE, &middle, size[0], color);
	}
	else if (shape == HOST_PHYSICS_SHAPE_CAPSULE)
	{
		real_point3d end;
		real_point3d base;
		real_vector3d height;

		end = *center;
		end.x += size[1];
		physics_draw_point(pose, &end, &end);
		base.x = 2.0f * middle.x - end.x;
		base.y = 2.0f * middle.y - end.y;
		base.z = 2.0f * middle.z - end.z;
		height.i = end.x - base.x;
		height.j = end.y - base.y;
		height.k = end.z - base.z;
		render_debug_pill(TRUE, &base, &height, size[0], color);
	}
	else if (shape == HOST_PHYSICS_SHAPE_BOX)
	{
		real_point3d corners[8];
		short index;

		/* corner n is at +x for bit 0, +y for bit 1, +z for bit 2 */
		for (index = 0; index < 8; index++)
		{
			real_point3d corner;

			corner.x = center->x + ((index & 1) ? size[0] : -size[0]);
			corner.y = center->y + ((index & 2) ? size[1] : -size[1]);
			corner.z = center->z + ((index & 4) ? size[2] : -size[2]);
			physics_draw_point(pose, &corner, &corners[index]);
		}
		/* each edge: from a corner to the one differing in one bit above */
		for (index = 0; index < 8; index++)
		{
			short bit;

			for (bit = 1; bit < 8; bit <<= 1)
			{
				if (!(index & bit))
					render_debug_line(TRUE, &corners[index], &corners[index | bit], color);
			}
		}
	}
}

void physics_draw_render(void)
{
	real_point3d const origin = { 0.0f, 0.0f, 0.0f };
	long id;

	if (!physics_draw_enabled())
		return;
	for (id = 1; id <= RAGDOLLS_MAX; id++)
	{
		struct ragdoll const *ragdoll = ragdolls_get(id);
		short part_index;

		if (!ragdoll || !ragdoll->posed)
			continue;
		for (part_index = 0; part_index < ragdoll->body->part_count; part_index++)
		{
			struct ragdoll_part const *part = &ragdoll->body->parts[part_index];
			real_argb_color const *color = !ragdoll->awake ? &physics_draw_asleep :
				ragdoll_part_piece(ragdoll, part_index) != 0 ? &physics_draw_torn : &physics_draw_awake;

			physics_draw_shape(&ragdoll->poses[part_index], part->shape, part->size, &part->center, color);
		}
	}
	for (id = PHYSICS_DEBRIS_FIRST_ID; id < PHYSICS_DEBRIS_FIRST_ID + PHYSICS_DEBRIS_MAX; id++)
	{
		struct physics_debris_body body;
		boolean awake;

		if (physics_debris_posed_body(id, &body, &awake))
		{
			physics_draw_shape(&body.pose, (short)body.shape, body.size, &origin,
				awake ? &physics_draw_debris_awake : &physics_draw_asleep);
		}
	}
}
