/*
VR_HANDS.C

The VR mode's first-person hands and arms (HALO_VR; vr.hands,
port/linux/README.md "VR"), for vr_render.c and first_person_weapons.c.

The first-person weapon and the arms are two models posed by one animation
graph, whose nodes the game builds into matrices each frame about the camera
the weapon is posed from. Each hand is put where its controller is: the
middle of the hand (its wrist and knuckles) at the controller's grip pose,
turned with the controller's pointing pose as the weapon's idle animation
has it turned from the camera. The right hand holds the weapon, so it is
the camera that is placed (vr_render_weapon_camera, from vr_hands_right_
centre): the weapon and the right hand are then drawn as the game animates
them. The left hand is moved there once the game (and the interpolation
between ticks) has built the nodes (vr_hands_pose), the left wrist with its
fingers, in the idle pose's shape: open, closing as the left grip is pulled.
It is on the weapon as animated only while it holds a long gun's foregrip
(vr.two_handed). The right index finger pulls as the trigger does, and the
weapon's own trigger, where it has one, with it.

With vr.hands = "arms", each arm reaches its hand from a shoulder: the
shoulders are below and beside the eyes, turned with the body (the head's
heading, which follows the head only past a turn of the head, and slowly),
and the upper arm and the forearm, their lengths the model's, are turned
to reach the wrist with the elbow bent down and out (two bones, solved by
the law of cosines). Beyond the arm's reach the shoulder comes forward, so
the hand stays in the controller and the arm straight. The forearm twists
with the hand. With vr.hands = "floating" the arms are collapsed into the
wrists instead (a node scaled to almost nothing draws its vertices at its
origin). A hand whose controller is not tracked is not drawn, nor its arm.
*/

#ifdef HALO_VR

#include "cseries.h"
#include "math/real_math.h"
#include "models/model_animation_definitions.h"
#include "render/render_debug.h"
#include "tag_files/tag_files.h"

#include "vr_hands.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

/* what is not drawn: scaled to this (not 0, which a vertex shader
normalizing the normals would divide by) */
#define COLLAPSED_SCALE 0.0001f
/* the body's heading: the head turns this far from it before it follows,
and it follows the rest of the way with this time constant (seconds) */
#define BODY_TURN_FREE 0.70f
#define BODY_TURN_SECONDS 1.5f
/* beyond an arm's reach, its shoulder comes forward this far at most
(metres), and the arm stretches the rest */
#define SHOULDER_REACH 0.05f
/* the fingers' curls (radians, at each joint from the knuckle): the right
index finger's at a full pull of the trigger, and the left hand's,
relaxed and with the grip pulled */
#define TRIGGER_CURL_0 0.35f
#define TRIGGER_CURL_1 0.55f
#define TRIGGER_CURL_2 0.35f
#define GRIP_OPEN_CURL -0.10f
#define GRIP_CLOSED_CURL 1.30f
/* the weapon's trigger node, at a full pull: back toward the grip, this
far (metres; the node's origin is not the trigger's pivot, and holds its
guard too, so it is moved, not turned) */
#define WEAPON_TRIGGER_TRAVEL 0.005f

enum
{
	_left = 0,
	_right,
	NUMBER_OF_SIDES,
};

/* the hand's fingers, as their nodes are named */
enum
{
	_finger_index = 0,
	_finger_middle,
	_finger_ring,
	_finger_pinky,
	_finger_thumb,
	NUMBER_OF_FINGERS,
	NUMBER_OF_FINGER_JOINTS = 3,
};

/* the models' nodes (first_person_weapon_vr_idle_pose's node_models) */
enum
{
	_vr_hands_node_weapon_bit = 0,
	_vr_hands_node_hands_bit,
};

/* the nodes' roles: of a side's hand, of its arm, or neither */
enum
{
	_node_other = 0,
	_node_left_hand,
	_node_right_hand,
	_node_left_arm,
	_node_right_arm,
};

/* an animation graph's node (model_animations.c's) */
struct vr_hands_graph_node
{
	char name[TAG_STRING_LENGTH+1];
	short next_sibling_node_index;
	short first_child_node_index;
	short parent_node_index;
	word pad;
	unsigned long flags;
	real_vector3d base_vector;
	real range;
	long pad1;
};

static struct
{
	/* the first-person animation graph whose nodes are known (the weapon
	in hand's), and whether it has both wrists, and both arms */
	long animation_graph_index;
	boolean usable;
	boolean arms_usable;
	short node_count;
	short wrist[NUMBER_OF_SIDES];
	short forearm[NUMBER_OF_SIDES];
	short upper_arm[NUMBER_OF_SIDES];
	short fingers[NUMBER_OF_SIDES][NUMBER_OF_FINGERS][NUMBER_OF_FINGER_JOINTS];
	/* the sign that curls each hand's fingers about the line of its
	knuckles, from the index finger's to the little finger's */
	real curl_sign[NUMBER_OF_SIDES];
	/* the weapon's trigger */
	short trigger;
	byte roles[MAXIMUM_NODES_PER_ANIMATION];
	/* the idle pose, in the camera's axes (forward, left, up; world units),
	and in it the middle of the right hand, the middle of the left hand
	from its wrist, and the arms' bones' lengths */
	real_matrix4x3 idle[MAXIMUM_NODES_PER_ANIMATION];
	real_point3d right_centre;
	real_vector3d left_centre;
	real upper_arm_length[NUMBER_OF_SIDES];
	real forearm_length[NUMBER_OF_SIDES];
	/* the body's heading, from the view's (vr_hands_frame.base_yaw) */
	boolean body_valid;
	real body_yaw;
} vr_hands = { NONE };

