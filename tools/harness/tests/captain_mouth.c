/*
CAPTAIN_MOUTH.C (test)

The real lower lip levelling of captain_mouth.c: Captain Keyes's model has
the right half of his lower lip, the vertices its "frame lip lower right"
node moves, 2 mm lower and 0.8 mm further back than the left half, so his
lips never meet on his right and he talks with a lopsided grin. The right
half is moved to the left half's level, each vertex as far as its weight on
that node (the middle of the lip, half). The vertices are the map's own
(a10's characters\captain\captain, its head). test_captain_mouth.py takes
the code from the sources (under_test.inc).
*/

#include "harness.h"

#include <math.h>

typedef struct { short x, y; } point2d;

#define square_root sqrtf

#include "under_test.inc"

enum
{
	LEFT = 32, /* frame lip lower left */
	RIGHT = 33, /* frame lip lower right */
	JAW = 25,
	HEAD = 22,
	UNUSED = 253, /* (a second node of no weight) */
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

/* the halves measured, and (if they are ones to level) the right moved */
static boolean lip_level(struct model_vertex_compressed *vertices, long count)
{
	struct lower_lip_half left;
	struct lower_lip_half right;
	real_vector3d offset;

	memset(&left, 0, sizeof(left));
	memset(&right, 0, sizeof(right));
	lower_lip_half_add(&left, vertices, count, LEFT);
	lower_lip_half_add(&right, vertices, count, RIGHT);
	if (!lower_lip_offset(&left, &right, &offset))
	{
		return FALSE;
	}
	lower_lip_right_half_move(vertices, count, RIGHT, &offset);
	return TRUE;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";
	struct model_vertex_compressed before[NUMBER_OF_VERTICES];

	lip_new();
	memcpy(before, lip, sizeof(lip));

	/* the captain's lip: the right half where the left half's mirror image
	is (within 0.1 mm), the middle halfway, nothing else moved */
	CASE("captain")
	{
		long index;

		CHECK(lip_level(lip, NUMBER_OF_VERTICES), "not levelled");
		CHECK(fabsf(lip[RIGHT_HIGH].position.z - lip[LEFT_HIGH].position.z) < 0.0001f &&
			fabsf(lip[RIGHT_LOW].position.z - lip[LEFT_LOW].position.z) < 0.0001f,
			"right half at %f and %f, left at %f and %f", lip[RIGHT_HIGH].position.z, lip[RIGHT_LOW].position.z,
			lip[LEFT_HIGH].position.z, lip[LEFT_LOW].position.z);
		CHECK(fabsf(lip[RIGHT_HIGH].position.x - lip[LEFT_HIGH].position.x) < 0.0001f &&
			fabsf(lip[RIGHT_LOW].position.x - lip[LEFT_LOW].position.x) < 0.0001f, "right half not as far forward");
		CHECK(lip[RIGHT_HIGH].position.y == before[RIGHT_HIGH].position.y &&
			lip[RIGHT_LOW].position.y == before[RIGHT_LOW].position.y &&
			lip[MIDDLE_HIGH].position.y == before[MIDDLE_HIGH].position.y, "moved across");
		CHECK(CLOSE(lip[MIDDLE_HIGH].position.z - before[MIDDLE_HIGH].position.z,
				(lip[RIGHT_HIGH].position.z - before[RIGHT_HIGH].position.z) / 2.f) &&
			CLOSE(lip[MIDDLE_LOW].position.x - before[MIDDLE_LOW].position.x,
				(lip[RIGHT_LOW].position.x - before[RIGHT_LOW].position.x) / 2.f),
			"the middle not moved half as far");
		for (index = 0; index < NUMBER_OF_VERTICES; index++)
		{
			if (index == LEFT_LOW || index == LEFT_HIGH || index == JAW_RIGHT || index == HEAD_RIGHT)
			{
				CHECK(!memcmp(&lip[index], &before[index], sizeof(lip[index])), "vertex %ld moved", index);
			}
		}
		return 0;
	}
	/* a lip already level (or levelled once already) is left as it is */
	CASE("level")
	{
		struct model_vertex_compressed levelled[NUMBER_OF_VERTICES];

		CHECK(lip_level(lip, NUMBER_OF_VERTICES), "not levelled");
		memcpy(levelled, lip, sizeof(lip));
		CHECK(!lip_level(lip, NUMBER_OF_VERTICES), "levelled again");
		CHECK(!memcmp(lip, levelled, sizeof(lip)), "moved again");
		return 0;
	}
	/* halves of different counts, none, or too far apart: not a lip to level */
	CASE("not a lip")
	{
		/* (one vertex fewer on the right) */
		lip[RIGHT_LOW].nodes[0] = JAW * 3;
		memcpy(before, lip, sizeof(lip));
		CHECK(!lip_level(lip, NUMBER_OF_VERTICES), "uneven halves levelled");
		CHECK(!memcmp(lip, before, sizeof(lip)), "uneven halves moved");
		CHECK(!lip_level(lip + JAW_RIGHT, 2), "no lip levelled");
		lip_new();
		lip[RIGHT_LOW].position.z -= 0.01f;
		lip[RIGHT_HIGH].position.z -= 0.01f;
		memcpy(before, lip, sizeof(lip));
		CHECK(!lip_level(lip, NUMBER_OF_VERTICES), "a lip 1 cm off levelled");
		CHECK(!memcmp(lip, before, sizeof(lip)), "a lip 1 cm off moved");
		return 0;
	}
	/* a vertex's weight on a node, whichever of its two it is */
	CASE("weights")
	{
		struct model_vertex_compressed second = vertex(0.f, 0.f, 0.f, JAW * 3, RIGHT * 3, 8192);

		CHECK(CLOSE(model_vertex_node_weight(&lip[RIGHT_HIGH], RIGHT), 1.f), "first node");
		CHECK(CLOSE(model_vertex_node_weight(&lip[MIDDLE_HIGH], RIGHT), 0.5f), "the middle's second node");
		CHECK(CLOSE(model_vertex_node_weight(&second, RIGHT), 1.f - 8192.f / 32767.f), "second node");
		CHECK(model_vertex_node_weight(&lip[LEFT_HIGH], RIGHT) == 0.f, "another node");
		return 0;
	}

	return 2;
}
