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
#include <string.h>

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

struct hs_function_definition *mcc_script_function(short index)
{
    if (!mcc_cache_tags_loaded()) return NULL;
    if (index == MCC_HS_DISTANCE_TO_OBJECT) return &mcc_distance_definition.definition;
    if (index == MCC_HS_MISSION_SEGMENT) return &mcc_mission_segment_definition;
    if (index == MCC_HS_CORE_SEGMENT) return &mcc_core_segment_definition;
    return mcc_campaign_function(index);
}

short mcc_script_find(char const *name)
{
    if (!mcc_cache_tags_loaded() || !name) return NONE;
    if (!csstrcasecmp(name, "objects_distance_to_object")) return MCC_HS_DISTANCE_TO_OBJECT;
    if (!csstrcasecmp(name, "mcc_mission_segment")) return MCC_HS_MISSION_SEGMENT;
    if (!csstrcasecmp(name, "core_save_name")) return MCC_HS_CORE_SEGMENT;
    if (!csstrcasecmp(name, "player_effect_set_max_vibrate"))
        return hs_find_function_by_name("player_effect_set_max_rumble");
    return mcc_campaign_find(name);
}

boolean mcc_script_call_valid(struct hs_syntax_node const *call, struct data_array *syntax)
{
    struct hs_function_definition *function = mcc_script_function(call->function_index);
    struct hs_syntax_node *node;
    long next;
    short argument;
    if (!function || !(call->flags & 8) || (call->flags & 3) ||
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

static char const *mcc_script_string(char const *strings, uint32_t bytes, long offset)
{
    return offset >= 0 && (uint32_t)offset < bytes && memchr(strings + offset, 0, bytes - offset) ?
        strings + offset : NULL;
}

/* Give MCC's developer flag ordinary scenario storage. This preserves
 * script reads/writes and checkpoint lifetime without adding an external
 * engine global or enabling native developer privileges. */
static short mcc_developer_global(struct mcc_runtime *runtime, struct scenario *scenario,
    struct data_array *syntax)
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
        if (memchr(old[i].name,0,sizeof(old[i].name)) && !csstrcasecmp(old[i].name,"developer_mode"))
            return old[i].type==_hs_type_short_integer ? (short)i : NONE;
    }
    for (slot=0;slot<syntax->count;slot++) if (!nodes[slot].datum_header) break;
    if (slot==syntax->maximum_count) return NONE;
    globals=mcc_runtime_allocate(runtime,(uint32_t)(count+1)*sizeof(*globals));
    if (!globals) return NONE;
    if (count) memcpy(globals,old,count*sizeof(*globals));
    memset(globals+count,0,sizeof(*globals));
    memcpy(globals[count].name,"developer_mode",15);
    globals[count].type=_hs_type_short_integer;
    globals[count].initialization_expression_index=0x10000|slot;
    initializer=nodes+slot;
    memset(initializer,0,sizeof(*initializer));
    initializer->datum_header=1;initializer->flags=9;
    initializer->type=initializer->constant_type=_hs_type_short_integer;
    initializer->next_node_index=NONE;initializer->source_offset=NONE;
    if (slot>=syntax->count) syntax->count=(short)(slot+1);
    syntax->actual_count++;
    syntax->first_free_absolute_index=(short)(slot+1);
    scenario->hs_globals.address=globals;scenario->hs_globals.count=count+1;
    return (short)count;
}

static boolean mcc_disabled_skull(char const *name)
{
    return name && (!csstrcasecmp(name,"debug_ice_cream_flavor_status_i_would_have_been_your_daddy") ||
        !csstrcasecmp(name,"debug_ice_cream_flavor_status_grunt_birthday_party") ||
        !csstrcasecmp(name,"debug_ice_cream_flavor_status_bandanna"));
}

int mcc_scripts_prepare(struct mcc_runtime *runtime)
{
    uint32_t address;
    struct scenario *scenario;
    struct data_array *syntax;
    struct hs_syntax_node *nodes;
    char const *strings;
    long i, missing = 0;
    short developer=NONE;
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
            /* These status queries describe disabled, unimplemented skulls.
             * They are read-only constants, not writable legacy globals. */
            if (!csstrcasecmp(name,"set")) {
                struct hs_syntax_node *target = datum_try_and_get(syntax,nodes[child].next_node_index);
                if (target && (target->flags & 1) && mcc_disabled_skull(
                    mcc_script_string(strings,scenario->hs_string_constants.size,target->string_offset))) {
                    struct hs_syntax_node *value=datum_try_and_get(syntax,target->next_node_index);
                    /* A literal disable is already satisfied. Retain the
                     * expression's requested result type and sibling link;
                     * never discard computed values or an enable request. */
                    if (value && (value->flags&7)==1 && value->type==_hs_type_boolean &&
                        value->constant_type==_hs_type_boolean && !value->boolean_value &&
                        value->next_node_index==NONE) {
                        node->flags=9;node->constant_type=_hs_type_boolean;
                        node->data=FALSE;node->source_offset=NONE;
                        continue;
                    }
                    error(_error_silent,"mcc: scripts cannot enable unsupported skull gameplay");
                    return FALSE;
                }
            }
            if (!csstrcasecmp(name, "objects_distance_to_object")) index = MCC_HS_DISTANCE_TO_OBJECT;
            else if (!csstrcasecmp(name, "mcc_mission_segment")) index = MCC_HS_MISSION_SEGMENT;
            else if (!csstrcasecmp(name, "core_save_name")) index = MCC_HS_CORE_SEGMENT;
            else if (!csstrcasecmp(name, "player_effect_set_max_vibrate"))
                index = hs_find_function_by_name("player_effect_set_max_rumble");
            else {
                index = mcc_campaign_find(name);
                if (index == NONE) index = hs_find_function_by_name(name);
            }
            if (index != NONE) node->function_index = nodes[child].function_index = index;
        } else if ((node->flags & 5) == 5 && ((uint16_t)node->data & 0x8000)) {
            name = mcc_script_string(strings, scenario->hs_string_constants.size, node->string_offset);
            if (!name) return FALSE;
            if (mcc_disabled_skull(name)) {
                if (node->type != _hs_type_boolean) return FALSE;
                node->flags &= ~4;
                node->constant_type = _hs_type_boolean;
                node->data = FALSE;
                continue;
            }
            if (!csstrcasecmp(name,"developer_mode")) {
                if (developer==NONE) developer=mcc_developer_global(runtime,scenario,syntax);
                if (developer==NONE) {
                    error(_error_silent,"mcc: no scenario storage for developer_mode");
                    return FALSE;
                }
                node->data=developer;
                continue;
            }
            index = hs_find_global_by_name(name);
            if (index != NONE && ((uint16_t)index & 0x8000)) node->short_value = index;
            else index = NONE;
        }
        if (name && index == NONE) {
            if (missing < 16) error(_error_silent, "mcc: unsupported script function or global '%s'", name);
            ++missing;
        }
    }
    /* Retain the MCC arena, including free slots for console expressions.
     * Its separate validator owns the larger traversal workspace. */
    syntax->data = nodes;
    return missing == 0 && mcc_parameters_prepare(runtime, scenario, syntax);
}