/* first_person_weapons.c's */
long first_person_weapon_vr_graph(short local_player_index);
short first_person_weapon_vr_idle_pose(short local_player_index, real_matrix4x3 *node_matrices,
	byte *node_models);
int halo_vr_test_hands(void);
void platform_log(char const *format, ...);

/* ---------- vectors */

static real vector_dot(real_vector3d const *a, real_vector3d const *b)
{
	return a->i * b->i + a->j * b->j + a->k * b->k;
}

static void vector_cross(real_vector3d const *a, real_vector3d const *b, real_vector3d *result)
{
	real_vector3d r;

	r.i = a->j * b->k - a->k * b->j;
	r.j = a->k * b->i - a->i * b->k;
	r.k = a->i * b->j - a->j * b->i;
	*result = r;
}

static real vector_normalize(real_vector3d *v)
{
	real length = (real)sqrt(vector_dot(v, v));

	if (length > 1e-9f)
	{
		v->i /= length;
		v->j /= length;
		v->k /= length;
	}
	return length;
}

static void vector_between(real_point3d const *from, real_point3d const *to, real_vector3d *result)
{
	result->i = to->x - from->x;
	result->j = to->y - from->y;
	result->k = to->z - from->z;
}

static void point_add(real_point3d const *point, real_vector3d const *v, real scale, real_point3d *result)
{
	result->x = point->x + v->i * scale;
	result->y = point->y + v->j * scale;
	result->z = point->z + v->k * scale;
}

/* v without its part along unit, normalized; FALSE if nothing is left */
static boolean vector_across(real_vector3d const *v, real_vector3d const *unit, real_vector3d *result)
{
	real along = vector_dot(v, unit);

	result->i = v->i - unit->i * along;
	result->j = v->j - unit->j * along;
	result->k = v->k - unit->k * along;
	return vector_normalize(result) > 1e-4f;
}

/* v turned about the unit axis (radians, right-handed) */
static void vector_turn(real_vector3d const *v, real_vector3d const *axis, real c, real s, real_vector3d *result)
{
	real_vector3d across;
	real along = vector_dot(v, axis) * (1.0f - c);
	real_vector3d r;

	vector_cross(axis, v, &across);
	r.i = v->i * c + across.i * s + axis->i * along;
	r.j = v->j * c + across.j * s + axis->j * along;
	r.k = v->k * c + across.k * s + axis->k * along;
	*result = r;
}

/* the matrix turned about the axis through the pivot */
static void matrix_turn(real_matrix4x3 *m, real_vector3d const *axis, real_point3d const *pivot, real angle)
{
	real c = (real)cos(angle), s = (real)sin(angle);
	real_vector3d offset;

	vector_turn(&m->forward, axis, c, s, &m->forward);
	vector_turn(&m->left, axis, c, s, &m->left);
	vector_turn(&m->up, axis, c, s, &m->up);
	vector_between(pivot, &m->position, &offset);
	vector_turn(&offset, axis, c, s, &offset);
	point_add(pivot, &offset, 1.0f, &m->position);
}

/* the rotation that takes the frame (direction, toward) to the other
(direction, toward), each pair at right angles, applied to the matrix */
static void matrix_reframe(real_matrix4x3 *m, real_vector3d const *from_direction, real_vector3d const *from_toward,
	real_vector3d const *to_direction, real_vector3d const *to_toward)
{
	real_vector3d from_third, to_third;
	real_vector3d *vectors[3] = { &m->forward, &m->left, &m->up };
	short index;

	vector_cross(from_direction, from_toward, &from_third);
	vector_cross(to_direction, to_toward, &to_third);
	for (index = 0; index < 3; index++)
	{
		real_vector3d v = *vectors[index];
		real a = vector_dot(&v, from_direction), b = vector_dot(&v, from_toward), c = vector_dot(&v, &from_third);

		vectors[index]->i = to_direction->i * a + to_toward->i * b + to_third.i * c;
		vectors[index]->j = to_direction->j * a + to_toward->j * b + to_third.j * c;
		vectors[index]->k = to_direction->k * a + to_toward->k * b + to_third.k * c;
	}
}

/* ---------- the graph */

static struct vr_hands_graph_node *graph_node(struct animation_graph *graph, short node_index)
{
	return TAG_BLOCK_GET_ELEMENT(&graph->nodes, node_index, struct vr_hands_graph_node);
}

static boolean node_descends(struct animation_graph *graph, short node_index, short ancestor_index)
{
	short depth = 0;

	while (node_index != NONE && depth++ < MAXIMUM_NODES_PER_ANIMATION)
	{
		if (node_index == ancestor_index)
			return TRUE;
		node_index = graph_node(graph, node_index)->parent_node_index;
	}
	return FALSE;
}

static short node_find(struct animation_graph *graph, short node_count, char const *name)
{
	short node_index;

	for (node_index = 0; node_index < node_count; node_index++)
	{
		if (strstr(graph_node(graph, node_index)->name, name))
			return node_index;
	}
	return NONE;
}

