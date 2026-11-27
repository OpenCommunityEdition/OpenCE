/*
PICTURE_FUNCTION_INPUTS.C (test)

An object's functions as a tick computes them (object_compute_function_values)
and as a frame drawn between ticks shows them (object_function_values_for_render),
in a fake world of one object. After the tick, something writes the object's
incoming values, as an invisible weapon writes its own into its parent's
(weapon_export_function_values); the frame's values must still come from the
inputs the tick used. test_picture_function_inputs.py takes the code, enums and
constants from the sources.
*/

#include <math.h>
#include <string.h>
#include "harness.h"
#include "enums.inc"

typedef unsigned short word;

/* the function definition, as the sources have it */
#include "structures.inc"

/* tags and objects: the members the code reads */
struct tag_block { long count; void *address; };
struct object_definition { struct { struct tag_block functions; } object; };
struct object_datum
{
	long definition_index;
	struct
	{
		real incoming_function_values[NUMBER_OF_INCOMING_OBJECT_FUNCTIONS];
		real outgoing_function_values[NUMBER_OF_OUTGOING_OBJECT_FUNCTIONS];
		unsigned long functions_active_flags;
	} object;
};
static struct object_function_definition functions[3];
static struct object_definition definitions[1];
static struct object_datum objects[MAXIMUM_OBJECTS_PER_MAP];
#define OBJECT_INDEX(salt, i) ((long)(((salt) << 16) | (i))) /* a datum index, salt and all */
#define object_get(object_index) (&objects[DATUM_INDEX_TO_ABSOLUTE_INDEX(object_index)])
#define object_definition_get(index) (&definitions[index])
#define TAG_BLOCK_GET_ELEMENT(block, index, type) (((type *)(block)->address) + (index))
#define object_definition_data_error(definition_index, problem) ((void)0)
#define csmemcpy memcpy
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define SET_FLAG(flags, bit, value) ((value) ? ((flags) |= FLAG(bit)) : ((flags) &= ~FLAG(bit)))
#define _real_epsilon 0.0001f

/* the game's clock and the picture's place between ticks (render_interpolation.c) */
static long test_game_time = 300;
static real test_fraction = 1.0f;
#define game_time_get() test_game_time
#define render_interpolation_fraction() test_fraction

/* the periodic functions without their tables: one is 1 (as the game's), any
other a sawtooth of the time; the linear transition as the game's */
static real periodic_function_evaluate(short type, real time)
{
	return type == _periodic_function_one ? 1.0f : (real)(time - floor(time));
}
static real transition_function_evaluate(short type, real value)
{
	(void)type;
	return value < 0.0f ? 0.0f : value > 1.0f ? 1.0f : value;
}

#include "under_test.inc"

static void function_new(struct object_function_definition *function, short type, short scale_by)
{
	memset(function, 0, sizeof(*function));
	function->function_type = type;
	function->scale_function_by_function_index = scale_by;
	function->period = 1.0f;
	function->runtime_one_over_period = 1.0f;
	function->upper_bound = 1.0f;
	function->runtime_reciprocal_bounds_range = 1.0f;
	function->map_result_to_transition_function = _transition_function_linear;
	function->turn_off_with_function_index = NONE;
}

/* one object at absolute index 5: function 0 is 'one' scaled by input A (a
vehicle's driver's seat power), function 1 a function of the time (the
cosine: here the stub's sawtooth), function 2 an additive 'one' scaled by
input D (a warthog's chaingun spin, its gun's rate of fire) */
static long object_new(long salt)
{
	long object_index = OBJECT_INDEX(salt, 5);
	struct object_datum *object = object_get(object_index);

	function_new(&functions[0], _periodic_function_one, 1);
	function_new(&functions[1], _periodic_function_cosine, 0);
	function_new(&functions[2], _periodic_function_one, 4);
	functions[2].flags = FLAG(_object_function_additive_bit);
	definitions[0].object.functions.count = 3;
	definitions[0].object.functions.address = functions;
	memset(object, 0, sizeof(*object));
	object->definition_index = 0;
	return object_index;
}

