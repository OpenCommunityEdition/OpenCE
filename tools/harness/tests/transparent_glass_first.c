/*
TRANSPARENT_GLASS_FIRST.C (test)

The real group_sorted_indices_cmpfn and transparent_object_glass_first
(rasterizer_transparent_geometry.c): a model's transparent parts all sort at
its object's centroid, so that a powerup's glass box and the glow inside it
tied, and qsort, which keeps no order, drew the glass before or after the
glow as the rest of the view fell: after it, the glass's tint dimmed the
over shield's glow from bright red to a dull purple. Of one object, its
glass is now drawn first, whatever order the groups come in.
test_transparent_glass_first.py takes the code from the sources (under_test.inc).
*/

#include "harness.h"

#define __cdecl

/* (what the sort reads of a group and its shader) */
struct shader
{
	struct
	{
		short type;
	} base;
};

struct transparent_geometry_group
{
	unsigned long geometry_flags;
	long object_index;
	long source_object_index;
	struct shader *shader;
	real z_sort;
	boolean cortana_hack;
};

static boolean shader_is_water_decal(struct shader const *shader)
{
	(void)shader;
	return FALSE;
}

#define MAXIMUM_GROUPS 8
static struct transparent_geometry_group transparent_geometry_groups[MAXIMUM_GROUPS];
short transparent_geometry_group_count; /* (match_assert's: checked by no assertion here) */

#include "under_test.inc"

static struct shader generic_shader = { { 5 } }; /* transparent generic: the over shield's glow */
static struct shader glass_shader = { { 8 } };   /* transparent glass: the powerup's glass box */

/* the groups' indices sorted from an order: the first of them */
static short sorted_first(short const *order, short count)
{
	short indices[MAXIMUM_GROUPS];

	memcpy(indices, order, count * sizeof(short));
	qsort(indices, count, sizeof(short), group_sorted_indices_cmpfn);
	return indices[0];
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	/* the over shield: its glow (0) and glass (1), at its object's centroid,
	queued in either order, among other objects' groups: the glass first */
	CASE("glass first")
	{
		static short const orders[4][4] = { { 0, 1, 2, 3 }, { 1, 0, 3, 2 }, { 2, 3, 0, 1 }, { 3, 1, 2, 0 } };
		int order;

		transparent_geometry_group_count = 4;
		transparent_geometry_groups[0].object_index = 0x10001;
		transparent_geometry_groups[0].shader = &generic_shader;
		transparent_geometry_groups[0].z_sort = -5.0f;
		transparent_geometry_groups[1].object_index = 0x10001;
		transparent_geometry_groups[1].shader = &glass_shader;
		transparent_geometry_groups[1].z_sort = -5.0f;
		/* another object's, nearer, and one farther */
		transparent_geometry_groups[2].object_index = 0x20002;
		transparent_geometry_groups[2].shader = &generic_shader;
		transparent_geometry_groups[2].z_sort = -3.0f;
		transparent_geometry_groups[3].object_index = 0x30003;
		transparent_geometry_groups[3].shader = &glass_shader;
		transparent_geometry_groups[3].z_sort = -9.0f;
		for (order = 0; order < 4; order++)
		{
			short indices[4];
			short position[4];
			short k;

			memcpy(indices, orders[order], sizeof(indices));
			qsort(indices, 4, sizeof(short), group_sorted_indices_cmpfn);
			for (k = 0; k < 4; k++)
				position[indices[k]] = k;
			CHECK(position[3] == 0 && position[2] == 3, "order %d: the farther first and the nearer last", order);
			CHECK(position[1] < position[0], "order %d: the over shield's glow before its glass", order);
		}
		return 0;
	}
	/* a glass part nearer than the glow is still drawn after it: depth first */
	CASE("depth first")
	{
		static short const order[2] = { 1, 0 };

		transparent_geometry_group_count = 2;
		transparent_geometry_groups[0].object_index = 0x10001;
		transparent_geometry_groups[0].shader = &generic_shader;
		transparent_geometry_groups[0].z_sort = -5.0f;
		transparent_geometry_groups[1].object_index = 0x10001;
		transparent_geometry_groups[1].shader = &glass_shader;
		transparent_geometry_groups[1].z_sort = -4.0f;
		CHECK(sorted_first(order, 2) == 0, "the nearer glass first");
		return 0;
	}
	/* two groups of one object neither or both glass: tied, as before */
	CASE("no glass")
	{
		transparent_geometry_group_count = 2;
		transparent_geometry_groups[0].object_index = 0x10001;
		transparent_geometry_groups[0].shader = &generic_shader;
		transparent_geometry_groups[0].z_sort = -5.0f;
		transparent_geometry_groups[1].object_index = 0x10001;
		transparent_geometry_groups[1].shader = &generic_shader;
		transparent_geometry_groups[1].z_sort = -5.0f;
		{
			short a = 0, b = 1;

			CHECK(group_sorted_indices_cmpfn(&a, &b) == 0 && group_sorted_indices_cmpfn(&b, &a) == 0, "not tied");
		}
		return 0;
	}

	fprintf(stderr, "no such case: %s\n", case_name);
	return 2;
}
