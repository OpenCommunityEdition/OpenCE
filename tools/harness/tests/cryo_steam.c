/*
CRYO_STEAM.C (test)

The real cryo_steam_vents_end_before_fade and cryo_steam_wake_without_steam
(cryo_steam.c): a10's opening cinematic destroyed its cryo tube's four
vents after its fade out to white, so that their steam drifted into the
next scene; and the short cryo scene that stands for the tutorial
(fast_setup) made the tube's steam as the tube opened, and left one vent of
it on for the rest of the level in a game of one player. The vents are
still made 90 ticks into the tube's shot, as on the Xbox, and destroyed as
the fade begins, the white held 45 ticks, not 15;
fast_setup's four object_creates of its steam are taken out. The nodes are
laid out as a10's are, in a data array of script nodes: a call (permanent)
names its function with its first child (a primitive of the function name
type), which its arguments follow. test_cryo_steam.py takes the code from
the sources (under_test.inc).
*/

#include "harness.h"

#include <stdarg.h>

typedef unsigned short word;
#define NUMBEROF(array) (sizeof(array) / sizeof((array)[0]))
#define csstrcmp strcmp

#include "under_test.inc"

#define PRIMITIVE 1
#define PERMANENT 8

static struct data_array *syntax;
static char strings[8192];
static long string_bytes;

static long string_add(char const *text)
{
	long offset = string_bytes;

	strcpy(strings + offset, text);
	string_bytes += (long)strlen(text) + 1;
	return offset;
}

static struct hs_syntax_node *node(long index)
{
	return (struct hs_syntax_node *)datum_get(syntax, index);
}

static long node_new(short type, short flags, char const *text)
{
	long index = datum_new(syntax);
	struct hs_syntax_node *new_node = node(index);

	new_node->type = type;
	new_node->flags = flags;
	new_node->next_node_index = NONE;
	new_node->string_offset = text ? string_add(text) : NONE;
	return index;
}

/* a call of the type given and its arguments, a NONE-ended list of nodes */
static long call_new(short type, char const *name, ...)
{
	long call = node_new(type, PERMANENT, NULL);
	long previous = node_new(_hs_function_name, PRIMITIVE | PERMANENT, name);
	long argument;
	va_list arguments;

	node(call)->data = previous;
	va_start(arguments, name);
	while ((argument = va_arg(arguments, long)) != NONE)
	{
		node(previous)->next_node_index = argument;
		previous = argument;
	}
	va_end(arguments);
	return call;
}

/* a primitive of the text given (an object's name, a string) */
static long name_new(char const *text)
{
	return node_new(_hs_type_string, PRIMITIVE | PERMANENT, text);
}

static long constant_new(short ticks)
{
	long constant = node_new(_hs_type_short_integer, PRIMITIVE | PERMANENT, "ticks");

	node(constant)->constant_type = _hs_type_short_integer;
	node(constant)->short_value = ticks;
	return constant;
}

static long sleep_new(long ticks)
{
	return call_new(_hs_type_void, "sleep", ticks, NONE);
}

/* (<function> <name>) */
static long named_new(char const *function, char const *name)
{
	return call_new(_hs_type_void, function, name_new(name), NONE);
}

/* (if (game_is_cooperative) (object_create <name>)) */
static long cooperative_new(char const *condition, char const *name)
{
	return call_new(_hs_type_void, "if", call_new(_hs_type_boolean, condition, NONE),
		named_new("object_create", name), NONE);
}

/* a begin of the expressions given, a NONE-ended list */
static long begin_new(long first, ...)
{
	long begin = call_new(_hs_type_void, "begin", NONE);
	long previous = node(begin)->data;
	long expression = first;
	va_list expressions;

	va_start(expressions, first);
	while (expression != NONE)
	{
		node(previous)->next_node_index = expression;
		previous = expression;
		expression = va_arg(expressions, long);
	}
	va_end(expressions);
	return begin;
}

