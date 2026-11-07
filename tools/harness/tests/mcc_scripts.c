/* Native MCC linker tests with generated syntax, no game assets. */
#include "cseries.h"
#include "math/real_math.h"
#include "errors.h"
#include "hs/hs.h"
#include "hs/hs_scenario_definitions.h"
#include "memory/data.h"
#include "objects/objects.h"
#include "scenario/scenario_definitions.h"
#include "mcc_runtime.h"
#include "mcc_scripts.h"
#include "mcc_campaign.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#undef malloc
#undef free
#undef memset
#undef memcpy
#undef memcmp
#undef strcmp
#undef strcpy
#undef strlen
#undef strstr
#undef strncmp

void mcc_executing_thread_evaluate(short f,long t,boolean i) {(void)f;(void)t;(void)i;}
void mcc_kill_thread_evaluate(short f,long t,boolean i) {(void)f;(void)t;(void)i;}
short const hs_external_global_count=443;
static boolean loaded;
static boolean resolver_case, alias_target_missing;
static unsigned native_lookups, campaign_lookups, parameter_prepares;
static struct data_array list_data;
struct data_array *object_list_header_data = &list_data;
static long arguments[3], returned;
static long equipment_unit;
static short equipment_profile;
static boolean equipment_reset;
static unsigned equipment_calls;
static unsigned deselect_calls;
void ai_scripting_deselect(void) { ++deselect_calls; }
void mcc_player_add_equipment(long unit, short profile, boolean reset) {
    equipment_unit=unit;equipment_profile=profile;equipment_reset=reset;++equipment_calls;
}
static int segment_events;
static char last_event[256];
static struct object_datum mock_object;
static int deleted_lists;
word const hs_object_type_masks[6] = {0xFFFF, 3, 2, 4, 0x380, 0x40};
struct hs_function_definition *mcc_campaign_function(short index) { (void)index;return NULL; }
short mcc_campaign_find(char const *name) {
    if (resolver_case) {
        ++campaign_lookups;
        if (!_stricmp(name,"physics_set_gravity")) return MCC_HS_CAMPAIGN_FIRST;
    }
    return NONE;
}
struct scenario *global_scenario_get(void) { return NULL; }
boolean ai_index_from_string(struct scenario *scenario, char const *name, long *result) {
    (void)scenario;
    if (strcmp(name, "falcon/pilot")) return FALSE;
    *result = (long)0x80000018; return TRUE;
}
long object_list_from_ai_reference(long reference) { return reference == NONE ? NONE : 0; }
void object_list_delete(long list) { (void)list; ++deleted_lists; }

void *csmemcpy(void *to, void const *from, unsigned long bytes) { return memcpy(to, from, bytes); }
void *csmemset(void *to, long value, unsigned long bytes) { return memset(to, value, bytes); }
char *csstrcpy(char *to, char const *from) {return strcpy(to,from);}
long csstrcasecmp(char const *a, char const *b) { return _stricmp(a, b); }
void error(short priority, char const *format, ...) {
    va_list args;
    if (priority != _error_log) return;
    ++segment_events;
    va_start(args, format);
    _vsnprintf(last_event, sizeof(last_event) - 1, format, args);
    va_end(args);
    last_event[sizeof(last_event) - 1] = 0;
}
boolean mcc_cache_tags_loaded(void) { return loaded; }
boolean mcc_cache_contains(void const *data, unsigned long bytes) { (void)data; (void)bytes; return loaded; }
int mcc_parameters_prepare(struct mcc_runtime *runtime, struct scenario *scenario, struct data_array *syntax) {
    (void)runtime; (void)scenario; (void)syntax; ++parameter_prepares; return 1;
}
boolean hs_macro_function_parse(short index, long expression) { (void)index; (void)expression; return TRUE; }
long *hs_macro_function_evaluate(short function, long thread, boolean initialize) {
    (void)function; (void)thread; return initialize ? arguments : NULL;
}
void hs_return(long thread, long value) { (void)thread; returned = value; }
short hs_find_function_by_name(char const *name) {
    if (resolver_case) {
        /* The real native lookup consults the guarded MCC hook first. */
        short index=mcc_script_find(name);
        if (index!=NONE) return index;
        ++native_lookups;
        if (!_stricmp(name,"sleep")) return 321;
        if (!_stricmp(name,"player_effect_set_max_rumble")) return alias_target_missing ? NONE : 322;
        /* Deliberate legacy collisions check MCC precedence and alias misses. */
        if (!_stricmp(name,"core_save_name")) return 323;
        if (!_stricmp(name,"player_effect_set_max_vibrate")) return 324;
        if (!_stricmp(name,"ai_select")) return 200;
        if (!_stricmp(name,"ai_deselect")) return 201;
        return NONE;
    }
    if (!strcmp(name, "sleep") || !strcmp(name,"set")) return 321;
    if (!strcmp(name, "player_effect_set_max_rumble")) return 322;
    return NONE;
}
short hs_find_global_by_name(char const *name) { return !strcmp(name, "known_global") ? (short)0x8009 : NONE; }
void *object_try_and_get_and_verify_type(long index, unsigned long mask) {
    (void)mask; return index >= 0 && index <= 2 ? &mock_object : NULL;
}
real_point3d *object_get_origin(long index, real_point3d *point) {
    point->x = index == 0 ? 1.f : index == 1 ? 8.f : 10.f;
    point->y = point->z = 0; return point;
}
long object_list_get_first(long list, long *reference) { (void)list; *reference = 0; return 0; }
long object_list_get_next(long list, long *reference) { (void)list; return ++*reference == 1 ? 1 : NONE; }
void *datum_try_and_get(struct data_array *data, long handle) {
    unsigned index = (unsigned)handle & 0xFFFFu;
    struct hs_syntax_node *node;
    if (data == &list_data) return handle == 0 ? arguments : NULL;
    if (handle == NONE || index >= (unsigned)data->count) return NULL;
    node = (struct hs_syntax_node *)data->data + index;
    return node->datum_header == (short)((unsigned)handle >> 16) ? node : NULL;
}
void *mcc_runtime_pointer(struct mcc_runtime *runtime, uint32_t address, uint32_t bytes) {
    uint32_t offset = address - runtime->report.tag_base;
    return address >= runtime->report.tag_base && offset <= runtime->used && bytes <= runtime->used - offset ?
        runtime->tags + offset : NULL;
}