/* the arm above a wrist: each ancestor of the wrist that only the arms'
model draws, up to one that something else hangs from (the weapon, the
other arm), and all that hangs from those but the wrist */
static void arm_mark(struct animation_graph *graph, short node_count, byte const *node_models, short side)
{
	short wrist = vr_hands.wrist[side], other_wrist = vr_hands.wrist[!side];
	short arm = graph_node(graph, wrist)->parent_node_index;
	short top = NONE;
	short node_index;

	while (arm > 0 && arm < node_count)
	{
		boolean shared = !TEST_FLAG(node_models[arm], _vr_hands_node_hands_bit) ||
			TEST_FLAG(node_models[arm], _vr_hands_node_weapon_bit) ||
			node_descends(graph, other_wrist, arm);

		for (node_index = 0; node_index < node_count && !shared; node_index++)
		{
			if (node_descends(graph, node_index, arm) && !node_descends(graph, node_index, wrist) &&
				TEST_FLAG(node_models[node_index], _vr_hands_node_weapon_bit))
			{
				shared = TRUE;
			}
		}
		if (shared)
			break;
		top = arm;
		arm = graph_node(graph, arm)->parent_node_index;
	}
	if (top == NONE)
		return;
	for (node_index = 0; node_index < node_count; node_index++)
	{
		if (node_descends(graph, node_index, top) && !node_descends(graph, node_index, wrist))
			vr_hands.roles[node_index] = side == _left ? _node_left_arm : _node_right_arm;
	}
}

/* a hand's fingers: the wrist's children, by their names, and theirs */
static void fingers_find(struct animation_graph *graph, short node_count, byte const *node_models, short side)
{
	static char const *const names[NUMBER_OF_FINGERS] = { "index", "middle", "ring", "pinky", "thumb" };
	short node_index, finger, joint;

	for (finger = 0; finger < NUMBER_OF_FINGERS; finger++)
	{
		for (joint = 0; joint < NUMBER_OF_FINGER_JOINTS; joint++)
			vr_hands.fingers[side][finger][joint] = NONE;
	}
	for (node_index = 0; node_index < node_count; node_index++)
	{
		struct vr_hands_graph_node *node = graph_node(graph, node_index);

		if (node->parent_node_index != vr_hands.wrist[side] ||
			TEST_FLAG(node_models[node_index], _vr_hands_node_weapon_bit))
		{
			continue;
		}
		for (finger = 0; finger < NUMBER_OF_FINGERS; finger++)
		{
			if (strstr(node->name, names[finger]))
			{
				short joint_index = node_index;

				for (joint = 0; joint < NUMBER_OF_FINGER_JOINTS && joint_index >= 0 && joint_index < node_count; joint++)
				{
					vr_hands.fingers[side][finger][joint] = joint_index;
					joint_index = graph_node(graph, joint_index)->first_child_node_index;
				}
				break;
			}
		}
	}
}

/* the middle of a hand: of its wrist and the knuckles hanging from it (not
the weapon's nodes) */
static void hand_centre(struct animation_graph *graph, real_matrix4x3 const *node_matrices, short node_count,
	byte const *node_models, short wrist, real_point3d *centre)
{
	real_point3d sum = node_matrices[wrist].position;
	short node_index, count = 1;

	for (node_index = 0; node_index < node_count; node_index++)
	{
		if (graph_node(graph, node_index)->parent_node_index == wrist &&
			!TEST_FLAG(node_models[node_index], _vr_hands_node_weapon_bit))
		{
			sum.x += node_matrices[node_index].position.x;
			sum.y += node_matrices[node_index].position.y;
			sum.z += node_matrices[node_index].position.z;
			count++;
		}
	}
	centre->x = sum.x / count;
	centre->y = sum.y / count;
	centre->z = sum.z / count;
}

/* the line of a hand's knuckles, from the index finger's to the little
finger's, in these nodes; FALSE without both */
static boolean knuckle_axis(real_matrix4x3 const *node_matrices, short side, real_vector3d *axis)
{
	short index = vr_hands.fingers[side][_finger_index][0];
	short pinky = vr_hands.fingers[side][_finger_pinky][0];

	if (index == NONE || pinky == NONE)
		return FALSE;
	vector_between(&node_matrices[index].position, &node_matrices[pinky].position, axis);
	return vector_normalize(axis) > 1e-6f;
}

