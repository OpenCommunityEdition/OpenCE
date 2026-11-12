/*
CAPTAIN_MOUTH.C (test)

The real lower lip levelling of captain_mouth.c: Captain Keyes's model holds
the right half of his lower lip, the vertices its "frame lip lower right"
node moves, 2 mm lower and 0.8 mm further back than the left half. Each
frame of his emotions moves that node back to the left's level (2 mm further
than the left's), so the lip sags only in a pose with no emotion laid over
it: there the node is moved as the emotions move it, by the halves' offset
taken into its parent's (the jaw's) frame. A pose that moves the node itself,
a pose with an emotion, another model's and another map's are left as they
are, and the model's vertices are never changed. The vertices, the jaw's
inverse matrix and the node's place are the map's own (a10's
characters\captain\captain), as is how much further than the left's the
emotions' frames move the right node (frames 3 to 5 of "stand emotions").
test_captain_mouth.py takes the code from the sources (under_test.inc).
*/

#include "harness.h"

#include <math.h>

typedef struct { short x, y; } point2d;
typedef struct { real i, j, k, w; } real_quaternion;
typedef struct { real scale; real_vector3d forward, left, up; real_point3d position; } real_matrix4x3;
struct real_orientation { real_quaternion rotation; real_point3d translation; real scale; };

#define square_root sqrtf
#define csstrcmp strcmp

/* the units, their definitions and the tags' names */
enum
{
	CAPTAIN_MODEL = 0x1920192,
	MARINE_MODEL = 0x1a401a4,
	CAPTAIN_UNIT = 1,
	MARINE_UNIT = 2,
};
struct object_datum { long definition_index; };
struct object_definition { struct { struct { long index; } model; } object; };
static struct object_datum objects[3] = { { 0 }, { CAPTAIN_UNIT }, { MARINE_UNIT } };
static struct object_definition definitions[3] = { { { { NONE } } }, { { { CAPTAIN_MODEL } } }, { { { MARINE_MODEL } } } };
#define object_get(index) (&objects[index])
#define object_definition_get(index) (&definitions[index])
/* (the tag of the captain's model's index, in the map loaded now) */
static char const *captain_model_tag_name = "characters\\captain\\captain";
static char const *tag_get_name(long index)
{
	return index == CAPTAIN_MODEL ? captain_model_tag_name : "characters\\marine_armored\\marine_armored";
}

#include "under_test.inc"

enum
{
	LEFT = 32, /* frame lip lower left */
	RIGHT = 33, /* frame lip lower right */
	JAW = 25,
	HEAD = 22,
	UNUSED = 253, /* (a second node of no weight) */
	NUMBER_OF_NODES = 34,
};

#define CLOSE(a, b) (fabsf((a) - (b)) < 0.0001f)

static struct model_vertex_compressed vertex(real x, real y, real z, byte first, byte second, short weight)
{
	struct model_vertex_compressed result;

	memset(&result, 0, sizeof(result));
	result.position.x = x;
	result.position.y = y;
	result.position.z = z;
	result.nodes[0] = first;
	result.nodes[1] = second;
	result.node_weight = weight;
	return result;
}

/* the lower lip and its neighbours, as the map has them */
enum
{
	LEFT_LOW, /* #96 */
	RIGHT_LOW, /* #129 */
	LEFT_HIGH, /* #177 */
	MIDDLE_HIGH, /* #178 */
	RIGHT_HIGH, /* #179 */
	MIDDLE_LOW, /* #180 */
	JAW_RIGHT, /* #216 */
	HEAD_RIGHT, /* #171 */
	NUMBER_OF_VERTICES,
};

static struct model_vertex_compressed lip[NUMBER_OF_VERTICES];

static void lip_new(void)
{
	lip[LEFT_LOW] = vertex(0.04249604f, 0.00905357f, 0.56477267f, LEFT * 3, UNUSED, 32767);
	lip[RIGHT_LOW] = vertex(0.04169961f, -0.00736977f, 0.56279588f, RIGHT * 3, UNUSED, 32767);
	lip[LEFT_HIGH] = vertex(0.04246883f, 0.00842891f, 0.56775534f, LEFT * 3, UNUSED, 32767);
	lip[MIDDLE_HIGH] = vertex(0.04404906f, 0.00083082f, 0.56697369f, LEFT * 3, RIGHT * 3, 16383);
	lip[RIGHT_HIGH] = vertex(0.04167790f, -0.00676162f, 0.56590593f, RIGHT * 3, UNUSED, 32767);
	lip[MIDDLE_LOW] = vertex(0.04439841f, 0.00084130f, 0.56316262f, LEFT * 3, RIGHT * 3, 16383);
	lip[JAW_RIGHT] = vertex(0.0403f, -0.0072f, 0.5678f, JAW * 3, UNUSED, 32767);
	lip[HEAD_RIGHT] = vertex(0.0403f, -0.0072f, 0.5671f, HEAD * 3, UNUSED, 32767);
}

