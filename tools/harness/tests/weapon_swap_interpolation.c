/* The real render_interpolation_tick and render_interpolation_object_node_matrices
(render_interpolation.c) over a holder that moves a world unit along x each tick,
a weapon and one other object, one node each. A weapon readied from the
inventory, where it lay hidden where it was put away, or picked up from the
ground, is drawn in the hand as the hand is drawn, at every frame of the tick
it comes there; an object hidden, moved and shown is drawn where it is shown;
a weapon held throughout blends. */
#include "harness.h"
#include <math.h>

#include "config.inc"

#define MAXIMUM_LOCAL_PLAYERS 4
#define MAXIMUM_OBJECTS_PER_MAP 8
#define _object_mask_all 0
enum { _object_type_biped, _object_type_vehicle, _object_type_weapon };
#define _object_mask_item (1 << _object_type_weapon)
#define _object_invisible_bit 0

#define HOLDER 0x10000L
#define WEAPON 0x10001L
#define OTHER 0x10002L
#define OBJECTS 3

struct object_data
{
	long flags;
	short type;
	long parent_object_index;
	char parent_node_index;
	struct { short size; } node_matrices;
};
struct object_datum { struct object_data object; };
struct object_iterator { long index; long next; };
struct observer_result
{
	real_point3d position;
	real_vector3d forward, up;
	real field_of_view;
};

static struct object_datum objects[OBJECTS];
static real_matrix4x3 nodes[OBJECTS];
static real_vector3d zero_vector;
#define global_zero_vector3d (&zero_vector)

static void object_iterator_new(struct object_iterator *iterator, long mask, long flags) { iterator->next = 0; }
static void *object_iterator_next(struct object_iterator *iterator)
{
	if (iterator->next >= OBJECTS)
		return NULL;
	iterator->index = 0x10000L + iterator->next;
	return &objects[iterator->next++];
}
static struct object_datum *object_get(long index) { return &objects[index & 0xFFFF]; }
static struct object_datum *object_try_and_get(long index) { return index == NONE ? NULL : object_get(index); }
/* (the tick's own pose: what the snapshots take, and the holder's simulated pose) */
static real_matrix4x3 *object_get_node_matrices(long index) { return &nodes[index & 0xFFFF]; }
static void *object_header_block_get(long index, void const *block) { return object_get_node_matrices(index); }
static int halo_interpolation_enabled(void) { return TRUE; }
/* (render_interpolation_followed_nodes calls it before it is defined) */
real_matrix4x3 *render_interpolation_object_node_matrices(long object_index);

#include "under_test.inc"

static void place(long index, short type, real x, real z)
{
	struct object_datum *object = object_get(index);

	object->object.flags = 0;
	object->object.type = type;
	object->object.parent_object_index = NONE;
	object->object.parent_node_index = NONE;
	object->object.node_matrices.size = sizeof(real_matrix4x3);
	nodes[index & 0xFFFF].scale = 1.0f;
	nodes[index & 0xFFFF].forward.i = 1.0f;
	nodes[index & 0xFFFF].left.j = 1.0f;
	nodes[index & 0xFFFF].up.k = 1.0f;
	set_real_point3d(&nodes[index & 0xFFFF].position, x, 0.0f, z);
}

/* a tick: the holder a world unit on, the weapon in its hand (0.4 over its
node) when held */
static void tick(boolean held)
{
	nodes[HOLDER & 0xFFFF].position.x += 1.0f;
	if (held)
	{
		nodes[WEAPON & 0xFFFF].position = nodes[HOLDER & 0xFFFF].position;
		nodes[WEAPON & 0xFFFF].position.z += 0.4f;
	}
	render_interpolation_tick();
}

static void hold(void)
{
	object_get(WEAPON)->object.flags = 0;
	object_get(WEAPON)->object.parent_object_index = HOLDER;
	object_get(WEAPON)->object.parent_node_index = 0;
}

/* the object as a frame that fraction into the tick draws it: NULL when it
is drawn as the tick has it */
static real_matrix4x3 const *drawn_at(long index, real fraction)
{
	interpolation_fraction = fraction;
	interpolation_frame++;
	return render_interpolation_object_node_matrices(index);
}

