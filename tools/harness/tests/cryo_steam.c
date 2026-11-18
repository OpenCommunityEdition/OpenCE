/*
CRYO_STEAM.C (test)

The real cryo_steam_wake_without_steam (cryo_steam.c): the short cryo
scene that stands for a10's tutorial (fast_setup) made the tube's steam as
the tube opened, and left one vent of it on for the rest of the level in a
game of one player. fast_setup's four object_creates of its steam are taken
out. The nodes are
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

static boolean wake(long root)
{
	return cryo_steam_wake_without_steam(syntax, strings, string_bytes, root);
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	syntax = game_state_data_new("script node", 2048, sizeof(struct hs_syntax_node));

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
