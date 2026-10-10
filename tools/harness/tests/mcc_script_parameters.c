#include "cseries.h"
#include "math/real_math.h"
#include "errors.h"
#include "hs/hs.h"
#include "hs/hs_scenario_definitions.h"
#include "memory/data.h"
#include "scenario/scenario_definitions.h"
#include "mcc_runtime.h"
#include "mcc_script_parameters.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#undef malloc
#undef free
#undef memset
#undef memcpy
#undef strcmp
#undef strncmp
#undef strcpy

#define CHECK(value) do {if(!(value)){fprintf(stderr,"check failed at %d: %s\n",__LINE__,#value);return 1;}}while(0)
struct hs_stack_frame {struct hs_stack_frame *previous;long expression_index;void *result;short size;byte data[2];};
struct hs_thread_datum {short identifier;byte type,flags;long script_index,sleep_until,previous_sleep_until;
    struct hs_stack_frame *stack;long result;byte stack_data[512];};
static struct hs_thread_datum thread,other_thread;
static struct scenario *scenario;
struct data_array *hs_syntax_data;
static int loaded=1,errors,refs,returned,disabled;
short const hs_external_global_count=443;
static long last_result;
static struct hs_thread_datum *hs_thread_get(long index) {return index==1 ? &other_thread : &thread;}
#define _hs_thread_sleeping_bit 1
static long hs_find_thread_by_script(short script) { return script==0 ? 0 : script==1 ? 1 : NONE; }
boolean mcc_cache_tags_loaded(void) {return loaded;}
void error(short priority,char const *message,...) {(void)priority;(void)message;}
void *csmemset(void *p,long value,unsigned long size) {return memset(p,(int)value,size);}
void *csmemcpy(void *p,void const *from,unsigned long size) {return memcpy(p,from,size);}
boolean hs_can_cast(short a,short b) {return a==b || (a==_hs_type_short_integer && b==_hs_type_long_integer) ||
    (loaded && a==_hs_type_ai && (HS_TYPE_IS_OBJECT(b)||b==_hs_type_object_list));}
long hs_cast(long index,short from,short to,long value) {(void)index;(void)from;(void)to;return value;}
void object_list_add_reference(long index) {if(index!=NONE)refs++;}
void object_list_remove_reference(long index) {if(index!=NONE){if(!refs)exit(20);refs--;}}
static void hs_syntax_error(long index) {(void)index;errors++;}
static long hs_type_default_value(short type) {(void)type;return NONE;}
boolean hs_scenario_script_disabled(short index) {(void)index;return disabled;}
struct scenario *global_scenario_get(void) {return scenario;}
struct hs_syntax_node *hs_syntax_get(long handle) {
    unsigned index=(unsigned)handle&65535;
    struct hs_syntax_node *nodes=hs_syntax_data->data;
    return handle!=NONE&&index<(unsigned)hs_syntax_data->count&&
        (unsigned short)nodes[index].datum_header==(unsigned short)((unsigned)handle>>16)?nodes+index:NULL;
}
static void *hs_stack_allocate(long index,long size) {
    void *out;(void)index;
    if(size<0||size>thread.stack_data+512-(thread.stack->data+thread.stack->size)){errors++;return NULL;}
    out=thread.stack->data+thread.stack->size;thread.stack->size+=(short)size;return out;
}
void hs_return(long index,long value) {(void)index;returned++;last_result=value;}
static void hs_evaluate(long index,long expression,long *out);
#include "mcc_script_runtime.inl"
static void hs_evaluate(long index,long expression,long *out) {
    struct hs_syntax_node *node=hs_syntax_get(expression);
    if(!node){errors++;return;}
    if(!mcc_hs_parameter_evaluate(index,node,out))*out=node->data;
}
void *mcc_runtime_pointer(struct mcc_runtime *runtime,uint32_t address,uint32_t bytes) {
    uint32_t offset=address-runtime->report.tag_base;
    return address>=runtime->report.tag_base&&offset<=runtime->used&&bytes<=runtime->used-offset?
        runtime->tags+offset:NULL;
}
void *tag_block_get_element_with_size(struct tag_block const *block,long index,long size) {
    return (unsigned char *)block->address+index*size;
}

