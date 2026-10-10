/*
FIRST_PERSON_LEGS.C

Your own legs in first person (Video Setup > FOV and Viewmodels > Legs): the
player's own body, which the game animates as everyone else sees it but does
not draw for its own player, drawn from the waist down. Look down while
standing, running, strafing or crouching and the legs move as the body's
animation moves them.

Only the drawing is changed: a copy of the body's pose, all but its hips and
legs folded into a lid over the waist, the whole set back a little behind
the eye so the camera never looks out through the hips. The body itself, its hit boxes and markers, are the game's
as ever. The legs are drawn in the world with the world's camera, so they
keep their place and size at any field of view.
*/

#include "cseries.h"
#include "real_math.h"
#include "objects/objects.h"
#include "models/model_definitions.h"
#include "units/units.h"
#include "render/render.h"
#include "rasterizer/rasterizer.h"
#include <xtl.h>
#include "rasterizer/xbox/rasterizer_xbox.h"

#include "object_mesh.h"


#include <string.h>

/* port/linux/src/port_config.c */
int config_boolean(const char *name);
unsigned long config_changes(void);

enum
{
	FIRST_PERSON_LEGS_MAXIMUM_NODES = 64,
	FIRST_PERSON_LEGS_MAXIMUM_RIM = 64,
};

/* how far the body is set back behind the eye at least, world units (about
12 cm), and at most (about 1 m) */
#define FIRST_PERSON_LEGS_SET_BACK 0.04f
#define FIRST_PERSON_LEGS_SET_BACK_MAXIMUM 0.33f
/* how far out of the view the waist's rim is kept (world units: 3 cm) */
#define FIRST_PERSON_LEGS_OUT_OF_VIEW 0.01f
/* the point the upper body folds into: the middle of the waist's rim, a
little above it (world units: about 3 cm), so the skin that joined hips to
chest closes the waist as a low lid, flush with the belt (the body's model is
a hollow shell: folded higher, it stood up as a cone before the eye; lower,
it sank into a funnel) */
#define FIRST_PERSON_LEGS_LID_UP 0.01f

static real_matrix4x3 first_person_legs_matrices[FIRST_PERSON_LEGS_MAXIMUM_NODES];
static boolean first_person_legs_enabled(
	void)
{
	static unsigned long changes = (unsigned long)-1;
	static boolean enabled = FALSE;

	if (changes != config_changes())
	{
		changes = config_changes();
		enabled = config_boolean("display.first_person_legs") != 0;
	}
	return enabled;
}

/* the player's own unit, seen from inside it: its legs drawn (on foot,
alive, not hidden) */
boolean first_person_legs_wanted(
	long object_index)
{
	struct object_datum *object = object_try_and_get(object_index);

	return first_person_legs_enabled() && object && object->object.type == _object_type_biped &&
		object->object.parent_object_index == NONE &&
		!TEST_FLAG(object->object.damage_flags, _object_dead_bit) &&
		!TEST_FLAG(object->object.flags, _object_invisible_bit);
}

/* a node of the legs: the hips, thighs, calves, feet and toes (by name, as
the game's characters name them: "bip01 l thigh") */
static boolean first_person_legs_node(
	char const *name)
{
	static char const *const parts[] = { "pelvis", "thigh", "calf", "foot", "toe", "leg" };
	char lower[TAG_STRING_LENGTH + 1];
	short index;

	for (index = 0; index < TAG_STRING_LENGTH && name[index]; index++)
		lower[index] = (char)(name[index] >= 'A' && name[index] <= 'Z' ? name[index] - 'A' + 'a' : name[index]);
	lower[index] = 0;
	for (index = 0; index < (short)NUMBEROF(parts); index++)
	{
		if (strstr(lower, parts[index]))
			return TRUE;
	}
	return FALSE;
}

/* the middle of the waist's rim in the model's space: the hips' vertices
on triangles that run up into the body above them (FALSE: none) */
static real_point3d first_person_legs_rim[FIRST_PERSON_LEGS_MAXIMUM_RIM];
static short first_person_legs_rim_count = 0;

static boolean first_person_legs_waist(
	struct object_mesh const *mesh,
	struct model const *model,
	short pelvis,
	real_point3d *middle)
{
	static long cached_key = NONE;
	static boolean cached_found = FALSE;
	static real_point3d cached_middle;
	real_point3d sum = { 0.f, 0.f, 0.f };
	long found = 0;
	long index;

	if (mesh->key == cached_key)
	{
		*middle = cached_middle;
		return cached_found;
	}
	first_person_legs_rim_count = 0;
	for (index = 0; index < mesh->triangle_count; index++)
	{
		boolean hips = FALSE;
		boolean above = FALSE;
		short corner;

		for (corner = 0; corner < 3; corner++)
		{
			struct object_mesh_vertex const *vertex = &mesh->vertices[mesh->triangles[index][corner]];
			short n;

			if (vertex->nodes[0] == pelvis && (vertex->nodes[1] == pelvis || vertex->weight >= 32767))
				hips = TRUE;
			for (n = 0; n < 2; n++)
			{
				if (vertex->nodes[n] < model->nodes.count && (n == 0 || vertex->weight < 32767) &&
					!first_person_legs_node(TAG_BLOCK_GET_ELEMENT(&model->nodes, vertex->nodes[n], struct model_node)->name))
				{
					above = TRUE;
				}
			}
		}
		if (!hips || !above)
			continue;
		for (corner = 0; corner < 3; corner++)
		{
			struct object_mesh_vertex const *vertex = &mesh->vertices[mesh->triangles[index][corner]];

			if (vertex->nodes[0] == pelvis && (vertex->nodes[1] == pelvis || vertex->weight >= 32767))
			{
				sum.x += vertex->position.x;
				sum.y += vertex->position.y;
				sum.z += vertex->position.z;
				found++;
				if (first_person_legs_rim_count < FIRST_PERSON_LEGS_MAXIMUM_RIM)
					first_person_legs_rim[first_person_legs_rim_count++] = vertex->position;
			}
		}
	}
	cached_key = mesh->key;
	cached_found = found > 0;
	if (found > 0)
	{
		cached_middle.x = sum.x / found;
		cached_middle.y = sum.y / found;
		cached_middle.z = sum.z / found + FIRST_PERSON_LEGS_LID_UP;
	}
	*middle = cached_middle;
	return cached_found;
}

