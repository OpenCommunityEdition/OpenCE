/* MCC script linking and MCC-only native functions. Unknown functions cause
 * a load failure; silently removing campaign logic would hide broken maps. */
#include "cseries.h"
#include "errors.h"
#include "ai/ai_script.h"
#include "hs/hs.h"
#include "hs/hs_scenario_definitions.h"
#include "hs/object_lists.h"
#include "memory/data.h"
#include "objects/objects.h"
#include "scenario/scenario_definitions.h"
#include "scenario/scenario.h"
#include "mcc_runtime.h"
#include "mcc_cache.h"
#include "mcc_scripts.h"
#include "mcc_script_parameters.h"
#include "mcc_syntax.h"
#include "mcc_campaign.h"
#include "mcc_player.h"
#include "mcc_script_library.h"
#include "mcc_skulls.h"
#include "mcc_game_speed.h"
#include <string.h>
#include <stdio.h>

boolean hs_macro_function_parse(short index, long expression);

boolean mcc_script_ai_cast(short actual_type, short desired_type)
{
    return mcc_cache_tags_loaded() && actual_type == _hs_type_ai &&
        (HS_TYPE_IS_OBJECT(desired_type) || desired_type == _hs_type_object_list);
}

int mcc_script_parse_ai(struct hs_syntax_node *node, char const *text)
{
    if (!node || (node->flags & 5) != 1 || !(node->flags & 8) ||
        !mcc_script_ai_cast(node->constant_type, node->type)) return -1;
    return text && node->source_offset >= 0 &&
        ai_index_from_string(global_scenario_get(), text + node->source_offset, &node->data);
}

long mcc_script_cast_ai(short desired_type, long reference)
{
    long list = object_list_from_ai_reference(reference), cursor, result = NONE;
    if (desired_type == _hs_type_object_list) return list;
    if (list != NONE) {
        long first = object_list_get_first(list, &cursor);
        struct object_datum *object = object_try_and_get(first);
        if (object && HS_TYPE_IS_OBJECT(desired_type) &&
            TEST_FLAG(hs_object_type_masks[desired_type - _hs_type_object], object->object.type))
            result = first;
        object_list_delete(list);
    }
    return result;
}

boolean mcc_script_can_cast(short actual_type,short desired_type)
{
    return mcc_script_ai_cast(actual_type,desired_type) || (mcc_cache_tags_loaded() &&
        actual_type==_hs_type_short_integer && desired_type==_hs_type_long_integer);
}

long mcc_script_cast(short actual_type,short desired_type,long value)
{
    /* Widen globals as well as parameters; sign extension is required. */
    if (actual_type==_hs_type_short_integer) return (long)(short)value;
    return mcc_script_cast_ai(desired_type,value);
}

static void mcc_distance_evaluate(short function, long thread, boolean initialize)
{
    long *arguments = hs_macro_function_evaluate(function, thread, initialize);
    union { real value; long bits; } result;
    long object, reference;
    real_point3d destination, origin;
    if (!arguments) return;
    result.value = -1.0f;
    if (object_try_and_get(arguments[1]) && arguments[0] != NONE &&
        datum_try_and_get(object_list_header_data, arguments[0])) {
        object_get_origin(arguments[1], &destination);
        object = object_list_get_first(arguments[0], &reference);
        while (object != NONE) {
            if (object_try_and_get(object)) {
                real distance;
                object_get_origin(object, &origin);
                distance = distance3d(&origin, &destination);
                if (result.value < 0 || distance < result.value) result.value = distance;
            }
            object = object_list_get_next(arguments[0], &reference);
        }
    }
    hs_return(thread, result.bits);
}

static struct {
    struct hs_function_definition definition;
    short second_parameter;
} mcc_distance_definition = {
    { _hs_type_real, 0, "objects_distance_to_object", hs_macro_function_parse,
        mcc_distance_evaluate, "Minimum distance to an object; -1 for no objects.",
        NULL, 2, { _hs_type_object_list } }, _hs_type_object
};

static void mcc_mission_segment_evaluate(short function, long thread, boolean initialize)
{
    long *arguments = hs_macro_function_evaluate(function, thread, initialize);
    char const *segment;
    if (!arguments) return;
    segment = (char const *)(uintptr_t)arguments[0];
    /* This port acknowledges a local segment event, without MCC's external
     * telemetry service. Keep the boolean result: shipped scripts use it in
     * conditional expressions before cinematics. MCC also uses the older
     * core_save_name spelling for segments; the Xbox function with that name
     * instead writes a debug save and must not be called by MCC map scripts.
     * Argument expressions still run normally, including any sleep/yield. */
    if (segment) error(_error_log, "mcc: mission segment '%.128s'", segment);
    hs_return(thread, segment != NULL);
}

