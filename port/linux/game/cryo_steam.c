/*
CRYO_STEAM.C

a10 opens in its cryo bay. Off Normal, and in co-op, a short scene stands
for the cryo technicians' tutorial (fast_setup). It made the Master Chief's
tube's steam (cryotube_1_steam_1 and 2, and in co-op the second tube's) as
the tube opened, and destroyed it after the climb out, but an if around the
wrong line left cryotube_1_steam_2 venting for the rest of the level in a
game of one player, on the Xbox too. The tube now opens without steam:
fast_setup's object_creates of it are taken out (its object_destroys of it
then destroy nothing).

The tutorial (tutorial_setup, on Normal in a game of one player) keeps its
steam, made as the Chief wakes and destroyed as its fade lifts.

As a10's scripts are made ready (hs_scenario_postprocess), after the climb's
foley is moved (cryo_exit_foley.c), fast_setup is changed only when it is as
the map has it: the two vents' object_creates of the first tube and the
second tube's in an if of (game_is_cooperative) each, all four. So a map
whose scripts differ, or one already changed (the postprocess runs again on
a revert), is left as it is. The expressions are taken out by their begin's
links alone. The scripts run on the host alone, which sends what they make
and destroy to the clients (network_coop.c), so nothing the machines send or
agree on changes.
*/

/* ---------- headers */

#include "cseries.h"
#include "math/real_math.h"
#include "memory/data.h"
#include "hs/hs.h"
#include "hs/hs_scenario_definitions.h"
#include "scenario/scenario_definitions.h"
#include "tag_files/tag_groups.h"

#include "cryo_steam.h"

#include <string.h>

/* ---------- constants */

#define A10_SCENARIO_NAME "levels\\a10\\a10"
#define FAST_SETUP_SCRIPT_NAME "fast_setup"

/* a syntax node's flags, as hs_runtime.c names them */
enum
{
	_hs_syntax_node_primitive_bit = 0,
	_hs_syntax_node_script_bit,
	_hs_syntax_node_variable_bit,
};

/* ---------- globals */

extern long global_scenario_index;

/* fast_setup's steam: made directly, and made in co-op */
static char const *const cryo_steam_first_tube_names[] =
{
	"cryotube_1_steam_1",
	"cryotube_1_steam_2",
};
static char const *const cryo_steam_second_tube_names[] =
{
	"cryotube_2_steam_1",
	"cryotube_2_steam_2",
};

/* ---------- private code */

/* the node a link names, or NULL if it names none of the nodes in use */
static struct hs_syntax_node *script_node_get(
	struct data_array *syntax,
	long index)
{
	long absolute_index;
	struct hs_syntax_node *node;

	if (index == NONE)
	{
		return NULL;
	}
	absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(index);
	if (absolute_index < 0 || absolute_index >= syntax->count)
	{
		return NULL;
	}
	node = (struct hs_syntax_node *)syntax->data + absolute_index;

	return node->datum_header ? node : NULL;
}

/* the script string at offset, or NULL unless it lies within them */
static char const *script_string_get(
	char const *strings,
	long string_bytes,
	long offset)
{
	return offset >= 0 && offset < string_bytes &&
		memchr(strings + offset, 0, (size_t)(string_bytes - offset)) ?
		strings + offset :
		NULL;
}

/* the name node of a call of one of the game's functions (it names the
function, and the call's arguments follow it), or NULL if the node is no
such call */
static struct hs_syntax_node *script_call_name_node(
	struct data_array *syntax,
	struct hs_syntax_node const *node)
{
	struct hs_syntax_node *name;

	if (!node ||
		TEST_FLAG(node->flags, _hs_syntax_node_primitive_bit) ||
		TEST_FLAG(node->flags, _hs_syntax_node_script_bit))
	{
		return NULL;
	}
	name = script_node_get(syntax, node->data);

	return name && name->type == _hs_function_name ? name : NULL;
}

/* whether the node is a call of the function of that name */
static boolean script_node_calls(
	struct data_array *syntax,
	char const *strings,
	long string_bytes,
	struct hs_syntax_node const *node,
	char const *function_name)
{
	struct hs_syntax_node const *name = script_call_name_node(syntax, node);
	char const *string = name ? script_string_get(strings, string_bytes, name->string_offset) : NULL;

	return string && !strcmp(string, function_name);
}

/* the string of a call's only argument, a primitive, or NULL if the node is
no call of the function of that name with one */
static char const *script_call_argument(
	struct data_array *syntax,
	char const *strings,
	long string_bytes,
	struct hs_syntax_node const *node,
	char const *function_name)
{
	struct hs_syntax_node const *argument;

	if (!script_node_calls(syntax, strings, string_bytes, node, function_name))
	{
		return NULL;
	}
	argument = script_node_get(syntax, script_call_name_node(syntax, node)->next_node_index);

	return argument &&
		TEST_FLAG(argument->flags, _hs_syntax_node_primitive_bit) &&
		argument->next_node_index == NONE ?
		script_string_get(strings, string_bytes, argument->string_offset) :
		NULL;
}

/* the name node of the begin at the root, or NULL if the root is no begin */
static struct hs_syntax_node *script_begin_name_node(
	struct data_array *syntax,
	char const *strings,
	long string_bytes,
	long root_expression_index)
{
	struct hs_syntax_node *root;

	if (!syntax || !syntax->data || !strings)
	{
		return NULL;
	}
	root = script_node_get(syntax, root_expression_index);

	return script_node_calls(syntax, strings, string_bytes, root, "begin") ?
		script_call_name_node(syntax, root) :
		NULL;
}