void *mcc_runtime_allocate(struct mcc_runtime *runtime,uint32_t bytes) {
    if(runtime->used+bytes>runtime->capacity)return NULL;
    {void *p=runtime->tags+runtime->used;runtime->used+=bytes;return p;}
}
#define CHECK(value) do { if (!(value)) { fprintf(stderr, "check failed at line %d\n", __LINE__); return 1; } } while (0)

static int check_resolver(struct mcc_runtime *runtime, struct data_array *syntax,
    struct hs_syntax_node *nodes, char *strings, char const *name, short expected, boolean missing)
{
    short prepared_index=!_stricmp(name,"sleep") ? 321 : expected;
    short resolved;
    unsigned preparation_native,preparation_campaign;
    boolean prepared;
    resolver_case=TRUE;alias_target_missing=missing;
    strcpy(strings,name);
    nodes[1].function_index=778;
    CHECK(!loaded && mcc_script_find(name)==NONE && mcc_script_find(NULL)==NONE);
    CHECK(!native_lookups && !campaign_lookups && !parameter_prepares);
    prepared=mcc_scripts_prepare(runtime);
    CHECK(prepared==(prepared_index!=NONE));
    CHECK(parameter_prepares==(unsigned)prepared);
    CHECK(nodes[0].function_index==(prepared ? prepared_index : 777));
    CHECK(nodes[1].function_index==(prepared ? prepared_index : 778));
    CHECK(syntax->count==4 && syntax->actual_count==4 && syntax->maximum_count==32767);
    CHECK(prepared ? !runtime->failure[0] : strstr(runtime->failure,"Unsupported script name:")!=NULL);
    preparation_native=native_lookups;preparation_campaign=campaign_lookups;
    native_lookups=campaign_lookups=0;loaded=TRUE;
    CHECK(mcc_script_find(NULL)==NONE && !native_lookups && !campaign_lookups);
    resolved=mcc_script_find(name);
    CHECK(resolved==expected);
    if (!_stricmp(name,"player_effect_set_max_vibrate")) CHECK(native_lookups==1);
    else CHECK(!native_lookups);
    printf("%s prepare=%d call=%d predicate=%d runtime=%d native=%u/%u campaign=%u/%u\n",
        name,prepared,nodes[0].function_index,nodes[1].function_index,resolved,
        preparation_native,native_lookups,preparation_campaign,campaign_lookups);
    native_lookups=campaign_lookups=0;loaded=FALSE;
    CHECK(mcc_script_find(name)==NONE && mcc_script_find(NULL)==NONE);
    CHECK(!native_lookups && !campaign_lookups);
    /* A later legacy lookup retains the native implementation. */
    CHECK(hs_find_function_by_name("core_save_name")==323);
    CHECK(hs_find_function_by_name("player_effect_set_max_vibrate")==324);
    CHECK(hs_find_function_by_name("ai_deselect")==201);
    free(runtime->tags);
    return 0;
}