int main(int argc, char **argv)
{
	char const *test_case = argc > 1 ? argv[1] : "";
	real shown[NUMBER_OF_OUTGOING_OBJECT_FUNCTIONS];

	if (!strcmp(test_case, "tick-inputs"))
	{
		/* the tick computes from the driver's seat power (1), then the
		vehicle's invisible gun writes its illumination (0.25) over input A:
		the picture still shows the tick's 1 */
		long object_index = object_new(1);
		struct object_datum *object = object_get(object_index);

		object->object.incoming_function_values[0] = 1.0f;
		test_fraction = 1.0f;
		object_compute_function_values(object_index);
		CHECK(object->object.outgoing_function_values[0] == 1.0f, "the tick's value %f", object->object.outgoing_function_values[0]);
		object->object.incoming_function_values[0] = 0.25f;
		test_fraction = 0.5f;
		object_function_values_for_render(object_index, shown);
		CHECK(shown[0] == 1.0f, "the picture's value %f, from the inputs written after the tick", shown[0]);
	}
	else if (!strcmp(test_case, "tick-unchanged"))
	{
		/* the tick still computes from the object's own inputs */
		long object_index = object_new(1);
		struct object_datum *object = object_get(object_index);

		object->object.incoming_function_values[0] = 0.5f;
		object_compute_function_values(object_index);
		CHECK(object->object.outgoing_function_values[0] == 0.5f, "the tick's value %f", object->object.outgoing_function_values[0]);
	}
	else if (!strcmp(test_case, "picture-moves"))
	{
		/* a function of the time still moves with the picture: its value a
		quarter of a tick back from the tick's */
		long object_index = object_new(1);
		struct object_datum *object = object_get(object_index);
		real huh = (57 * 5 + test_game_time) * 0.033333335f;
		real expected = (real)((huh - 0.25f * 0.033333335f) - floor(huh - 0.25f * 0.033333335f));

		object_compute_function_values(object_index);
		test_fraction = 0.75f;
		object_function_values_for_render(object_index, shown);
		CHECK(fabs(shown[1] - expected) < 1e-5, "the picture's value %f, expected %f", shown[1], expected);
		CHECK(shown[1] != object->object.outgoing_function_values[1], "the picture's value %f is the tick's", shown[1]);
	}
	else if (!strcmp(test_case, "additive-tick-inputs"))
	{
		/* an additive function steps by what its tick's inputs give: the
		picture is that part of the tick's step back from the tick's value,
		whatever the gun has written into input D since */
		long object_index = object_new(1);
		struct object_datum *object = object_get(object_index);

		object->object.incoming_function_values[3] = 0.25f;
		test_fraction = 1.0f;
		object_compute_function_values(object_index);
		CHECK(object->object.outgoing_function_values[2] == 0.25f, "the tick's value %f", object->object.outgoing_function_values[2]);
		object->object.incoming_function_values[3] = 0.75f;
		test_fraction = 0.5f;
		object_function_values_for_render(object_index, shown);
		CHECK(fabs(shown[2] - 0.125f) < 1e-6, "the picture's value %f, not half the tick's step back from its value", shown[2]);
	}
	else if (!strcmp(test_case, "no-kept-inputs"))
	{
		/* the inputs kept for an object gone are not another's: a new object
		at its index, not yet ticked, shows the values it holds (here as a
		game state loaded them), not ones made from the inputs kept for the
		object before it */
		long old_index = object_new(1);
		long new_index;
		struct object_datum *object;

		object_get(old_index)->object.incoming_function_values[0] = 1.0f;
		object_compute_function_values(old_index);
		new_index = object_new(2);
		object = object_get(new_index);
		object->object.outgoing_function_values[0] = 0.75f;
		object->object.incoming_function_values[0] = 0.25f;
		test_fraction = 0.5f;
		object_function_values_for_render(new_index, shown);
		CHECK(shown[0] == 0.75f, "the new object's picture %f, not the value it holds", shown[0]);
	}
	else if (!strcmp(test_case, "forgotten"))
	{
		/* a game state loaded (object_function_inputs_forget): the object
		shows the values it was loaded with until its next tick */
		long object_index = object_new(1);
		struct object_datum *object = object_get(object_index);

		object->object.incoming_function_values[0] = 1.0f;
		object_compute_function_values(object_index);
		object_function_inputs_forget();
		object->object.outgoing_function_values[0] = 0.5f;
		object->object.incoming_function_values[0] = 0.25f;
		test_fraction = 0.5f;
		object_function_values_for_render(object_index, shown);
		CHECK(shown[0] == 0.5f, "the loaded object's picture %f, not the value it was loaded with", shown[0]);
	}
	else
	{
		CHECK(FALSE, "no such case: %s", test_case);
	}

	return 0;
}