/* a10's cryo, from the tube to the cleanup after the fade, with the vents
made 90 ticks into the tube's shot (or after the fade, when vents_late)
and the white held the ticks given */
static long cryo_new(boolean vents_late, short white)
{
	long tube = named_new("object_create_anew", "x10_tube");
	long vents = call_new(_hs_type_void, "object_create_anew_containing", name_new("x10_cryo_steam"), NONE);
	long sound = named_new("sound_impulse_start", "sound\\dialog\\x10\\ass04");
	long assistant = named_new("object_destroy", "assistant");

	return begin_new(
		tube,
		named_new("object_create_anew", "x10_chief"),
		named_new("camera_set", "booth_hi_1"),
		named_new("camera_set", "tube_1a"),
		sleep_new(constant_new(90)),
		vents_late ? sound : vents,
		vents_late ? sleep_new(call_new(_hs_type_short_integer, "camera_time", NONE)) : sound,
		vents_late ? call_new(_hs_type_void, "fade_out", constant_new(1), NONE) :
			sleep_new(call_new(_hs_type_short_integer, "camera_time", NONE)),
		vents_late ? sleep_new(constant_new(white)) : call_new(_hs_type_void, "fade_out", constant_new(1), NONE),
		vents_late ? vents : sleep_new(constant_new(white)),
		assistant,
		named_new("object_destroy", "x10_tube"),
		call_new(_hs_type_void, "object_destroy_containing", name_new("x10_cryo_steam"), NONE),
		named_new("camera_control", "false"),
		NONE);
}

/* a10's fast_setup, its tubes and their steam (all four, or without the
named one), each steam made once, or the named one twice */
static long fast_setup_new(char const *left_out, char const *twice)
{
	static char const *const steam[] =
		{ "cryotube_1_steam_1", "cryotube_1_steam_2", "cryotube_2_steam_1", "cryotube_2_steam_2" };
	long expressions[16];
	short count = 0;
	short steam_index;
	long begin;
	long previous;
	short index;

	expressions[count++] = named_new("object_create", "cryotube_1");
	expressions[count++] = cooperative_new("game_is_cooperative", "cryotube_2");
	expressions[count++] = named_new("unit_enter_vehicle", "player0");
	for (steam_index = 0; steam_index < 4; steam_index++)
	{
		short made;

		if (left_out && !strcmp(left_out, steam[steam_index]))
			continue;
		for (made = 0; made < (twice && !strcmp(twice, steam[steam_index]) ? 2 : 1); made++)
		{
			expressions[count++] = steam_index < 2 ?
				named_new("object_create", steam[steam_index]) :
				cooperative_new("game_is_cooperative", steam[steam_index]);
		}
	}
	expressions[count++] = call_new(_hs_type_void, "cinematic_start", NONE);
	expressions[count++] = named_new("unit_open", "cryotube_1");
	expressions[count++] = named_new("object_destroy", "cryotube_1_steam_1");
	expressions[count++] = named_new("object_destroy", "cryotube_1_steam_2");
	begin = call_new(_hs_type_void, "begin", NONE);
	previous = node(begin)->data;
	for (index = 0; index < count; index++)
	{
		node(previous)->next_node_index = expressions[index];
		previous = expressions[index];
	}
	return begin;
}

/* the begin's expressions, in order: a call by its function's name and its
first argument (a name, a constant's value, a call's function; an if's,
the call its branch makes) */
static char const *begin_order(long begin)
{
	static char order[4096];
	long index = node(node(begin)->data)->next_node_index;
	long walked = 0;

	order[0] = 0;
	while (index != NONE && walked++ < 200)
	{
		struct hs_syntax_node *expression = node(index);
		struct hs_syntax_node *name = node(expression->data);
		struct hs_syntax_node *argument = name->next_node_index != NONE ? node(name->next_node_index) : NULL;

		if (order[0])
			strcat(order, " ");
		strcat(order, strings + name->string_offset);
		if (!strcmp(strings + name->string_offset, "if"))
		{
			struct hs_syntax_node *branch = node(argument->next_node_index);

			sprintf(order + strlen(order), ":%s", strings + node(node(branch->data)->next_node_index)->string_offset);
		}
		else if (argument && TEST_FLAG(argument->flags, 0) && argument->type == _hs_type_short_integer)
		{
			sprintf(order + strlen(order), ":%d", argument->short_value);
		}
		else if (argument && TEST_FLAG(argument->flags, 0))
		{
			sprintf(order + strlen(order), ":%s", strings + argument->string_offset);
		}
		else if (argument)
		{
			sprintf(order + strlen(order), ":(%s)", strings + node(argument->data)->string_offset);
		}
		index = expression->next_node_index;
	}
	return order;
}

/* the nodes as they are, to see that nothing changed */
static unsigned char before[2048 * sizeof(struct hs_syntax_node)];
static long before_count;

static void nodes_keep(void)
{
	memcpy(before, syntax->data, (size_t)(syntax->maximum_count * syntax->size));
	before_count = syntax->count;
}