/* the weapon in the first local player's hand: its graph's wrists, arms,
fingers and idle pose, learnt when it changes; whether the hands can be
posed */
static boolean hands_learn(void)
{
	long animation_graph_index = first_person_weapon_vr_graph(0);
	byte node_models[MAXIMUM_NODES_PER_ANIMATION];
	struct animation_graph *graph;
	real_point3d left_centre;
	short node_count, node_index, side;

	if (animation_graph_index == vr_hands.animation_graph_index)
		return vr_hands.usable;
	vr_hands.animation_graph_index = animation_graph_index;
	vr_hands.usable = FALSE;
	vr_hands.arms_usable = FALSE;
	if (animation_graph_index == NONE)
		return FALSE;
	node_count = first_person_weapon_vr_idle_pose(0, vr_hands.idle, node_models);
	if (node_count <= 0 || node_count > MAXIMUM_NODES_PER_ANIMATION)
		return FALSE;
	for (node_index = 0; node_index < node_count; node_index++)
	{
		if (TEST_FLAG(node_models[node_index], _vr_hands_node_hands_bit))
			break;
	}
	if (node_index == node_count)
	{
		/* (the arms' model not matched to the graph yet: again next frame) */
		vr_hands.animation_graph_index = NONE;
		return FALSE;
	}
	graph = animation_graph_definition_get(animation_graph_index);
	vr_hands.node_count = node_count;
	memset(vr_hands.roles, _node_other, sizeof(vr_hands.roles));
	/* ("frame l wriste" in the game's own graphs) */
	vr_hands.wrist[_left] = node_find(graph, node_count, "l wrist");
	vr_hands.wrist[_right] = node_find(graph, node_count, "r wrist");
	if (vr_hands.wrist[_left] == NONE || vr_hands.wrist[_right] == NONE ||
		vr_hands.wrist[_left] == vr_hands.wrist[_right])
	{
		platform_log("vr: hands: %s has no wrists: the game's arms", tag_get_name(animation_graph_index));
		return FALSE;
	}
	/* the hands, but the weapon's own nodes */
	for (node_index = 0; node_index < node_count; node_index++)
	{
		for (side = 0; side < NUMBER_OF_SIDES; side++)
		{
			if (node_descends(graph, node_index, vr_hands.wrist[side]) &&
				!TEST_FLAG(node_models[node_index], _vr_hands_node_weapon_bit))
			{
				vr_hands.roles[node_index] = side == _left ? _node_left_hand : _node_right_hand;
			}
		}
	}
	vr_hands.arms_usable = TRUE;
	for (side = 0; side < NUMBER_OF_SIDES; side++)
	{
		short wrist = vr_hands.wrist[side];
		real_vector3d v;

		arm_mark(graph, node_count, node_models, side);
		fingers_find(graph, node_count, node_models, side);
		/* the arm: the forearm and the upper arm above the wrist, both the
		arm's */
		vr_hands.forearm[side] = graph_node(graph, wrist)->parent_node_index;
		vr_hands.upper_arm[side] = vr_hands.forearm[side] > 0 ?
			graph_node(graph, vr_hands.forearm[side])->parent_node_index : NONE;
		if (vr_hands.upper_arm[side] <= 0 ||
			vr_hands.roles[vr_hands.forearm[side]] != (side == _left ? _node_left_arm : _node_right_arm) ||
			vr_hands.roles[vr_hands.upper_arm[side]] != (side == _left ? _node_left_arm : _node_right_arm))
		{
			vr_hands.arms_usable = FALSE;
		}
		else
		{
			vector_between(&vr_hands.idle[vr_hands.upper_arm[side]].position,
				&vr_hands.idle[vr_hands.forearm[side]].position, &v);
			vr_hands.upper_arm_length[side] = vector_normalize(&v);
			vector_between(&vr_hands.idle[vr_hands.forearm[side]].position, &vr_hands.idle[wrist].position, &v);
			vr_hands.forearm_length[side] = vector_normalize(&v);
			if (vr_hands.upper_arm_length[side] < 1e-4f || vr_hands.forearm_length[side] < 1e-4f)
				vr_hands.arms_usable = FALSE;
		}
		/* the fingers curl toward the thumb's side of the knuckles */
		vr_hands.curl_sign[side] = 1.0f;
		{
			short index = vr_hands.fingers[side][_finger_index][0];
			short index_middle = vr_hands.fingers[side][_finger_index][1];
			short thumb = vr_hands.fingers[side][_finger_thumb][0];
			real_vector3d axis, along, toward, thumbward;

			if (knuckle_axis(vr_hands.idle, side, &axis) && index_middle != NONE && thumb != NONE)
			{
				vector_between(&vr_hands.idle[index].position, &vr_hands.idle[index_middle].position, &along);
				vector_cross(&axis, &along, &toward);
				vector_between(&vr_hands.idle[index].position, &vr_hands.idle[thumb].position, &thumbward);
				vr_hands.curl_sign[side] = vector_dot(&toward, &thumbward) >= 0.0f ? 1.0f : -1.0f;
			}
		}
	}
	vr_hands.trigger = node_find(graph, node_count, "trigger");
	if (vr_hands.trigger != NONE && !TEST_FLAG(node_models[vr_hands.trigger], _vr_hands_node_weapon_bit))
		vr_hands.trigger = NONE;
	hand_centre(graph, vr_hands.idle, node_count, node_models, vr_hands.wrist[_right], &vr_hands.right_centre);
	hand_centre(graph, vr_hands.idle, node_count, node_models, vr_hands.wrist[_left], &left_centre);
	vector_between(&vr_hands.idle[vr_hands.wrist[_left]].position, &left_centre, &vr_hands.left_centre);
	vr_hands.usable = TRUE;
	platform_log("vr: hands: %s: wrists %d (%s) and %d (%s), arms %d %d and %d %d (%.2f and %.2f m), trigger %d",
		tag_get_name(animation_graph_index), vr_hands.wrist[_left], graph_node(graph, vr_hands.wrist[_left])->name,
		vr_hands.wrist[_right], graph_node(graph, vr_hands.wrist[_right])->name, vr_hands.upper_arm[_left],
		vr_hands.forearm[_left], vr_hands.upper_arm[_right], vr_hands.forearm[_right],
		vr_hands.upper_arm_length[_right] * 3.048f, vr_hands.forearm_length[_right] * 3.048f, vr_hands.trigger);
	return TRUE;
}

/* ---------- maths */

