/*
CRYO_EXIT_FOLEY.C

a10 starts with the Master Chief waking in a cryo tube. On Normal, in a
game of one player, the cryo technicians' tutorial plays (tutorial_setup):
the tube opens early on, and the player is asked to press the action
button, which has him climb out (the biped's exit, biped_update); the scene
fades out and back in (45 ticks), and the foley of his climb
(sound\sinomatixx_foley\a10_cryoexit_foley, 5.9 seconds) starts as it does
(tutorial_action). Its steps fall where his feet do: onto the tube's ledge
some 100 ticks into the climb (CT-driver exit, 250 frames), and the drop to
the floor some 190 ticks in. On every other difficulty, and in co-op, a
short scene stands for the tutorial (fast_setup), which starts the foley,
then opens the tube and has him climb out 45 ticks later: the foley ran 90
ticks (three seconds) ahead of him, on the Xbox too.

As a10's scripts are made ready (hs_scenario_postprocess), fast_setup's
start of the foley is moved to CRYO_EXIT_FOLEY_DELAY ticks after its
unit_exit_vehicle of player0, as on Normal: past the sleeps that follow the
climb, the one it falls in split in two (a new sleep before the foley, of
the ticks up to it, and the rest after), so that the tube opens, he climbs
out, and everything else happens, when it did. The begin is changed only
when it is as the map has it: the foley's start, the tube's unit_open right
after it, a unit_exit_vehicle later on, and after it nothing but sleeps of a
constant and ifs before the foley's place (so a map whose script differs, or
one already moved, is left as it is). The new sleep's nodes are copies of
the one split, permanent as the map's are (hs_node_gc keeps them), and each
node is named by one link. The scripts run on the host alone, which sends
the sound to the clients as it starts (network_coop.c), as before.
*/

/* ---------- headers */

#include "cseries.h"
#include "math/real_math.h"
#include "memory/data.h"
#include "hs/hs.h"
#include "hs/hs_scenario_definitions.h"
#include "scenario/scenario_definitions.h"
#include "tag_files/tag_groups.h"

#include "cryo_exit_foley.h"

#include <string.h>

/* ---------- constants */

#define A10_SCENARIO_NAME "levels\\a10\\a10"
#define FAST_SETUP_SCRIPT_NAME "fast_setup"
#define CRYO_EXIT_FOLEY_NAME "sound\\sinomatixx_foley\\a10_cryoexit_foley"

/* the ticks from the climb out to the foley on Normal (tutorial_action's
fade out and in) */
#define CRYO_EXIT_FOLEY_DELAY 45

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

/* whether the node is a start of the cryo exit foley */
static boolean script_node_starts_foley(
	struct data_array *syntax,
	char const *strings,
	long string_bytes,
	struct hs_syntax_node const *node)
{
	struct hs_syntax_node const *sound;
	char const *string;

	if (!script_node_calls(syntax, strings, string_bytes, node, "sound_looping_start"))
	{
		return FALSE;
	}
	sound = script_node_get(syntax, script_call_name_node(syntax, node)->next_node_index);
	string = sound && TEST_FLAG(sound->flags, _hs_syntax_node_primitive_bit) ?
		script_string_get(strings, string_bytes, sound->string_offset) :
		NULL;

	return string && !strcmp(string, CRYO_EXIT_FOLEY_NAME);
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
		ticks->next_node_index == NONE &&
		ticks->short_value >= 0 ?
		ticks :
		NULL;
}

/* a copy of a (sleep <ticks>) call (its call, name and ticks nodes), of the
ticks given, or NONE if the nodes have no room for it */
static long script_sleep_copy(
	struct data_array *syntax,
	char const *strings,
	long string_bytes,
	struct hs_syntax_node const *sleep,
	short ticks)
{
	long indices[3];
	struct hs_syntax_node const *originals[3];
	long node_index;

	originals[0] = sleep;
	originals[1] = script_call_name_node(syntax, sleep);
	originals[2] = script_sleep_ticks(syntax, strings, string_bytes, sleep);
	for (node_index = 0; node_index < 3; node_index++)
	{
		indices[node_index] = datum_new(syntax);
		if (indices[node_index] == NONE)
		{
			while (--node_index >= 0)
			{
				datum_delete(syntax, indices[node_index]);
			}
			return NONE;
		}
	}
	for (node_index = 0; node_index < 3; node_index++)
	{
		struct hs_syntax_node *copy = (struct hs_syntax_node *)datum_get(syntax, indices[node_index]);
		short datum_header = copy->datum_header;

		*copy = *originals[node_index];
		copy->datum_header = datum_header;
	}
	((struct hs_syntax_node *)datum_get(syntax, indices[0]))->next_node_index = NONE;
	((struct hs_syntax_node *)datum_get(syntax, indices[0]))->data = indices[1];
	((struct hs_syntax_node *)datum_get(syntax, indices[1]))->next_node_index = indices[2];
	((struct hs_syntax_node *)datum_get(syntax, indices[2]))->short_value = ticks;

	return indices[0];
}