static boolean nodes_unchanged(void)
{
	return before_count == syntax->count &&
		!memcmp(before, syntax->data, (size_t)(syntax->maximum_count * syntax->size));
}

static boolean vents(long root)
{
	return cryo_steam_vents_end_before_fade(syntax, strings, string_bytes, root);
}

static boolean wake(long root)
{
	return cryo_steam_wake_without_steam(syntax, strings, string_bytes, root);
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	syntax = game_state_data_new("script node", 2048, sizeof(struct hs_syntax_node));

	/* the vents still made 90 ticks into the tube's shot, destroyed as the
	fade begins, and the white held 45 ticks; nothing else moves, and no
	node is made */
	CASE("vents")
	{
		long cryo = cryo_new(FALSE, 15);
		long count = syntax->count;

		CHECK(vents(cryo), "not changed");
		CHECK(!strcmp(begin_order(cryo),
			"object_create_anew:x10_tube object_create_anew:x10_chief camera_set:booth_hi_1 camera_set:tube_1a "
			"sleep:90 object_create_anew_containing:x10_cryo_steam sound_impulse_start:sound\\dialog\\x10\\ass04 "
			"sleep:(camera_time) object_destroy_containing:x10_cryo_steam fade_out:1 sleep:45 "
			"object_destroy:assistant object_destroy:x10_tube camera_control:false"), "the scene %s", begin_order(cryo));
		CHECK(syntax->count == count, "%ld nodes made", syntax->count - count);
		return 0;
	}
	/* once: a second time (hs_scenario_postprocess runs again on a revert)
	changes nothing */
	CASE("vents once")
	{
		long cryo = cryo_new(FALSE, 15);

		CHECK(vents(cryo), "not changed");
		nodes_keep();
		CHECK(!vents(cryo), "changed again");
		CHECK(nodes_unchanged(), "changed again");
		return 0;
	}
	/* the vents made after the fade, a white of another length, no begin:
	left as they are */
	CASE("other scenes")
	{
		long vents_late = cryo_new(TRUE, 15);
		long other_white = cryo_new(FALSE, 30);
		long call = named_new("object_create_anew", "x10_tube");

		nodes_keep();
		CHECK(!vents(vents_late), "the vents made after the fade changed");
		CHECK(!vents(other_white), "another white changed");
		CHECK(!vents(call), "no begin changed");
		CHECK(!vents(NONE), "no root changed");
		CHECK(!cryo_steam_vents_end_before_fade(syntax, NULL, string_bytes, other_white), "no strings changed");
		CHECK(nodes_unchanged(), "changed");
		return 0;
	}
	/* fast_setup's four steam creates taken out; the tubes, the second's if,
	the rest, as they were */
	CASE("wake")
	{
		long setup = fast_setup_new(NULL, NULL);

		CHECK(wake(setup), "not changed");
		CHECK(!strcmp(begin_order(setup),
			"object_create:cryotube_1 if:cryotube_2 unit_enter_vehicle:player0 cinematic_start unit_open:cryotube_1 "
			"object_destroy:cryotube_1_steam_1 object_destroy:cryotube_1_steam_2"), "the scene %s", begin_order(setup));
		return 0;
	}
	CASE("wake once")
	{
		long setup = fast_setup_new(NULL, NULL);

		CHECK(wake(setup), "not changed");
		nodes_keep();
		CHECK(!wake(setup), "changed again");
		CHECK(nodes_unchanged(), "changed again");
		return 0;
	}
	/* one of the four missing, one made twice, the second tube's under
	another condition: left as they are */
	CASE("other wakes")
	{
		long missing = fast_setup_new("cryotube_2_steam_2", NULL);
		long doubled = fast_setup_new(NULL, "cryotube_1_steam_1");
		long other = begin_new(
			named_new("object_create", "cryotube_1_steam_1"),
			named_new("object_create", "cryotube_1_steam_2"),
			cooperative_new("game_is_solo", "cryotube_2_steam_1"),
			cooperative_new("game_is_solo", "cryotube_2_steam_2"),
			NONE);

		nodes_keep();
		CHECK(!wake(missing), "a scene without one changed");
		CHECK(!wake(doubled), "a scene with one twice changed");
		CHECK(!wake(other), "another condition changed");
		CHECK(!wake(NONE), "no root changed");
		CHECK(nodes_unchanged(), "changed");
		return 0;
	}

	return 2;
}
