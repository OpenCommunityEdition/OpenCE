/*
CUSTOM_EDITION_SCRIPTS.C

Makes the compiled scripts of Halo Custom Edition maps run on this build's
script interpreter (custom_edition_cache.h).

A compiled script refers to every function it calls, and every engine
global it uses, by its index in the engine's table. Halo PC's tables aren't
this build's: Halo PC has entries this build doesn't, mixed in with the
shared ones, so past some point the same index means a different entry
(timberland.map's player_effect_start would call the function 29 places
before it, and beavercreek_halo3.yelo's switch_bsp would call playback).
Compiled scripts also keep every name in their string data, so each call
and engine global is looked up again here by name, with the game's own
hs_find_function_by_name and hs_find_global_by_name.

Anything this build doesn't have does nothing (missing_value). A call of a
missing function (OpenSauce's post-processing effects, say) becomes a
constant of the call's type, and so does a read of a missing engine global;
a set of one becomes a constant too, so nothing is written. The constant
is the type's default: nothing, false, 0, an enumeration's first value, the
empty string, or NONE for objects, tags and the scenario's lists. Only a
script index has no such default, so a map that would need one is refused.
Both builds number their value types alike: every call in the maps examined
has the same types here (docs/custom_edition_caches.md, and
tools/custom_edition_script_names.py checks a map's calls against this
build's tables).
*/

/* ---------- headers */

#include "cseries.h"
#include "errors.h"
#include "math/real_math.h"
#include "memory/data.h"
#include "tag_files/tag_groups.h"
#include "hs/hs.h"
#include "hs/hs_scenario_definitions.h"
#include "scenario/scenario_definitions.h"
#include "cache_file_formats.h"
#include "custom_edition_cache.h"

#include <string.h>

/* ---------- constants */

/* a syntax node's flags, as hs_runtime.c names them */
enum
{
	_hs_syntax_node_primitive_bit = 0,
	_hs_syntax_node_script_bit,
	_hs_syntax_node_global_bit,
	_hs_syntax_node_permanent_bit,
};

/* engine globals are told from the scenario's by this bit of their
designator (hs_find_global_by_name) */
#define HS_EXTERNAL_GLOBAL_DESIGNATOR_BIT 15

/* missing names are logged up to this many times */
#define MAXIMUM_MISSING_NAME_MESSAGES 8

/* ---------- structures */

struct scripts_conversion
{
	struct hs_syntax_node *nodes;
	long node_count;
	char const *strings;
	unsigned long string_bytes;
	/* where the empty string at the end of the string data is (an Xbox
	address, as a string constant's value is), or 0 */
	long empty_string;
	long functions_renumbered;
	long globals_renumbered;
	/* what is missing and has no harmless value: the map can't run */
	long missing_count;
	long made_inert;
};

/* ---------- private code */

/* the string at `offset` in the script string data, or NULL unless it lies
within it */
static char const *script_string_get(
	struct scripts_conversion const *conversion,
	long offset)
{
	return offset >= 0 &&
		(unsigned long)offset < conversion->string_bytes &&
		memchr(conversion->strings + offset, 0, conversion->string_bytes - offset) ?
		conversion->strings + offset :
		NULL;
}

/* the harmless value of a value type, which a call of a function this build
lacks or a read of an engine global it lacks gives instead; FALSE for a
script's index, which has none */
static boolean missing_value(
	struct scripts_conversion const *conversion,
	short type,
	long *value)
{
	if (type >= _hs_type_void && type <= _hs_type_long_integer)
		*value = 0;
	else if (type == _hs_type_string)
		*value = conversion->empty_string;
	else if (type >= _hs_type_enum_game_difficulty && type <= _hs_type_enum_hud_corner)
		*value = 0;
	else if (type != _hs_type_script && hs_type_valid(type))
		*value = NONE;
	else
		return FALSE;
	return type != _hs_type_string || conversion->empty_string;
}

