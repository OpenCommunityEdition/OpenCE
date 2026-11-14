/*
FIRST_PERSON_LEGS_MIRROR.C (test)

The real first_person_legs_node_matrices and first_person_legs_mirror_node_matrices (first_person_legs.c): the
first-person legs are set back behind the eye, and a mirror, drawn before the window's own view with a camera of
its own, draws the whole body and what it carries set back as far, as the window's own view finds it, so that the
legs meet their reflection. The fake nodes are translations (their rotations the identity, every node's inverse
the identity); the eye looks down along +x. test_first_person_legs_mirror.py takes the constants and the code from
the sources (config.inc, mesh.inc, under_test.inc).
*/

#include <math.h>
#include "harness.h"

#define TAG_STRING_LENGTH 31
#define NUMBEROF(a) (sizeof(a) / sizeof((a)[0]))

#include "config.inc"

typedef struct { real scale; real_vector3d forward, left, up; real_point3d position; } real_matrix4x3;
typedef struct { real_vector3d n; real d; } real_plane3d;

static void matrix4x3_multiply(real_matrix4x3 const *a, real_matrix4x3 const *b, real_matrix4x3 *out)
{
	real_matrix4x3 product = *a;

	product.position.x += b->position.x;
	product.position.y += b->position.y;
	product.position.z += b->position.z;
	*out = product;
}
static void matrix4x3_transform_point(real_matrix4x3 const *matrix, real_point3d const *point, real_point3d *out)
{
	out->x = point->x + matrix->position.x;
	out->y = point->y + matrix->position.y;
	out->z = point->z + matrix->position.z;
}
static real dot_product3d(real_vector3d const *a, real_vector3d const *b) { return a->i * b->i + a->j * b->j + a->k * b->k; }
static void vector_from_points3d(real_point3d const *from, real_point3d const *to, real_vector3d *out)
{
	out->i = to->x - from->x;
	out->j = to->y - from->y;
	out->k = to->z - from->z;
}
static real square_root(real value) { return sqrtf(value); }
static real normalize3d(real_vector3d *v)
{
	real length = sqrtf(dot_product3d(v, v));

	if (length > 0.f)
	{
		v->i /= length;
		v->j /= length;
		v->k /= length;
	}
	return length;
}
static real plane3d_distance_to_point(real_plane3d const *plane, real_point3d const *point)
{
	return plane->n.i * point->x + plane->n.j * point->y + plane->n.k * point->z - plane->d;
}
static real_vector3d const zero_vector = { 0.f, 0.f, 0.f };
static real_vector3d const *global_zero_vector3d = &zero_vector;

/* the settings */
static int legs_setting = 1;
static unsigned long setting_changes = 1;
static int config_boolean(const char *name) { CHECK(!strcmp(name, "display.first_person_legs"), "%s", name); return legs_setting; }
static unsigned long config_changes(void) { return setting_changes; }

/* the model: its nodes, their names and parents */
struct tag_block { long count; void *address; };
#define TAG_BLOCK_GET_ELEMENT(block, element, type) (&((type *)(block)->address)[(long)(element)])
struct model_node { char name[32]; short parent_node_index; };
struct model { struct tag_block nodes; };
enum { ROOT, PELVIS, SPINE, LEFT_THIGH, LEFT_CALF, HEAD, NODE_COUNT };
static struct model_node node_names[NODE_COUNT] = {
	{ "frame root", NONE }, { "bip01 pelvis", ROOT }, { "bip01 spine", PELVIS }, { "bip01 l thigh", PELVIS },
	{ "bip01 l calf", LEFT_THIGH }, { "bip01 head", SPINE }
};
static struct model model = { { NODE_COUNT, node_names } };
static struct model *model_definition_get(long index) { CHECK(index == 5, "the body's model"); return &model; }

/* the objects: the body (1), its weapon (2), another player's body (3) and its weapon (4) */
enum { _object_type_biped = 0, _object_type_weapon = 2 };
enum { _object_dead_bit = 0, _object_invisible_bit = 0 };
struct object_datum
{
	long definition_index;
	struct
	{
		short type;
		long parent_object_index;
		unsigned long flags;
		unsigned long damage_flags;
		struct { long size; } node_matrices;
	} object;
};
static struct object_datum objects[5];
static struct object_datum *object_try_and_get(long index) { return index >= 1 && index < 5 ? &objects[index] : NULL; }
struct object_definition { struct { struct { long index; } model; } object; };
static struct object_definition body_definition = { { { 5 } } };
static struct object_definition *object_definition_get(long index) { CHECK(index == 9, "the body's definition"); return &body_definition; }