int main(int argc, char **argv) {
    struct mcc_runtime runtime = {0};
    struct scenario *scenario;
    struct data_array *syntax;
    struct hs_syntax_node *nodes;
    struct hs_function_definition *extension;
    char *strings;
    uint32_t pointer;
    int expected = 1;
    short segment_index;
    CHECK(argc >= 2 && argc <= 4);
    segment_index = !strcmp(argv[1], "core_segment") ? MCC_HS_CORE_SEGMENT : MCC_HS_MISSION_SEGMENT;
    if (!strcmp(argv[1], "issue89_registry")) {
        char const *names[3]={"abs_integer","get_executing_running_thread","kill_thread"};
        long values[5]={0,1,-1,2147483647L,(-2147483647L-1)};
        int j;
        for(j=0;j<3;j++) {
            loaded=0;CHECK(mcc_script_find(names[j])==NONE);
            CHECK(!mcc_script_function((short)(0x7004+j)));
            loaded=1;CHECK(mcc_script_find(names[j])==0x7004+j);
            extension=mcc_script_function((short)(0x7004+j));CHECK(extension);
            CHECK(extension->parameter_count==(j==1 ? 0 : 1));
            CHECK(extension->return_type==(j==2 ? _hs_type_void : _hs_type_long_integer));
            if(j!=1) CHECK(extension->parameter_types[0]==_hs_type_long_integer);
        }
        extension=mcc_script_function(0x7004);
        for(j=0;j<5;j++) {
            arguments[0]=values[j];returned=123;
            extension->evaluate(0x7004,0,FALSE);CHECK(returned==123);
            extension->evaluate(0x7004,0,TRUE);
            CHECK((unsigned long)returned==(values[j]<0 ? 0UL-(unsigned long)values[j] : (unsigned long)values[j]));
        }
        return 0;
    }
    runtime.used = 0x100000;
    runtime.capacity=runtime.used+65536;
    runtime.tags = malloc(runtime.capacity);
    CHECK(runtime.tags);
    memset(runtime.tags, 0, runtime.used);
    runtime.tag_index = runtime.tags + 40;
    runtime.report.tag_base = (uint32_t)(uintptr_t)runtime.tags;
    scenario = (struct scenario *)(runtime.tags + 0x100);
    syntax = (struct data_array *)(runtime.tags + 0x1000);
    nodes = (struct hs_syntax_node *)(syntax + 1);
    strings = (char *)(runtime.tags + 0xF0000);
    pointer = (uint32_t)(uintptr_t)scenario;
    memcpy(runtime.tags + 60, &pointer, 4);
    scenario->hs_syntax_data.address = syntax;
    scenario->hs_syntax_data.size = sizeof(*syntax) + 32767 * sizeof(*nodes);
    scenario->hs_string_constants.address = strings;
    scenario->hs_string_constants.size = 128;
    syntax->size = sizeof(*nodes);
    syntax->maximum_count = 32767;
    syntax->count = 4;
    syntax->actual_count = 4;
    syntax->first_free_absolute_index = 4;
    syntax->valid = TRUE;
    syntax->signature = 'd@t@';
    syntax->data = nodes;
    nodes[0].datum_header = nodes[1].datum_header = nodes[2].datum_header = nodes[3].datum_header = 1;
    nodes[0].flags = 8;
    nodes[0].next_node_index = NONE;
    nodes[0].data = 0x10001;
    nodes[0].function_index = 777;
    nodes[1].type = _hs_function_name;
    nodes[1].flags = 1;
    nodes[1].next_node_index = 0x10002;
    nodes[2].type = _hs_type_object_list;
    nodes[2].flags = 1;
    nodes[2].next_node_index = 0x10003;
    nodes[3].type = _hs_type_object;
    nodes[3].flags = 1;
    nodes[3].next_node_index = NONE;
    strcpy(strings, "sleep");
    if (!strcmp(argv[1],"resolver") || !strcmp(argv[1],"resolver_missing_alias")) {
        CHECK(argc==4);
        return check_resolver(&runtime,syntax,nodes,strings,argv[2],(short)strtol(argv[3],NULL,10),
            !strcmp(argv[1],"resolver_missing_alias"));
    }
    if (!strcmp(argv[1],"ai_deselect")) {
        strcpy(strings,"ai_deselect");resolver_case=TRUE;
        nodes[0].type=_hs_type_void;nodes[1].next_node_index=NONE;
        CHECK(!loaded && !mcc_script_function(0x7007));
        CHECK(mcc_scripts_prepare(&runtime));
        CHECK(nodes[0].function_index==0x7007 && nodes[1].function_index==0x7007);
        loaded=TRUE;extension=mcc_script_function(0x7007);CHECK(extension);
        CHECK(extension->return_type==_hs_type_void && extension->parameter_count==0);
        CHECK(mcc_script_find("ai_select")==NONE && hs_find_function_by_name("ai_select")==200);
        CHECK(mcc_script_call_valid(nodes,syntax));
        returned=123;extension->evaluate(0x7007,0,TRUE);
        CHECK(deselect_calls==1 && returned==0);
        /* Like native zero-argument evaluators, completion does not depend on
         * an argument frame or the initialize flag. */
        returned=123;extension->evaluate(0x7007,0,FALSE);
        CHECK(deselect_calls==2 && returned==0);
        nodes[1].next_node_index=0x10002;CHECK(!mcc_script_call_valid(nodes,syntax));
        nodes[1].next_node_index=NONE;
        nodes[0].data=0x20001;CHECK(!mcc_script_call_valid(nodes,syntax));
        nodes[0].data=0x10001;
        nodes[0].flags=0;CHECK(!mcc_script_call_valid(nodes,syntax));
        nodes[0].flags=9;CHECK(!mcc_script_call_valid(nodes,syntax));
        nodes[0].flags=10;CHECK(!mcc_script_call_valid(nodes,syntax));
        nodes[0].flags=8;loaded=FALSE;
        CHECK(!mcc_script_call_valid(nodes,syntax) && !mcc_script_function(0x7007));
        CHECK(mcc_script_find("ai_deselect")==NONE && hs_find_function_by_name("ai_deselect")==201);
        free(runtime.tags);return 0;
    }
    if (!strncmp(argv[1], "issue89_link_", 13)) {
        strcpy(strings,argv[1]+13);
        nodes[0].type=!strcmp(strings,"kill_thread") ? _hs_type_void : _hs_type_long_integer;
        nodes[2].type=_hs_type_long_integer;nodes[2].next_node_index=NONE;
        if (!strcmp(strings,"get_executing_running_thread")) nodes[1].next_node_index=NONE;
    }
    if (!strcmp(argv[1], "unknown")) { strcpy(strings, "not_implemented"); expected = 0; }
    if (!strcmp(argv[1], "bad_child")) { nodes[0].data = 0x10008; expected = 0; }
    if (!strcmp(argv[1], "child_salt")) { nodes[0].data = 0x20001; expected = 0; }
    if (!strcmp(argv[1], "short_data")) { scenario->hs_syntax_data.size = sizeof(*syntax) + 4 * sizeof(*nodes); expected = 0; }
    if (!strcmp(argv[1], "capacity")) syntax->count = 19002;
    if (!strcmp(argv[1], "negative_count")) { syntax->count = -1; expected = 0; }
    if (!strcmp(argv[1], "full_capacity")) {
        syntax->count = 32767;
        syntax->actual_count = 6;
        nodes[32765] = nodes[0]; nodes[32765].data = 0x17FFE;
        nodes[32766] = nodes[1]; nodes[32766].next_node_index = NONE;
    }
    if (!strcmp(argv[1], "unterminated")) { memset(strings, 'x', 128); expected = 0; }
    if (!strcmp(argv[1], "equipment")) {
        strcpy(strings, "player_add_equipment");
        nodes[0].type=_hs_type_void;nodes[2].type=_hs_type_unit;
        nodes[3].type=_hs_type_starting_profile;nodes[3].next_node_index=0x10004;
        nodes[4].datum_header=1;nodes[4].flags=1;nodes[4].type=_hs_type_boolean;
        nodes[4].next_node_index=NONE;syntax->count=syntax->actual_count=5;
    }
    if (!strcmp(argv[1], "alias")) strcpy(strings, "player_effect_set_max_vibrate");
    if (!strcmp(argv[1], "extension")) strcpy(strings, "objects_distance_to_object");
    if (!strcmp(argv[1], "segment") || !strcmp(argv[1], "core_segment")) {
        strcpy(strings, segment_index == MCC_HS_CORE_SEGMENT ? "core_save_name" : "mcc_mission_segment");
        nodes[0].type = _hs_type_boolean;
        nodes[2].type = _hs_type_string;
        nodes[2].next_node_index = NONE;
    }
    if (!strcmp(argv[1], "global")) {
        nodes[0].flags = 5; nodes[0].data = 0x8001; strcpy(strings, "known_global");
    }
    if (!strcmp(argv[1], "skull_read") || !strcmp(argv[1], "skull_wrong_type")) {
        nodes[0].flags=5;nodes[0].data=0x8001;nodes[0].type=_hs_type_boolean;
        strcpy(strings,"debug_ice_cream_flavor_status_grunt_birthday_party");
        if (!strcmp(argv[1], "skull_wrong_type")) {nodes[0].type=_hs_type_real;expected=0;}
    }
    if (!strcmp(argv[1], "skull_write")) {
        strcpy(strings,"set");strcpy(strings+4,"debug_ice_cream_flavor_status_i_would_have_been_your_daddy");
        nodes[2].flags=5;nodes[2].data=0x8001;nodes[2].string_offset=4;nodes[2].type=_hs_type_boolean;expected=0;
    }
    if (!strcmp(argv[1], "developer_read") || !strcmp(argv[1], "developer_cast")) {
        nodes[0].flags=13;nodes[0].data=0x8001;
        nodes[0].type=!strcmp(argv[1],"developer_cast") ? _hs_type_real : _hs_type_short_integer;
        strcpy(strings,"developer_mode");
    }
    if (!strcmp(argv[1], "developer_write")) {
        strcpy(strings,"set");strcpy(strings+4,"developer_mode");
        nodes[2].flags=5;nodes[2].data=0x8001;nodes[2].string_offset=4;
        nodes[2].type=_hs_type_short_integer;
    }
    if (!strcmp(argv[1],"skull_disable") || !strcmp(argv[1],"skull_enable") || !strcmp(argv[1],"skull_computed")) {
        strcpy(strings,"set");strcpy(strings+4,"debug_ice_cream_flavor_status_bandanna");
        nodes[0].type=_hs_type_void;
        nodes[2].flags=5;nodes[2].data=0x8001;nodes[2].string_offset=4;nodes[2].type=_hs_type_boolean;
        nodes[3].flags=9;nodes[3].type=nodes[3].constant_type=_hs_type_boolean;nodes[3].data=(long)0xffffff00;
        if (!strcmp(argv[1],"skull_enable")) {nodes[3].data=1;expected=0;}
        if (!strcmp(argv[1],"skull_computed")) {nodes[3].flags=13;expected=0;}
    }
    if (!strcmp(argv[1],"developer_capacity") || !strcmp(argv[1],"developer_allocation")) {
        nodes[0].flags=13;nodes[0].data=0x8001;nodes[0].type=_hs_type_short_integer;
        strcpy(strings,"developer_mode");expected=0;
        if (!strcmp(argv[1],"developer_capacity")) scenario->hs_globals.count=1024-hs_external_global_count;
        else runtime.capacity=runtime.used;
    }
    if (!strncmp(argv[1],"score_",6)) {
        strcpy(strings,"set");strcpy(strings+4,"debug_score");
        nodes[2].flags=13;nodes[2].data=0x8001;nodes[2].string_offset=4;
        nodes[2].type=_hs_type_long_integer;
        if (!strcmp(argv[1],"score_capacity")) {scenario->hs_globals.count=1024-hs_external_global_count;expected=0;}
        if (!strcmp(argv[1],"score_allocation")) {runtime.capacity=runtime.used;expected=0;}
    }
    if (argc>=3) strcpy(strings+(!strcmp(argv[1],"skull_read") || !strcmp(argv[1],"skull_wrong_type") ? 0 : 4),argv[2]);
    if (argc==4) nodes[0].type=!strcmp(argv[3],"real") ? _hs_type_real :
        !strcmp(argv[3],"long") ? _hs_type_long_integer : _hs_type_boolean;
    if (strstr(argv[1],"prediction")==argv[1]) {
        strcpy(strings,"set");strcpy(strings+4,"object_prediction");nodes[0].type=_hs_type_void;
        nodes[2].flags=5;nodes[2].data=0x8001;nodes[2].string_offset=4;nodes[2].type=_hs_type_boolean;
        nodes[3].flags=9;nodes[3].type=nodes[3].constant_type=_hs_type_boolean;nodes[3].data=(long)0xffffff01;
        if (!strcmp(argv[1],"prediction_disable")) {nodes[3].data=0;expected=0;}
        if (!strcmp(argv[1],"prediction_computed")) {nodes[3].flags=13;expected=0;}
    }
    if (!strcmp(argv[1],"scratch")) {
        char counter[3]={'f',argc>=3 ? argv[2][0] : '0',0};
        scenario->hs_string_constants.size=1152;
        strcpy(strings,"set");strcpy(strings+4,counter);
        nodes[2].flags=nodes[3].flags=13;nodes[2].data=nodes[3].data=0x8001;
        nodes[2].string_offset=nodes[3].string_offset=4;
        nodes[2].type=nodes[3].type=_hs_type_real;
    }
    if (!strcmp(argv[1],"speed") || !strcmp(argv[1],"score")) {
        strcpy(strings,!strcmp(argv[1],"score") ? "debug_score" : "game_speed_value");
        nodes[0].flags=nodes[2].flags=13;nodes[0].data=nodes[2].data=0x8001;
        nodes[0].string_offset=nodes[2].string_offset=0;
        nodes[0].type=nodes[2].type=!strcmp(argv[1],"score") ? _hs_type_long_integer : _hs_type_real;
    }
    CHECK(mcc_scripts_prepare(&runtime) == expected);
    if (!strcmp(argv[1],"score_write")) {
        struct hs_global *global=scenario->hs_globals.address;
        CHECK(scenario->hs_globals.count==1 && nodes[2].data==0 && (nodes[2].flags&4));
        CHECK(global->type==_hs_type_long_integer && !strcmp(global->name,"debug_score"));
        free(runtime.tags);return 0;
    }
    if (!strcmp(argv[1],"speed") || !strcmp(argv[1],"score")) {
        struct hs_global *global=scenario->hs_globals.address;
        struct hs_syntax_node *initial=datum_try_and_get(syntax,global->initialization_expression_index);
        CHECK(scenario->hs_globals.count==1 && nodes[0].data==0 && nodes[2].data==0);
        CHECK(global->type==nodes[0].type && initial);
        CHECK(initial->data==(!strcmp(argv[1],"score") ? 0 : 0x3f800000));
        CHECK(!strcmp(global->name,!strcmp(argv[1],"score") ? "debug_score" : "game_speed_value"));
        free(runtime.tags);return 0;
    }
    if (!strcmp(argv[1],"scratch")) {
        struct hs_global *global=scenario->hs_globals.address;
        struct hs_syntax_node *initial=datum_try_and_get(syntax,global->initialization_expression_index);
        char const *relocated=(char const *)scenario->hs_string_constants.address+nodes[2].string_offset;
        CHECK(scenario->hs_globals.count==1 && nodes[2].data==0 && nodes[3].data==0);
        CHECK(global->type==_hs_type_real && initial && initial->data==0 && initial->type==_hs_type_real);
        CHECK(!strcmp(global->name,relocated) && !strncmp(relocated,"__mcc_scratch_f",14));
        CHECK(nodes[2].string_offset==nodes[3].string_offset && syntax->actual_count==5);
        CHECK(nodes[2].string_offset<scenario->hs_string_constants.size-1024);
        CHECK(!memcmp((char const *)scenario->hs_string_constants.address+scenario->hs_string_constants.size-1024,
            runtime.tags+0xE0000,1024));
        free(runtime.tags);return 0;
    }
    if (strstr(argv[1],"prediction")==argv[1]) {
        if (expected) CHECK(nodes[0].flags==9 && nodes[0].data==0 && nodes[0].type==_hs_type_void && nodes[0].constant_type==nodes[0].type);
        free(runtime.tags);return 0;
    }
    if (!strcmp(argv[1],"skull_disable")) {
        CHECK(nodes[0].flags==9 && nodes[0].data==FALSE && nodes[0].constant_type==nodes[0].type);
        CHECK((argc==4 || nodes[0].type==_hs_type_void) && nodes[0].next_node_index==NONE);
        free(runtime.tags);return 0;
    }
    if (!strcmp(argv[1],"developer_write")) {
        struct hs_global *global=scenario->hs_globals.address;
        struct hs_syntax_node *initial;
        CHECK(scenario->hs_globals.count==1 && nodes[2].data==0 && (nodes[2].flags&4));
        CHECK(!strcmp(global->name,"developer_mode") && global->type==_hs_type_short_integer);
        initial=datum_try_and_get(syntax,global->initialization_expression_index);
        CHECK(initial && initial->flags==9 && initial->short_value==0 && initial->type==_hs_type_short_integer);
        CHECK(initial->next_node_index==NONE && initial->source_offset==NONE);
    }
    if (!strcmp(argv[1], "developer_read") || !strcmp(argv[1], "developer_cast")) {
        CHECK((nodes[0].flags&4) && nodes[0].data==0 && scenario->hs_globals.count==1);
        CHECK(((struct hs_global *)scenario->hs_globals.address)->type==_hs_type_short_integer);
        CHECK(syntax->actual_count==5);
        CHECK(nodes[0].type==(!strcmp(argv[1],"developer_cast") ? _hs_type_real : _hs_type_short_integer));
        free(runtime.tags);return 0;
    }
    if (!expected) { free(runtime.tags); return 0; }
    if (!strcmp(argv[1], "skull_read")) {
        CHECK(!(nodes[0].flags&4) && nodes[0].data==FALSE && nodes[0].constant_type==_hs_type_boolean);
        free(runtime.tags);return 0;
    }
    CHECK(syntax->maximum_count == 32767);
    CHECK(syntax->data == nodes);
    if (!strcmp(argv[1], "full_capacity")) {
        CHECK(nodes[32765].function_index == 321 && nodes[32766].function_index == 321);
    }
    CHECK(mcc_script_find("player_add_equipment") == NONE);
    CHECK(!mcc_script_function(MCC_HS_DISTANCE_TO_OBJECT));
    CHECK(!mcc_script_function(MCC_HS_MISSION_SEGMENT));
    CHECK(!mcc_script_function(MCC_HS_CORE_SEGMENT));
    CHECK(mcc_script_find("core_save_name") == NONE);
    CHECK(mcc_script_find("objects_distance_to_object") == NONE);
    CHECK(mcc_script_find("mcc_mission_segment") == NONE);
    loaded = TRUE;
    {
        struct hs_syntax_node ai = {0};
        CHECK(mcc_script_ai_cast(_hs_type_ai, _hs_type_unit));
        CHECK(mcc_script_can_cast(_hs_type_short_integer, _hs_type_long_integer));
        CHECK(mcc_script_cast(_hs_type_short_integer, _hs_type_long_integer, 0xFFFF) == -1);
        CHECK(mcc_script_cast(_hs_type_short_integer, _hs_type_long_integer, 32767) == 32767);
        CHECK(mcc_script_cast(_hs_type_short_integer, _hs_type_long_integer, 0x8000) == -32768);
        CHECK(!mcc_script_can_cast(_hs_type_long_integer, _hs_type_short_integer));
        ai.flags = 9; ai.constant_type = _hs_type_ai; ai.type = _hs_type_unit;
        CHECK(mcc_script_parse_ai(&ai, "falcon/pilot") == 1 && ai.data == (long)0x80000018);
        CHECK(mcc_script_parse_ai(&ai, "missing/pilot") == 0);
        ai.flags |= 4;
        CHECK(mcc_script_parse_ai(&ai, "falcon/pilot") == -1);
        CHECK(mcc_script_cast_ai(_hs_type_unit, 24) == 0 && deleted_lists == 1);
        CHECK(mcc_script_cast_ai(_hs_type_vehicle, 24) == NONE && deleted_lists == 2);
        CHECK(mcc_script_cast_ai(_hs_type_object_list, 24) == 0 && deleted_lists == 2);
        CHECK(mcc_script_cast_ai(_hs_type_unit, NONE) == NONE);
        loaded = FALSE;
        CHECK(!mcc_script_ai_cast(_hs_type_ai, _hs_type_unit));
        CHECK(!mcc_script_can_cast(_hs_type_short_integer, _hs_type_long_integer));
        loaded = TRUE;
    }
    if (!strncmp(argv[1], "issue89_link_", 13)) {
        short index=mcc_script_find(strings);
        CHECK(index!=NONE && nodes[0].function_index==index && nodes[1].function_index==index);
        CHECK(mcc_script_call_valid(nodes,syntax));
        if (!strcmp(strings,"get_executing_running_thread")) nodes[1].next_node_index=0x10002;
        else nodes[2].type=_hs_type_string;
        CHECK(!mcc_script_call_valid(nodes,syntax));
        free(runtime.tags);return 0;
    }
    CHECK(mcc_script_find("player_effect_set_max_vibrate") == 322);
    CHECK(mcc_script_find("MCC_MISSION_SEGMENT") == MCC_HS_MISSION_SEGMENT);
    CHECK(mcc_script_find("CORE_SAVE_NAME") == MCC_HS_CORE_SEGMENT);
    if (!strcmp(argv[1], "equipment")) {
        short index=mcc_script_find("PLAYER_ADD_EQUIPMENT");
        CHECK(index!=NONE && nodes[0].function_index==index && nodes[1].function_index==index);
        extension=mcc_script_function(index);
        CHECK(extension && extension->parameter_count==3 && extension->return_type==_hs_type_void);
        CHECK(mcc_script_call_valid(nodes,syntax));
        nodes[4].type=_hs_type_real;CHECK(!mcc_script_call_valid(nodes,syntax));
        nodes[4].type=_hs_type_boolean;nodes[4].next_node_index=0x10002;
        CHECK(!mcc_script_call_valid(nodes,syntax));
        arguments[0]=123;arguments[1]=0xFFFF0007;arguments[2]=(long)0xFFFFFF01;
        extension->evaluate(index,0,FALSE);CHECK(!equipment_calls);
        extension->evaluate(index,0,TRUE);
        CHECK(equipment_calls==1 && equipment_unit==123 && equipment_profile==7 && equipment_reset==TRUE);
        arguments[2]=(long)0xFFFFFF00;extension->evaluate(index,0,TRUE);
        CHECK(equipment_calls==2 && equipment_reset==FALSE);
        loaded=FALSE;CHECK(!mcc_script_function(index));
    } else if (!strcmp(argv[1], "global")) CHECK((unsigned short)nodes[0].short_value == 0x8009);
    else if (!strcmp(argv[1], "alias")) CHECK(nodes[0].function_index == 322);
    else if (!strcmp(argv[1], "extension")) {
        union { long bits; float value; } result;
        CHECK(nodes[0].function_index == MCC_HS_DISTANCE_TO_OBJECT);
        CHECK(mcc_script_call_valid(&nodes[0], syntax));
        nodes[3].type = _hs_type_real;
        CHECK(!mcc_script_call_valid(&nodes[0], syntax));
        extension = mcc_script_function(MCC_HS_DISTANCE_TO_OBJECT);
        CHECK(extension && extension->parameter_types[1] == _hs_type_object);
        arguments[0] = 0; arguments[1] = 2;
        extension->evaluate(MCC_HS_DISTANCE_TO_OBJECT, 0, TRUE);
        result.bits = returned;
        CHECK(result.value == 2.f);
        arguments[0] = NONE;
        extension->evaluate(MCC_HS_DISTANCE_TO_OBJECT, 0, TRUE);
        result.bits = returned;
        CHECK(result.value == -1.f);
    } else if (!strcmp(argv[1], "segment") || !strcmp(argv[1], "core_segment")) {
        char long_segment[200];
        CHECK(nodes[0].function_index == segment_index);
        CHECK(nodes[1].function_index == segment_index);
        CHECK(mcc_script_call_valid(&nodes[0], syntax));
        extension = mcc_script_function(segment_index);
        CHECK(extension && extension->return_type == _hs_type_boolean);
        CHECK(extension->parameter_count == 1 && extension->parameter_types[0] == _hs_type_string);
        nodes[2].type = _hs_type_real;
        CHECK(!mcc_script_call_valid(&nodes[0], syntax));
        nodes[2].type = _hs_type_string;
        nodes[1].next_node_index = NONE;
        CHECK(!mcc_script_call_valid(&nodes[0], syntax));
        nodes[1].next_node_index = 0x20002;
        CHECK(!mcc_script_call_valid(&nodes[0], syntax));
        nodes[1].next_node_index = 0x10002;
        nodes[2].next_node_index = 0x10003;
        CHECK(!mcc_script_call_valid(&nodes[0], syntax));
        nodes[2].next_node_index = NONE;
        arguments[0] = (long)(uintptr_t)"cine1_intro";
        returned = -123;
        extension->evaluate(segment_index, 0, FALSE);
        CHECK(returned == -123 && !segment_events); /* argument expression still yielding */
        extension->evaluate(segment_index, 0, TRUE);
        CHECK(returned == TRUE && segment_events == 1);
        CHECK(!strcmp(last_event, "mcc: mission segment 'cine1_intro'"));
        arguments[0] = (long)(uintptr_t)"03_escape"; /* use the evaluated argument each time */
        extension->evaluate(segment_index, 0, TRUE);
        CHECK(returned == TRUE && segment_events == 2);
        CHECK(!strcmp(last_event, "mcc: mission segment '03_escape'"));
        memset(long_segment, 'x', sizeof(long_segment)); long_segment[199] = 0;
        arguments[0] = (long)(uintptr_t)long_segment;
        extension->evaluate(segment_index, 0, TRUE);
        CHECK(returned == TRUE && segment_events == 3 && strlen(last_event) == 151);
        arguments[0] = 0;
        extension->evaluate(segment_index, 0, TRUE);
        CHECK(returned == FALSE && segment_events == 3);
        loaded = FALSE;
        CHECK(!mcc_script_function(segment_index));
        CHECK(mcc_script_find("mcc_mission_segment") == NONE);
    } else CHECK(nodes[0].function_index == 321 && nodes[1].function_index == 321);
    free(runtime.tags);
    return 0;
}