static void quaternion_from_matrix(real_matrix4x3 const *m, real q[4])
{
	/* the rotation's matrix, its columns the forward, left and up vectors */
	real r[3][3];
	real trace, s;
	short i;

	for (i = 0; i < 3; i++)
	{
		r[i][0] = m->n[0][i];
		r[i][1] = m->n[1][i];
		r[i][2] = m->n[2][i];
	}
	trace = r[0][0] + r[1][1] + r[2][2];
	if (trace > 0.0f)
	{
		s = (real)sqrt(trace + 1.0f) * 2.0f;
		q[3] = 0.25f * s;
		q[0] = (r[2][1] - r[1][2]) / s;
		q[1] = (r[0][2] - r[2][0]) / s;
		q[2] = (r[1][0] - r[0][1]) / s;
	}
	else if (r[0][0] > r[1][1] && r[0][0] > r[2][2])
	{
		s = (real)sqrt(1.0f + r[0][0] - r[1][1] - r[2][2]) * 2.0f;
		q[3] = (r[2][1] - r[1][2]) / s;
		q[0] = 0.25f * s;
		q[1] = (r[0][1] + r[1][0]) / s;
		q[2] = (r[0][2] + r[2][0]) / s;
	}
	else if (r[1][1] > r[2][2])
	{
		s = (real)sqrt(1.0f + r[1][1] - r[0][0] - r[2][2]) * 2.0f;
		q[3] = (r[0][2] - r[2][0]) / s;
		q[0] = (r[0][1] + r[1][0]) / s;
		q[1] = 0.25f * s;
		q[2] = (r[1][2] + r[2][1]) / s;
	}
	else
	{
		s = (real)sqrt(1.0f + r[2][2] - r[0][0] - r[1][1]) * 2.0f;
		q[3] = (r[1][0] - r[0][1]) / s;
		q[0] = (r[0][2] + r[2][0]) / s;
		q[1] = (r[1][2] + r[2][1]) / s;
		q[2] = 0.25f * s;
	}
}

static void matrix_from_quaternion(real const q[4], real_matrix4x3 *m)
{
	real x = q[0], y = q[1], z = q[2], w = q[3];

	m->forward.i = 1.0f - 2.0f * (y * y + z * z);
	m->forward.j = 2.0f * (x * y + z * w);
	m->forward.k = 2.0f * (x * z - y * w);
	m->left.i = 2.0f * (x * y - z * w);
	m->left.j = 1.0f - 2.0f * (x * x + z * z);
	m->left.k = 2.0f * (y * z + x * w);
	m->up.i = 2.0f * (x * z + y * w);
	m->up.j = 2.0f * (y * z - x * w);
	m->up.k = 1.0f - 2.0f * (x * x + y * y);
}

/* a, moved toward b by t (0 to 1): along the line between them, turning the
shorter way round */
static void matrix_blend(real_matrix4x3 const *a, real_matrix4x3 const *b, real t, real_matrix4x3 *result)
{
	real qa[4], qb[4], q[4], dot = 0.0f, length = 0.0f;
	real scale = a->scale + (b->scale - a->scale) * t;
	real_point3d position;
	short i;

	quaternion_from_matrix(a, qa);
	quaternion_from_matrix(b, qb);
	for (i = 0; i < 4; i++)
		dot += qa[i] * qb[i];
	for (i = 0; i < 4; i++)
	{
		q[i] = qa[i] + ((dot < 0.0f ? -qb[i] : qb[i]) - qa[i]) * t;
		length += q[i] * q[i];
	}
	length = (real)sqrt(length);
	if (length < 1e-6f)
	{
		*result = *b;
		return;
	}
	for (i = 0; i < 4; i++)
		q[i] /= length;
	position.x = a->position.x + (b->position.x - a->position.x) * t;
	position.y = a->position.y + (b->position.y - a->position.y) * t;
	position.z = a->position.z + (b->position.z - a->position.z) * t;
	matrix_from_quaternion(q, result);
	result->position = position;
	result->scale = scale;
}

/* ---------- the fingers */

/* a finger curled by these angles at its joints, about the hand's knuckle
line, toward the palm */
static void finger_curl(real_matrix4x3 *node_matrices, short side, short finger, real_vector3d const *axis,
	real const angles[NUMBER_OF_FINGER_JOINTS])
{
	short const *joints = vr_hands.fingers[side][finger];
	short joint, moved;

	for (joint = 0; joint < NUMBER_OF_FINGER_JOINTS; joint++)
	{
		real_point3d pivot;

		if (joints[joint] == NONE)
			break;
		pivot = node_matrices[joints[joint]].position;
		for (moved = joint; moved < NUMBER_OF_FINGER_JOINTS && joints[moved] != NONE; moved++)
			matrix_turn(&node_matrices[joints[moved]], axis, &pivot, angles[joint] * vr_hands.curl_sign[side]);
	}
}

/* the right index finger on the trigger, and the weapon's trigger, back
along the weapon (backward: the camera's, in world units) */
static void trigger_pull(real_matrix4x3 *node_matrices, real pull, real_vector3d const *backward)
{
	real angles[NUMBER_OF_FINGER_JOINTS] = { TRIGGER_CURL_0 * pull, TRIGGER_CURL_1 * pull, TRIGGER_CURL_2 * pull };
	real_vector3d axis;

	if (pull <= 0.0f)
		return;
	if (knuckle_axis(node_matrices, _right, &axis))
		finger_curl(node_matrices, _right, _finger_index, &axis, angles);
	if (vr_hands.trigger != NONE)
		point_add(&node_matrices[vr_hands.trigger].position, backward, pull, &node_matrices[vr_hands.trigger].position);
}