/* the mesh: the hips' rim (on a triangle that runs up into the spine) */
#include "mesh.inc"
static struct object_mesh_vertex vertices[3];
static unsigned short triangles[1][3] = { { 0, 1, 2 } };
static struct object_mesh mesh;
static struct object_mesh const *object_mesh_get(long object_index) { return object_index == 1 || object_index == 3 ? &mesh : NULL; }

/* the players, the director and the scripted camera */
enum { _director_perspective_first_person = 0, _director_perspective_following = 1 };
struct player_datum { long unit_index; };
static struct player_datum player = { 1 };
static short perspective = _director_perspective_first_person;
static long scripted_first_person = NONE;
static long local_player_get_player_index(short local_player_index) { return local_player_index == 0 ? 7 : NONE; }
static struct player_datum *player_get(long player_index) { CHECK(player_index == 7, "the local player's"); return &player; }
static short director_get_perspective(short local_player_index) { CHECK(local_player_index == 0, "the window's"); return perspective; }
static boolean scripted_camera_object_is_first_person_camera(long object_index) { return object_index == scripted_first_person; }

/* the windows: the window's own view looking down, a mirror's looking up at it from below the floor */
struct render_camera { real_point3d position; real_vector3d forward; };
struct render_frustum { real_plane3d world_planes[6]; real z_near; };
static struct { struct render_camera camera; struct render_frustum frustum; } global_window_parameters;
static struct { long scene_index; } render;

#include "under_test.inc"

static struct render_camera view_camera, mirror_camera;
static struct render_frustum view_frustum, mirror_frustum;
static real_matrix4x3 nodes[NODE_COUNT], weapon_nodes[2], other_nodes[NODE_COUNT];

/* a camera at eye looking down pitch radians below level along +x, the bottom of its view half_fov below that
(the bottom plane's normal out of the view) */
static void view(struct render_camera *camera, struct render_frustum *frustum, real eye_z, real pitch, real half_fov)
{
	real below = pitch + half_fov;

	memset(camera, 0, sizeof(*camera));
	memset(frustum, 0, sizeof(*frustum));
	camera->position.z = eye_z;
	camera->forward.i = cosf(pitch);
	camera->forward.k = -sinf(pitch);
	frustum->world_planes[2].n.i = -sinf(below);
	frustum->world_planes[2].n.k = -cosf(below);
	frustum->world_planes[2].d = frustum->world_planes[2].n.k * eye_z;
	frustum->z_near = 0.0625f;
}

static void translation(real_matrix4x3 *matrix, real x, real y, real z)
{
	memset(matrix, 0, sizeof(*matrix));
	matrix->scale = 1.f;
	matrix->forward.i = matrix->left.j = matrix->up.k = 1.f;
	matrix->position.x = x;
	matrix->position.y = y;
	matrix->position.z = z;
}

