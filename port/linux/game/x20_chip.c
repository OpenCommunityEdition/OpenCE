/*
X20_CHIP.C

a10's bridge cinematic (x20, its last part: final_words) has Captain Keyes
take Cortana's chip out of her projector for the Master Chief. Her
projection goes out (cortana on off, then object_destroy cortana_x20), and
50 ticks later, as it sets the camera to chip_1c, the script takes the pipe
from his right hand and makes the chip in that hand, behind him as he
stands at the console; 30 ticks into the shot it moves the chip to his left
hand, which he holds up empty before the camera as he turns: the chip came
into it there, in view, out of nowhere, on the Xbox too.

As a10's scripts are made ready (hs_scenario_postprocess), the pipe's
taking, the chip's making and its move to his left hand are moved to right
after her projection is destroyed, before the 50 ticks: the chip is in his
left hand from then on, which is at the projector's slot as the shot opens,
and he raises it from there into view.
Each expression is moved only when the script is as the map has it, so a
map whose script differs, or one already changed (the postprocess runs
again on a revert), is left as it is. The links are changed, no node made
or freed; the scripts run on the host alone, which sends what they make to
the clients, so nothing the machines send or agree on changes.
*/

/* ---------- headers */

#include "cseries.h"
#include "math/real_math.h"
#include "memory/data.h"
#include "hs/hs.h"
#include "hs/hs_scenario_definitions.h"
#include "scenario/scenario_definitions.h"
#include "tag_files/tag_groups.h"

#include "x20_chip.h"

#include <string.h>

/* ---------- constants */

#define A10_SCENARIO_NAME "levels\\a10\\a10"
#define X20_CHIP_SCRIPT_NAME "final_words"
/* the most expressions of the script's begin looked through */
#define X20_CHIP_MAXIMUM_EXPRESSIONS 256
/* the expressions moved: the pipe taken, the chip made and put in his right
hand; and, after the shot's two camera_sets and its (sleep 30), the chip
moved to his left hand */
#define X20_CHIP_MOVED_EXPRESSIONS 5
#define X20_CHIP_LEFT_HAND_AFTER 3
#define X20_CHIP_LEFT_HAND_EXPRESSIONS 2

/* a syntax node's flags, as hs_runtime.c names them */
enum
{
	_hs_syntax_node_primitive_bit = 0,
	_hs_syntax_node_script_bit,
	_hs_syntax_node_variable_bit,
};

/* ---------- globals */

extern long global_scenario_index;

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

/* a (sleep <ticks>) call's ticks, a constant: its node, or NULL if the node
is no such call */
static struct hs_syntax_node *script_sleep_ticks(
	struct data_array *syntax,
	char const *strings,
	long string_bytes,
	struct hs_syntax_node const *node)
{
	struct hs_syntax_node *ticks;

	if (!script_node_calls(syntax, strings, string_bytes, node, "sleep"))
	{
		return NULL;
	}
	ticks = script_node_get(syntax, script_call_name_node(syntax, node)->next_node_index);