/* the left hand's fingers in the idle pose's shape about its wrist (where
the wrist is now), relaxed, and closed as far as the grip is pulled */
static void left_fingers_free(real_matrix4x3 *node_matrices, real_matrix4x3 const *camera, real grip)
{
	short wrist = vr_hands.wrist[_left];
	real_matrix4x3 idle_wrist, inverse_idle_wrist, move;
	real_vector3d axis;
	short node_index, finger;

	matrix4x3_multiply(camera, &vr_hands.idle[wrist], &idle_wrist);
	matrix4x3_inverse(&idle_wrist, &inverse_idle_wrist);
	matrix4x3_multiply(&node_matrices[wrist], &inverse_idle_wrist, &move);
	for (node_index = 0; node_index < vr_hands.node_count; node_index++)
	{
		if (vr_hands.roles[node_index] == _node_left_hand && node_index != wrist)
		{
			real_matrix4x3 idle;

			matrix4x3_multiply(camera, &vr_hands.idle[node_index], &idle);
			matrix4x3_multiply(&move, &idle, &node_matrices[node_index]);
		}
	}
	if (knuckle_axis(node_matrices, _left, &axis))
	{
		real curl = GRIP_OPEN_CURL + (GRIP_CLOSED_CURL - GRIP_OPEN_CURL) * grip;
		real angles[NUMBER_OF_FINGER_JOINTS] = { curl * 0.8f, curl, curl * 0.7f };
		real thumb[NUMBER_OF_FINGER_JOINTS] = { 0.0f, curl * 0.4f, curl * 0.4f };

		for (finger = 0; finger < _finger_thumb; finger++)
			finger_curl(node_matrices, _left, finger, &axis, angles);
		finger_curl(node_matrices, _left, _finger_thumb, &axis, thumb);
	}
}

/* ---------- the arms */

static real angle_wrap(real angle)
{
	while (angle > _pi)
		angle -= 2.0f * _pi;
	while (angle < -_pi)
		angle += 2.0f * _pi;
	return angle;
}

/* the shoulders, where the body is: below and beside the eyes, turned
with the body's heading, which faces between the head's and the hands'
(visible[]: where they are), and follows it past a turn and slowly */
static void shoulders_place(struct vr_hands_frame const *frame, boolean const visible[NUMBER_OF_SIDES],
	real_point3d shoulders[NUMBER_OF_SIDES], real_vector3d *forward, real_vector3d *left, real_vector3d *up)
{
	real yaw, difference, facing = frame->head_yaw;
	real_vector3d hands = { 0.0f, 0.0f, 0.0f };
	short side, count = 0;

	for (side = 0; side < NUMBER_OF_SIDES; side++)
	{
		real_matrix4x3 const *hand = side == _left ? &frame->left : &frame->right;

		if (visible[side])
		{
			hands.i += hand->position.x - frame->head.x;
			hands.j += hand->position.y - frame->head.y;
			count++;
		}
	}
	if (count && (real)sqrt(hands.i * hands.i + hands.j * hands.j) > 0.1f * frame->units * count)
		facing += 0.5f * angle_wrap((real)atan2(hands.j, hands.i) - frame->base_yaw - frame->head_yaw);
	if (!vr_hands.body_valid)
	{
		vr_hands.body_yaw = facing;
		vr_hands.body_valid = TRUE;
	}
	difference = facing - vr_hands.body_yaw;
	difference = angle_wrap(difference);
	if (difference > BODY_TURN_FREE)
	{
		vr_hands.body_yaw += difference - BODY_TURN_FREE;
		difference = BODY_TURN_FREE;
	}
	else if (difference < -BODY_TURN_FREE)
	{
		vr_hands.body_yaw += difference + BODY_TURN_FREE;
		difference = -BODY_TURN_FREE;
	}
	if (frame->seconds > 0.0f)
		vr_hands.body_yaw += difference * (1.0f - (real)exp(-frame->seconds / BODY_TURN_SECONDS));
	yaw = frame->base_yaw + vr_hands.body_yaw;
	forward->i = (real)cos(yaw);
	forward->j = (real)sin(yaw);
	forward->k = 0.0f;
	left->i = -forward->j;
	left->j = forward->i;
	left->k = 0.0f;
	up->i = up->j = 0.0f;
	up->k = 1.0f;
	for (side = 0; side < NUMBER_OF_SIDES; side++)
	{
		real out = side == _left ? frame->shoulder[0] : -frame->shoulder[0];

		shoulders[side] = frame->head;
		point_add(&shoulders[side], left, out * frame->units, &shoulders[side]);
		point_add(&shoulders[side], up, frame->shoulder[1] * frame->units, &shoulders[side]);
		point_add(&shoulders[side], forward, frame->shoulder[2] * frame->units, &shoulders[side]);
	}
}