static void reset(void)
{
	short index;

	memset(objects, 0, sizeof(objects));
	for (index = 1; index < 5; index++)
		objects[index].object.parent_object_index = NONE;
	objects[1].definition_index = objects[3].definition_index = 9;
	objects[1].object.type = objects[3].object.type = _object_type_biped;
	objects[1].object.node_matrices.size = objects[3].object.node_matrices.size = NODE_COUNT * sizeof(real_matrix4x3);
	objects[2].object.type = objects[4].object.type = _object_type_weapon;
	objects[2].object.parent_object_index = 1;
	objects[4].object.parent_object_index = 3;
	objects[2].object.node_matrices.size = objects[4].object.node_matrices.size = 2 * sizeof(real_matrix4x3);
	translation(&nodes[ROOT], 0.f, 0.f, 0.f);
	translation(&nodes[PELVIS], 0.f, 0.f, 0.4f);
	translation(&nodes[SPINE], 0.f, 0.f, 0.5f);
	translation(&nodes[LEFT_THIGH], 0.f, 0.05f, 0.35f);
	translation(&nodes[LEFT_CALF], 0.02f, 0.05f, 0.2f);
	translation(&nodes[HEAD], 0.f, 0.f, 0.62f);
	memcpy(other_nodes, nodes, sizeof(nodes));
	translation(&weapon_nodes[0], 0.1f, -0.1f, 0.45f);
	translation(&weapon_nodes[1], 0.2f, -0.1f, 0.47f);

	memset(&mesh, 0, sizeof(mesh));
	mesh.key = 1;
	mesh.model_index = 5;
	mesh.node_count = NODE_COUNT;
	mesh.vertex_count = 3;
	mesh.triangle_count = 1;
	mesh.vertices = vertices;
	mesh.triangles = triangles;
	for (index = 0; index < NODE_COUNT; index++)
		translation(&mesh.inverse[index], 0.f, 0.f, 0.f);
	/* (the hips' rim, ahead of the hips and out to the sides, and the spine above) */
	vertices[0].position.x = 0.08f; vertices[0].position.y = 0.1f; vertices[0].nodes[0] = vertices[0].nodes[1] = PELVIS; vertices[0].weight = 32767;
	vertices[1].position.x = 0.08f; vertices[1].position.y = -0.1f; vertices[1].nodes[0] = vertices[1].nodes[1] = PELVIS; vertices[1].weight = 32767;
	vertices[2].position.z = 0.1f; vertices[2].nodes[0] = vertices[2].nodes[1] = SPINE; vertices[2].weight = 32767;
	first_person_legs_waist_key = NONE;

	player.unit_index = 1;
	perspective = _director_perspective_first_person;
	scripted_first_person = NONE;
	legs_setting = 1;
	setting_changes++;
	/* the window's own view, looking well down (the legs set back past the least), and a mirror's below the floor */
	view(&view_camera, &view_frustum, 0.62f, 1.1f, 0.5f);
	view(&mirror_camera, &mirror_frustum, -0.62f, -1.1f, 0.5f);
	first_person_legs_mirror_view(NONE, NULL, NULL);
}

static real distance(real_point3d const *a, real_point3d const *b)
{
	real dx = a->x - b->x, dy = a->y - b->y, dz = a->z - b->z;

	return sqrtf(dx * dx + dy * dy + dz * dz);
}

/* the window's own view drawn: its legs, and how far they are set back (their root's move) */
static real_matrix4x3 const *legs_drawn(real_vector3d *offset)
{
	real_matrix4x3 const *legs;

	render.scene_index++;
	global_window_parameters.camera = view_camera;
	global_window_parameters.frustum = view_frustum;
	legs = first_person_legs_node_matrices(1, nodes);
	CHECK(legs != NULL, "the window's own view draws the legs");
	offset->i = legs[ROOT].position.x - nodes[ROOT].position.x;
	offset->j = legs[ROOT].position.y - nodes[ROOT].position.y;
	offset->k = legs[ROOT].position.z - nodes[ROOT].position.z;
	return legs;
}

/* a mirror's scene begun, for the window of local player 0 */
static void mirror_begun(void)
{
	render.scene_index++;
	global_window_parameters.camera = mirror_camera;
	global_window_parameters.frustum = mirror_frustum;
	first_person_legs_mirror_view(0, &view_camera, &view_frustum);
}