/* the pose to draw the legs with: the body's own (as it is drawn now), all
but the legs folded into the hips, the whole set back behind the eye; NULL:
none (a body without named legs) */
real_matrix4x3 const *first_person_legs_node_matrices(
	long object_index,
	real_matrix4x3 const *nodes)
{
	struct object_datum *object = object_try_and_get(object_index);
	struct object_definition *definition;
	struct model const *model;
	long count;
	short pelvis = NONE;
	short thighs = 0;
	short index;
	real_point3d fold;
	real_vector3d back;
	real_matrix4x3 skin;
	real set_back;

	if (!object || !nodes)
		return NULL;
	definition = object_definition_get(object->definition_index);
	if (!definition || definition->object.model.index == NONE)
		return NULL;
	model = model_definition_get(definition->object.model.index);
	count = object->object.node_matrices.size / (long)sizeof(real_matrix4x3);
	if (!model || count <= 0 || count > FIRST_PERSON_LEGS_MAXIMUM_NODES || model->nodes.count < count)
		return NULL;

	for (index = 0; index < count; index++)
	{
		struct model_node const *node = TAG_BLOCK_GET_ELEMENT(&model->nodes, index, struct model_node);

		if (pelvis == NONE && strstr(node->name, "pelvis"))
			pelvis = index;
		if (strstr(node->name, "thigh"))
			thighs++;
	}
	if (pelvis == NONE || thighs == 0)
		return NULL;
	/* (the waist's middle as the hips are posed now) */
	{
		struct object_mesh const *mesh = object_mesh_get(object_index);
		real_point3d middle;

		if (!mesh || pelvis >= mesh->node_count || !first_person_legs_waist(mesh, model, pelvis, &middle))
			return NULL;
		matrix4x3_multiply(&nodes[pelvis], &mesh->inverse[pelvis], &skin);
		matrix4x3_transform_point(&skin, &middle, &fold);
	}

	/* (set back away from the way the eye looks, level) */
	back.i = -global_window_parameters.camera.forward.i;
	back.j = -global_window_parameters.camera.forward.j;
	back.k = 0.f;
	if (normalize3d(&back) <= 0.f)
		back = *global_zero_vector3d;

	/* (as far back as keeps the waist's rim out of the bottom of the view,
	as Halo 2 keeps its first person body's open top out of it: looking
	ahead, the least; looking down, the legs stand behind the eye, the
	thighs, knees and feet seen as they swing forward) */
	set_back = FIRST_PERSON_LEGS_SET_BACK;
	{
		real_plane3d const *bottom = &global_window_parameters.frustum.world_planes[2];
		real toward = dot_product3d(&bottom->n, &back);
		short corner;

		if (toward > 0.05f)
		{
			for (corner = 0; corner < first_person_legs_rim_count; corner++)
			{
				real_point3d p;
				real distance;

				matrix4x3_transform_point(&skin, &first_person_legs_rim[corner], &p);
				distance = plane3d_distance_to_point(bottom, &p) + toward * FIRST_PERSON_LEGS_SET_BACK;
				if (distance < FIRST_PERSON_LEGS_OUT_OF_VIEW)
				{
					real needed = FIRST_PERSON_LEGS_SET_BACK + (FIRST_PERSON_LEGS_OUT_OF_VIEW - distance) / toward;

					if (needed > set_back)
						set_back = needed;
				}
			}
		}
		if (set_back > FIRST_PERSON_LEGS_SET_BACK_MAXIMUM)
			set_back = FIRST_PERSON_LEGS_SET_BACK_MAXIMUM;
	}

	for (index = 0; index < count; index++)
	{
		struct model_node const *node = TAG_BLOCK_GET_ELEMENT(&model->nodes, index, struct model_node);
		real_matrix4x3 *matrix = &first_person_legs_matrices[index];

		*matrix = nodes[index];
		/* (the root kept: it is the body's frame, not a part of it) */
		if (node->parent_node_index != NONE && !first_person_legs_node(node->name))
		{
			matrix->position = fold;
			matrix->scale = 0.001f;
		}
		matrix->position.x += back.i * set_back;
		matrix->position.y += back.j * set_back;
		matrix->position.z += back.k * set_back;
	}
	return first_person_legs_matrices;
}
