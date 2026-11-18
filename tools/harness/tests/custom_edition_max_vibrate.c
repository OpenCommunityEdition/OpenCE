/*
CUSTOM_EDITION_MAX_VIBRATE.C (test)

The real function_call_convert (custom_edition_scripts.c), with
function_find_by_name, script_string_shared and the code they call, over a
Custom Edition map's compiled script: a call of Halo PC's
player_effect_set_max_vibrate, the name Halo PC gives this build's
player_effect_set_max_rumble, is given this build's function, and its name in
the string data is made this build's, which the game looks up again when it
loads the map (hs_compile.c). Without the alias the call did nothing (a
constant). A name that another value shares in the string data, as the
compiler keeps each text once, is not changed, and a build name longer than
Halo PC's, which would not fit, is not taken.
test_custom_edition_max_vibrate.py takes the constants, structures and code
from the sources (config.inc, under_test.inc).
*/

#include <ctype.h>
#include <stdarg.h>
#include "harness.h"

#define NUMBEROF(a) (sizeof(a) / sizeof((a)[0]))

/* cseries.c's, in any case */
static long csstrcasecmp(char const *a, char const *b)
{
	while (*a && tolower((unsigned char)*a) == tolower((unsigned char)*b))
	{
		a++;
		b++;
	}
	return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

#include "config.inc"

/* the log */
enum { _error_silent };
static char log_text[4096];
static void error(int priority, char const *format, ...)
{
	size_t used = strlen(log_text);
	va_list arguments;

	va_start(arguments, format);
	vsnprintf(log_text + used, sizeof(log_text) - used, format, arguments);
	va_end(arguments);
	strncat(log_text, "\n", sizeof(log_text) - strlen(log_text) - 1);
}

/* this build's functions, found by name in any case as hs.c finds them */
static char const *const build_functions[] = { "begin", "set", "sleep", "player_effect_set_max_rumble" };
enum { RUMBLE_INDEX = 3 };
static short hs_find_function_by_name(char const *name)
{
	short index;

	for (index = 0; index < (short)NUMBEROF(build_functions); index++)
	{
		if (!csstrcasecmp(build_functions[index], name))
			return index;
	}
	return NONE;
}

/* no engine globals */
static short hs_find_global_by_name(char const *name)
{
	return NONE;
}

#include "under_test.inc"

/* a map's script: its string data and syntax nodes */
static char strings[256];
static unsigned long string_bytes;
static struct hs_syntax_node nodes[16];
static long node_count;
static struct scripts_conversion conversion;

static long string_add(char const *text)
{
	long offset = (long)string_bytes;

	strcpy(strings + offset, text);
	string_bytes += strlen(text) + 1;
	return offset;
}

static long node_add(short type, short flags, long string_offset, long data)
{
	struct hs_syntax_node *node = &nodes[node_count];

	memset(node, 0, sizeof(*node));
	node->datum_header = (short)(0x8000 | node_count);
	node->type = type;
	node->flags = flags;
	node->next_node_index = NONE;
	node->string_offset = string_offset;
	node->data = data;
	return node_count++;
}

/* a call of the function named at `name_offset`, with Halo PC's index for
it (not this build's), of no arguments: the call's node */
static long call_add(long name_offset)
{
	long name = node_add(_hs_function_name, FLAG(_hs_syntax_node_primitive_bit), name_offset, 0);
	long call = node_add(_hs_type_void, 0, NONE, ((long)0x8000 << 16) | name);

	nodes[name].function_index = 400;
	nodes[call].function_index = 400;
	return call;
}

/* the string data, ending with the empty string, and a canary after it */
static void conversion_set_up(void)
{
	string_add("");
	memset(&conversion, 0, sizeof(conversion));
	conversion.nodes = nodes;
	conversion.node_count = node_count;
	conversion.strings = strings;
	conversion.string_bytes = string_bytes;
	conversion.empty_string = 0x40440000 + (long)string_bytes - 1;
	strcpy(strings + string_bytes, "canary");
}

static void canary_check(void)
{
	CHECK(!strcmp(strings + string_bytes, "canary"), "the string data's end was written over");
}

/* a call of this build's function, not made a constant */
static void call_check(long call, short function_index)
{
	struct hs_syntax_node const *name = &nodes[DATUM_INDEX_TO_ABSOLUTE_INDEX(nodes[call].data)];

	CHECK(!TEST_FLAG(nodes[call].flags, _hs_syntax_node_primitive_bit),
		"the call was made a constant (log: %s)", log_text);
	CHECK(nodes[call].function_index == function_index && name->function_index == function_index,
		"the call has function %d (its name %d), not this build's %d", nodes[call].function_index,
		name->function_index, function_index);
}

/* a call made a constant that does nothing */
static void inert_check(long call)
{
	CHECK(TEST_FLAG(nodes[call].flags, _hs_syntax_node_primitive_bit) && nodes[call].data == 0,
		"the call was not made a constant (function %d)", nodes[call].function_index);
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";

	/* the alias finds this build's player_effect_set_max_rumble */
	CASE("alias-finds-rumble")
	{
		long call = call_add(string_add("player_effect_set_max_vibrate"));

		conversion_set_up();
		CHECK(function_call_convert(&conversion, &nodes[call]), "the call was not laid out as one");
		call_check(call, RUMBLE_INDEX);
		CHECK(conversion.functions_renumbered == 1 && conversion.made_inert == 0,
			"%ld calls renumbered, %ld made constants", conversion.functions_renumbered, conversion.made_inert);
		return 0;
	}
	/* the name in the string data is this build's, which every call sharing
	it then finds, and which the game finds as it loads the map */
	CASE("name-replaced")
	{
		long offset = string_add("player_effect_set_max_vibrate");
		long next = string_add("sleep");
		long first = call_add(offset);
		long second = call_add(offset);

		conversion_set_up();
		CHECK(function_call_convert(&conversion, &nodes[first]), "the first call was not laid out as one");
		CHECK(!strcmp(strings + offset, "player_effect_set_max_rumble"), "the name reads '%s'", strings + offset);
		CHECK(hs_find_function_by_name(strings + offset) == RUMBLE_INDEX, "the game finds no function by the name");
		CHECK(!strcmp(strings + next, "sleep"), "the next string reads '%s'", strings + next);
		CHECK(function_call_convert(&conversion, &nodes[second]), "the second call was not laid out as one");
		call_check(second, RUMBLE_INDEX);
		canary_check();
		return 0;
	}
	/* written in any case, as Halo PC's compiler keeps a name as written */
	CASE("any-case")
	{
		long offset = string_add("Player_Effect_Set_Max_Vibrate");
		long call = call_add(offset);

		conversion_set_up();
		CHECK(function_call_convert(&conversion, &nodes[call]), "the call was not laid out as one");
		call_check(call, RUMBLE_INDEX);
		CHECK(!strcmp(strings + offset, "player_effect_set_max_rumble"), "the name reads '%s'", strings + offset);
		return 0;
	}
	/* a function this build does not have still does nothing, its name
	unchanged */
	CASE("unknown-inert")
	{
		long offset = string_add("player_effect_set_max_wobble");
		long call = call_add(offset);

		conversion_set_up();
		CHECK(function_call_convert(&conversion, &nodes[call]), "the call was not laid out as one");
		inert_check(call);
		CHECK(!strcmp(strings + offset, "player_effect_set_max_wobble"), "the name reads '%s'", strings + offset);
		CHECK(strstr(log_text, "'player_effect_set_max_wobble'") != NULL, "the missing name was not logged: %s",
			log_text);
		return 0;
	}
	/* a string constant of the same text shares the name's bytes: neither is
	changed, and the call does nothing, as before */
	CASE("shared-text-inert")
	{
		long offset = string_add("player_effect_set_max_vibrate");
		long call = call_add(offset);

		node_add(_hs_type_string, FLAG(_hs_syntax_node_primitive_bit), offset, 0x40440000 + offset);
		conversion_set_up();
		CHECK(function_call_convert(&conversion, &nodes[call]), "the call was not laid out as one");
		inert_check(call);
		CHECK(!strcmp(strings + offset, "player_effect_set_max_vibrate"), "the shared text reads '%s'",
			strings + offset);
		return 0;
	}
	/* (built with a table whose Halo PC name is shorter than this build's)
	this build's name would not fit in its place: not taken, nothing written */
	CASE("longer-refused")
	{
		long offset = string_add("player_effect_set_max_vib");
		long next = string_add("sleep");
		long call = call_add(offset);

		conversion_set_up();
		CHECK(function_call_convert(&conversion, &nodes[call]), "the call was not laid out as one");
		inert_check(call);
		CHECK(!strcmp(strings + offset, "player_effect_set_max_vib"), "the name reads '%s'", strings + offset);
		CHECK(!strcmp(strings + next, "sleep"), "the next string reads '%s'", strings + next);
		canary_check();
		return 0;
	}
	fprintf(stderr, "unknown case '%s'\n", case_name);
	return 2;
}
