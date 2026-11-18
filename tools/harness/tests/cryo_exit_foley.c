/*
CRYO_EXIT_FOLEY.C (test)

The real cryo_exit_foley_move (cryo_exit_foley.c): a10's short cryo scene
(fast_setup, off Normal and in co-op) started the foley of the Master Chief
climbing out of his cryo tube, then opened the tube and had him climb out
45 ticks later; on Normal the foley starts 45 ticks after he begins to
climb (the player's button press, then the fade out and in), where its
steps fall with his. The foley's start is moved to 45 ticks after the
climb out, the sleep it falls in split in two, and nothing else moves in
time. The nodes are laid out as a10's are, in a data array of script nodes:
a call (permanent) names its function with its first child (a primitive of
the function name type), which its arguments follow; a call of one of the
scenario's scripts, (player0), has the script bit. test_cryo_exit_foley.py
takes the code from the sources (under_test.inc).
*/

#include "harness.h"

#include <stdarg.h>

#include "under_test.inc"

#define PRIMITIVE 1
#define SCRIPT 2
#define PERMANENT 8
#define FOLEY "sound\\sinomatixx_foley\\a10_cryoexit_foley"

static struct data_array *syntax;
static char strings[2048];
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

/* a call (of a function, or with SCRIPT of a script) and its arguments, a
NULL-ended list of primitives' texts */
static long call_new(short flags, char const *name, ...)
{
	long call = node_new(_hs_type_void, PERMANENT | flags, NULL);
	long previous = node_new(_hs_function_name, PRIMITIVE | PERMANENT, name);
	char const *argument;
	va_list arguments;

	node(call)->data = previous;
	va_start(arguments, name);
	while ((argument = va_arg(arguments, char const *)) != NULL)
	{
		long primitive = node_new(_hs_type_string, PRIMITIVE | PERMANENT, argument);

		node(previous)->next_node_index = primitive;
		previous = primitive;
	}
	va_end(arguments);
	return call;
}

static long sleep_new(short ticks)
{
	long call = node_new(_hs_type_void, PERMANENT, NULL);
	long name = node_new(_hs_function_name, PRIMITIVE | PERMANENT, "sleep");
	long constant = node_new(_hs_type_short_integer, PRIMITIVE | PERMANENT, "ticks");

	node(call)->data = name;
	node(name)->next_node_index = constant;
	node(constant)->short_value = ticks;
	return call;
}

static long exit_new(void)
{
	long exit = call_new(0, "unit_exit_vehicle", NULL);
	long player = call_new(SCRIPT, "player0", NULL);

	node(node(exit)->data)->next_node_index = player;
	return exit;
}