static struct hs_function_definition mcc_mission_segment_definition = {
    _hs_type_boolean, 0, "mcc_mission_segment", hs_macro_function_parse,
    mcc_mission_segment_evaluate, "Records a local mission segment and acknowledges it.",
    NULL, 1, { _hs_type_string }
};

/* H1A repurposed this name for campaign segments with a boolean result.
 * Keep it outside the legacy table, including its original debug-save API.
 * Format reference: https://c20.reclaimers.net/h1/scripting/#functions */
static struct hs_function_definition mcc_core_segment_definition = {
    _hs_type_boolean, 0, "core_save_name", hs_macro_function_parse,
    mcc_mission_segment_evaluate, "Records a local mission segment and acknowledges it.",
    NULL, 1, { _hs_type_string }
};

static void mcc_equipment_evaluate(short function, long thread, boolean initialize)
{
    long *arguments = hs_macro_function_evaluate(function, thread, initialize);
    if (!arguments) return;
    mcc_player_add_equipment(arguments[0], (short)arguments[1], (boolean)arguments[2]);
    hs_return(thread, 0);
}

static struct {
    struct hs_function_definition definition;
    short other_parameters[2];
} mcc_equipment_definition = {
    { _hs_type_void, 0, "player_add_equipment", hs_macro_function_parse,
        mcc_equipment_evaluate, "Adds a profile, or replaces all player equipment on reset.",
        NULL, 3, { _hs_type_unit } }, { _hs_type_starting_profile, _hs_type_boolean }
};

/* MCC integer arithmetic retains the 32-bit result for LONG_MIN without
 * invoking C signed-overflow undefined behavior. */
static void mcc_abs_integer_evaluate(short function, long thread, boolean initialize)
{
    long *arguments = hs_macro_function_evaluate(function, thread, initialize);
    unsigned long value;
    if (!arguments) return;
    value = (unsigned long)arguments[0];
    if (arguments[0] < 0) value = 0u - value;
    hs_return(thread, (long)value);
}

static struct hs_function_definition mcc_abs_integer_definition = {
    _hs_type_long_integer, 0, "abs_integer", hs_macro_function_parse,
    mcc_abs_integer_evaluate, "Returns the absolute value of a 32-bit integer.",
    NULL, 1, { _hs_type_long_integer }
};

static struct hs_function_definition mcc_executing_thread_definition = {
    _hs_type_long_integer, 0, "get_executing_running_thread", hs_macro_function_parse,
    mcc_executing_thread_evaluate, "Returns the current script thread handle.",
    NULL, 0, { 0 }
};

static struct hs_function_definition mcc_kill_thread_definition = {
    _hs_type_void, 0, "kill_thread", hs_macro_function_parse,
    mcc_kill_thread_evaluate, "Terminates the specified script thread.",
    NULL, 1, { _hs_type_long_integer }
};

/* MCC scenarios may clear the debug selection. Use the existing operation;
 * the Xbox/CE map allowlist still excludes it and other debug controls. */
static void mcc_ai_deselect_evaluate(short function, long thread, boolean initialize)
{
    (void)function;
    (void)initialize;
    ai_scripting_deselect();
    hs_return(thread, 0);
}

static struct hs_function_definition mcc_ai_deselect_definition = {
    _hs_type_void, 0, "ai_deselect", hs_macro_function_parse,
    mcc_ai_deselect_evaluate, "Clears the selected AI encounter.",
    NULL, 0, { 0 }
};

struct hs_function_definition *mcc_script_function(short index)
{
    if (!mcc_cache_tags_loaded()) return NULL;
    if (index == MCC_HS_DISTANCE_TO_OBJECT) return &mcc_distance_definition.definition;
    if (index == MCC_HS_MISSION_SEGMENT) return &mcc_mission_segment_definition;
    if (index == MCC_HS_CORE_SEGMENT) return &mcc_core_segment_definition;
    if (index == MCC_HS_PLAYER_EQUIPMENT) return &mcc_equipment_definition.definition;
    if (index == MCC_HS_ABS_INTEGER) return &mcc_abs_integer_definition;
    if (index == MCC_HS_EXECUTING_THREAD) return &mcc_executing_thread_definition;
    if (index == MCC_HS_KILL_THREAD) return &mcc_kill_thread_definition;
    if (index == MCC_HS_AI_DESELECT) return &mcc_ai_deselect_definition;
    if (index >= MCC_HS_LIBRARY_FIRST) return mcc_script_library_function(index);
    return mcc_campaign_function(index);
}

