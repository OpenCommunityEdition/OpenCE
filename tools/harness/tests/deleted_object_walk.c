/*
DELETED_OBJECT_WALK.C

An object deleted is handed to every part of every object whose type has a
handler (objects_fix_for_deleted_object, object_types.c), as the walk of every
object did, and those handlers drop their references to it; a unit's seat and
weapon label test (unit_can_use_weapon) remembered for the map answers as the
test by name does, also after the remembered tests overflow, and a new map's
answers are not an old map's.
*/

#include <stddef.h>
#include "harness.h"

#define NUMBER_OF_OBJECT_TYPES 12
#define MAXIMUM_CHILDREN_PER_OBJECT_TYPE_DEFINITION 4
#define SET_FLAG(flags, bit, value) ((value) ? ((flags) |= FLAG(bit)) : ((flags) &= ~FLAG(bit)))

/* ---------- objects */

struct object_type_definition
{
	void (*handle_deleted_object)(long, long);
	struct object_type_definition *part_definitions[MAXIMUM_CHILDREN_PER_OBJECT_TYPE_DEFINITION];
};

#define OBJECT_COUNT 300

struct object_datum
{
	struct { short type; long umbrella_shield_object_index; } object;
	long reference;
	long handled;
};

static struct object_datum objects[OBJECT_COUNT];

struct object_iterator
{
	unsigned long type_flags;
	long next;
	long index;
};

static void object_iterator_new(struct object_iterator *iterator, unsigned long type_flags, byte flags)
{
	(void)flags;
	iterator->type_flags = type_flags;
	iterator->next = 0;
	iterator->index = NONE;
}

static void *object_iterator_next(struct object_iterator *iterator)
{
	while (iterator->next < OBJECT_COUNT)
	{
		long index = iterator->next++;

		if (objects[index].object.type >= 0 && TEST_FLAG(iterator->type_flags, objects[index].object.type))
		{
			iterator->index = index;
			return &objects[index];
		}
	}
	return NULL;
}

#define object_get(object_index) (&objects[object_index])

/* handlers that drop a reference to the deleted object, as unit_handle_deleted_object
and projectile_handle_deleted_object do */
static void unit_handle_deleted_object(long object_index, long deleted_object_index)
{
	objects[object_index].handled++;
	if (objects[object_index].reference == deleted_object_index)
		objects[object_index].reference = NONE;
}

static void projectile_handle_deleted_object(long object_index, long deleted_object_index)
{
	objects[object_index].handled++;
	if (objects[object_index].reference == deleted_object_index)
		objects[object_index].reference = NONE;
}

static struct object_type_definition object_part = { NULL, { NULL } };
static struct object_type_definition unit_part = { unit_handle_deleted_object, { NULL } };
static struct object_type_definition projectile_part = { projectile_handle_deleted_object, { NULL } };
static struct object_type_definition device_part = { NULL, { NULL } };
static struct object_type_definition biped_part = { NULL, { NULL } };
/* the types as object_types.c builds them: units' handler in their second part,
projectiles' in theirs, none in the others' */
static struct object_type_definition types[NUMBER_OF_OBJECT_TYPES] = {
	{ NULL, { &object_part, &unit_part, &biped_part, NULL } },
	{ NULL, { &object_part, &unit_part, NULL } },
	{ NULL, { &object_part, NULL } },
	{ NULL, { &object_part, NULL } },
	{ NULL, { &object_part, NULL } },
	{ NULL, { &object_part, &projectile_part, NULL } },
	{ NULL, { &object_part, NULL } },
	{ NULL, { &object_part, &device_part, NULL } },
	{ NULL, { &object_part, &device_part, NULL } },
	{ NULL, { &object_part, &device_part, NULL } },
	{ NULL, { &object_part, NULL } },
	{ NULL, { &object_part, NULL } },
};

static struct object_type_definition *object_type_definition_get(short object_type)
{
	return &types[object_type];
}

/* ---------- units' label tests */

struct unit_datum { long definition_index; char const *seat_label; };
struct unit_definition { struct { struct { long index; } animation_graph; } object; };
struct weapon_datum { char const *label; };
struct unit_globals { long state[4]; long used_time; };

#define UNIT_COUNT 2000
#define WEAPON_COUNT 2000
#define GRAPH_COUNT 32
#define SEAT_LABEL_COUNT 24
#define WEAPON_LABEL_COUNT 40

static struct unit_datum units[UNIT_COUNT];
static struct unit_definition unit_definitions[GRAPH_COUNT];
static struct weapon_datum weapons[WEAPON_COUNT];
static struct unit_globals unit_globals_storage;
static struct unit_globals *unit_globals = &unit_globals_storage;
static char seat_labels[SEAT_LABEL_COUNT][8];
static char weapon_labels[WEAPON_LABEL_COUNT][8];
static long map_serial;
static long tests_by_name;

static struct unit_datum *unit_get(long unit_index)
{
	return &units[unit_index];
}
#define unit_definition_get(definition_index) (&unit_definitions[definition_index])
#define weapon_get(weapon_index) ((void)(weapon_index))
#define unit_get_seat_label(unit_index) (units[unit_index].seat_label)
#define weapon_get_label(weapon_index) (weapons[weapon_index].label)
#define game_engine_allow_pick_up(unit_index, weapon_index) TRUE
#define mcc_grenades_active() FALSE
#define mcc_grenades_reset() ((void)0)