/* ---------- public code */

boolean cryo_exit_foley_move(
	struct data_array *syntax,
	char const *strings,
	long string_bytes,
	long root_expression_index)
{
	struct hs_syntax_node *root;
	struct hs_syntax_node *previous;
	struct hs_syntax_node *foley_previous = NULL;
	struct hs_syntax_node *foley = NULL;
	struct hs_syntax_node *exit = NULL;
	struct hs_syntax_node *sleep_previous = NULL;
	struct hs_syntax_node *sleep = NULL;
	struct hs_syntax_node *sleep_ticks = NULL;
	long foley_index = NONE;
	long index;
	long walked;
	long elapsed = 0;
	short ticks_before = 0;

	if (!syntax || !syntax->data || !strings)
	{
		return FALSE;
	}
	root = script_node_get(syntax, root_expression_index);
	if (!script_node_calls(syntax, strings, string_bytes, root, "begin"))
	{
		return FALSE;
	}

	/* the begin's expressions, after its name (no more of them than there
	are nodes, should the links loop): the foley's start, the climb out, and
	the sleep the foley's new place falls in */
	previous = script_node_get(syntax, root->data);
	for (walked = 0, index = previous->next_node_index; walked < syntax->count; walked++)
	{
		struct hs_syntax_node *expression = script_node_get(syntax, index);

		if (!expression)
		{
			break;
		}
		if (!foley)
		{
			if (script_node_starts_foley(syntax, strings, string_bytes, expression))
			{
				foley_previous = previous;
				foley = expression;
				foley_index = index;
			}
		}
		else if (!exit)
		{
			if (script_node_calls(syntax, strings, string_bytes, expression, "unit_exit_vehicle"))
			{
				exit = expression;
			}
		}
		else if ((sleep_ticks = script_sleep_ticks(syntax, strings, string_bytes, expression)) != NULL)
		{
			if (elapsed + sleep_ticks->short_value >= CRYO_EXIT_FOLEY_DELAY)
			{
				sleep_previous = previous;
				sleep = expression;
				ticks_before = (short)(CRYO_EXIT_FOLEY_DELAY - elapsed);
				break;
			}
			elapsed += sleep_ticks->short_value;
		}
		/* (an if takes no time here: co-op's second player's climb out) */
		else if (!script_node_calls(syntax, strings, string_bytes, expression, "if"))
		{
			break;
		}
		previous = expression;
		index = expression->next_node_index;
	}
	if (!sleep)
	{
		return FALSE;
	}

	/* (the tube opens right after the foley starts, as the map has it) */
	if (!script_node_calls(syntax, strings, string_bytes, script_node_get(syntax, foley->next_node_index), "unit_open"))
	{
		return FALSE;
	}

	/* the foley's start taken from its place, then put after the sleep (when
	the ticks end with it) or between the sleep's two halves */
	if (ticks_before == sleep_ticks->short_value)
	{
		foley_previous->next_node_index = foley->next_node_index;
		foley->next_node_index = sleep->next_node_index;
		sleep->next_node_index = foley_index;
	}
	else
	{
		long sleep_before = script_sleep_copy(syntax, strings, string_bytes, sleep, ticks_before);

		if (sleep_before == NONE)
		{
			return FALSE;
		}
		foley_previous->next_node_index = foley->next_node_index;
		foley->next_node_index = sleep_previous->next_node_index;
		sleep_previous->next_node_index = sleep_before;
		((struct hs_syntax_node *)datum_get(syntax, sleep_before))->next_node_index = foley_index;
		sleep_ticks->short_value -= ticks_before;
	}

	return TRUE;
}

void cryo_exit_foley_scripts_loaded(
	struct scenario *scenario,
	struct data_array *syntax)
{
	char const *scenario_name = global_scenario_index != NONE ? tag_get_name(global_scenario_index) : NULL;
	long script_index;

	if (!scenario || !scenario_name || csstrcmp(scenario_name, A10_SCENARIO_NAME) ||
		!scenario->hs_scripts.address || !scenario->hs_string_constants.address)
	{
		return;
	}

	for (script_index = 0; script_index < scenario->hs_scripts.count; script_index++)
	{
		struct hs_script const *script = TAG_BLOCK_GET_ELEMENT(&scenario->hs_scripts, script_index, struct hs_script);

		if (!csstrcmp(script->name, FAST_SETUP_SCRIPT_NAME))
		{
			cryo_exit_foley_move(
				syntax,
				(char const *)scenario->hs_string_constants.address,
				scenario->hs_string_constants.size,
				script->root_expression_index);
			break;
		}
	}

	return;
}