/* Preparation runs before MCC activation; runtime lookup stays guarded below.
 * A recognized alias returns its target result even when that target is absent. */
static short mcc_script_resolve(char const *name, boolean native_fallback)
{
    short index;
    if (!csstrcasecmp(name, "player_add_equipment")) return MCC_HS_PLAYER_EQUIPMENT;
    if (!csstrcasecmp(name, "objects_distance_to_object")) return MCC_HS_DISTANCE_TO_OBJECT;
    if (!csstrcasecmp(name, "mcc_mission_segment")) return MCC_HS_MISSION_SEGMENT;
    if (!csstrcasecmp(name, "core_save_name")) return MCC_HS_CORE_SEGMENT;
    if (!csstrcasecmp(name, "player_effect_set_max_vibrate"))
        return hs_find_function_by_name("player_effect_set_max_rumble");
    if (!csstrcasecmp(name, "abs_integer")) return MCC_HS_ABS_INTEGER;
    if (!csstrcasecmp(name, "get_executing_running_thread")) return MCC_HS_EXECUTING_THREAD;
    if (!csstrcasecmp(name, "kill_thread")) return MCC_HS_KILL_THREAD;
    if (!csstrcasecmp(name, "ai_deselect")) return MCC_HS_AI_DESELECT;
    index = mcc_campaign_find(name);
    if (index == NONE) index = mcc_script_library_find(name);
    return index == NONE && native_fallback ? hs_find_function_by_name(name) : index;
}

short mcc_script_find(char const *name)
{
    return mcc_cache_tags_loaded() && name ? mcc_script_resolve(name, FALSE) : NONE;
}

boolean mcc_script_call_arguments_valid(struct hs_syntax_node const *call, struct data_array *syntax)
{
    struct hs_function_definition *function = mcc_script_function(call->function_index);
    struct hs_syntax_node *node;
    long next;
    short argument;
    if (!function || (call->flags & 3) ||
        !(node = datum_try_and_get(syntax, call->data))) return FALSE;
    next = node->next_node_index;
    if (call->function_index == MCC_HS_CAMPAIGN_FIRST + 2) {
        if (next == NONE) return TRUE;
        node = datum_try_and_get(syntax, next);
        return node && node->type == _hs_type_script && node->next_node_index == NONE;
    }
    for (argument = 0; argument < function->parameter_count; ++argument) {
        node = datum_try_and_get(syntax, next);
        if (!node || node->type != function->parameter_types[argument]) return FALSE;
        next = node->next_node_index;
    }
    return next == NONE;
}

/* Map admission still requires authored nodes. Saved console expressions use
 * the same argument grammar while their temporary nodes have another owner. */
boolean mcc_script_call_valid(struct hs_syntax_node const *call, struct data_array *syntax)
{
    return (call->flags & 8) && mcc_script_call_arguments_valid(call, syntax);
}

static char const *mcc_script_string(char const *strings, uint32_t bytes, long offset)
{
    return offset >= 0 && (uint32_t)offset < bytes && memchr(strings + offset, 0, bytes - offset) ?
        strings + offset : NULL;
}

/* Give MCC compatibility variables ordinary scenario storage. This preserves
 * script reads/writes and checkpoint lifetime without adding an external
 * engine global or enabling native developer privileges. */