/* The node (a call, or a reference to an engine global) made a constant of
its own type, for the missing function or global `name`; logged, and
counted as missing when its type has no harmless value. */
static void node_make_inert(
	struct scripts_conversion *conversion,
	struct hs_syntax_node *node,
	char const *kind,
	char const *name)
{
	long value;

	if (!missing_value(conversion, node->type, &value))
	{
		if (conversion->missing_count < MAXIMUM_MISSING_NAME_MESSAGES)
		{
			error(_error_silent, "custom edition: the scripts use the %s '%s', which this build does not have", kind,
				name);
		}
		conversion->missing_count++;
		return;
	}
	if (conversion->made_inert < MAXIMUM_MISSING_NAME_MESSAGES)
	{
		error(_error_silent, "custom edition: the scripts use the %s '%s', which this build does not have: it does nothing",
			kind, name);
	}
	/* (still permanent: hs_node_gc frees a scenario node that isn't, and the
	script then halts on it as "unused or changed") */
	node->flags = FLAG(_hs_syntax_node_primitive_bit) | (node->flags & FLAG(_hs_syntax_node_permanent_bit));
	node->constant_type = node->type;
	node->data = value;
	conversion->made_inert++;
}

/* whether the node refers to an engine global this build does not have */
static boolean engine_global_missing(
	struct scripts_conversion const *conversion,
	struct hs_syntax_node const *node)
{
	char const *name = script_string_get(conversion, node->string_offset);
	short designator;

	if (!name || !TEST_FLAG(node->flags, _hs_syntax_node_primitive_bit) ||
		!TEST_FLAG(node->flags, _hs_syntax_node_global_bit) ||
		!TEST_FLAG(node->short_value, HS_EXTERNAL_GLOBAL_DESIGNATOR_BIT))
	{
		return FALSE;
	}
	designator = hs_find_global_by_name(name);
	return designator == NONE || !TEST_FLAG(designator, HS_EXTERNAL_GLOBAL_DESIGNATOR_BIT);
}

/* A call names its function with its first child, which has the function's
index too; FALSE when the call is not laid out so. */
static boolean function_call_convert(
	struct scripts_conversion *conversion,
	struct hs_syntax_node *call)
{
	long name_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(call->data);
	struct hs_syntax_node *name_node;
	char const *name;
	short function_index;

	if (name_index >= conversion->node_count)
	{
		return FALSE;
	}
	name_node = &conversion->nodes[name_index];
	name = script_string_get(conversion, name_node->string_offset);
	if (!name || name_node->type != _hs_function_name)
	{
		return FALSE;
	}
	function_index = hs_find_function_by_name(name);
	if (function_index == NONE)
	{
		node_make_inert(conversion, call, "function", name);
	}
	/* (a set of an engine global this build lacks writes nothing: its first
	argument is the global, which the interpreter would write by its index) */
	else if (!strcmp(name, "set") && name_node->next_node_index != NONE &&
		DATUM_INDEX_TO_ABSOLUTE_INDEX(name_node->next_node_index) < conversion->node_count &&
		engine_global_missing(conversion,
			&conversion->nodes[DATUM_INDEX_TO_ABSOLUTE_INDEX(name_node->next_node_index)]))
	{
		node_make_inert(conversion, call, "engine global",
			script_string_get(conversion,
				conversion->nodes[DATUM_INDEX_TO_ABSOLUTE_INDEX(name_node->next_node_index)].string_offset));
	}
	else if (call->function_index != function_index)
	{
		call->function_index = function_index;
		name_node->function_index = function_index;
		conversion->functions_renumbered++;
	}

	return TRUE;
}

/* FALSE when the global's name is not in the string data */
static boolean engine_global_convert(
	struct scripts_conversion *conversion,
	struct hs_syntax_node *reference)
{
	char const *name = script_string_get(conversion, reference->string_offset);
	short designator;

	if (!name)
	{
		return FALSE;
	}
	designator = hs_find_global_by_name(name);
	if (designator == NONE || !TEST_FLAG(designator, HS_EXTERNAL_GLOBAL_DESIGNATOR_BIT))
	{
		node_make_inert(conversion, reference, "engine global", name);
	}
	else if (reference->short_value != designator)
	{
		reference->short_value = designator;
		conversion->globals_renumbered++;
	}

	return TRUE;
}