/* the halves measured: how far forward and up of the right the left is */
static boolean lip_measure(struct model_vertex_compressed const *vertices, long count, real_vector3d *offset)
{
	struct lower_lip_half left;
	struct lower_lip_half right;

	memset(&left, 0, sizeof(left));
	memset(&right, 0, sizeof(right));
	lower_lip_half_add(&left, vertices, count, LEFT);
	lower_lip_half_add(&right, vertices, count, RIGHT);
	return lower_lip_offset(&left, &right, offset);
}

/* the jaw's inverse matrix as the model stands, and where the model holds
the right lower lip's node in the jaw's frame */
static real_matrix4x3 const jaw_inverse =
{
	1.f,
	{ 0.98901588f, -0.14780889f, 0.f },
	{ 0.f, 0.f, -1.f },
	{ 0.14780900f, 0.98901594f, 0.f },
	{ -0.09181457f, -0.57493675f, 0.f },
};
static real_point3d const right_node_translation = { 0.03299236f, -0.02462883f, 0.00590815f };

/* a pose: every node where the model has it (here, only the lip's nodes
matter), and the levelling as a map's tags have set it up */
static struct real_orientation pose[NUMBER_OF_NODES];

static void pose_new(void)
{
	real_vector3d offset;

	memset(pose, 0, sizeof(pose));
	pose[RIGHT].translation = right_node_translation;
	pose[LEFT].translation.x = 0.03412249f;
	pose[LEFT].translation.y = -0.02292138f;
	pose[LEFT].translation.z = -0.00796535f;
	lip_new();
	lip_measure(lip, NUMBER_OF_VERTICES, &offset);
	matrix4x3_transform_vector(&jaw_inverse, &offset, &captain_mouth_node_offset);
	captain_mouth_node_translation = right_node_translation;
	captain_mouth_node_index = RIGHT;
	captain_mouth_model_index = CAPTAIN_MODEL;
	captain_model_tag_name = "characters\\captain\\captain";
}