int main(int argc,char **argv) {
    struct mcc_runtime runtime={0};
    struct hs_script *scripts;
    struct hs_syntax_node *nodes;
    struct hs_global *globals;
    unsigned char *parameter;
    uint32_t *block;
    short type=_hs_type_long_integer,owner;
    int valid=1,i;
    struct hs_stack_frame *frame;
    struct mcc_hs_arguments *state;
    CHECK(argc==2);
    runtime.used=1048576;runtime.tags=malloc(runtime.used);CHECK(runtime.tags);
    memset(runtime.tags,0,runtime.used);runtime.report.tag_base=(uint32_t)(uintptr_t)runtime.tags;
    scenario=(void *)(runtime.tags+256);scripts=(void *)(runtime.tags+4096);
    hs_syntax_data=(void *)(runtime.tags+8192);nodes=(void *)(hs_syntax_data+1);
    parameter=runtime.tags+0xE0000;globals=(void *)(runtime.tags+0xF0000);
    scenario->hs_scripts.count=2;scenario->hs_scripts.address=scripts;
    scenario->hs_globals.address=globals;
    scripts[0].script_type=_hs_script_static;scripts[0].return_type=type;scripts[0].root_expression_index=0x10000;
    scripts[1].script_type=_hs_script_static;scripts[1].return_type=type;scripts[1].root_expression_index=NONE;
    block=(void *)((unsigned char *)scripts+0x50);block[0]=1;block[1]=(uint32_t)(uintptr_t)parameter;
    strcpy((char *)parameter,"argument");memcpy(parameter+32,&type,2);
    hs_syntax_data->data=nodes;hs_syntax_data->count=5;
    for(i=0;i<5;i++){nodes[i].datum_header=1;nodes[i].next_node_index=NONE;nodes[i].type=type;nodes[i].flags=9;}
    nodes[0].flags=0x1D;nodes[0].data=0;
    nodes[1].datum_header=0;
    nodes[2].flags=10;nodes[2].script_index=0;nodes[2].data=0x10003;
    nodes[3].type=_hs_function_name;nodes[3].next_node_index=0x10004;
    nodes[4].data=42;
    if(!strncmp(argv[1],"sleep_",6)) {
        int named=strcmp(argv[1],"sleep_self")!=0;
        struct hs_thread_datum saved;
        memset(&thread,0,sizeof(thread));memset(&other_thread,0,sizeof(other_thread));
        frame=(void *)(thread.stack_data+32);thread.stack=frame;frame->expression_index=0x10002;
        other_thread.sleep_until=123;nodes[4].data=1;
        if(!named)nodes[3].next_node_index=NONE;
        if(!strcmp(argv[1],"sleep_missing"))nodes[4].data=9;
        if(!strcmp(argv[1],"sleep_finished"))other_thread.sleep_until=NONE;
        mcc_sleep_forever_evaluate(0,0,TRUE);
        if(named) {
            CHECK(!returned && other_thread.sleep_until!=NONE-1);
            saved=thread;memset(&thread,0,sizeof(thread));thread=saved;
            frame->size=0;mcc_sleep_forever_evaluate(0,0,FALSE);
            if(!strcmp(argv[1],"sleep_missing"))CHECK(other_thread.sleep_until==123);
            else if(!strcmp(argv[1],"sleep_finished"))CHECK(other_thread.sleep_until==NONE);
            else {
                CHECK(other_thread.sleep_until==NONE-1 && other_thread.previous_sleep_until==123);
                CHECK(TEST_FLAG(other_thread.flags,_hs_thread_sleeping_bit));
                frame->size=0;mcc_sleep_forever_evaluate(0,0,TRUE);
                frame->size=0;mcc_sleep_forever_evaluate(0,0,FALSE);
                CHECK(other_thread.previous_sleep_until==123);
            }
            CHECK(thread.sleep_until==0);
        } else CHECK(thread.sleep_until==NONE-1 && !thread.flags);
        CHECK(returned && !errors);free(runtime.tags);return 0;
    }
    if(!strcmp(argv[1],"count")){block[0]=17;valid=0;}
    else if(!strcmp(argv[1],"type")){type=_hs_type_void;memcpy(parameter+32,&type,2);valid=0;}
    else if(!strcmp(argv[1],"pointer")){block[1]=1;valid=0;}
    else if(!strcmp(argv[1],"slot")){nodes[0].data=1;valid=0;}
    else if(!strcmp(argv[1],"orphan")){scripts[0].root_expression_index=NONE;valid=0;}
    else if(!strcmp(argv[1],"flags")){nodes[0].flags=0x19;valid=0;}
    else if(!strcmp(argv[1],"call_arity")){nodes[3].next_node_index=NONE;valid=0;}
    else if(!strcmp(argv[1],"call_type")){nodes[4].type=_hs_type_boolean;valid=0;}
    else if(!strcmp(argv[1],"predicate")){nodes[3].type=_hs_type_long_integer;valid=0;}
    else if(!strcmp(argv[1],"script_kind")){scripts[0].script_type=_hs_script_startup;valid=0;}
    else if(!strcmp(argv[1],"global_scope")){scenario->hs_globals.count=1;globals[0].initialization_expression_index=0x10000;valid=0;}
    else if(!strcmp(argv[1],"lists")){type=_hs_type_object_list;memcpy(parameter+32,&type,2);nodes[0].type=nodes[4].type=type;}
    else if(!strcmp(argv[1],"ai_cast")){type=_hs_type_ai;memcpy(parameter+32,&type,2);nodes[4].type=type;nodes[0].type=_hs_type_unit;}
    else if(!strcmp(argv[1],"global_capacity")||!strcmp(argv[1],"global_overflow")) {
        scenario->hs_globals.count=581+(!strcmp(argv[1],"global_overflow"));
        for(i=0;i<scenario->hs_globals.count;i++)globals[i].initialization_expression_index=NONE;
        valid=scenario->hs_globals.count==581;
    } else if(!strcmp(argv[1],"maximum")) {
        block[0]=16;hs_syntax_data->count=20;
        for(i=0;i<16;i++) {
            strcpy((char *)parameter+i*36,"argument");memcpy(parameter+i*36+32,&type,2);
            nodes[i+4].datum_header=1;nodes[i+4].flags=9;nodes[i+4].type=type;
            nodes[i+4].next_node_index=i==15?NONE:0x10005+i;
        }
    } else if(!strcmp(argv[1],"high_node")) {
        hs_syntax_data->count=32767;
        nodes[32766]=nodes[0];nodes[0].datum_header=0;
        scripts[0].root_expression_index=0x17FFE;
    } else if(!strcmp(argv[1],"negative_count")) {
        hs_syntax_data->count=(short)32768;valid=0;
    } else if(!strcmp(argv[1],"disabled"))disabled=1;
    loaded=0;CHECK(mcc_parameters_prepare(&runtime,scenario,hs_syntax_data)==valid);loaded=1;
    if(!valid){CHECK(!mcc_parameters_signature(0));free(runtime.tags);return 0;}
    if(!strcmp(argv[1],"high_node")) {
        struct hs_thread_datum saved;
        CHECK(mcc_parameter_type(nodes+32766,&owner)==type&&owner==0);
        CHECK(mcc_parameter_compile(nodes+32766)==1);
        memset(&thread,0,sizeof(thread));frame=(void *)(thread.stack_data+32);
        frame->previous=(void *)thread.stack_data;frame->expression_index=0x10002;thread.stack=frame;
        CHECK(mcc_hs_script_evaluate(0,0,TRUE));
        saved=thread;memset(&thread,0,sizeof(thread));thread=saved;
        frame->size=0;CHECK(mcc_hs_script_evaluate(0,0,FALSE));
        CHECK(((struct mcc_hs_arguments *)frame->data)->result==42);
        frame->size=0;CHECK(mcc_hs_script_evaluate(0,0,FALSE));
        CHECK(returned==1&&last_result==42&&!errors);
        goto done;
    }
    CHECK(mcc_parameter_type(nodes,&owner)==type&&owner==0);
    CHECK(mcc_parameter_compile(nodes)==1);
    if(!strcmp(argv[1],"maximum")){CHECK(mcc_parameters_signature(0)->count==16);goto done;}
    if(!strcmp(argv[1],"global_capacity"))goto done;
    if(!strcmp(argv[1],"metadata")||!strcmp(argv[1],"ai_cast")){CHECK(mcc_parameters_signature(0)->count==1);goto done;}
    if(!strcmp(argv[1],"legacy")){loaded=0;CHECK(mcc_parameter_type(nodes,NULL)==-1);CHECK(!mcc_parameters_signature(0));goto done;}
    memset(&thread,0,sizeof(thread));frame=(void *)(thread.stack_data+32);
    frame->previous=(void *)thread.stack_data;frame->expression_index=0x10002;thread.stack=frame;
    CHECK(mcc_hs_script_evaluate(0,0,TRUE));
    state=(void *)frame->data;CHECK(state->values[0]==42&&state->phase==0);
    if(!strcmp(argv[1],"snapshot")) {
        unsigned char snapshot[sizeof(thread)];memcpy(snapshot,&thread,sizeof(thread));
        memset(&thread,0,sizeof(thread));memcpy(&thread,snapshot,sizeof(thread));
    }
    frame->size=0;CHECK(mcc_hs_script_evaluate(0,0,FALSE));
    if(disabled){CHECK(returned==1&&last_result==NONE&&!refs);goto done;}
    CHECK(state->phase==1&&state->result==42);
    if(!strcmp(argv[1],"lists")){CHECK(refs==1);mcc_hs_release_thread(0);CHECK(!refs);state->references=0;}
    if(!strcmp(argv[1],"nested_scope")||!strcmp(argv[1],"recursive_arguments")) {
        struct hs_stack_frame *nested=(void *)(frame->data+frame->size+16);
        struct mcc_hs_arguments *other;
        long *value;short actual;
        nested->previous=frame;nested->expression_index=0x10002;thread.stack=nested;
        nested->size=0;CHECK(mcc_hs_script_evaluate(0,0,TRUE));other=(void *)nested->data;
        other->values[0]=84;
        if(!strcmp(argv[1],"nested_scope"))other->phase=1;
        value=mcc_hs_parameter_address(0,nodes,&actual);
        CHECK(value&&*value==(!strcmp(argv[1],"nested_scope")?84:42));thread.stack=frame;
    }
    if(!strcmp(argv[1],"set")) {
        struct hs_stack_frame *setter=(void *)(frame->data+frame->size+16);
        setter->previous=frame;setter->expression_index=0x10003;setter->size=0;thread.stack=setter;
        nodes[0].next_node_index=0x10004;nodes[4].data=77;
        CHECK(mcc_parameter_set_valid(nodes,nodes+4)==1);
        CHECK(mcc_hs_parameter_set(0,nodes,TRUE));setter->size=0;
        CHECK(mcc_hs_parameter_set(0,nodes,FALSE));CHECK(state->values[0]==77);thread.stack=frame;
        returned=0;
    }
    if(!strcmp(argv[1],"overflow")) {
        struct hs_stack_frame *overflow=(void *)(thread.stack_data+480);
        overflow->size=0;overflow->previous=frame;overflow->expression_index=0x10002;thread.stack=overflow;
        CHECK(mcc_hs_script_evaluate(0,0,TRUE));CHECK(errors==1);mcc_hs_release_thread(0);errors=0;thread.stack=frame;
    }
    frame->size=0;CHECK(mcc_hs_script_evaluate(0,0,FALSE));CHECK(returned==1&&last_result==42&&!errors&&!refs);
done:
    mcc_parameters_dispose();CHECK(!mcc_parameters_signature(0));free(runtime.tags);return 0;
}