static short mcc_local_global(struct mcc_runtime *runtime, struct scenario *scenario,
    struct data_array *syntax, char const *name, short type)
{
    extern short const hs_external_global_count;
    struct hs_global *old,*globals;
    struct hs_syntax_node *nodes=(struct hs_syntax_node *)(syntax+1),*initializer;
    long count=scenario->hs_globals.count,i,slot;
    if (count<0 || count>=1024-hs_external_global_count) return NONE;
    old=mcc_runtime_pointer(runtime,(uint32_t)(uintptr_t)scenario->hs_globals.address,
        (uint32_t)count*sizeof(*old));
    if (count && !old) return NONE;
    for (i=0;i<count;i++) {
        if (memchr(old[i].name,0,sizeof(old[i].name)) && !csstrcasecmp(old[i].name,name))
            return old[i].type==type ? (short)i : NONE;
    }
    for (slot=0;slot<syntax->count;slot++) if (!nodes[slot].datum_header) break;
    if (slot==syntax->maximum_count) return NONE;
    globals=mcc_runtime_allocate(runtime,(uint32_t)(count+1)*sizeof(*globals));
    if (!globals) return NONE;
    if (count) memcpy(globals,old,count*sizeof(*globals));
    memset(globals+count,0,sizeof(*globals));
    strcpy(globals[count].name,name);
    globals[count].type=type;
    globals[count].initialization_expression_index=0x10000|slot;
    initializer=nodes+slot;
    memset(initializer,0,sizeof(*initializer));
    initializer->datum_header=1;initializer->flags=9;
    initializer->type=initializer->constant_type=type;
    if (!csstrcasecmp(name,"game_speed_value")) initializer->data=0x3f800000;
    initializer->next_node_index=NONE;initializer->source_offset=NONE;
    if (slot>=syntax->count) syntax->count=(short)(slot+1);
    syntax->actual_count++;
    syntax->first_free_absolute_index=(short)(slot+1);
    scenario->hs_globals.address=globals;scenario->hs_globals.count=count+1;
    return (short)count;
}

/* MCC external globals the Xbox engine lacks, and the f0-f5 debug floats
 * whose native renderer controls a map must not reach. Each is given the
 * scenario storage above, starting at zero (game_speed_value at one): script
 * reads, writes and checkpoints work. The engine reads the skulls'
 * (mcc_skulls.c) and game_paused (mcc_game_speed.c); nothing reads the
 * others. The list is MCC's editing kit's (hs_doc.txt). A scenario global's
 * name has at most 31 characters, so longer names, and f0-f5 (which the
 * native table also has), use an internal name appended to the script
 * strings: the native recompiler resolves the token by name again. Bandanna
 * is the existing infinite ammunition and grenades control (an external
 * global). */