/* whether the weapon is drawn in the hand as a frame that fraction into the
tick draws the holder */
static boolean drawn_in_hand(real fraction)
{
	real_matrix4x3 const *weapon = drawn_at(WEAPON, fraction);
	real_matrix4x3 const *holder = render_interpolation_object_node_matrices(HOLDER);

	return weapon && holder && fabs(weapon->position.x - holder->position.x) < 1e-4f &&
		fabs(weapon->position.y - holder->position.y) < 1e-4f &&
		fabs(weapon->position.z - holder->position.z - 0.4f) < 1e-4f;
}

/* whether the weapon is drawn halfway along its tick's move (a world unit) */
static boolean blended_halfway(void)
{
	real_matrix4x3 const *weapon = drawn_at(WEAPON, 0.5f);

	return weapon && fabs(weapon->position.x - (nodes[WEAPON & 0xFFFF].position.x - 0.5f)) < 1e-4f;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	interpolation_rendering = TRUE;
	place(HOLDER, _object_type_biped, 0.0f, 0.0f);
	place(WEAPON, _object_type_weapon, 0.0f, 0.4f);
	place(OTHER, _object_type_vehicle, 20.0f, 0.0f);

	/* held throughout: blends, as before */
	CASE("held")
	{
		hold();
		tick(TRUE);
		tick(TRUE);
		CHECK(blended_halfway(), "held weapon not blended");
		CHECK(drawn_in_hand(0.25f), "held weapon not in the hand");
	}
	/* readied from the inventory, where it lay hidden where it was put away
	(a quick swap back while moving): in the hand at every frame of the tick,
	not slid there from that place; held, it blends again */
	CASE("readied")
	{
		place(WEAPON, _object_type_weapon, -3.0f, 1.4f);
		object_get(WEAPON)->object.flags = 1L << _object_invisible_bit;
		tick(FALSE);
		tick(FALSE);
		hold();
		tick(TRUE);
		CHECK(drawn_in_hand(0.25f) && drawn_in_hand(0.75f), "readied weapon not drawn in the hand");
		tick(TRUE);
		CHECK(blended_halfway(), "held weapon not blended a tick on");
	}
	/* picked up from the ground: in the hand, not slid up from the ground */
	CASE("picked up")
	{
		place(WEAPON, _object_type_weapon, 2.5f, 0.0f);
		tick(FALSE);
		tick(FALSE);
		hold();
		tick(TRUE);
		CHECK(drawn_in_hand(0.25f) && drawn_in_hand(0.75f), "picked-up weapon not drawn in the hand");
		tick(TRUE);
		CHECK(blended_halfway(), "held weapon not blended a tick on");
	}
	/* any object hidden, moved and shown: drawn where it is shown, not slid
	from where it was hidden; blended again a tick on */
	CASE("shown")
	{
		object_get(OTHER)->object.flags = 1L << _object_invisible_bit;
		tick(FALSE);
		tick(FALSE);
		nodes[OTHER & 0xFFFF].position.x += 3.0f;
		object_get(OTHER)->object.flags = 0;
		tick(FALSE);
		CHECK(drawn_at(OTHER, 0.5f) == NULL, "shown object slid from where it was hidden");
		tick(FALSE);
		CHECK(drawn_at(OTHER, 0.5f) != NULL, "shown object not blended a tick on");
	}
	/* an object hidden throughout (a projectile drawn only by its glow,
	light and contrail) moving: blended between the ticks as before */
	CASE("stays hidden")
	{
		real_matrix4x3 const *drawn;

		object_get(OTHER)->object.flags = 1L << _object_invisible_bit;
		tick(FALSE);
		nodes[OTHER & 0xFFFF].position.x += 2.0f;
		tick(FALSE);
		drawn = drawn_at(OTHER, 0.5f);
		CHECK(drawn && fabs(drawn->position.x - (nodes[OTHER & 0xFFFF].position.x - 1.0f)) < 1e-4f,
			"hidden object not blended halfway along its move");
	}
	return 0;
}
