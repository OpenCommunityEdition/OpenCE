/*
X20_CHIP.C (test)

The real x20_chip_from_console (x20_chip.c): a10's bridge cinematic's last
part (final_words) destroys Cortana's projection, sleeps 50 ticks, takes
the pipe from Captain Keyes's right hand, makes the chip and puts it there,
cuts to the next shot, sleeps 30 ticks and moves the chip to his left hand,
which he holds up before the camera: the chip came into it out of nowhere.
The pipe's taking, the chip's making and its move to his left hand go to
right after the projection is destroyed, before the 50 ticks. The script
destroys her projection earlier too; that one is not the handover's. The
nodes are laid out as a10's are. test_x20_chip.py takes the code from the
sources (under_test.inc).
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

/* final_words: an earlier destroy of the projection, then the handover,
with a sleep of the ticks given between the camera_sets and the move to
the left hand */
static long final_words_new(short shot_sleep)
{
	return begin_new(
		named_new("switch_bsp", "1"),
		named_new("object_destroy", "cortana_x20"),
		named_new("object_create", "cortana_x20"),
		named_new("camera_set", "chip_1b2"),
		sleep_new(constant_new(10)),
		named_new("effect_new", "cortana on off"),
		sleep_new(constant_new(30)),
		named_new("object_destroy", "cortana_x20"),
		sleep_new(constant_new(50)),
		named_new("objects_detach", "keyes"),
		named_new("object_destroy", "pipe"),
		named_new("object_destroy", "cortana_chip"),
		named_new("object_create", "cortana_chip"),
		named_new("objects_attach", "keyes"),
		named_new("camera_set", "chip_1c"),
		named_new("camera_set", "chip_1c2"),
		sleep_new(constant_new(shot_sleep)),
		named_new("objects_detach", "keyes"),
		named_new("objects_attach", "keyes"),
		named_new("sound_impulse_start", "keyes14"),
		named_new("print", "good luck"),
		NONE);
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

static boolean chip(long root)
{
	return x20_chip_from_console(syntax, strings, string_bytes, root);
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	syntax = game_state_data_new("script node", 2048, sizeof(struct hs_syntax_node));

	/* the chip made in his right hand and moved to his left right after the
	projection goes, before the 50 ticks; the earlier destroy, the shot and
	the rest as they were; no node made */
	CASE("handover")
	{
		long words = final_words_new(30);
		long count = syntax->count;

		CHECK(chip(words), "not changed");
		CHECK(!strcmp(begin_order(words),
			"switch_bsp:1 object_destroy:cortana_x20 object_create:cortana_x20 camera_set:chip_1b2 sleep:10 "
			"effect_new:cortana on off sleep:30 object_destroy:cortana_x20 objects_detach:keyes object_destroy:pipe "
			"object_destroy:cortana_chip object_create:cortana_chip objects_attach:keyes objects_detach:keyes "
			"objects_attach:keyes sleep:50 camera_set:chip_1c camera_set:chip_1c2 sleep:30 "
			"sound_impulse_start:keyes14 print:good luck"), "the scene %s", begin_order(words));
		CHECK(syntax->count == count, "%ld nodes made", syntax->count - count);
		return 0;
	}
	/* once: a second time changes nothing */
	CASE("once")
	{
		long words = final_words_new(30);

		CHECK(chip(words), "not changed");
		nodes_keep();
		CHECK(!chip(words), "changed again");
		CHECK(nodes_unchanged(), "changed again");
		return 0;
	}
	/* a shot of a sleep that is no constant, no begin: left as they are */
	CASE("other scenes")
	{
		long other = begin_new(named_new("object_destroy", "cortana_x20"), sleep_new(constant_new(50)),
			named_new("objects_detach", "keyes"), NONE);
		long call = named_new("object_destroy", "cortana_x20");

		nodes_keep();
		CHECK(!chip(other), "a shorter scene changed");
		CHECK(!chip(call), "no begin changed");
		CHECK(!chip(NONE), "no root changed");
		CHECK(nodes_unchanged(), "changed");
		return 0;
	}

	return 2;
}