#define MCC_LOCAL_NAME_BYTES 32
static struct {
    char const *name, *internal;
    short type;
    boolean external;
} const mcc_local_globals[] = {
    {"f0","__mcc_scratch_f0",_hs_type_real,FALSE},
    {"f1","__mcc_scratch_f1",_hs_type_real,FALSE},
    {"f2","__mcc_scratch_f2",_hs_type_real,FALSE},
    {"f3","__mcc_scratch_f3",_hs_type_real,FALSE},
    {"f4","__mcc_scratch_f4",_hs_type_real,FALSE},
    {"f5","__mcc_scratch_f5",_hs_type_real,FALSE},
    {"game_speed_value",NULL,_hs_type_real,FALSE},
    /* Debug score is a map-local long, not a multiplayer score or authority control. */
    {"debug_score",NULL,_hs_type_long_integer,FALSE},
    /* Not the engine's developer privileges. */
    {"developer_mode",NULL,_hs_type_short_integer,FALSE},
    {"debug_ice_cream_flavor_status_bandanna","cheat_infinite_ammo",_hs_type_boolean,TRUE},
    {"debug_ice_cream_flavor_status_grunt_birthday_party","__mcc_skull_birthday_party",_hs_type_boolean,FALSE},
    {"debug_ice_cream_flavor_status_assassins","__mcc_skull_assassins",_hs_type_boolean,FALSE},
    {"debug_ice_cream_flavor_status_famine","__mcc_skull_famine",_hs_type_boolean,FALSE},
    {"debug_ice_cream_flavor_status_i_would_have_been_your_daddy","__mcc_skull_iwhbyd",_hs_type_boolean,FALSE},
    {"debug_ice_cream_flavor_status_blind","__mcc_skull_blind",_hs_type_boolean,FALSE},
    {"debug_ice_cream_flavor_status_ghost","__mcc_skull_ghost",_hs_type_boolean,FALSE},
    {"debug_ice_cream_flavor_status_black_eye","__mcc_skull_black_eye",_hs_type_boolean,FALSE},
    {"debug_ice_cream_flavor_status_catch","__mcc_skull_catch",_hs_type_boolean,FALSE},
    {"debug_ice_cream_flavor_status_sputnik","__mcc_skull_sputnik",_hs_type_boolean,FALSE},
    {"debug_ice_cream_flavor_status_mythic","__mcc_skull_mythic",_hs_type_boolean,FALSE},
    {"debug_ice_cream_flavor_status_anger","__mcc_skull_anger",_hs_type_boolean,FALSE},
    {"debug_ice_cream_flavor_status_thunderstorm","__mcc_skull_thunderstorm",_hs_type_boolean,FALSE},
    {"debug_ice_cream_flavor_status_iron","__mcc_skull_iron",_hs_type_boolean,FALSE},
    {"debug_ice_cream_flavor_status_thats_just_wrong","__mcc_skull_thats_just_wrong",_hs_type_boolean,FALSE},
    {"debug_ice_cream_flavor_status_boom","__mcc_skull_boom",_hs_type_boolean,FALSE},
    {"debug_ice_cream_flavor_status_eye_patch","__mcc_skull_eye_patch",_hs_type_boolean,FALSE},
    {"debug_ice_cream_flavor_status_fog","__mcc_skull_fog",_hs_type_boolean,FALSE},
    {"debug_ice_cream_flavor_status_foreign","__mcc_skull_foreign",_hs_type_boolean,FALSE},
    {"debug_ice_cream_flavor_status_grunt_funeral","__mcc_skull_grunt_funeral",_hs_type_boolean,FALSE},
    {"debug_ice_cream_flavor_status_pinata","__mcc_skull_pinata",_hs_type_boolean,FALSE},
    {"debug_ice_cream_flavor_status_recession","__mcc_skull_recession",_hs_type_boolean,FALSE},
    {"debug_ice_cream_flavor_status_malfunction","__mcc_skull_malfunction",_hs_type_boolean,FALSE},
    {"debug_ice_cream_flavor_status_streaking","__mcc_skull_streaking",_hs_type_boolean,FALSE},
    {"debug_ice_cream_flavor_status_styx","__mcc_skull_styx",_hs_type_boolean,FALSE},
    {"debug_ice_cream_flavor_status_feather","__mcc_skull_feather",_hs_type_boolean,FALSE},
    {"debug_ice_cream_flavor_status_bonded_pair","__mcc_skull_bonded_pair",_hs_type_boolean,FALSE},
    {"debug_ice_cream_flavor_status_masterblaster","__mcc_skull_masterblaster",_hs_type_boolean,FALSE},
    {"debug_ice_cream_flavor_force_coop","__mcc_skull_force_coop",_hs_type_boolean,FALSE},
    /* Debug and settings switches with no effect here. */
    {"rasterizer_debug_shader_transparent_generic","__mcc_debug_transparent_generic",_hs_type_boolean,FALSE},
    {"rasterizer_effects_level",NULL,_hs_type_short_integer,FALSE},
    {"rasterizer_fps",NULL,_hs_type_boolean,FALSE},
    {"debug_sound_cache_graph",NULL,_hs_type_boolean,FALSE},
    {"sound_obstruction_ratio",NULL,_hs_type_real,FALSE},
    {"debug_sound_channels_detail",NULL,_hs_type_boolean,FALSE},
    {"debug_sound_hardware",NULL,_hs_type_boolean,FALSE},
    {"hs_verbose",NULL,_hs_type_boolean,FALSE},
    {"breakpoints_enabled",NULL,_hs_type_boolean,FALSE},
    {"debug_structure_automatic",NULL,_hs_type_boolean,FALSE},
    {"director_camera_switching",NULL,_hs_type_boolean,FALSE},
    {"slow_server_startup_safety_zone_in_seconds","__mcc_slow_server_startup",_hs_type_long_integer,FALSE},
    {"error_suppress_all",NULL,_hs_type_boolean,FALSE},
    {"mouse_acceleration",NULL,_hs_type_real,FALSE},
    /* MCC's client prediction and its breadcrumbs override; both start
     * false in MCC. The port's netcode predicts for itself, and its
     * breadcrumbs are always available (breadcrumbs_nav_points_active). */
    {"object_prediction",NULL,_hs_type_boolean,FALSE},
    {"breadcrumbs_navpoints_enabled_override","__mcc_breadcrumbs_override",_hs_type_boolean,FALSE},
    {"game_paused",NULL,_hs_type_boolean,FALSE}
};

static short mcc_local_global_find(char const *name)
{
    short i;
    for (i=0;i<(short)NUMBEROF(mcc_local_globals);i++)
        if (!csstrcasecmp(name,mcc_local_globals[i].name)) return i;
    return NONE;
}

