#include "harness.h"
#include <math.h>
#include <strings.h>

/* The ragdoll of a fake biped (ragdoll_bodies.c), shaped as the stock
marine's is: its nodes run along their x axes, and its collision model has a
box in the frame of most nodes, matched to them by name, not by order.

	0 bip01 pelvis   the root, a box
	1 bip01 l thigh  long: a capsule
	2 bip01 r thigh
	3 bip01 l calf   long
	4 bip01 spine    a box
	5 bip01 neck     a collision node without geometry: moves with the spine
	6 bip01 head     a box, joined to the spine (the neck has no part)
	7 bip01 l toe0   a sliver: moves with the calf (but a foot, a limb, would be a part)
	8 bip01 ponytail1  no collision node at all: moves with the head */
struct tag_block { long count; void *address; long definition; };
#define TAG_BLOCK_GET_ELEMENT(block, index, type) ((type *)(block)->address + (index))
typedef struct { real_vector3d v; real w; } real_quaternion;
typedef struct { real scale; real_vector3d forward, left, up; real_point3d position; } real_matrix4x3;
struct model_node { char name[32]; short next_sibling_node_index, first_child_node_index, parent_node_index; short pad;
	real_point3d default_translation; real_quaternion default_rotation; real node_distance_from_parent; long unused[8];
	real_matrix4x3 runtime_default_inverse_matrix; };
struct collision_vertex { real_point3d point; long first_edge_index; };
struct collision_bsp { struct tag_block vertices; };
struct collision_node { char name[32]; short region_index, parent_node_index, next_sibling_node_index, first_child_node_index;
	long unused[2]; short pad; short runtime_damage_part; struct tag_block bsps; };
#define _pi ((real)3.14159265358979323846f)
#define DEGREES_TO_RADIANS(angle) ((real)(((real)angle) * ((real)M_PI) / 180.f))
#define square_root(x) sqrtf(x)
#define csstrcasecmp(a, b) strcasecmp(a, b)

#include "config.inc"
#include "under_test.inc"

#define COUNT(a) ((long)(sizeof(a) / sizeof((a)[0])))
#define NODES 9

static struct model_node nodes[64];
static struct collision_node collision_nodes[64];
static struct collision_bsp bsps[64];
static struct collision_vertex vertices[64][8];
static struct tag_block model_block, collision_block;
static struct ragdoll_body body;

static void node(short index, char const *name, short parent, real x, real y, real z)
{
	strcpy(nodes[index].name, name);
	nodes[index].parent_node_index = parent;
	nodes[index].default_translation.x = x;
	nodes[index].default_translation.y = y;
	nodes[index].default_translation.z = z;
}

/* the collision node at slot, of a box from a to b (8 vertices); or none */
static void piece(short slot, char const *name, real ax, real ay, real az, real bx, real by, real bz, boolean geometry)
{
	short corner;

	strcpy(collision_nodes[slot].name, name);
	collision_nodes[slot].bsps.count = geometry ? 1 : 0;
	collision_nodes[slot].bsps.address = &bsps[slot];
	bsps[slot].vertices.count = 8;
	bsps[slot].vertices.address = vertices[slot];
	for (corner = 0; corner < 8; corner++)
	{
		vertices[slot][corner].point.x = corner & 1 ? bx : ax;
		vertices[slot][corner].point.y = corner & 2 ? by : ay;
		vertices[slot][corner].point.z = corner & 4 ? bz : az;
	}
}