/* an arm from its shoulder to its wrist (posed already): the elbow where
the upper arm and the forearm, their lengths the model's, meet, bent down
and out; the bones turned from the idle pose's to lie along the arm, the
forearm twisted with the hand */
static void arm_reach(real_matrix4x3 *node_matrices, real_matrix4x3 const *camera, short side,
	real_point3d const *shoulder_place, real_vector3d const *forward, real_vector3d const *left,
	real_vector3d const *up, real units)
{
	short wrist = vr_hands.wrist[side], forearm = vr_hands.forearm[side], upper_arm = vr_hands.upper_arm[side];
	real a = vr_hands.upper_arm_length[side], b = vr_hands.forearm_length[side];
	real_point3d shoulder = *shoulder_place, elbow;
	real_point3d const *hand = &node_matrices[wrist].position;
	real_matrix4x3 idle_upper, idle_fore, idle_wrist;
	real_vector3d along, hint, bend, direction, toward, idle_direction, idle_toward, v;
	real reach, cosine, sine, out = side == _left ? 1.0f : -1.0f;

	vector_between(&shoulder, hand, &along);
	reach = vector_normalize(&along);
	if (reach < 1e-6f)
	{
		along = *forward;
		reach = 0.0f;
	}
	/* beyond reach, the shoulder comes forward a little, and the arm
	stretches the rest (the bones apart at the elbow); too near, the
	shoulder goes back */
	if (reach > (a + b) * 0.999f)
	{
		real forward_by = reach - (a + b) * 0.999f;

		if (forward_by > SHOULDER_REACH * units)
			forward_by = SHOULDER_REACH * units;
		reach -= forward_by;
		point_add(hand, &along, -reach, &shoulder);
		if (reach > (a + b) * 0.999f)
		{
			real stretch = reach / ((a + b) * 0.999f);

			a *= stretch;
			b *= stretch;
		}
	}
	else if (reach < (real)fabs(a - b) + 0.05f * a)
	{
		reach = (real)fabs(a - b) + 0.05f * a;
		point_add(hand, &along, -reach, &shoulder);
	}
	cosine = (a * a + reach * reach - b * b) / (2.0f * a * reach);
	cosine = cosine > 1.0f ? 1.0f : cosine < -1.0f ? -1.0f : cosine;
	sine = (real)sqrt(1.0f - cosine * cosine);
	/* the elbow down and out, a little back */
	hint.i = up->i * -1.0f + left->i * out * 0.6f - forward->i * 0.3f;
	hint.j = up->j * -1.0f + left->j * out * 0.6f - forward->j * 0.3f;
	hint.k = up->k * -1.0f + left->k * out * 0.6f - forward->k * 0.3f;
	if (!vector_across(&hint, &along, &bend))
	{
		v.i = -forward->i;
		v.j = -forward->j;
		v.k = -forward->k;
		if (!vector_across(&v, &along, &bend))
			vector_across(left, &along, &bend);
	}
	elbow = shoulder;
	point_add(&elbow, &along, a * cosine, &elbow);
	point_add(&elbow, &bend, a * sine, &elbow);

	matrix4x3_multiply(camera, &vr_hands.idle[upper_arm], &idle_upper);
	matrix4x3_multiply(camera, &vr_hands.idle[forearm], &idle_fore);
	matrix4x3_multiply(camera, &vr_hands.idle[wrist], &idle_wrist);

	/* the upper arm: along the arm to the elbow, turned toward the bend */
	vector_between(&idle_upper.position, &idle_fore.position, &idle_direction);
	vector_normalize(&idle_direction);
	vector_between(&idle_upper.position, &idle_wrist.position, &v);
	vector_normalize(&v);
	{
		real_vector3d idle_elbow;

		vector_between(&idle_upper.position, &idle_fore.position, &idle_elbow);
		if (!vector_across(&idle_elbow, &v, &idle_toward) || !vector_across(&idle_toward, &idle_direction, &idle_toward))
		{
			v.i = -camera->up.i;
			v.j = -camera->up.j;
			v.k = -camera->up.k;
			vector_across(&v, &idle_direction, &idle_toward);
		}
	}
	vector_between(&shoulder, &elbow, &direction);
	vector_normalize(&direction);
	if (!vector_across(&bend, &direction, &toward))
		vector_across(&along, &direction, &toward);
	node_matrices[upper_arm] = idle_upper;
	matrix_reframe(&node_matrices[upper_arm], &idle_direction, &idle_toward, &direction, &toward);
	node_matrices[upper_arm].position = shoulder;

	/* the forearm: from the elbow to the wrist, twisted as the hand is */
	vector_between(&idle_fore.position, &idle_wrist.position, &idle_direction);
	vector_normalize(&idle_direction);
	if (!vector_across(&idle_wrist.up, &idle_direction, &idle_toward))
		vector_across(&idle_wrist.forward, &idle_direction, &idle_toward);
	vector_between(&elbow, hand, &direction);
	vector_normalize(&direction);
	if (!vector_across(&node_matrices[wrist].up, &direction, &toward))
		vector_across(&node_matrices[wrist].forward, &direction, &toward);
	node_matrices[forearm] = idle_fore;
	matrix_reframe(&node_matrices[forearm], &idle_direction, &idle_toward, &direction, &toward);
	node_matrices[forearm].position = elbow;
	/* debug.vr_test_hands: the arm's bones */
	if (halo_vr_test_hands())
	{
		render_debug_line(FALSE, &shoulder, &elbow, global_real_argb_white);
		render_debug_line(FALSE, &elbow, hand, global_real_argb_white);
	}
}

/* ---------- the hands */

