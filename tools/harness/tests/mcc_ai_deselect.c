/* Native selection state and script policy, using source-extracted functions. */
#include "harness.h"
#include <ctype.h>
#define NUMBEROF(a) (sizeof(a)/sizeof((a)[0]))
#define csmemset memset
#define csstrcmp strcmp
#include "types.inc"

struct ai_debug_actor_record { boolean valid; long sentinel; };
static struct {
    long selected_squad_index, selected_actor_index;
    boolean evaluation_context_valid;
    byte evaluation_context[32];
    struct ai_debug_actor_record actor_record[NUMBER_OF_AI_DEBUG_ACTOR_RECORDS];
    boolean idle_look_valid;
    long idle_look_unit_index;
    short idle_look_prop_count;
    long unrelated;
} ai_debug, before;
static struct { boolean ai_initialized_for_map; } ai_state;
#define ai_globals (&ai_state)
static void ai_debug_select_actor(long encounter, long actor);
static long returned, returned_thread;
static void hs_return(long thread, long value) { returned_thread=thread;returned=value; }

/* The native policy is compiled below; only its unrelated services are mocked. */
struct hs_function_definition {
    char const *name;
    boolean (*parse)(short, long);
    short parameter_count, parameter_types[1];
};
static boolean hs_macro_function_parse(short index, long expression) { return TRUE; }
static struct hs_function_definition native_select={"ai_select", hs_macro_function_parse, 1, {_hs_type_ai}};
static struct hs_function_definition native_deselect={"ai_deselect", hs_macro_function_parse, 0, {0}};
static struct hs_function_definition *hs_function_get(short index) {
    return index==NATIVE_AI_SELECT ? &native_select : &native_deselect;
}
static struct hs_function_definition *mcc_script_function(short index) { return NULL; }
static boolean mcc_script_call_valid(struct hs_syntax_node const *call, struct data_array *syntax) { return FALSE; }
static boolean mcc_script_call_arguments_valid(struct hs_syntax_node const *call, struct data_array *syntax) { return FALSE; }
static struct data_array syntax_data;
static struct data_array *hs_syntax_data=&syntax_data;
static struct hs_syntax_node nodes[3];
static void *datum_try_and_get(struct data_array *array, long handle) {
    long index=DATUM_INDEX_TO_ABSOLUTE_INDEX(handle);
    return handle!=NONE && index<array->count && nodes[index].datum_header==(short)((unsigned long)handle>>16) ? nodes+index : NULL;
}
#define hs_type_valid(type) ((type)>=_hs_type_void && (type)<NUMBER_OF_HS_TYPES)
static boolean hs_scenario_downloaded;
static char const *const hs_functions_denied_to_downloaded_maps[]={NULL};
static char const *const hs_globals_denied_to_downloaded_maps[]={NULL};
static boolean hs_name_listed(char const *name, char const *const *list) { return FALSE; }
static struct { short minimum,maximum; } hs_special_form_argument_counts[1];
#define _hs_function_set 4
#define _hs_function_wake 21
static int mcc_parameter_set_valid(struct hs_syntax_node const *node, struct hs_syntax_node const *value) { return -1; }
#define hs_global_get_type(index) _hs_unparsed
#define hs_external_global_settable_by_maps(index) FALSE
static struct { char const *name; } unused_global;
#define hs_global_external_get(index) (&unused_global)
#include "under_test.inc"

int main(int argc, char **argv)
{
    char const *case_name=argc>1 ? argv[1] : "";
    char const *refused;
    long i;
    memset(&ai_debug,0x5a,sizeof(ai_debug));
    ai_debug.selected_squad_index=17;ai_debug.selected_actor_index=29;
    ai_state.ai_initialized_for_map=TRUE;
    returned=999;returned_thread=NONE;
    CASE("inactive") ai_state.ai_initialized_for_map=FALSE;
    CASE("already-clear") ai_debug.selected_squad_index=ai_debug.selected_actor_index=NONE;
    CASE("actor-only") ai_debug.selected_squad_index=NONE;
    before=ai_debug;
    CASE("clear") {
        mcc_ai_deselect_evaluate(0x7007,73,TRUE);
        CHECK(ai_debug.selected_squad_index==NONE && ai_debug.selected_actor_index==NONE,"selection");
        CHECK(!ai_debug.evaluation_context_valid && !ai_debug.idle_look_valid,"valid bits");
        CHECK(ai_debug.idle_look_unit_index==NONE && !ai_debug.idle_look_prop_count,"idle look");
        for(i=0;i<(long)sizeof(ai_debug.evaluation_context);i++) CHECK(!ai_debug.evaluation_context[i],"context");
        for(i=0;i<NUMBER_OF_AI_DEBUG_ACTOR_RECORDS;i++) CHECK(!ai_debug.actor_record[i].valid && !ai_debug.actor_record[i].sentinel,"records");
        CHECK(ai_debug.unrelated==before.unrelated && returned==0 && returned_thread==73,"completion");
        return 0;
    }
    if (!strcmp(case_name,"inactive") || !strcmp(case_name,"already-clear")) {
        mcc_ai_deselect_evaluate(0x7007,73,FALSE);
        CHECK(!memcmp(&before,&ai_debug,sizeof(before)),"unchanged state");
        CHECK(returned==0 && returned_thread==73,"completion");
        return 0;
    }
    CASE("actor-only") {
        mcc_ai_deselect_evaluate(0x7007,73,TRUE);
        CHECK(ai_debug.selected_actor_index==NONE && !ai_debug.evaluation_context_valid,"actor clear");
        CHECK(!memcmp(before.evaluation_context,ai_debug.evaluation_context,sizeof(ai_debug.evaluation_context)),"native context lifetime");
        for(i=0;i<NUMBER_OF_AI_DEBUG_ACTOR_RECORDS;i++) CHECK(!ai_debug.actor_record[i].valid && ai_debug.actor_record[i].sentinel==before.actor_record[i].sentinel,"native record lifetime");
        return 0;
    }
    CASE("legacy-policy") {
        syntax_data.count=3;syntax_data.size=sizeof(nodes[0]);syntax_data.data=(byte *)nodes;
        nodes[0].datum_header=nodes[1].datum_header=nodes[2].datum_header=1;
        nodes[0].flags=8;nodes[0].type=_hs_type_void;nodes[0].data=0x10001;nodes[0].next_node_index=NONE;
        nodes[1].flags=9;nodes[1].type=_hs_function_name;nodes[1].next_node_index=NONE;
        nodes[0].function_index=NATIVE_AI_DESELECT;
        CHECK(hs_syntax_node_refusal(nodes,&refused)==_hs_node_refusal_function && !strcmp(refused,"ai_deselect"),"Xbox/CE deselect refused");
        nodes[0].function_index=NATIVE_AI_SELECT;nodes[1].next_node_index=0x10002;
        nodes[2].flags=9;nodes[2].type=_hs_type_ai;nodes[2].next_node_index=NONE;
        CHECK(hs_syntax_node_refusal(nodes,&refused)==_hs_node_refusal_function && !strcmp(refused,"ai_select"),"neighbor remains refused");
        return 0;
    }
    CASE("client-policy") {
        CHECK(!hs_expression_changes_no_game("(ai_deselect)"),"client cannot clear selection");
        CHECK(!hs_expression_changes_no_game("(AI_DESELECT)"),"case independent");
        CHECK(hs_expression_changes_no_game("(show_hud true)"),"ordinary local controls");
        return 0;
    }
    CHECK(FALSE,"unknown case");
    return 1;
}