/* the index of the name in names, or NONE */
static short cryo_steam_name_index(
	char const *name,
	char const *const *names,
	short name_count)
{
	short name_index;

	for (name_index = 0; name && name_index < name_count; name_index++)
	{
		if (!strcmp(name, names[name_index]))
		{
			return name_index;
		}
	}

	return NONE;
}

/* the second tube's steam an (if (game_is_cooperative) (object_create
<name>)) makes, by its index in cryo_steam_second_tube_names, or NONE if the
node is no such if */
static short cryo_steam_cooperative_create(
	struct data_array *syntax,
	char const *strings,
	long string_bytes,
	struct hs_syntax_node const *node)
{
	struct hs_syntax_node const *condition;
	struct hs_syntax_node const *branch;

	if (!script_node_calls(syntax, strings, string_bytes, node, "if"))
	{
		return NONE;
	}
	condition = script_node_get(syntax, script_call_name_node(syntax, node)->next_node_index);
	branch = condition ? script_node_get(syntax, condition->next_node_index) : NULL;
	if (!condition ||
		!script_node_calls(syntax, strings, string_bytes, condition, "game_is_cooperative") ||
		!branch ||
		branch->next_node_index != NONE)
	{
		return NONE;
	}

	return cryo_steam_name_index(
		script_call_argument(syntax, strings, string_bytes, branch, "object_create"),
		cryo_steam_second_tube_names,
		(short)NUMBEROF(cryo_steam_second_tube_names));
}

/* ---------- public code */

boolean cryo_steam_wake_without_steam(
	struct data_array *syntax,
	char const *strings,
	long string_bytes,
	long root_expression_index)
{
	struct hs_syntax_node *name = script_begin_name_node(syntax, strings, string_bytes, root_expression_index);
	boolean first_tube_found[NUMBEROF(cryo_steam_first_tube_names)];
	boolean second_tube_found[NUMBEROF(cryo_steam_second_tube_names)];
	struct hs_syntax_node *previous;
	long index;
	long walked;
	short name_index;

	if (!name)
	{
		return FALSE;
	}

	/* each of the four made once, before anything is taken out */
	memset(first_tube_found, 0, sizeof(first_tube_found));
	memset(second_tube_found, 0, sizeof(second_tube_found));
	index = name->next_node_index;
	for (walked = 0; index != NONE && walked < syntax->count; walked++)
	{
		struct hs_syntax_node *expression = script_node_get(syntax, index);
		short first = expression ? cryo_steam_name_index(
			script_call_argument(syntax, strings, string_bytes, expression, "object_create"),
			cryo_steam_first_tube_names,
			(short)NUMBEROF(cryo_steam_first_tube_names)) : NONE;
		short second = expression ? cryo_steam_cooperative_create(syntax, strings, string_bytes, expression) : NONE;

		if (!expression || (first != NONE && first_tube_found[first]) ||
			(second != NONE && second_tube_found[second]))
		{
			return FALSE;
		}
		if (first != NONE)
		{
			first_tube_found[first] = TRUE;
		}
		if (second != NONE)
		{
			second_tube_found[second] = TRUE;
		}
		index = expression->next_node_index;
	}
	for (name_index = 0; name_index < (short)NUMBEROF(first_tube_found); name_index++)
	{
		if (!first_tube_found[name_index] || !second_tube_found[name_index])
		{
			return FALSE;
		}
	}

	/* taken out of the begin */
	previous = name;
	index = name->next_node_index;
	for (walked = 0; index != NONE && walked < syntax->count; walked++)
	{
		struct hs_syntax_node *expression = script_node_get(syntax, index);

		if (cryo_steam_name_index(
				script_call_argument(syntax, strings, string_bytes, expression, "object_create"),
				cryo_steam_first_tube_names,
				(short)NUMBEROF(cryo_steam_first_tube_names)) != NONE ||
			cryo_steam_cooperative_create(syntax, strings, string_bytes, expression) != NONE)
		{
			previous->next_node_index = expression->next_node_index;
		}
		else
		{
			previous = expression;
		}
		index = expression->next_node_index;
	}

	return TRUE;
}

void cryo_steam_scripts_loaded(
	struct scenario *scenario,
	struct data_array *syntax)
{
	char const *scenario_name = global_scenario_index != NONE ? tag_get_name(global_scenario_index) : NULL;
	long index;

	if (!scenario || !scenario_name || csstrcmp(scenario_name, A10_SCENARIO_NAME) ||
		!scenario->hs_scripts.address || !scenario->hs_string_constants.address)
	{
		return;
	}

	for (index = 0; index < scenario->hs_scripts.count; index++)
	{
		struct hs_script const *script = TAG_BLOCK_GET_ELEMENT(&scenario->hs_scripts, index, struct hs_script);

		if (!csstrcmp(script->name, FAST_SETUP_SCRIPT_NAME))
		{
			cryo_steam_wake_without_steam(
				syntax,
				(char const *)scenario->hs_string_constants.address,
				scenario->hs_string_constants.size,
				script->root_expression_index);
		}
	}

	return;
}