/* Append every internal name once, in fixed slots. The final 1 KiB of the
 * string data belongs to console source, not map strings, and stays last. */
static long mcc_local_global_strings(struct mcc_runtime *runtime, struct scenario *scenario,
    char const **strings)
{
    long base;
    short i;
    char *expanded;
    unsigned long bytes=NUMBEROF(mcc_local_globals)*MCC_LOCAL_NAME_BYTES;
    if (scenario->hs_string_constants.size<1024) return NONE;
    expanded=mcc_runtime_allocate(runtime,(uint32_t)(scenario->hs_string_constants.size+bytes));
    if (!expanded) return NONE;
    base=scenario->hs_string_constants.size-1024;
    memcpy(expanded,*strings,(unsigned long)base);
    memset(expanded+base,0,bytes+1024);
    for (i=0;i<(short)NUMBEROF(mcc_local_globals);i++)
        if (mcc_local_globals[i].internal)
            strcpy(expanded+base+i*MCC_LOCAL_NAME_BYTES,mcc_local_globals[i].internal);
    *strings=expanded;
    scenario->hs_string_constants.address=expanded;
    scenario->hs_string_constants.size+=bytes;
    return base;
}

int mcc_scripts_prepare(struct mcc_runtime *runtime)
{
    uint32_t address;
    struct scenario *scenario;
    struct data_array *syntax;
    struct hs_syntax_node *nodes;
    char const *strings;
    long i, missing = 0;
    short locals[NUMBEROF(mcc_local_globals)];
    long local_strings=NONE;
    for (i=0;i<(long)NUMBEROF(locals);i++) locals[i]=NONE;
    mcc_skulls_reset();
    mcc_game_paused_bind(NONE);
    mcc_script_library_reset();
    memcpy(&address, runtime->tag_index + (runtime->report.scenario_handle & 0xFFFFu) * 32 + 20, 4);
    scenario = mcc_runtime_pointer(runtime, address, sizeof(*scenario));
    if (!scenario) return FALSE;
    if (!scenario->hs_syntax_data.size) return runtime->report.parameterized_script_count == 0;
    syntax = mcc_runtime_pointer(runtime, (uint32_t)(uintptr_t)scenario->hs_syntax_data.address,
        (uint32_t)scenario->hs_syntax_data.size);
    strings = mcc_runtime_pointer(runtime, (uint32_t)(uintptr_t)scenario->hs_string_constants.address,
        (uint32_t)scenario->hs_string_constants.size);
    if (!syntax || !strings || !mcc_syntax_graph_valid(syntax, scenario->hs_syntax_data.size)) {
        error(_error_silent, "mcc: invalid script syntax header or graph (MCC capacity: %d nodes)", MCC_SYNTAX_CAPACITY);
        return FALSE;
    }
    nodes = (struct hs_syntax_node *)(syntax + 1);
    syntax->data = nodes;
    for (i = 0; i < syntax->count; ++i) {
        struct hs_syntax_node *node = &nodes[i];
        char const *name = NULL;
        short index = NONE;
        if (!node->datum_header) continue;
        if (!(node->flags & 3)) {
            uint32_t child = (uint32_t)node->data & 0xFFFFu;
            if (child >= (uint32_t)syntax->count || !nodes[child].datum_header ||
                (uint16_t)nodes[child].datum_header != (uint16_t)((uint32_t)node->data >> 16) ||
                nodes[child].type != _hs_function_name)
                return FALSE;
            name = mcc_script_string(strings, scenario->hs_string_constants.size, nodes[child].string_offset);
            if (!name) return FALSE;
            index = mcc_script_resolve(name, TRUE);
            if (index != NONE) node->function_index = nodes[child].function_index = index;
        } else if ((node->flags & 5) == 5 && ((uint16_t)node->data & 0x8000)) {
            name = mcc_script_string(strings, scenario->hs_string_constants.size, node->string_offset);
            if (!name) return FALSE;
            index = mcc_local_global_find(name);
            if (index != NONE) {
                short local = index;
                char const *global_name = mcc_local_globals[local].internal ?
                    mcc_local_globals[local].internal : mcc_local_globals[local].name;
                if (mcc_local_globals[local].internal && local_strings == NONE &&
                    (local_strings = mcc_local_global_strings(runtime, scenario, &strings)) == NONE) return FALSE;
                if (locals[local] == NONE) locals[local] = mcc_local_globals[local].external ?
                    hs_find_global_by_name(global_name) :
                    mcc_local_global(runtime, scenario, syntax, global_name, mcc_local_globals[local].type);
                /* The recompiler checks each use's cast from the stored type;
                 * an aliased engine global must have MCC's type. */
                if (locals[local] == NONE ||
                    (mcc_local_globals[local].external && (!((uint16_t)locals[local] & 0x8000) ||
                        hs_global_external_get((short)(locals[local] & 0x7fff))->type !=
                            mcc_local_globals[local].type))) {
                    error(_error_silent, "mcc: no scenario storage for %s", name);
                    return FALSE;
                }
                if (mcc_local_globals[local].external) node->short_value = locals[local];
                else {
                    node->data = locals[local];
                    mcc_skulls_bind(mcc_skull_find(name), locals[local]);
                }
                if (mcc_local_globals[local].internal)
                    node->string_offset = local_strings + local * MCC_LOCAL_NAME_BYTES;
                continue;
            }
            index = hs_find_global_by_name(name);
            if (index != NONE && ((uint16_t)index & 0x8000)) node->short_value = index;
            else index = NONE;
        }
        if (name && index == NONE) {
            if (!runtime->failure[0]) snprintf(runtime->failure,sizeof(runtime->failure),
                "Unsupported script name: %.24s. See debug.txt.",name);
            if (missing < 16) error(_error_silent, "mcc: unsupported script function or global '%s'", name);
            ++missing;
        }
    }
    /* Every skull, and game_paused, has storage, whether or not the map's
     * scripts name it, so the console can set any of them
     * (mcc_script_find_global). */
    for (i = 0; i < (long)NUMBEROF(mcc_local_globals); i++) {
        short skull = mcc_skull_find(mcc_local_globals[i].name);
        boolean paused = !csstrcasecmp(mcc_local_globals[i].name, "game_paused");
        if ((skull == NONE && !paused) || mcc_local_globals[i].external) continue;
        if (locals[i] == NONE) locals[i] = mcc_local_global(runtime, scenario, syntax,
            mcc_local_globals[i].internal ? mcc_local_globals[i].internal : mcc_local_globals[i].name,
            mcc_local_globals[i].type);
        if (locals[i] == NONE) error(_error_silent, "mcc: no scenario storage for %s", mcc_local_globals[i].name);
        else if (paused) mcc_game_paused_bind(locals[i]);
        else mcc_skulls_bind(skull, locals[i]);
    }
    /* Retain the MCC arena, including free slots for console expressions.
     * Its separate validator owns the larger traversal workspace. */
    syntax->data = nodes;
    return missing == 0 && mcc_parameters_prepare(runtime, scenario, syntax);
}

