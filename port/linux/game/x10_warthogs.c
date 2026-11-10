/*
X10_WARTHOGS.C

a10's opening cinematic (x10) ends in the hangar, with Sergeant Johnson's
speech to his marines (hangar_2). Its words depend on the difficulty: four
ifs, one for each, end with his "Am I right, marines?". After them, as the
marines answer, hangar_2 wakes x10_hog_4, whose warthog (x10_warthog_2,
hog_2_driver at the wheel) drives along the hangar's north side, behind him
as he says "Damn right I am!". Three things were wrong with it, on the Xbox
too:

- Legendary's if wakes x10_hog_4 as well, before Johnson's last line on
  that difficulty. The wake after the ifs then ends the script's sleep (a
  wake of a sleeping script ends its sleep: hs_wake), and the script
  destroys the warthog 57 ticks into its run, before any shot shows it: on
  Legendary there was no warthog.
- The warthog's run (the recording x10_hog_4_run, 307 ticks) holds the
  throttle down from tick 9 to tick 300. It leaves the shot some 90 ticks
  in, then drives on to the hangar's west end (some 150 ticks in), turns
  south there and runs on into the far wall at full speed, where its crash
  sound plays (create_crashing_effects, vehicles.c) while Johnson walks
  among his marines; only at the run's end did the script destroy it.
- Both of the scene's warthog drivers (hog_2_driver, and hog_3_driver of
  x10_hog_5's warthog at "Today's your lucky day") are placed with
  Johnson's variant (JOHNSON_VARIANT: the marine model's sgt_johnson-100
  head and arms, which no random choice picks), so two more of him drove
  past him.

As a10's scripts are made ready (hs_scenario_postprocess), Legendary's own
wake of x10_hog_4 is taken out of its if's begin, so the script is woken
once, after the ifs, as on every other difficulty; x10_hog_4's sleep for
the rest of the run, (sleep (recording_time x10_warthog_2)), becomes a
sleep of X10_HOG_RUN_TICKS, so the warthog and its driver are destroyed out
of every shot and short of the hangar's west end; and the two drivers are
placed with no variant, so each gets a marine's head and arms at random, as
the other marines of the scene do (they also drive x10_hog_2's and
x10_hog_3's warthogs, earlier in the hangar). Each is changed only when it
is as the map has it, so a map whose scripts differ, or one already changed
(the postprocess runs again on a revert), is left as it is. The sleep's
ticks take the place of its recording_time call in the call's own node, a
constant now, as the map's are; the call's name and argument are named by
no link. The scripts run on the host alone, which sends what they make and
destroy, and each object's head and arms (its region permutations), to the
clients (network_coop.c), so nothing the machines send or agree on changes.
*/

/* ---------- headers */

#include "cseries.h"
#include "math/real_math.h"
#include "memory/data.h"
#include "hs/hs.h"
#include "hs/hs_scenario_definitions.h"
#include "objects/object_types.h"
#include "scenario/scenario_definitions.h"
#include "tag_files/tag_groups.h"

#include "x10_warthogs.h"

#include <string.h>

/* ---------- constants */

#define A10_SCENARIO_NAME "levels\\a10\\a10"
#define HANGAR_SCRIPT_NAME "hangar_2"
#define HOG_SCRIPT_NAME "x10_hog_4"
#define HOG_VEHICLE_NAME "x10_warthog_2"

/* the ticks into its run that the warthog is destroyed: past the shot it
crosses (it leaves it some 90 ticks in), short of the hangar's west end
(some 150 ticks in) */
#define X10_HOG_RUN_TICKS 120

/* the marine model's variant for Sergeant Johnson's head and arms (its
sgt_johnson-100 permutations) */
#define JOHNSON_VARIANT 100

/* a syntax node's flags, as hs_runtime.c names them */
enum
{
	_hs_syntax_node_primitive_bit = 0,
	_hs_syntax_node_script_bit,
	_hs_syntax_node_variable_bit,
};

/* ---------- globals */

extern long global_scenario_index;