	return ticks &&
		TEST_FLAG(ticks->flags, _hs_syntax_node_primitive_bit) &&
		!TEST_FLAG(ticks->flags, _hs_syntax_node_variable_bit) &&
		ticks->type == _hs_type_short_integer &&
		ticks->next_node_index == NONE ?
		ticks :
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

/* whether the expressions from destroyed on are the handover as the map
has it: (object_destroy cortana_x20), (sleep 50), the pipe taken from his
right hand, the chip made and put in it, the shot's two camera_sets, its
(sleep 30), and the chip moved to his left hand */
static boolean x20_chip_handover_at(
	struct data_array *syntax,
	char const *strings,
	long string_bytes,
	long const *expressions,
	short destroyed)
{
	short sleep = destroyed + 1;
	short first = sleep + 1;
	short last = first + X20_CHIP_MOVED_EXPRESSIONS - 1;
	short left_detach = last + X20_CHIP_LEFT_HAND_AFTER + 1;
	short left_attach = left_detach + 1;
	char const *projection = script_call_argument(syntax, strings, string_bytes,
		script_node_get(syntax, expressions[destroyed]), "object_destroy");
	char const *pipe = script_call_argument(syntax, strings, string_bytes,
		script_node_get(syntax, expressions[first + 1]), "object_destroy");
	char const *chip_destroyed = script_call_argument(syntax, strings, string_bytes,
		script_node_get(syntax, expressions[first + 2]), "object_destroy");
	char const *chip_made = script_call_argument(syntax, strings, string_bytes,
		script_node_get(syntax, expressions[first + 3]), "object_create");

	return projection && !strcmp(projection, "cortana_x20") &&
		script_sleep_ticks(syntax, strings, string_bytes, script_node_get(syntax, expressions[sleep])) &&
		script_node_calls(syntax, strings, string_bytes, script_node_get(syntax, expressions[first]),
			"objects_detach") &&
		pipe && !strcmp(pipe, "pipe") &&
		chip_destroyed && !strcmp(chip_destroyed, "cortana_chip") &&
		chip_made && !strcmp(chip_made, "cortana_chip") &&
		script_node_calls(syntax, strings, string_bytes, script_node_get(syntax, expressions[last]),
			"objects_attach") &&
		script_node_calls(syntax, strings, string_bytes, script_node_get(syntax, expressions[last + 1]),
			"camera_set") &&
		script_node_calls(syntax, strings, string_bytes, script_node_get(syntax, expressions[last + 2]),
			"camera_set") &&
		script_sleep_ticks(syntax, strings, string_bytes, script_node_get(syntax, expressions[last + 3])) &&
		script_node_calls(syntax, strings, string_bytes, script_node_get(syntax, expressions[left_detach]),
			"objects_detach") &&
		script_node_calls(syntax, strings, string_bytes, script_node_get(syntax, expressions[left_attach]),
			"objects_attach");
}

/* ---------- public code */

boolean x20_chip_from_console(
	struct data_array *syntax,
	char const *strings,
	long string_bytes,
	long root_expression_index)
{
	struct hs_syntax_node *name = script_begin_name_node(syntax, strings, string_bytes, root_expression_index);
	long expressions[X20_CHIP_MAXIMUM_EXPRESSIONS];
	short expression_count = 0;
	short destroyed;
	long link;

	if (!name)
	{
		return FALSE;
	}
	for (link = name->next_node_index;
		link != NONE && expression_count < X20_CHIP_MAXIMUM_EXPRESSIONS;
		link = script_node_get(syntax, link)->next_node_index)
	{
		if (!script_node_get(syntax, link))
		{
			return FALSE;
		}
		expressions[expression_count++] = link;
	}
	/* (the script destroys her projection more than once: the handover's) */
	for (destroyed = 0;
		destroyed + 2 + X20_CHIP_MOVED_EXPRESSIONS + X20_CHIP_LEFT_HAND_AFTER + X20_CHIP_LEFT_HAND_EXPRESSIONS <
			expression_count;
		destroyed++)
	{
		if (x20_chip_handover_at(syntax, strings, string_bytes, expressions, destroyed))
		{
			short sleep = destroyed + 1;
			short first = sleep + 1;
			short last = first + X20_CHIP_MOVED_EXPRESSIONS - 1;
			short left_detach = last + X20_CHIP_LEFT_HAND_AFTER + 1;
			short left_attach = left_detach + 1;

			/* the destroy, the moved expressions, the move to the left hand,
			the sleep, the shot's camera_sets and sleep, and what followed the
			move */
			script_node_get(syntax, expressions[destroyed])->next_node_index = expressions[first];
			script_node_get(syntax, expressions[last])->next_node_index = expressions[left_detach];
			script_node_get(syntax, expressions[left_attach])->next_node_index = expressions[sleep];
			script_node_get(syntax, expressions[sleep])->next_node_index = expressions[last + 1];
			script_node_get(syntax, expressions[last + 3])->next_node_index = expressions[left_attach + 1];
			return TRUE;
		}
	}

	return FALSE;
}

void x20_chip_scripts_loaded(
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

		if (!csstrcmp(script->name, X20_CHIP_SCRIPT_NAME))
		{
			x20_chip_from_console(
				syntax,
				(char const *)scenario->hs_string_constants.address,
				scenario->hs_string_constants.size,
				script->root_expression_index);
		}
	}

	return;
}