static void biped(void)
{
	memset(nodes, 0, sizeof(nodes));
	memset(collision_nodes, 0, sizeof(collision_nodes));
	node(0, "bip01 pelvis", NONE, 0, 0, 0.36f);
	node(1, "bip01 l thigh", 0, 0, 0, 0.036f);
	node(2, "bip01 r thigh", 0, 0, 0, -0.036f);
	node(3, "bip01 l calf", 1, 0.16f, 0, 0);
	node(4, "bip01 spine", 0, 0.034f, 0, 0);
	node(5, "bip01 neck", 4, 0.09f, 0, 0);
	node(6, "bip01 head", 5, 0.015f, 0, 0);
	node(7, "bip01 l toe0", 3, 0.156f, 0, 0);
	node(8, "bip01 ponytail1", 6, 0.011f, 0.006f, 0);
	/* (in another order than the nodes', one name in capitals) */
	piece(0, "bip01 head", -0.01f, -0.03f, -0.03f, 0.08f, 0.05f, 0.03f, TRUE);
	piece(1, "bip01 pelvis", -0.04f, -0.04f, -0.05f, 0.03f, 0.05f, 0.05f, TRUE);
	piece(2, "BIP01 L THIGH", -0.02f, -0.04f, -0.03f, 0.17f, 0.05f, 0.04f, TRUE);
	piece(3, "bip01 r thigh", -0.02f, -0.04f, -0.04f, 0.17f, 0.05f, 0.03f, TRUE);
	piece(4, "bip01 l calf", -0.02f, -0.05f, -0.03f, 0.16f, 0.04f, 0.03f, TRUE);
	piece(5, "bip01 spine", -0.02f, -0.05f, -0.05f, 0.04f, 0.06f, 0.05f, TRUE);
	piece(6, "bip01 neck", 0, 0, 0, 0, 0, 0, FALSE);
	piece(7, "bip01 l toe0", -0.005f, -0.005f, -0.005f, 0.005f, 0.005f, 0.005f, TRUE);
	model_block.count = NODES;
	model_block.address = nodes;
	collision_block.count = 8;
	collision_block.address = collision_nodes;
}

static boolean near(real a, real b)
{
	return fabsf(a - b) < 1e-4f;
}

static boolean near_degrees(real radians, real degrees)
{
	return fabsf(radians - degrees * (real)M_PI / 180.0f) < 1e-4f;
}

static boolean made(char const *model, char const *overrides, long size)
{
	return ragdoll_body_build(model, &model_block, &collision_block, overrides, size, &body);
}

static boolean plain(void)
{
	return made("test\\plain", NULL, 0);
}

static short part_of(short node_index)
{
	return body.node_parts[node_index];
}