/* the scene's warthog drivers */
static char const *const x10_warthog_driver_names[] =
{
	"hog_2_driver",
	"hog_3_driver",
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

/* whether the node is a call of the function of that name whose first
argument is a primitive of the name given (a script's, or an object's) */
static boolean script_node_calls_with(
	struct data_array *syntax,
	char const *strings,
	long string_bytes,
	struct hs_syntax_node const *node,
	char const *function_name,
	char const *argument_name)
{
	struct hs_syntax_node const *argument;
	char const *string;

	if (!script_node_calls(syntax, strings, string_bytes, node, function_name))
	{
		return FALSE;
	}
	argument = script_node_get(syntax, script_call_name_node(syntax, node)->next_node_index);
	string = argument && TEST_FLAG(argument->flags, _hs_syntax_node_primitive_bit) ?
		script_string_get(strings, string_bytes, argument->string_offset) :
		NULL;

	return string && !strcmp(string, argument_name);
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

/* ---------- public code */

boolean x10_warthogs_wake_once(
	struct data_array *syntax,
	char const *strings,
	long string_bytes,
	long root_expression_index)
{
	struct hs_syntax_node *name = script_begin_name_node(syntax, strings, string_bytes, root_expression_index);
	struct hs_syntax_node *expression;
	long walked;

	if (!name)
	{
		return FALSE;
	}

	/* the begin's expressions (no more of them than there are nodes, should
	the links loop): an if, and right after it the warthog's wake */
	expression = script_node_get(syntax, name->next_node_index);
	for (walked = 0; expression && walked < syntax->count; walked++)
	{
		struct hs_syntax_node *next = script_node_get(syntax, expression->next_node_index);

		if (script_node_calls(syntax, strings, string_bytes, expression, "if") &&
			script_node_calls_with(syntax, strings, string_bytes, next, "wake", HOG_SCRIPT_NAME))
		{
			/* the if's condition, then the begin it runs */
			struct hs_syntax_node const *condition = script_node_get(syntax,
				script_call_name_node(syntax, expression)->next_node_index);
			struct hs_syntax_node *branch = condition ? script_node_get(syntax, condition->next_node_index) : NULL;
			struct hs_syntax_node *previous = script_node_calls(syntax, strings, string_bytes, branch, "begin") ?
				script_call_name_node(syntax, branch) :
				NULL;
			struct hs_syntax_node *inner = previous ? script_node_get(syntax, previous->next_node_index) : NULL;
			long inner_walked;

			for (inner_walked = 0; inner && inner_walked < syntax->count; inner_walked++)
			{
				if (script_node_calls_with(syntax, strings, string_bytes, inner, "wake", HOG_SCRIPT_NAME))
				{
					/* (taken out of the begin) */
					previous->next_node_index = inner->next_node_index;
					return TRUE;
				}
				previous = inner;
				inner = script_node_get(syntax, inner->next_node_index);
			}
		}
		expression = next;
	}

	return FALSE;
}

boolean x10_warthogs_run_shortened(
	struct data_array *syntax,
	char const *strings,
	long string_bytes,
	long root_expression_index)
{
	struct hs_syntax_node *name = script_begin_name_node(syntax, strings, string_bytes, root_expression_index);
	struct hs_syntax_node *expression;
	boolean played = FALSE;
	long walked;

	if (!name)
	{
		return FALSE;
	}

	/* the begin's expressions: the run's start, then the sleep for the rest
	of it */
	expression = script_node_get(syntax, name->next_node_index);
	for (walked = 0; expression && walked < syntax->count; walked++)
	{
		if (script_node_calls_with(syntax, strings, string_bytes, expression, "recording_play", HOG_VEHICLE_NAME))
		{
			played = TRUE;
		}
		else if (played && script_node_calls(syntax, strings, string_bytes, expression, "sleep"))
		{
			struct hs_syntax_node *ticks = script_node_get(syntax,
				script_call_name_node(syntax, expression)->next_node_index);

			if (!script_node_calls_with(syntax, strings, string_bytes, ticks, "recording_time", HOG_VEHICLE_NAME) ||
				ticks->type != _hs_type_short_integer ||
				ticks->next_node_index != NONE)
			{
				return FALSE;
			}
			/* (the call made a constant of its type) */
			ticks->constant_type = _hs_type_short_integer;
			SET_FLAG(ticks->flags, _hs_syntax_node_primitive_bit, TRUE);
			ticks->data = 0;
			ticks->short_value = X10_HOG_RUN_TICKS;
			return TRUE;
		}
		expression = script_node_get(syntax, expression->next_node_index);
	}

	return FALSE;
}

boolean x10_warthogs_driver_placed(
	char const *name,
	struct scenario_object_datum *object)
{
	short driver_index;

	if (!name || !object || object->variant_number != JOHNSON_VARIANT)
	{
		return FALSE;
	}
	for (driver_index = 0; driver_index < (short)NUMBEROF(x10_warthog_driver_names); driver_index++)
	{
		if (!strcmp(name, x10_warthog_driver_names[driver_index]))
		{
			/* (none: object_choose_random_region_permutations) */
			object->variant_number = 0;
			return TRUE;
		}
	}

	return FALSE;
}

void x10_warthogs_scripts_loaded(
	struct scenario *scenario,
	struct data_array *syntax)
{
	char const *scenario_name = global_scenario_index != NONE ? tag_get_name(global_scenario_index) : NULL;
	struct tag_block *bipeds;
	long element_size;
	long index;

	if (!scenario || !scenario_name || csstrcmp(scenario_name, A10_SCENARIO_NAME))
	{
		return;
	}

	if (scenario->hs_scripts.address && scenario->hs_string_constants.address)
	{
		for (index = 0; index < scenario->hs_scripts.count; index++)
		{
			struct hs_script const *script = TAG_BLOCK_GET_ELEMENT(&scenario->hs_scripts, index, struct hs_script);

			if (!csstrcmp(script->name, HANGAR_SCRIPT_NAME))
			{
				x10_warthogs_wake_once(
					syntax,
					(char const *)scenario->hs_string_constants.address,
					scenario->hs_string_constants.size,
					script->root_expression_index);
			}
			else if (!csstrcmp(script->name, HOG_SCRIPT_NAME))
			{
				x10_warthogs_run_shortened(
					syntax,
					(char const *)scenario->hs_string_constants.address,
					scenario->hs_string_constants.size,
					script->root_expression_index);
			}
		}
	}

	bipeds = scenario_get_object_type_scenario_datums(scenario, _object_type_biped, &element_size);
	if (bipeds->address && scenario->object_names.address)
	{
		for (index = 0; index < bipeds->count; index++)
		{
			struct scenario_object_datum *object = (struct scenario_object_datum *)tag_block_get_element_with_size(
				bipeds,
				index,
				element_size);

			if (VALID_INDEX(object->name_index, scenario->object_names.count))
			{
				x10_warthogs_driver_placed(
					TAG_BLOCK_GET_ELEMENT(&scenario->object_names, object->name_index, struct scenario_object_name)->name,
					object);
			}
		}
	}

	return;
}