/* The console's names for MCC's map-local globals: MCC's own, for those
 * kept under an internal name (an external one's, for Bandanna). NONE when
 * the map has no storage for it, as for f0-f5 when its scripts do not use
 * them (the console then reaches the native ones). */
short mcc_script_find_global(char const *name)
{
    short local;
    if (!mcc_cache_tags_loaded() || !name || (local = mcc_local_global_find(name)) == NONE ||
        !mcc_local_globals[local].internal) return NONE;
    return hs_find_global_by_name(mcc_local_globals[local].internal);
}

/* The console's completions: MCC's functions, or its names for the globals
 * this map has (the scenario's own names are listed already). */
void mcc_script_enumerate(boolean functions, void (*add)(char const *name))
{
    short index;
    struct hs_function_definition *function;
    if (!mcc_cache_tags_loaded()) return;
    if (functions) {
        for (index = MCC_HS_DISTANCE_TO_OBJECT; index <= MCC_HS_AI_DESELECT; index++)
            if ((function = mcc_script_function(index)) != NULL) add(function->name);
        for (index = MCC_HS_CAMPAIGN_FIRST; (function = mcc_script_function(index)) != NULL; index++)
            add(function->name);
        for (index = MCC_HS_LIBRARY_FIRST; (function = mcc_script_function(index)) != NULL; index++)
            add(function->name);
        return;
    }
    for (index = 0; index < (short)NUMBEROF(mcc_local_globals); index++)
        if (mcc_local_globals[index].internal && mcc_script_find_global(mcc_local_globals[index].name) != NONE)
            add(mcc_local_globals[index].name);
}
