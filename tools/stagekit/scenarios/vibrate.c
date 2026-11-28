/*
VIBRATE.C

A startup script that calls player_effect_set_max_vibrate 0.5 0.75, as a
Custom Edition map made with Halo PC's tools has it: Halo PC's name for this
build's player_effect_set_max_rumble, the call given Halo PC's function
index, and the name in the script string data (tests/test_vibrate.py).
*/

#include "../stagekit.h"
#include "hs/hs.h"
#include "hs/hs_scenario_definitions.h"
#include "memory/data.h"

/* a syntax node's flags (hs_compile.c): a primitive, kept with the scenario */
#define NODE_PRIMITIVE 0x1
#define NODE_PERMANENT 0x8

/* StageKit's own: some index that is not this build's for the function */
#define HALO_PC_FUNCTION_INDEX 400

/* the room for the console's source after a map's strings
(hs_compile.c's HS_MAXIMUM_DYNAMIC_SOURCE_DATA_BYTES, which it keeps to
itself): the strings end that much before the string data does */
#define SCRIPT_CONSOLE_ROOM 0x400

static char const function_name[] = "player_effect_set_max_vibrate";

/* a node made as datum_new makes it; its datum index */
static long script_node(
	struct data_array *array,
	short type,
	short flags,
	long string_offset)
{
	short absolute_index = array->first_free_absolute_index;
	struct hs_syntax_node *node = (struct hs_syntax_node *)array->data + absolute_index;

	memset(node, 0, sizeof(*node));
	node->datum_header = array->next_identifier++;
	node->type = type;
	node->flags = flags;
	node->next_node_index = NONE;
	node->string_offset = string_offset;
	array->first_free_absolute_index++;
	array->actual_count++;
	array->count = absolute_index + 1;
	return ((long)(unsigned short)node->datum_header << 16) | absolute_index;
}

static struct hs_syntax_node *script_node_get(
	struct data_array *array,
	long index)
{
	return (struct hs_syntax_node *)array->data + (index & 0xFFFF);
}

static void vibrate_build(
	struct stage *stage)
{
	struct data_array *array = stage->scenario->hs_syntax_data.address;
	struct hs_script *script = cache_block(&stage->scenario->hs_scripts, 1, sizeof(*script));
	long name, left, right, call;

	stage->scenario->hs_string_constants.size = sizeof(function_name) + SCRIPT_CONSOLE_ROOM;
	stage->scenario->hs_string_constants.address = cache_allocate(stage->scenario->hs_string_constants.size);
	strcpy(stage->scenario->hs_string_constants.address, function_name);
	name = script_node(array, _hs_function_name, NODE_PRIMITIVE | NODE_PERMANENT, 0);
	left = script_node(array, _hs_type_real, NODE_PRIMITIVE | NODE_PERMANENT, NONE);
	right = script_node(array, _hs_type_real, NODE_PRIMITIVE | NODE_PERMANENT, NONE);
	call = script_node(array, _hs_type_void, NODE_PERMANENT, NONE);
	script_node_get(array, name)->function_index = HALO_PC_FUNCTION_INDEX;
	script_node_get(array, name)->next_node_index = left;
	script_node_get(array, left)->constant_type = _hs_type_real;
	script_node_get(array, left)->real_value = 0.5f;
	script_node_get(array, left)->next_node_index = right;
	script_node_get(array, right)->constant_type = _hs_type_real;
	script_node_get(array, right)->real_value = 0.75f;
	script_node_get(array, call)->function_index = HALO_PC_FUNCTION_INDEX;
	script_node_get(array, call)->data = name;

	strcpy(script->name, "vibrate");
	script->script_type = _hs_script_startup;
	script->return_type = _hs_type_void;
	script->root_expression_index = call;
}

struct stagekit_scenario const scenario_vibrate = {
	"vibrate", "a startup script calls Halo PC's player_effect_set_max_vibrate", vibrate_build };
