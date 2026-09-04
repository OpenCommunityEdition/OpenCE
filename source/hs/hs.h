/*
HS.H

header included in hcex build.
*/

#ifndef __HS_H
#define __HS_H
#pragma once

/* ---------- constants */

enum hs_type
{
	_hs_unparsed = 0,
	_hs_special_form,
	_hs_function_name,
	_hs_passthrough,
	_hs_type_void,
	_hs_type_boolean,
	_hs_type_real,
	_hs_type_short_integer,
	_hs_type_long_integer,
	_hs_type_string,
	_hs_type_script,
	_hs_type_trigger_volume,
	_hs_type_cutscene_flag,
	_hs_type_cutscene_camera_point,
	_hs_type_cutscene_title,
	_hs_type_cutscene_recording,
	_hs_type_device_group,
	_hs_type_ai,
	_hs_type_ai_command_list,
	_hs_type_starting_profile,
	_hs_type_conversation,
	_hs_type_navpoint,
	_hs_type_hud_message,
	_hs_type_object_list,
	_hs_type_sound,
	_hs_type_effect,
	_hs_type_damage,
	_hs_type_looping_sound,
	_hs_type_animation_graph,
	_hs_type_actor_variant,
	_hs_type_damage_effect,
	_hs_type_object_definition,
	_hs_type_enum_game_difficulty,
	_hs_type_enum_team,
	_hs_type_enum_ai_default_state,
	_hs_type_enum_actor_type,
	_hs_type_enum_hud_corner,
	_hs_type_object,
	_hs_type_unit,
	_hs_type_vehicle,
	_hs_type_weapon,
	_hs_type_device,
	_hs_type_scenery,
	_hs_type_object_name,
	_hs_type_unit_name,
	_hs_type_vehicle_name,
	_hs_type_weapon_name,
	_hs_type_device_name,
	_hs_type_scenery_name,
	NUMBER_OF_HS_TYPES
};

enum
{
	NUMBER_OF_HS_OBJECT_TYPES = 6
};

/* ---------- macros */

#define hs_type_valid(type) ((type)>=_hs_type_void && (type)<NUMBER_OF_HS_TYPES)
#define HS_TYPE_IS_TAG_REFERENCE(type) ((type)>=_hs_type_sound && (type)<=_hs_type_object_definition)
#define HS_TYPE_IS_ENUM(type) ((type)>=_hs_type_enum_game_difficulty && (type)<=_hs_type_enum_hud_corner)
#define HS_TYPE_IS_OBJECT_NAME(type) ((type)>=_hs_type_object_name && (type)<=_hs_type_scenery_name)
#define HS_TYPE_IS_OBJECT(type) ((type)>=_hs_type_object && (type)<=_hs_type_scenery)

/* ---------- structures */

struct hs_enum_definition
{
	short count;
	short pad;
	char const **values;
};

struct hs_function_definition;
struct hs_external_global_definition;

/* ---------- prototypes/HS.C */

void hs_dispose(
	void);

/* ---------- prototypes/HS_RUNTIME.C */

void hs_runtime_dispose(
	void);

void hs_runtime_dispose_from_old_map(
	void);
void hs_runtime_initialize(
	void);
void hs_runtime_initialize_for_new_map(
	void);
void hs_runtime_update(
	void);
long hs_runtime_evaluate(
	long expression_index);
char const *hs_runtime_get_executing_thread_name(
	void);
boolean hs_wake_by_name(
	char const *name);
void hs_evaluate_wake(
	short function_index,
	long thread_index,
	boolean initialize);
void hs_return(
	long thread_index,
	long value);
long *hs_macro_function_evaluate(
	short function_index,
	long thread_index,
	boolean initialize);

/* ---------- prototypes/HS.C */

void hs_recompile(
	void);

boolean hs_compile_and_evaluate(const char *expression);

short hs_tokens_enumerate(char const *substring, long type_flags, char const **results, short maximum_count);

boolean hs_can_cast(
	short actual_type,
	short desired_type);
struct hs_function_definition *hs_function_get(
	short function_index);
struct hs_external_global_definition *hs_global_external_get(
	short global_index);
short hs_find_function_by_name(
	char const *name);
short hs_find_script_by_name(
	char const *name);
short hs_find_global_by_name(
	char const *name);
short hs_global_get_type(
	short global_index);
char const *hs_global_get_name(
	short global_index);
boolean hs_parse(
	long expression_index,
	short expected_type);
void hs_compile(
	long source_size,
	char const *source,
	char const **error_message,
	char const **error_source);

/* ---------- globals */

extern word hs_object_type_masks[NUMBER_OF_HS_OBJECT_TYPES];
extern tag hs_tag_reference_type_group_tags[];
extern struct hs_enum_definition hs_enum_table[NUMBER_OF_HS_TYPES];
extern char const *hs_type_names[NUMBER_OF_HS_TYPES];
extern char const *hs_script_type_names[];

/* ---------- public code */

#endif // __HS_H