/* FALSE after logging why when the scenario's scripts cannot run */
static boolean scenario_scripts_convert(
	byte *tag_cache,
	unsigned long loaded_bytes,
	int32_t tag_index,
	struct scenario *scenario)
{
	struct scripts_conversion conversion = { 0 };
	uint32_t syntax_bytes = 0;
	uint32_t string_bytes = 0;
	struct data_array *syntax = custom_edition_cache_data_get(tag_cache, loaded_bytes, &scenario->hs_syntax_data, &syntax_bytes);
	long node_index;

	if (!syntax)
	{
		return TRUE;
	}
	conversion.strings = custom_edition_cache_data_get(tag_cache, loaded_bytes, &scenario->hs_string_constants, &string_bytes);
	conversion.string_bytes = conversion.strings ? string_bytes : 0;
	/* (every string in the data ends with a 0, so the last byte is one, read
	as the empty string) */
	if (conversion.string_bytes && !conversion.strings[conversion.string_bytes - 1])
		conversion.empty_string = (long)scenario->hs_string_constants.address + (long)conversion.string_bytes - 1;
	if (syntax_bytes < sizeof(*syntax) ||
		syntax->size != sizeof(struct hs_syntax_node) ||
		syntax->count < 0 ||
		syntax->count > syntax->maximum_count ||
		syntax_bytes < sizeof(*syntax) + (unsigned long)syntax->maximum_count * sizeof(struct hs_syntax_node))
	{
		error(
			_error_silent,
			"custom edition: the script syntax of '%s' is not laid out as this build's",
			custom_edition_cache_tag_name(tag_cache, loaded_bytes, tag_index));
		return FALSE;
	}
	/* the nodes follow the array (its data pointer is set there when the
	scripts are loaded, hs_scenario_postprocess), and those in use come
	first */
	conversion.nodes = (struct hs_syntax_node *)(syntax + 1);
	conversion.node_count = syntax->count;

	/* the calls first, then the engine globals: a set of a global this build
	lacks is found while its global is still a reference */
	for (node_index = 0; node_index < 2 * conversion.node_count; node_index++)
	{
		boolean globals = node_index >= conversion.node_count;
		struct hs_syntax_node *node = &conversion.nodes[node_index % conversion.node_count];
		boolean converted = TRUE;

		if (!node->datum_header)
		{
			continue;
		}
		/* (a call of one of the scenario's scripts names it by the scenario's
		own index) */
		if (!globals && !TEST_FLAG(node->flags, _hs_syntax_node_primitive_bit) &&
			!TEST_FLAG(node->flags, _hs_syntax_node_script_bit))
		{
			converted = function_call_convert(&conversion, node);
		}
		else if (globals && TEST_FLAG(node->flags, _hs_syntax_node_primitive_bit) &&
			TEST_FLAG(node->flags, _hs_syntax_node_global_bit) &&
			TEST_FLAG(node->short_value, HS_EXTERNAL_GLOBAL_DESIGNATOR_BIT))
		{
			converted = engine_global_convert(&conversion, node);
		}
		if (!converted)
		{
			error(
				_error_silent,
				"custom edition: script syntax node %ld of '%s' does not name what it uses",
				node_index % conversion.node_count,
				custom_edition_cache_tag_name(tag_cache, loaded_bytes, tag_index));
			return FALSE;
		}
	}
	error(
		_error_silent,
		"custom edition: %ld script calls and %ld engine global references were given this build's index",
		conversion.functions_renumbered,
		conversion.globals_renumbered);
	if (conversion.missing_count)
	{
		error(
			_error_silent,
			"custom edition: the scripts use %ld functions or engine globals this build does not have, giving script indices, and cannot run",
			conversion.missing_count);
		return FALSE;
	}

	return TRUE;
}

/* ---------- public code */

boolean custom_edition_scripts_convert(
	byte *tag_cache,
	unsigned long loaded_bytes)
{
	struct scenario *scenario;
	int32_t tag_index = NONE;

	while ((scenario = custom_edition_cache_tag_next(tag_cache, loaded_bytes, SCENARIO_TAG, sizeof(*scenario), &tag_index)) != NULL)
	{
		if (!scenario_scripts_convert(tag_cache, loaded_bytes, tag_index, scenario))
		{
			return FALSE;
		}
	}

	return TRUE;
}