/* the answer the map's tags give: another map, at the same addresses, answers otherwise */
static boolean label_answer(long graph, char const *seat_label, char const *weapon_label)
{
	unsigned long key = (unsigned long)graph * 131 + (unsigned long)(seat_label - seat_labels[0]) * 7 +
		(unsigned long)(weapon_label - weapon_labels[0]) + (unsigned long)map_serial * 977;

	return (key * 2654435761UL >> 13) % 3 == 0;
}

static boolean unit_set_or_test_seat_and_weapon_label(long unit_index, char const *seat_label,
	char const *weapon_label, boolean set)
{
	CHECK(!set, "a test, not a set");
	tests_by_name++;
	return label_answer(unit_definitions[units[unit_index].definition_index].object.animation_graph.index,
		seat_label, weapon_label);
}

#include "under_test.inc"

static unsigned long random_state = 99;

static unsigned long next_random(void)
{
	random_state = random_state * 1103515245 + 12345;
	return (random_state >> 8) & 0xFFFFFF;
}

/* objects of every type, each unit and projectile holding a reference */
static void case_walk(void)
{
	long index;
	int deletion;

	for (index = 0; index < OBJECT_COUNT; index++)
	{
		objects[index].object.type = (short)(next_random() % NUMBER_OF_OBJECT_TYPES);
		objects[index].object.umbrella_shield_object_index = NONE;
		objects[index].reference = (long)(next_random() % OBJECT_COUNT);
	}
	for (deletion = 0; deletion < 100; deletion++)
	{
		long deleted = (long)(next_random() % OBJECT_COUNT);

		for (index = 0; index < OBJECT_COUNT; index++)
			objects[index].handled = 0;
		objects_fix_for_deleted_object(deleted);
		for (index = 0; index < OBJECT_COUNT; index++)
		{
			struct object_type_definition *definition = &types[objects[index].object.type];
			long expected = 0;
			int part;

			if (objects[index].object.type < 0)
				continue;
			for (part = 0; definition->part_definitions[part]; part++)
				expected += definition->part_definitions[part]->handle_deleted_object != NULL;
			CHECK(objects[index].handled == expected, "object %ld (type %d) handled %ld times, expected %ld",
				index, objects[index].object.type, objects[index].handled, expected);
			CHECK(!expected || objects[index].reference != deleted, "object %ld still refers to %ld", index, deleted);
		}
		/* (the object deleted is gone from the walk) */
		objects[deleted].object.type = -1;
	}
}

static void make_units(long graphs, long seats, long weapon_labels_used)
{
	long index;

	for (index = 0; index < GRAPH_COUNT; index++)
		unit_definitions[index].object.animation_graph.index = 0x10000 * (index + 1) + index;
	for (index = 0; index < UNIT_COUNT; index++)
	{
		units[index].definition_index = (long)(next_random() % graphs);
		units[index].seat_label = seat_labels[next_random() % seats];
	}
	for (index = 0; index < WEAPON_COUNT; index++)
		weapons[index].label = weapon_labels[next_random() % weapon_labels_used];
}

static void ask(long count)
{
	long query;

	for (query = 0; query < count; query++)
	{
		long unit_index = (long)(next_random() % UNIT_COUNT);
		long weapon_index = (long)(next_random() % WEAPON_COUNT);
		boolean expected = label_answer(
			unit_definitions[units[unit_index].definition_index].object.animation_graph.index,
			units[unit_index].seat_label,
			weapons[weapon_index].label);

		CHECK(unit_can_use_weapon(unit_index, weapon_index) == expected, "unit %ld weapon %ld", unit_index, weapon_index);
	}
}

/* a map's few label tests each made by name once; then more than are kept */
static void case_labels(void)
{
	units_initialize_for_new_map();
	make_units(6, 5, 10);
	ask(100000);
	CHECK(tests_by_name <= 6 * 5 * 10, "%ld tests by name for at most 300 different ones", tests_by_name);
	make_units(GRAPH_COUNT, SEAT_LABEL_COUNT, WEAPON_LABEL_COUNT);
	ask(200000);
}

/* a new map's tags at the same addresses answer otherwise */
static void case_new_map(void)
{
	units_initialize_for_new_map();
	make_units(6, 5, 10);
	ask(20000);
	map_serial++;
	units_initialize_for_new_map();
	tests_by_name = 0;
	ask(20000);
	CHECK(tests_by_name > 0, "the new map's tests made by name");
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";
	int index;

	for (index = 0; index < SEAT_LABEL_COUNT; index++)
		snprintf(seat_labels[index], sizeof(seat_labels[index]), "s%d", index);
	for (index = 0; index < WEAPON_LABEL_COUNT; index++)
		snprintf(weapon_labels[index], sizeof(weapon_labels[index]), "w%d", index);

	CASE("walk") case_walk();
	else CASE("labels") case_labels();
	else CASE("new-map") case_new_map();
	else
	{
		fprintf(stderr, "unknown case %s\n", case_name);
		return 2;
	}
	return 0;
}