static boolean pose_untouched(struct real_orientation const *before)
{
	return !memcmp(pose, before, sizeof(pose));
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";
	struct model_vertex_compressed before[NUMBER_OF_VERTICES];
	struct real_orientation pose_before[NUMBER_OF_NODES];
	real_vector3d offset;

	lip_new();
	memcpy(before, lip, sizeof(lip));

	/* the captain's lip: the left half 0.8 mm in front of and 1.9 mm above
	the right, nothing across; measuring changes no vertex */
	CASE("measured")
	{
		CHECK(lip_measure(lip, NUMBER_OF_VERTICES, &offset), "not a lip to level");
		CHECK(CLOSE(offset.i, 0.00079368f) && CLOSE(offset.k, 0.00191310f), "offset %f forward, %f up", offset.i, offset.k);
		CHECK(offset.j == 0.f, "offset %f across", offset.j);
		CHECK(!memcmp(lip, before, sizeof(lip)), "the model's vertices were changed");
		return 0;
	}
	/* a lip already level is left as it is */
	CASE("level")
	{
		CHECK(lip_measure(lip, NUMBER_OF_VERTICES, &offset), "not a lip to level");
		lip[RIGHT_LOW].position.x += offset.i;
		lip[RIGHT_LOW].position.z += offset.k;
		lip[RIGHT_HIGH].position.x += offset.i;
		lip[RIGHT_HIGH].position.z += offset.k;
		CHECK(!lip_measure(lip, NUMBER_OF_VERTICES, &offset), "a level lip is one to level");
		return 0;
	}
	/* halves of different counts, none, or too far apart: not a lip to level */
	CASE("not a lip")
	{
		/* (one vertex fewer on the right) */
		lip[RIGHT_LOW].nodes[0] = JAW * 3;
		CHECK(!lip_measure(lip, NUMBER_OF_VERTICES, &offset), "uneven halves are a lip to level");
		CHECK(!lip_measure(lip + JAW_RIGHT, 2, &offset), "no lip is one to level");
		lip_new();
		lip[RIGHT_LOW].position.z -= 0.01f;
		lip[RIGHT_HIGH].position.z -= 0.01f;
		CHECK(!lip_measure(lip, NUMBER_OF_VERTICES, &offset), "a lip 1 cm off is one to level");
		return 0;
	}
	/* in the jaw's frame the offset is what each emotion's frame moves the
	right node further than the left: 1.13 mm and 1.70 mm (frames 3 to 5) */
	CASE("as the emotions")
	{
		pose_new();
		CHECK(fabsf(captain_mouth_node_offset.i - 0.00113f) < 0.00015f &&
			fabsf(captain_mouth_node_offset.j - 0.00170f) < 0.00015f &&
			fabsf(captain_mouth_node_offset.k) < 0.00005f,
			"the node moved (%f, %f, %f), the emotions move it (0.00113, 0.00170, 0)", captain_mouth_node_offset.i,
			captain_mouth_node_offset.j, captain_mouth_node_offset.k);
		return 0;
	}
	/* a pose with no emotion that holds the node where the model has it: the
	node moved by the offset, nothing else */
	CASE("no emotion")
	{
		pose_new();
		memcpy(pose_before, pose, sizeof(pose));
		captain_mouth_node_orientations(CAPTAIN_UNIT, FALSE, pose);
		CHECK(CLOSE(pose[RIGHT].translation.x, right_node_translation.x + captain_mouth_node_offset.i) &&
			CLOSE(pose[RIGHT].translation.y, right_node_translation.y + captain_mouth_node_offset.j) &&
			CLOSE(pose[RIGHT].translation.z, right_node_translation.z + captain_mouth_node_offset.k) &&
			pose[RIGHT].translation.y != right_node_translation.y,
			"the node is at (%f, %f, %f)", pose[RIGHT].translation.x, pose[RIGHT].translation.y, pose[RIGHT].translation.z);
		pose[RIGHT].translation = right_node_translation;
		CHECK(pose_untouched(pose_before), "another node was moved");
		return 0;
	}
	/* a pose an emotion's frame was laid over is level already: left as it is */
	CASE("emotion")
	{
		pose_new();
		/* (frame 3 of his emotions) */
		pose[RIGHT].translation.x += 0.00179f;
		pose[RIGHT].translation.y += 0.00262f;
		pose[LEFT].translation.x += 0.00066f;
		pose[LEFT].translation.y += 0.00092f;
		memcpy(pose_before, pose, sizeof(pose));
		captain_mouth_node_orientations(CAPTAIN_UNIT, TRUE, pose);
		CHECK(pose_untouched(pose_before), "a pose with an emotion was changed");
		/* (and one where the emotion's frame left the node alone) */
		pose_new();
		memcpy(pose_before, pose, sizeof(pose));
		captain_mouth_node_orientations(CAPTAIN_UNIT, TRUE, pose);
		CHECK(pose_untouched(pose_before), "a pose with an emotion was changed");
		return 0;
	}
	/* a pose that moves the node itself (think_1: 0.7 mm and more) is left as
	it was made */
	CASE("posed")
	{
		pose_new();
		pose[RIGHT].translation.x = 0.03373f;
		pose[RIGHT].translation.y = -0.02515f;
		memcpy(pose_before, pose, sizeof(pose));
		captain_mouth_node_orientations(CAPTAIN_UNIT, FALSE, pose);
		CHECK(pose_untouched(pose_before), "a pose that moves the node was changed");
		return 0;
	}
	/* another unit's pose, and one whose node 33 is where the captain's is */
	CASE("another model")
	{
		pose_new();
		memcpy(pose_before, pose, sizeof(pose));
		captain_mouth_node_orientations(MARINE_UNIT, FALSE, pose);
		CHECK(pose_untouched(pose_before), "another model's pose was changed");
		return 0;
	}
	/* a map whose tags were not looked at (a Custom Edition or MCC map after
	an Xbox one): the index kept is another tag's there */
	CASE("another map")
	{
		pose_new();
		captain_model_tag_name = "characters\\cyborg\\cyborg";
		memcpy(pose_before, pose, sizeof(pose));
		captain_mouth_node_orientations(CAPTAIN_UNIT, FALSE, pose);
		CHECK(pose_untouched(pose_before), "another map's model was changed");
		captain_mouth_model_index = NONE;
		captain_model_tag_name = "characters\\captain\\captain";
		captain_mouth_node_orientations(CAPTAIN_UNIT, FALSE, pose);
		CHECK(pose_untouched(pose_before), "a map without the captain's lip changed a pose");
		return 0;
	}

	return 2;
}