/* vr_render_weapon_camera: the middle of the right hand in the weapon in
hand's idle pose, from the camera it is posed from (forward, left, up, in
world units), which the camera is placed to put in the right controller;
FALSE when the weapon's hands cannot be posed */
boolean vr_hands_right_centre(real_point3d *centre)
{
	if (!hands_learn())
		return FALSE;
	*centre = vr_hands.right_centre;
	return TRUE;
}

/* a side's hand and arm not drawn (its controller not tracked) */
static void side_hide(real_matrix4x3 *node_matrices, short side)
{
	byte hand = side == _left ? _node_left_hand : _node_right_hand;
	byte arm = side == _left ? _node_left_arm : _node_right_arm;
	short node_index;

	for (node_index = 0; node_index < vr_hands.node_count; node_index++)
	{
		if (vr_hands.roles[node_index] == hand || vr_hands.roles[node_index] == arm)
			node_matrices[node_index].scale = COLLAPSED_SCALE;
	}
}

/* first_person_weapons.c, once the game has built the first-person nodes
(node_matrices, in the world) of this animation graph: the left hand to the
left controller, the fingers and trigger pulled, the arms to the hands (or
into the wrists) */
void vr_hands_pose(
	short local_player_index,
	long animation_graph_index,
	real_matrix4x3 *node_matrices,
	short node_count)
{
	struct vr_hands_frame frame;
	real_matrix4x3 *left;
	short node_index, side;
	boolean visible[NUMBER_OF_SIDES];

	if (local_player_index != 0 || !vr_render_hands(&frame) || !hands_learn() ||
		animation_graph_index != vr_hands.animation_graph_index || node_count != vr_hands.node_count)
	{
		return;
	}
	left = &node_matrices[vr_hands.wrist[_left]];
	visible[_left] = frame.left_valid;
	visible[_right] = frame.right_valid;

	/* the left hand: in the controller, or on the long gun's foregrip */
	if (frame.left_valid)
	{
		real_matrix4x3 animated[MAXIMUM_NODES_PER_ANIMATION];
		real_matrix4x3 held;
		real_vector3d centre;
		real weight = frame.two_handed;

		if (weight > 0.0f)
			memcpy(animated, node_matrices, sizeof(real_matrix4x3) * node_count);
		/* turned as the idle pose has it from the camera, its middle at the
		controller */
		matrix4x3_multiply(&frame.left, &vr_hands.idle[vr_hands.wrist[_left]], &held);
		matrix4x3_transform_vector(&frame.left, &vr_hands.left_centre, &centre);
		held.position.x = frame.left.position.x - centre.i;
		held.position.y = frame.left.position.y - centre.j;
		held.position.z = frame.left.position.z - centre.k;
		held.scale = left->scale;
		*left = held;
		left_fingers_free(node_matrices, &frame.camera, frame.left_grip);
		if (weight > 0.0f)
		{
			/* (the animation's hand, on the weapon, its fingers as they are) */
			for (node_index = 0; node_index < node_count; node_index++)
			{
				if (vr_hands.roles[node_index] == _node_left_hand)
					matrix_blend(&node_matrices[node_index], &animated[node_index], weight, &node_matrices[node_index]);
			}
		}
	}
	if (frame.right_valid)
	{
		real_vector3d backward;

		backward.i = -frame.camera.forward.i * WEAPON_TRIGGER_TRAVEL * frame.units;
		backward.j = -frame.camera.forward.j * WEAPON_TRIGGER_TRAVEL * frame.units;
		backward.k = -frame.camera.forward.k * WEAPON_TRIGGER_TRAVEL * frame.units;
		trigger_pull(node_matrices, frame.right_trigger, &backward);
	}

	/* debug.vr_test_hands: where the controllers are, a cross at each grip
	and a line where it points */
	if (halo_vr_test_hands())
	{
		for (side = 0; side < NUMBER_OF_SIDES; side++)
		{
			real_matrix4x3 const *hand = side == _left ? &frame.left : &frame.right;
			real_argb_color const *color = side == _right ? global_real_argb_red : global_real_argb_green;
			real_point3d ahead;

			if (!visible[side])
				continue;
			point_add(&hand->position, &hand->forward, 0.05f, &ahead);
			render_debug_point(FALSE, &hand->position, 0.03f, color);
			render_debug_line(FALSE, &hand->position, &ahead, color);
		}
	}

	/* the arms: to the hands from the shoulders, or into the wrists */
	if (frame.arms && vr_hands.arms_usable)
	{
		real_point3d shoulders[NUMBER_OF_SIDES];
		real_vector3d forward, left_axis, up;

		shoulders_place(&frame, visible, shoulders, &forward, &left_axis, &up);
		for (side = 0; side < NUMBER_OF_SIDES; side++)
		{
			if (visible[side])
				arm_reach(node_matrices, &frame.camera, side, &shoulders[side], &forward, &left_axis, &up, frame.units);
		}
	}
	else
	{
		for (node_index = 0; node_index < node_count; node_index++)
		{
			byte role = vr_hands.roles[node_index];

			if (role == _node_left_arm || role == _node_right_arm)
			{
				node_matrices[node_index] = node_matrices[vr_hands.wrist[role == _node_left_arm ? _left : _right]];
				node_matrices[node_index].scale = COLLAPSED_SCALE;
			}
		}
	}
	for (side = 0; side < NUMBER_OF_SIDES; side++)
	{
		if (!visible[side])
			side_hide(node_matrices, side);
	}
}

#endif