/* every node of the pose drawn moved by offset from nodes, whole */
static boolean moved_whole(real_matrix4x3 const *drawn, real_matrix4x3 const *from, short count, real_vector3d const *offset)
{
	short index;

	for (index = 0; index < count; index++)
	{
		real_point3d expected = from[index].position;

		expected.x += offset->i;
		expected.y += offset->j;
		expected.z += offset->k;
		if (distance(&drawn[index].position, &expected) > 1e-5f || drawn[index].scale != from[index].scale)
			return FALSE;
	}
	return TRUE;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";
	real_vector3d offset;
	real_matrix4x3 legs[NODE_COUNT];
	real_matrix4x3 const *drawn;

	/* the legs as the window's own view draws them, and the body in the mirror drawn before it, in the next frame */
	CASE("meets")
	{
		reset();
		memcpy(legs, legs_drawn(&offset), sizeof(legs));
		CHECK(sqrtf(dot_product3d(&offset, &offset)) > FIRST_PERSON_LEGS_SET_BACK + 0.01f,
			"looking down, the legs set back past the least (%f)", sqrtf(dot_product3d(&offset, &offset)));
		CHECK(offset.i < 0.f && fabsf(offset.k) < 1e-6f, "set back level, behind the eye (%f %f %f)", offset.i, offset.j, offset.k);
		CHECK(legs[HEAD].scale < 0.01f, "the window's own view folds the upper body");
		mirror_begun();
		drawn = first_person_legs_mirror_node_matrices(1, nodes);
		CHECK(drawn != nodes, "the body in the mirror is set back");
		CHECK(moved_whole(drawn, nodes, NODE_COUNT, &offset), "the whole body in the mirror, set back as far as the legs");
		CHECK(distance(&drawn[LEFT_CALF].position, &legs[LEFT_CALF].position) < 1e-5f &&
			distance(&drawn[LEFT_THIGH].position, &legs[LEFT_THIGH].position) < 1e-5f &&
			distance(&drawn[PELVIS].position, &legs[PELVIS].position) < 1e-5f,
			"the legs meet their reflection");
		first_person_legs_mirror_view(NONE, NULL, NULL);
		CHECK(first_person_legs_mirror_node_matrices(1, nodes) == nodes, "the mirror done, the body is as it is");
	}
	/* what the body carries set back with it; another player and what they carry as they are */
	CASE("carried")
	{
		reset();
		legs_drawn(&offset);
		mirror_begun();
		first_person_legs_mirror_node_matrices(1, nodes);
		drawn = first_person_legs_mirror_node_matrices(2, weapon_nodes);
		CHECK(drawn != weapon_nodes && moved_whole(drawn, weapon_nodes, 2, &offset), "the weapon it carries set back with it");
		CHECK(first_person_legs_mirror_node_matrices(3, other_nodes) == other_nodes, "another player's body as it is");
		CHECK(first_person_legs_mirror_node_matrices(4, weapon_nodes) == weapon_nodes, "what another carries as it is");
	}
	/* what it carries, in a scene its body was not set back in (a body it could not be found for, or the next) */
	CASE("carried alone")
	{
		reset();
		mirror_begun();
		first_person_legs_mirror_node_matrices(1, nodes);
		render.scene_index++;
		CHECK(first_person_legs_mirror_node_matrices(2, weapon_nodes) == weapon_nodes, "a scene the body was not drawn in");
		reset();
		mirror_begun();
		mesh.vertices[2].nodes[0] = mesh.vertices[2].nodes[1] = PELVIS;
		CHECK(first_person_legs_mirror_node_matrices(1, nodes) == nodes, "a body without a waist as it is");
		CHECK(first_person_legs_mirror_node_matrices(2, weapon_nodes) == weapon_nodes, "and what it carries");
	}
	/* only the body seen from inside in the window's own view */
	CASE("perspective")
	{
		reset();
		perspective = _director_perspective_following;
		mirror_begun();
		CHECK(first_person_legs_mirror_node_matrices(1, nodes) == nodes, "seen from behind, the body as it is");
		scripted_first_person = 1;
		CHECK(first_person_legs_mirror_node_matrices(1, nodes) != nodes, "a scripted first-person camera's body set back");
		reset();
		player.unit_index = 3;
		mirror_begun();
		CHECK(first_person_legs_mirror_node_matrices(1, nodes) == nodes, "a body not the window's player's as it is");
		CHECK(first_person_legs_mirror_node_matrices(3, other_nodes) != other_nodes, "the window's player's set back");
	}
	/* no legs, no mirror: as the game draws it */
	CASE("as is")
	{
		reset();
		CHECK(first_person_legs_mirror_node_matrices(1, nodes) == nodes, "outside a mirror");
		legs_setting = 0;
		setting_changes++;
		mirror_begun();
		CHECK(first_person_legs_mirror_node_matrices(1, nodes) == nodes, "the legs off");
		legs_setting = 1;
		setting_changes++;
		objects[1].object.damage_flags = FLAG(_object_dead_bit);
		CHECK(first_person_legs_mirror_node_matrices(1, nodes) == nodes, "a dead body");
		objects[1].object.damage_flags = 0;
		first_person_legs_mirror_view(0, NULL, NULL);
		CHECK(first_person_legs_mirror_node_matrices(1, nodes) == nodes, "a mirror with no view");
	}
	return 0;
}