static char const overrides[] =
	"# the test's ragdolls\n"
	"model test\\other\n"
	"mass 5\n"
	"\n"
	"model test\\a\n"
	"  mass 100   # heavier\n"
	"part bip01 neck\n"
	"fold bip01 r thigh\n"
	"fold bip01 pelvis\n"
	"joint bip01 head: twist -10 20 swing 30 35\n"
	"joint bip01 l calf: twist 10 -20 swing 1 1\n"
	"box bip01 spine: 0.1 0.1 0.1\n"
	"sphere bip01 nobody: 1\n"
	"capsule bip01 l calf: 0.0001 0.1\n"
	"break bip01 head: 40\n"
	"break bip01 l calf: 0.5\n"
	"break bip01 pelvis: 30\n"
	"mass -5\n"
	"model test\\b\n"
	"mass 5\n"
	"model test\\none\n"
	"none\n";

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";
	short index;
	real total;

	biped();
	/* a part each for the nodes with big pieces; the others move with
	their nearest ancestor's part */
	CASE("parts")
	{
		CHECK(plain(), "no body");
		CHECK(body.part_count == 6 && body.node_count == NODES, "%d parts", body.part_count);
		CHECK(body.parts[0].node_index == 0 && body.parts[0].parent_part_index == NONE, "the root");
		CHECK(part_of(1) == 1 && part_of(2) == 2 && part_of(3) == 3 && part_of(4) == 4, "the limbs and spine");
		CHECK(part_of(5) == 4, "the neck moves with the spine: %d", part_of(5));
		CHECK(part_of(6) == 5 && body.parts[5].node_index == 6, "the head");
		CHECK(part_of(7) == 3, "the toe moves with the calf: %d", part_of(7));
		CHECK(part_of(8) == 5, "the ponytail moves with the head: %d", part_of(8));
		CHECK(body.parts[1].parent_part_index == 0 && body.parts[3].parent_part_index == 1 &&
			body.parts[5].parent_part_index == 4, "the joints' parents");
		/* a limb is a part however small its piece */
		strcpy(nodes[7].name, "bip01 l foot");
		strcpy(collision_nodes[7].name, "bip01 l foot");
		CHECK(plain() && body.part_count == 7 && part_of(7) == 6 && body.parts[6].parent_part_index == 3, "the foot");
		return 0;
	}
	/* long pieces are capsules along x, the rest boxes, at their centres */
	CASE("shapes")
	{
		struct ragdoll_part const *thigh, *spine;

		plain();
		thigh = &body.parts[part_of(1)];
		spine = &body.parts[part_of(4)];
		CHECK(thigh->shape == _ragdoll_shape_capsule, "thigh: shape %d", thigh->shape);
		CHECK(near(thigh->size[0], 0.04f) && near(thigh->size[1], 0.055f), "thigh: %g %g", thigh->size[0], thigh->size[1]);
		CHECK(near(thigh->center.x, 0.075f) && near(thigh->center.y, 0.005f) && near(thigh->center.z, 0.005f),
			"thigh: centre %g %g %g", thigh->center.x, thigh->center.y, thigh->center.z);
		CHECK(body.parts[part_of(3)].shape == _ragdoll_shape_capsule, "calf");
		CHECK(spine->shape == _ragdoll_shape_box && near(spine->size[0], 0.03f) && near(spine->size[1], 0.055f) &&
			near(spine->size[2], 0.05f), "spine: %d %g %g %g", spine->shape, spine->size[0], spine->size[1], spine->size[2]);
		CHECK(body.parts[0].shape == _ragdoll_shape_box && body.parts[5].shape == _ragdoll_shape_box, "pelvis, head");
		return 0;
	}
	/* the masses by volume (a folded node's added to its part's), all of
	them the body's */
	CASE("masses")
	{
		/* (the toe's sliver no thinner than the smallest shape) */
		real calf_volume = 0.09f * 0.045f * 0.03f * 8.0f + 0.01f * 0.01f * 0.01f * 8.0f;
		real thigh_volume = 0.095f * 0.045f * 0.035f * 8.0f;

		plain();
		total = 0.0f;
		for (index = 0; index < body.part_count; index++)
		{
			CHECK(body.parts[index].mass > 0.0f, "part %d: %g kg", index, body.parts[index].mass);
			total += body.parts[index].mass;
		}
		CHECK(near(body.mass, 80.0f) && fabsf(total - 80.0f) < 0.01f, "%g kg, the parts %g", body.mass, total);
		CHECK(fabsf(body.parts[part_of(3)].mass / body.parts[part_of(1)].mass - calf_volume / thigh_volume) < 1e-3f,
			"calf %g, thigh %g", body.parts[part_of(3)].mass, body.parts[part_of(1)].mass);
		return 0;
	}
	/* the limits by the bones' names */
	CASE("limits")
	{
		struct ragdoll_part const *calf, *thigh, *head, *spine;

		plain();
		calf = &body.parts[part_of(3)];
		thigh = &body.parts[part_of(1)];
		head = &body.parts[part_of(6)];
		spine = &body.parts[part_of(4)];
		CHECK(near_degrees(calf->twist_min, -5) && near_degrees(calf->twist_max, 5) &&
			near_degrees(calf->swing_normal, 5) && near_degrees(calf->swing_plane, 70), "calf: a hinge");
		CHECK(near_degrees(thigh->swing_normal, 50) && near_degrees(thigh->swing_plane, 50), "thigh");
		CHECK(near_degrees(head->twist_max, 40), "head");
		CHECK(near_degrees(spine->twist_max, 15) && near_degrees(spine->swing_plane, 20), "spine");
		return 0;
	}
	/* the model's section, and only its, changes it; bad lines change nothing */
	CASE("overrides")
	{
		struct ragdoll_part const *neck, *head, *spine, *calf;

		CHECK(made("TEST\\A", overrides, (long)strlen(overrides)), "no body");
		CHECK(near(body.mass, 100.0f), "mass %g", body.mass);
		CHECK(body.part_count == 6, "%d parts", body.part_count);
		CHECK(part_of(0) == 0, "the root is always a part");
		CHECK(part_of(2) == 0, "the r thigh folded: %d", part_of(2));
		neck = &body.parts[part_of(5)];
		CHECK(neck->node_index == 5 && neck->shape == _ragdoll_shape_sphere, "the neck is a part");
		/* half as far across as the head, its child, but no smaller than 0.02 */
		CHECK(near(neck->size[0], 0.02f), "neck: radius %g", neck->size[0]);
		head = &body.parts[part_of(6)];
		CHECK(head->parent_part_index == part_of(5), "the head on the neck");
		CHECK(near_degrees(head->twist_min, -10) && near_degrees(head->twist_max, 20) &&
			near_degrees(head->swing_normal, 30) && near_degrees(head->swing_plane, 35), "head's joint");
		spine = &body.parts[part_of(4)];
		CHECK(spine->shape == _ragdoll_shape_box && near(spine->size[0], 0.1f) && near(spine->size[2], 0.1f), "spine's box");
		calf = &body.parts[part_of(3)];
		CHECK(near_degrees(calf->twist_max, 5) && calf->shape == _ragdoll_shape_capsule && near(calf->size[0], 0.0375f),
			"the calf's bad lines applied: %g %d %g", calf->twist_max, calf->shape, calf->size[0]);
		CHECK(near(head->break_damage, 40.0f) && calf->break_damage == 0.0f, "the head's break, not the calf's: %g %g",
			head->break_damage, calf->break_damage);
		CHECK(body.parts[0].break_damage == 0.0f && spine->break_damage == 0.0f, "no joint at the root to break");
		return 0;
	}
	/* `none`, and a model the overrides do not name */
	CASE("none")
	{
		CHECK(!made("test\\none", overrides, (long)strlen(overrides)), "a body for none");
		CHECK(made("test\\c", overrides, (long)strlen(overrides)) && near(body.mass, 80.0f), "an unnamed model");
		return 0;
	}
	/* no further than the overrides' size: the last line cut short */
	CASE("size")
	{
		char const text[] = "model test\\a\nmass 100";

		CHECK(made("test\\a", text, (long)strlen(text) - 1) && near(body.mass, 10.0f), "mass %g", body.mass);
		CHECK(made("test\\a", text, 4) && near(body.mass, 80.0f), "mass %g", body.mass);
		return 0;
	}
	/* no body without data to make one from */
	CASE("no data")
	{
		CHECK(ragdoll_body_build("test\\a", &model_block, NULL, NULL, 0, &body) == FALSE, "no collision model");
		for (index = 0; index < 8; index++)
			collision_nodes[index].bsps.count = 0;
		CHECK(!plain(), "no geometry");
		biped();
		model_block.count = 1;
		CHECK(!plain(), "one node");
		biped();
		collision_block.address = &collision_nodes[1];
		collision_block.count = 1;
		CHECK(!plain(), "only the pelvis's piece: one part");
		return 0;
	}
	/* each node after its parent, or no body */
	CASE("order")
	{
		nodes[3].parent_node_index = 7;
		CHECK(!plain(), "a parent after its child");
		biped();
		nodes[0].parent_node_index = 1;
		CHECK(!plain(), "a root with a parent");
		biped();
		nodes[4].parent_node_index = -2;
		CHECK(!plain(), "a parent out of range");
		return 0;
	}
	/* a piece with a vertex that is not a number, or far off, is no piece */
	CASE("bounds")
	{
		vertices[0][3].point.y = NAN;
		CHECK(plain() && part_of(6) == part_of(4), "the head: %d", part_of(6));
		biped();
		vertices[0][3].point.y = 1e9f;
		CHECK(plain() && part_of(6) == part_of(4), "the head: %d", part_of(6));
		return 0;
	}
	/* a root without a piece: a sphere half as far across as its furthest child */
	CASE("rootless")
	{
		strcpy(collision_nodes[1].name, "bip01 nothing");
		nodes[4].default_translation.x = 0.1f;
		CHECK(plain() && body.parts[0].node_index == 0, "no body");
		CHECK(body.parts[0].shape == _ragdoll_shape_sphere && near(body.parts[0].size[0], 0.05f),
			"pelvis: %d %g", body.parts[0].shape, body.parts[0].size[0]);
		CHECK(near(body.parts[0].center.x, 0) && near(body.parts[0].center.z, 0), "at its origin");
		return 0;
	}
	/* no more parts than the host takes: a chain of 64 links, each one asked
	to be a part (each too small a share of the whole to be one by itself) */
	CASE("capacity")
	{
		static char chain[64 * 16 + 32];
		long length;

		length = sprintf(chain, "model test\\chain\n");
		for (index = 0; index < 64; index++)
		{
			char name[32];

			sprintf(name, "link %d", index);
			node(index, name, index - 1, 0.1f, 0, 0);
			piece(index, name, 0, -0.02f, -0.02f, 0.1f, 0.02f, 0.02f, TRUE);
			length += sprintf(chain + length, "part %s\n", name);
		}
		model_block.count = 64;
		collision_block.count = 64;
		CHECK(!plain(), "a part without the overrides");
		CHECK(made("test\\chain", chain, length) && body.part_count == RAGDOLL_BODY_PARTS_MAX, "%d parts", body.part_count);
		for (index = 0; index < 64; index++)
			CHECK(part_of(index) >= 0 && part_of(index) < RAGDOLL_BODY_PARTS_MAX, "node %d: part %d", index, part_of(index));
		model_block.count = 65;
		CHECK(!plain(), "65 nodes");
		return 0;
	}
	fprintf(stderr, "unknown case '%s'\n", case_name);
	return 2;
}
