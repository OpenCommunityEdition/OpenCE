/*
WIDGET_POOL.C (test)

The real widget pool (widgets.c) over fake objects that each carry an assault
rifle's one light volume widget (its flashlight). test_widget_pool.py takes the
pool's size, the widget types' groups (config.inc) and the code (under_test.inc).
*/

#include "harness.h"
#include "config.inc"

enum { NUMBER_OF_WIDGET_TYPES = 5 };
struct widget_datum { struct datum_header header; short type; long type_datum_index; long next_widget_index; };
static struct data_array *widget_data;
#define widget_get(widget_index) ((struct widget_datum *)datum_get(widget_data, (widget_index)))

/* the widget types, by their tags' groups as widgets.c lists them; their procedures are recorders */
static long made[NUMBER_OF_WIDGET_TYPES], deleted[NUMBER_OF_WIDGET_TYPES];
static boolean failing_type = NONE;
static long widget_new_generic(int type, long definition_index)
{
	if (type == failing_type) return NONE;
	made[type]++;
	return 0x10000 | (long)made[type];
}
static void widget_delete_generic(int type, long index) { deleted[type]++; }
#define TYPE_PROCS(n) \
	static long new_##n(long d) { return widget_new_generic(n, d); } \
	static void delete_##n(long i) { widget_delete_generic(n, i); }
TYPE_PROCS(0) TYPE_PROCS(1) TYPE_PROCS(2) TYPE_PROCS(3) TYPE_PROCS(4)
struct widget_type_definition
{
	unsigned long group_tag;
	boolean needs_lighting;
	long (*new_proc)(long);
	void (*delete_proc)(long);
};
static struct widget_type_definition widget_type_definitions[NUMBER_OF_WIDGET_TYPES] = {
	{ GROUP_TAG_0, TRUE, new_0, delete_0 }, { GROUP_TAG_1, FALSE, new_1, delete_1 },
	{ GROUP_TAG_2, FALSE, new_2, delete_2 }, { GROUP_TAG_3, FALSE, new_3, delete_3 },
	{ GROUP_TAG_4, FALSE, new_4, delete_4 } };

/* objects, each with a definition listing its widgets */
struct tag_reference { unsigned long group_tag; const char *name; long length; long index; };
struct object_definition_widget { struct tag_reference type; long unused[4]; };
struct tag_block { long count; void *address; long definition; };
struct object_definition { struct { struct tag_block widgets; } object; };
struct object_datum { struct { long first_widget_index; } object; long definition_index; };
#define TAG_BLOCK_GET_ELEMENT(block, index, type) (&((type *)(block)->address)[index])
enum { OBJECTS = 4096 };
static struct object_datum objects[OBJECTS];
static struct object_definition definitions[2];
static struct object_definition_widget rifle_widgets[1], failing_widgets[1];
static struct object_datum *object_get(long object_index) { return &objects[object_index]; }
static struct object_definition *object_definition_get(long definition_index) { return &definitions[definition_index]; }

#include "under_test.inc"

/* make n rifles from object first on; how many got their widget */
static long make_rifles(long first, long count)
{
	long index, with_widget = 0;

	for (index = first; index < first + count && index < OBJECTS; index++)
	{
		objects[index].definition_index = 0;
		widgets_new(index);
		with_widget += objects[index].object.first_widget_index != NONE;
	}
	return with_widget;
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";
	long capacity = MAXIMUM_WIDGETS_PER_MAP, made_now;

	widget_data = game_state_data_new("widget", MAXIMUM_WIDGETS_PER_MAP, sizeof(struct widget_datum));
	rifle_widgets[0].type.group_tag = GROUP_TAG_3;
	rifle_widgets[0].type.index = 7;
	definitions[0].object.widgets.count = 1;
	definitions[0].object.widgets.address = rifle_widgets;

	/* a light volume's group is the fourth widget type */
	CASE("finds-the-type")
	{
		CHECK(tag_group_to_widget_type(GROUP_TAG_3) == 3, "the light volume group maps to type %d", tag_group_to_widget_type(GROUP_TAG_3));
		CHECK(tag_group_to_widget_type(0x7A7A7A7A) == NONE, "an unknown group maps to a type");
		return 0;
	}
	/* rifles past the pool: exactly the pool's size get a widget, the rest none (drawn without it) */
	CASE("fills-and-refuses")
	{
		made_now = make_rifles(0, capacity + 100);
		CHECK(made_now == capacity, "%ld rifles got a widget from a pool of %ld", made_now, capacity);
		CHECK(widget_data->count == capacity, "the pool holds %ld of %ld", widget_data->count, capacity);
		CHECK(objects[capacity].object.first_widget_index == NONE, "the first rifle past the pool got a widget");
		printf("holds %ld of a 128-player game's assault rifles\n", capacity < 128 ? capacity : 128);
		return 0;
	}
	/* deleting objects frees their widgets for the next ones */
	CASE("frees-and-reuses")
	{
		long index;

		make_rifles(0, capacity);
		for (index = 0; index < 10; index++)
			widgets_delete(index);
		CHECK(widget_data->count == capacity - 10, "%ld widgets left after deleting 10 of %ld", widget_data->count, capacity);
		CHECK(deleted[3] == 10, "%ld light volumes deleted, not 10", deleted[3]);
		made_now = make_rifles(capacity, 20);
		CHECK(made_now == 10, "%ld new rifles got the 10 freed widgets", made_now);
		return 0;
	}
	/* a widget whose making fails leaves the object without it, and its slot free */
	CASE("failed-widget-frees-its-slot")
	{
		failing_type = 3;
		made_now = make_rifles(0, 1);
		CHECK(made_now == 0, "the object kept a widget whose making failed");
		CHECK(widget_data->count == 0, "the failed widget's slot is still taken (%ld used)", widget_data->count);
		return 0;
	}
	fprintf(stderr, "unknown case: %s\n", case_name);
	return 2;
}
