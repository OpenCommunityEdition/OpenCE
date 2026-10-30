#include "harness.h"

typedef unsigned short word;
#define PIN(x, lo, hi) ((x) < (lo) ? (lo) : (x) > (hi) ? (hi) : (x))
#include "declarations.inc"

/* A co-op client's objects: a unit and a rifle the host attaches to it.
A rifle waiting to go into its carrier's inventory is off the map (not connected). */
#define OBJECT_COUNT 8
#define OBJECT(n) (0x10000L | (n))
struct object_datum { struct { unsigned long flags; long parent_object_index; } object; };
static struct object_datum objects[OBJECT_COUNT];
static int errors;

static struct object_datum *object_get(long index) { return &objects[index & 0xFFFF]; }
static long object_find(short name_index, long object_index, long definition_index, unsigned long mask)
{
	return object_index;
}
/* like objects.c's: disconnecting the child from the map first asserts that it is connected (objects.c#958) */
static void object_attach_to_marker(long parent_index, char const *parent_marker, long child_index, char const *child_marker)
{
	CHECK(TEST_FLAG(object_get(child_index)->object.flags, _object_connected_to_map_bit),
		"objects.c#958: object %lx attached off the map", child_index);
	object_get(child_index)->object.parent_object_index = parent_index;
}
static void objects_scripting_detach(long parent_index, long child_index)
{
	object_get(child_index)->object.parent_object_index = NONE;
}
#define object_marker_name(index, marker) ""
#define network_coop_note_attach(parent_index, parent_marker, child_index, child_marker) ((void)0)
#define error(priority, ...) (errors++)

#include "under_test.inc"

#define UNIT OBJECT(1)
#define RIFLE OBJECT(2)

/* the host's attach of the rifle to the unit's hand */
static void attach(boolean on_map)
{
	struct distributed_coop_event event = {0};

	objects[1].object.flags = FLAG(_object_connected_to_map_bit);
	objects[1].object.parent_object_index = NONE;
	objects[2].object.flags = on_map ? FLAG(_object_connected_to_map_bit) : 0;
	objects[2].object.parent_object_index = NONE;
	event.type = _coop_attach;
	event.object_index = UNIT;
	event.target = RIFLE;
	event.reals[0] = -1.0f;
	client_apply_attach(&event);
}

int main(int argc, char **argv)
{
	char const *case_name = argc == 2 ? argv[1] : "";

	CASE("on-map")
	{
		attach(TRUE);
		CHECK(objects[2].object.parent_object_index == UNIT, "the rifle is not attached");
	}
	else CASE("off-map")
	{
		attach(FALSE);
		CHECK(objects[2].object.parent_object_index == NONE && errors == 1, "the rifle off the map is attached");
	}
	else return 2;
	return 0;
}