/* a begin of the expressions given, a NONE-ended list */
static long begin_new(long first, ...)
{
	long begin = call_new(0, "begin", NULL);
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

/* the functions the begin calls, in order (a sleep with its ticks) */
static char const *begin_order(long begin)
{
	static char order[1024];
	long index = node(node(begin)->data)->next_node_index;
	long walked = 0;

	order[0] = 0;
	while (index != NONE && walked++ < 200)
	{
		struct hs_syntax_node *name = node(node(index)->data);

		if (order[0])
			strcat(order, " ");
		strcat(order, strings + name->string_offset);
		if (!strcmp(strings + name->string_offset, "sleep"))
			sprintf(order + strlen(order), ":%d", node(name->next_node_index)->short_value);
		index = node(index)->next_node_index;
	}
	return order;
}

static boolean move(long root)
{
	return cryo_exit_foley_move(syntax, strings, string_bytes, root);
}

/* a10's fast_setup, from the camera's fade in to past the climb out, with
the looping sound given */
static long fast_setup_new(char const *sound)
{
	return begin_new(
		call_new(0, "fade_in", "1", "1", "1", "15", NULL),
		sleep_new(15),
		call_new(0, "sound_looping_start", sound, "none", "1", NULL),
		call_new(0, "unit_open", "cryotube_1", NULL),
		sleep_new(30),
		call_new(0, "if", NULL),
		sleep_new(15),
		exit_new(),
		sleep_new(30),
		call_new(0, "if", NULL),
		sleep_new(150),
		call_new(0, "object_destroy", "cryotube_1_steam_1", NULL),
		NONE);
}

/* the nodes as they are, to see that nothing changed */
static unsigned char before[256 * sizeof(struct hs_syntax_node)];
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

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	syntax = game_state_data_new("script node", 256, sizeof(struct hs_syntax_node));

	/* the foley starts 45 ticks into the climb, between the two halves of
	the sleep it falls in; the tube opens, he climbs out, and the rest
	follows, when they did */
	CASE("fast setup")
	{
		long root = fast_setup_new(FOLEY);
		long count = syntax->count;

		CHECK(move(root), "not moved");
		CHECK(!strcmp(begin_order(root),
			"fade_in sleep:15 unit_open sleep:30 if sleep:15 unit_exit_vehicle sleep:30 if sleep:15 "
			"sound_looping_start sleep:135 object_destroy"),
			"in the order %s", begin_order(root));
		CHECK(syntax->count == count + 3, "%ld nodes made", syntax->count - count);
		return 0;
	}
	/* sleeps that end 45 ticks into the climb: the foley after them, no
	nodes made */
	CASE("exact")
	{
		long root = begin_new(
			call_new(0, "sound_looping_start", FOLEY, "none", "1", NULL),
			call_new(0, "unit_open", "cryotube_1", NULL),
			sleep_new(45),
			exit_new(),
			sleep_new(30),
			call_new(0, "if", NULL),
			sleep_new(15),
			sleep_new(150),
			NONE);
		long count = syntax->count;

		CHECK(move(root), "not moved");
		CHECK(!strcmp(begin_order(root),
			"unit_open sleep:45 unit_exit_vehicle sleep:30 if sleep:15 sound_looping_start sleep:150"),
			"in the order %s", begin_order(root));
		CHECK(syntax->count == count, "nodes made");
		return 0;
	}
	/* moved once: a second time (hs_scenario_postprocess runs again) changes
	nothing */
	CASE("moved once")
	{
		long root = fast_setup_new(FOLEY);

		CHECK(move(root), "not moved");
		nodes_keep();
		CHECK(!move(root), "moved again");
		CHECK(nodes_unchanged(), "changed again");
		return 0;
	}
	/* Normal's tutorial_action: the foley's start, then the climb out at
	once (the tube opened long before): left as it is */
	CASE("normal")
	{
		long root = begin_new(
			call_new(0, "fade_in", "1", "1", "1", "15", NULL),
			sleep_new(15),
			call_new(0, "sound_looping_start", FOLEY, "none", "1", NULL),
			exit_new(),
			sleep_new(170),
			NONE);

		nodes_keep();
		CHECK(!move(root), "moved");
		CHECK(nodes_unchanged(), "changed");
		return 0;
	}
	/* another sound, no climb out, the climb out first, or something that
	may take time after it: left as it is */
	CASE("another begin")
	{
		long other = fast_setup_new("sound\\sinomatixx_foley\\a10_other_foley");
		long no_exit = begin_new(
			call_new(0, "sound_looping_start", FOLEY, "none", "1", NULL),
			call_new(0, "unit_open", "cryotube_1", NULL),
			sleep_new(150),
			NONE);
		long exit_first = begin_new(
			exit_new(),
			call_new(0, "sound_looping_start", FOLEY, "none", "1", NULL),
			call_new(0, "unit_open", "cryotube_1", NULL),
			sleep_new(150),
			NONE);
		long waits = begin_new(
			call_new(0, "sound_looping_start", FOLEY, "none", "1", NULL),
			call_new(0, "unit_open", "cryotube_1", NULL),
			exit_new(),
			call_new(0, "sleep_until", "ready", NULL),
			sleep_new(150),
			NONE);
		long short_after = begin_new(
			call_new(0, "sound_looping_start", FOLEY, "none", "1", NULL),
			call_new(0, "unit_open", "cryotube_1", NULL),
			exit_new(),
			sleep_new(30),
			NONE);

		nodes_keep();
		CHECK(!move(other), "another sound moved");
		CHECK(!move(no_exit), "moved with no climb out");
		CHECK(!move(exit_first), "moved after the climb out");
		CHECK(!move(waits), "moved past a wait");
		CHECK(!move(short_after), "moved past the begin's end");
		CHECK(nodes_unchanged(), "changed");
		return 0;
	}
	/* nodes with no room for the split: left as it is */
	CASE("full")
	{
		long root = fast_setup_new(FOLEY);

		syntax->maximum_count = syntax->count + 2;
		nodes_keep();
		CHECK(!move(root), "moved");
		CHECK(nodes_unchanged(), "changed");
		return 0;
	}
	/* no begin, no root, no nodes or strings: left as it is */
	CASE("no begin")
	{
		long root = fast_setup_new(FOLEY);
		long call = call_new(0, "sound_looping_start", FOLEY, "none", "1", NULL);

		nodes_keep();
		CHECK(!move(call), "a call moved");
		CHECK(!move(NONE), "no root moved");
		CHECK(!cryo_exit_foley_move(syntax, NULL, string_bytes, root), "no strings moved");
		CHECK(!cryo_exit_foley_move(NULL, strings, string_bytes, root), "no nodes moved");
		CHECK(nodes_unchanged(), "changed");
		return 0;
	}

	return 2;
}
