#include "cseries.h"
#include "real_math.h"
#include "scenario/scenario_definitions.h"
#include "hs/hs_scenario_definitions.h"
#include "memory/data.h"
#include "hs/hs.h"
#include "native_scenario_scripts.h"
#undef malloc
#undef calloc
#undef free
#undef memcpy
#undef memset
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define NODE_COUNT 19001
#define MAXIMUM_SCRIPTS 512
#define MAXIMUM_GLOBALS 1024
#define SCRIPT_SIZE 92
#define GLOBAL_SIZE 92
/* a syntax node's flags (hs_runtime.c) */
#define NODE_PRIMITIVE 1
#define NODE_SCRIPT 2
#define NODE_GLOBAL 4
#define NODE_PERMANENT 8
#define EXTERNAL_GLOBAL 0x8000
/* The game's tables, present in the session but not in every probe that links
   this file; without them active scripts are refused. */
extern short hs_find_function_by_name(char const *) __attribute__((weak));
extern short hs_find_global_by_name(char const *) __attribute__((weak));
#define SYNTAX_SIZE (56+20*NODE_COUNT)
#define WITNESS(name,test) typedef char name[(test)?1:-1]
WITNESS(syntax_header_size,sizeof(struct data_array)==56);
WITNESS(syntax_node_size,sizeof(struct hs_syntax_node)==20);
WITNESS(scenario_syntax_offset,offsetof(struct scenario,hs_syntax_data)==0x474);
WITNESS(scenario_strings_offset,offsetof(struct scenario,hs_string_constants)==0x488);
WITNESS(scenario_scripts_offset,offsetof(struct scenario,hs_scripts)==0x49c);
WITNESS(scenario_globals_offset,offsetof(struct scenario,hs_globals)==0x4a8);
WITNESS(scenario_references_offset,offsetof(struct scenario,hs_references)==0x4b4);
WITNESS(script_size,sizeof(struct hs_script)==SCRIPT_SIZE);
WITNESS(global_size,sizeof(struct hs_global)==GLOBAL_SIZE);
#define MAXIMUM_REFERENCES 4096
#define REFERENCE_SIZE 40
struct script_storage {
    struct tag_data syntax,strings;
    struct hs_script *scripts;struct hs_global *globals;long script_count,global_count;
    /* the tags the scripts name (hs_references), for the session to load */
    uint32_t *reference_groups,*reference_handles;uint32_t reference_count;
};
/* Retail PC scripts name functions and engine globals by index into Halo PC's
   tables, which order them differently from this build's. Each compiled
   script keeps the names, so calls and engine globals are found again by
   name, as port/linux/game/custom_edition_scripts.c does for Custom Edition
   maps; anything this build lacks becomes a constant of its type. */
struct conversion {
    struct hs_syntax_node *nodes;long count;const char *strings;uint32_t string_size;
    long empty_string,renumbered,inert,missing;
};
static uint32_t read32(const unsigned char *p)
{ return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static short read16(const unsigned char *p)
{ unsigned short bits=(unsigned short)(p[0]|p[1]<<8);short result;memcpy(&result,&bits,2);return result; }
static long signed32(const unsigned char *p)
{ uint32_t bits=read32(p);long result;memcpy(&result,&bits,4);return result; }
static const char *script_string(const struct conversion *c,long offset)
{
    return offset>=0 && (uint32_t)offset<c->string_size &&
        memchr(c->strings+offset,0,c->string_size-(uint32_t)offset)?c->strings+offset:NULL;
}
static struct hs_syntax_node *script_node(const struct conversion *c,long index)
{
    long absolute=DATUM_INDEX_TO_ABSOLUTE_INDEX(index);
    return index!=NONE && absolute>=0 && absolute<c->count?&c->nodes[absolute]:NULL;
}
static void make_inert(struct conversion *c,struct hs_syntax_node *node,const char *kind,const char *name)
{
    long value;
    if(node->type>=_hs_type_void && node->type<=_hs_type_long_integer)value=0;
    else if(node->type==_hs_type_string && c->empty_string)value=c->empty_string;
    else if(HS_TYPE_IS_ENUM(node->type))value=0;
    else if(node->type!=_hs_type_string && node->type!=_hs_type_script && hs_type_valid(node->type))value=NONE;
    else {
        fprintf(stderr,"native scripts: the %s '%s' is missing and has no harmless value\n",kind,name);
        c->missing++;return;
    }
    if(c->inert<8)fprintf(stderr,"native scripts: the %s '%s' is missing here: it does nothing\n",kind,name);
    node->flags=(short)(NODE_PRIMITIVE|(node->flags&NODE_PERMANENT));
    node->constant_type=node->type;node->data=value;c->inert++;
}
static int engine_global_missing(const struct conversion *c,const struct hs_syntax_node *node)
{
    const char *name=script_string(c,node->string_offset);short designator;
    if(!name || (node->flags&(NODE_PRIMITIVE|NODE_GLOBAL))!=(NODE_PRIMITIVE|NODE_GLOBAL) ||
       !(node->short_value&EXTERNAL_GLOBAL))return 0;
    designator=hs_find_global_by_name(name);
    return designator==NONE || !(designator&EXTERNAL_GLOBAL);
}
static int function_call_convert(struct conversion *c,struct hs_syntax_node *call)
{
    struct hs_syntax_node *name_node=script_node(c,call->data),*argument;
    const char *name;short function_index;
    if(!name_node || name_node->type!=_hs_function_name)return 0;
    name=script_string(c,name_node->string_offset);if(!name)return 0;
    function_index=hs_find_function_by_name(name);
    argument=script_node(c,name_node->next_node_index);
    if(function_index==NONE)make_inert(c,call,"function",name);
    /* a set of a missing engine global writes nothing */
    else if(!strcmp(name,"set") && argument && engine_global_missing(c,argument))
        make_inert(c,call,"engine global",script_string(c,argument->string_offset));
    else if(call->function_index!=function_index) {
        call->function_index=function_index;name_node->function_index=function_index;c->renumbered++;
    }
    return 1;
}
static int engine_global_convert(struct conversion *c,struct hs_syntax_node *reference)
{
    const char *name=script_string(c,reference->string_offset);short designator;
    if(!name)return 0;
    designator=hs_find_global_by_name(name);
    if(designator==NONE || !(designator&EXTERNAL_GLOBAL))make_inert(c,reference,"engine global",name);
    else if(reference->short_value!=designator) {
        hs_slot_store_short(&reference->data,designator);c->renumbered++;
    }
    return 1;
}
/* String constants hold addresses in the map's string data; they are moved
   to the copy. Calls first, then engine globals, so that a set of a missing
   global is seen while its global is still a reference. */
static int nodes_convert(struct conversion *c,uint32_t strings_address,const char **error)
{
    long pass,i,stale=0;
    for(i=0;i<c->count;++i) {
        struct hs_syntax_node *node=&c->nodes[i];uint32_t offset;
        if(!node->datum_header || (node->flags&(NODE_PRIMITIVE|NODE_GLOBAL))!=NODE_PRIMITIVE ||
           node->type!=_hs_type_string)continue;
        offset=(uint32_t)node->data-strings_address;
        if(!script_string(c,(long)offset)) {
            /* Campaign levels keep a stale base in the node's address (the
               compiler's own, 0x02b67800 + offset in b30); the node's
               string offset is the string. */
            offset=(uint32_t)node->string_offset;
            if(!script_string(c,node->string_offset)) {if(error)*error="native script string constant";return 0;}
            stale++;
        }
        node->data=(long)(uintptr_t)(c->strings+offset);
    }
    if(stale)fprintf(stderr,"native scripts: %ld string constants name their string by offset\n",stale);
    for(pass=0;pass<2;++pass)for(i=0;i<c->count;++i) {
        struct hs_syntax_node *node=&c->nodes[i];int converted=1;
        if(!node->datum_header)continue;
        if(!pass && !(node->flags&(NODE_PRIMITIVE|NODE_SCRIPT)))converted=function_call_convert(c,node);
        else if(pass && (node->flags&(NODE_PRIMITIVE|NODE_GLOBAL))==(NODE_PRIMITIVE|NODE_GLOBAL) &&
                (node->short_value&EXTERNAL_GLOBAL))converted=engine_global_convert(c,node);
        if(!converted) {
            fprintf(stderr,"native scripts: syntax node %ld does not name what it uses\n",i);
            if(error)*error="native script node names";return 0;
        }
    }
    fprintf(stderr,"native scripts: %ld nodes, %ld renumbered, %ld made inert\n",c->count,c->renumbered,c->inert);
    if(c->missing) {if(error)*error="native scripts use missing functions or globals";return 0;}
    return 1;
}
void mac_native_scenario_scripts_free(struct mac_native_scenario_scripts *out)
{
    struct script_storage *s;
    if(!out||!out->state)return;s=out->state;
    free(s->syntax.address);free(s->strings.address);free(s->scripts);free(s->globals);
    free(s->reference_groups);free(s->reference_handles);free(s);out->state=NULL;
}
static const unsigned char *block_span(const struct mac_retail_cache *cache,const unsigned char *field,
    uint32_t maximum,uint32_t element_size,uint32_t *count)
{
    *count=read32(field);
    if(*count>maximum)return NULL;
    if(!*count)return field;
    return mac_retail_tag_span(cache,read32(field+4),*count*element_size);
}
int mac_native_scenario_scripts_decode(const struct mac_retail_cache *cache,
    struct mac_native_scenario_scripts *out,const char **error)
{
    const unsigned char *root,*syntax,*strings,*syntax_field,*strings_field,*scripts,*globals;
    struct script_storage *s;struct data_array *array;struct hs_syntax_node *nodes;
    struct conversion conversion;
    const unsigned char *references;
    uint32_t i,string_size,script_count,global_count,count,actual_count,first_free,reference_count;
    if(error)*error="native script output or cache";
    if(!out)return 0;out->state=NULL;
    if(!cache||!cache->tags)return 0;
    root=mac_retail_tag_span(cache,MAC_RETAIL_BASE+cache->scenario_offset,sizeof(struct scenario));
    if(!root)return 0;
    /* source files are not kept in retail maps; the tags scripts name
       (hs_references) are read below, for the session to load */
    if(read32(root+offsetof(struct scenario,hs_source_files))) {
        if(error)*error="retail script source files";return 0;
    }
    references=block_span(cache,root+offsetof(struct scenario,hs_references),MAXIMUM_REFERENCES,REFERENCE_SIZE,&reference_count);
    if(!references) {if(error)*error="native script reference block";return 0;}
    scripts=block_span(cache,root+offsetof(struct scenario,hs_scripts),MAXIMUM_SCRIPTS,SCRIPT_SIZE,&script_count);
    globals=block_span(cache,root+offsetof(struct scenario,hs_globals),MAXIMUM_GLOBALS,GLOBAL_SIZE,&global_count);
    if(!scripts || !globals) {if(error)*error="native script or global block";return 0;}
    syntax_field=root+offsetof(struct scenario,hs_syntax_data);
    strings_field=root+offsetof(struct scenario,hs_string_constants);
    string_size=read32(strings_field);
    if(read32(syntax_field)!=SYNTAX_SIZE || read32(syntax_field+4) || read32(strings_field+4) ||
       string_size<0x400 || string_size>0x100000 || (read32(syntax_field+12)&3)) {
        if(error)*error="native script data size or flags";return 0;
    }
    syntax=mac_retail_tag_span(cache,read32(syntax_field+12),SYNTAX_SIZE);
    strings=mac_retail_tag_span(cache,read32(strings_field+12),string_size);
    if(!syntax || !strings) {if(error)*error="native script data span";return 0;}
    first_free=(uint16_t)read16(syntax+44);count=(uint16_t)read16(syntax+46);actual_count=(uint16_t)read16(syntax+48);
    if(!memchr(syntax,0,32) || read16(syntax+32)!=NODE_COUNT || read16(syntax+34)!=20 ||
       syntax[36]!=1 || syntax[37]>1 || read32(syntax+40)!=UINT32_C(0x64407440) ||
       count>NODE_COUNT || actual_count>count || first_free>NODE_COUNT || !read16(syntax+50)) {
        if(error)*error="native script array header";return 0;
    }
    if((count || script_count || global_count) && (!hs_find_function_by_name || !hs_find_global_by_name)) {
        if(error)*error="active retail scripts need the game's function and global tables";return 0;
    }
    for(i=count;i<NODE_COUNT;++i)if(read16(syntax+56+20*i)) {
        if(error)*error="retail syntax node past the array's count";return 0;
    }
    s=calloc(1,sizeof(*s));if(!s){if(error)*error="native script allocation";return 0;}
    out->state=s;s->syntax.size=SYNTAX_SIZE;s->strings.size=(long)string_size;
    s->syntax.address=malloc(SYNTAX_SIZE);s->strings.address=malloc(string_size);
    s->scripts=calloc(script_count?script_count:1,sizeof(*s->scripts));
    s->globals=calloc(global_count?global_count:1,sizeof(*s->globals));
    if(!s->syntax.address||!s->strings.address||!s->scripts||!s->globals) {
        mac_native_scenario_scripts_free(out);if(error)*error="native script storage allocation";return 0;
    }
    array=s->syntax.address;memset(array,0,sizeof(*array));memcpy(array->name,syntax,32);
    array->maximum_count=NODE_COUNT;array->size=20;array->valid=syntax[36];array->identifier_zero_invalid=syntax[37];
    array->signature=read32(syntax+40);array->first_free_absolute_index=(short)first_free;
    array->count=(short)count;array->actual_count=(short)actual_count;
    array->next_identifier=read16(syntax+50);array->data=array+1;
    nodes=array->data;
    for(i=0;i<NODE_COUNT;++i) {
        const unsigned char *p=syntax+56+20*i;
        nodes[i].datum_header=read16(p);nodes[i].index=read16(p+2);nodes[i].type=read16(p+4);nodes[i].flags=read16(p+6);
        nodes[i].next_node_index=signed32(p+8);nodes[i].string_offset=signed32(p+12);nodes[i].data=signed32(p+16);
    }
    memcpy(s->strings.address,strings,string_size);
    if(reference_count) {
        s->reference_groups=calloc(reference_count,sizeof(*s->reference_groups));
        s->reference_handles=calloc(reference_count,sizeof(*s->reference_handles));
        if(!s->reference_groups||!s->reference_handles) {
            mac_native_scenario_scripts_free(out);if(error)*error="native script reference allocation";return 0;
        }
    }
    for(i=0;i<reference_count;++i) {
        /* struct hs_reference: 24 unused bytes, then a tag_reference (group,
           name address, name length, handle) */
        const unsigned char *p=references+REFERENCE_SIZE*i+24;
        uint32_t handle=read32(p+12);struct mac_retail_tag tag;
        if(handle!=UINT32_MAX && (!mac_retail_tag_get(cache,handle&65535,&tag) ||
           tag.handle!=handle || tag.group!=read32(p))) {
            mac_native_scenario_scripts_free(out);if(error)*error="native script reference does not name its tag";return 0;
        }
        s->reference_groups[i]=read32(p);s->reference_handles[i]=handle;
    }
    s->reference_count=reference_count;
    for(i=0;i<script_count;++i) {
        const unsigned char *p=scripts+SCRIPT_SIZE*i;struct hs_script *script=&s->scripts[i];
        if(!memchr(p,0,32)) {mac_native_scenario_scripts_free(out);if(error)*error="native script name";return 0;}
        memcpy(script->name,p,32);script->script_type=read16(p+32);script->return_type=read16(p+34);
        script->root_expression_index=signed32(p+36);
    }
    for(i=0;i<global_count;++i) {
        const unsigned char *p=globals+GLOBAL_SIZE*i;struct hs_global *global=&s->globals[i];
        if(!memchr(p,0,32)) {mac_native_scenario_scripts_free(out);if(error)*error="native script global name";return 0;}
        memcpy(global->name,p,32);global->type=read16(p+32);global->initialization_expression_index=signed32(p+40);
    }
    s->script_count=(long)script_count;s->global_count=(long)global_count;
    memset(&conversion,0,sizeof(conversion));
    conversion.nodes=nodes;conversion.count=(long)count;
    conversion.strings=s->strings.address;conversion.string_size=string_size;
    /* the string data ends with zeros (the console's room), read as "" */
    if(!conversion.strings[string_size-1])conversion.empty_string=(long)(uintptr_t)(conversion.strings+string_size-1);
    if(count && !nodes_convert(&conversion,read32(strings_field+12),error)) {
        mac_native_scenario_scripts_free(out);return 0;
    }
    if(error)*error=NULL;return 1;
}

/* port: the structure bsp a campaign level's own scripts switch to first
   (b40's and c40's startup scripts, d40's opening cinematic), read from the
   map's compiled scripts in the order they run: each startup script, into
   the scripts it calls, until a switch_bsp with a constant argument. The
   port builds one bsp per session and does not yet switch while playing, so
   the level starts in that bsp (its start points are not in it; scripts move
   the player). */
struct bsp_search {
    const unsigned char *nodes,*strings,*scripts;
    uint32_t string_size,script_count;
    unsigned char visited[MAXIMUM_SCRIPTS];
    int found,budget;
};
static const unsigned char *bsp_search_node(const struct bsp_search *q,long index)
{
    unsigned long i=(unsigned long)index&0xffff;
    return i<NODE_COUNT?q->nodes+20*i:NULL;
}
static const char *bsp_search_string(const struct bsp_search *q,long offset)
{
    return offset>0 && (uint32_t)offset<q->string_size &&
        memchr(q->strings+offset,0,q->string_size-(uint32_t)offset)?(const char *)q->strings+offset:NULL;
}
static int bsp_search_expression(struct bsp_search *q,long index,unsigned depth);
static int bsp_search_script(struct bsp_search *q,uint32_t script,unsigned depth)
{
    const unsigned char *p=q->scripts+SCRIPT_SIZE*script;
    if(depth>16 || q->visited[script] || q->budget<=0)return 0;
    q->visited[script]=1;
    return bsp_search_expression(q,signed32(p+36),depth+1);
}
static int bsp_search_expression(struct bsp_search *q,long index,unsigned depth)
{
    const unsigned char *node=bsp_search_node(q,index),*child;
    long next;unsigned guard=0;int first=1;const char *name=NULL;uint32_t i;
    if(!node || --q->budget<=0)return 0;
    if(read16(node+6)&NODE_PRIMITIVE)return 0;
    for(next=signed32(node+16);next!=-1 && (child=bsp_search_node(q,next))!=NULL && guard++<512;next=signed32(child+8)) {
        if(first) {
            name=bsp_search_string(q,signed32(child+12));first=0;
            if(name && !strcmp(name,"switch_bsp")) {
                const unsigned char *argument=bsp_search_node(q,signed32(child+8));
                if(argument && (read16(argument+6)&NODE_PRIMITIVE)) {
                    q->found=(int)(unsigned short)read16(argument+16);return 1;
                }
                return 0;
            }
            if(name)for(i=0;i<q->script_count;++i)
                if(!strcmp(name,(const char *)q->scripts+SCRIPT_SIZE*i)) {
                    if(bsp_search_script(q,i,depth))return 1;
                    break;
                }
            if(name && !strcmp(name,"wake"))return 0;
            continue;
        }
        if(!(read16(child+6)&NODE_PRIMITIVE) && bsp_search_expression(q,next,depth+1))return 1;
    }
    return 0;
}
int mac_native_scenario_scripts_start_bsp(const struct mac_retail_cache *cache,uint32_t *bsp)
{
    const unsigned char *root,*syntax,*strings,*scripts;
    struct bsp_search q;
    uint32_t script_count,string_size,i;
    if(!cache || !bsp || cache->scenario_offset>UINT32_MAX-MAC_RETAIL_BASE)return 0;
    root=mac_retail_tag_span(cache,MAC_RETAIL_BASE+cache->scenario_offset,0x5b0);
    if(!root || read16(root+60)!=0)return 0;
    script_count=read32(root+offsetof(struct scenario,hs_scripts));
    string_size=read32(root+offsetof(struct scenario,hs_string_constants));
    if(script_count>MAXIMUM_SCRIPTS || read32(root+offsetof(struct scenario,hs_syntax_data))!=SYNTAX_SIZE ||
       string_size<0x400 || string_size>0x100000)return 0;
    scripts=script_count?mac_retail_tag_span(cache,read32(root+offsetof(struct scenario,hs_scripts)+4),script_count*SCRIPT_SIZE):NULL;
    syntax=mac_retail_tag_span(cache,read32(root+offsetof(struct scenario,hs_syntax_data)+12),SYNTAX_SIZE);
    strings=mac_retail_tag_span(cache,read32(root+offsetof(struct scenario,hs_string_constants)+12),string_size);
    if(!scripts || !syntax || !strings)return 0;
    memset(&q,0,sizeof(q));
    q.nodes=syntax+56;q.strings=strings;q.scripts=scripts;q.string_size=string_size;q.script_count=script_count;
    q.found=-1;q.budget=100000;
    for(i=0;i<script_count;++i) {
        const unsigned char *p=scripts+SCRIPT_SIZE*i;
        if(!memchr(p,0,32) || read16(p+32)!=0)continue;
        q.visited[i]=1;
        if(bsp_search_expression(&q,signed32(p+36),0))break;
    }
    if(q.found<0 || (uint32_t)q.found>=cache->bsp_count)return 0;
    *bsp=(uint32_t)q.found;return 1;
}
int mac_native_scenario_scripts_attach(const struct mac_native_scenario_scripts *scripts,struct scenario *scenario)
{
    const struct script_storage *s;
    if(!scripts||!scripts->state||!scenario)return 0;s=scripts->state;
    scenario->hs_syntax_data=s->syntax;scenario->hs_string_constants=s->strings;
    memset(&scenario->hs_scripts,0,sizeof(scenario->hs_scripts));
    memset(&scenario->hs_globals,0,sizeof(scenario->hs_globals));
    scenario->hs_scripts.count=s->script_count;scenario->hs_scripts.address=s->scripts;
    scenario->hs_globals.count=s->global_count;scenario->hs_globals.address=s->globals;
    memset(&scenario->hs_references,0,sizeof(scenario->hs_references));
    memset(&scenario->hs_source_files,0,sizeof(scenario->hs_source_files));return 1;
}
static int contains(const struct tag_data *data,const void *pointer,size_t size)
{
    uintptr_t base=(uintptr_t)data->address,address=(uintptr_t)pointer;
    return pointer && address>=base && address-base<=(size_t)data->size && size<=(size_t)data->size-(address-base);
}
int mac_native_scenario_scripts_contains(const struct mac_native_scenario_scripts *scripts,const void *pointer,size_t size)
{
    const struct script_storage *s;
    if(!scripts||!scripts->state)return 0;s=scripts->state;
    return contains(&s->syntax,pointer,size)||contains(&s->strings,pointer,size);
}
int mac_native_scenario_scripts_references(const struct mac_native_scenario_scripts *scripts,
    uint32_t *groups,uint32_t *handles,uint32_t capacity,uint32_t *count)
{
    const struct script_storage *s;uint32_t i;
    if(count)*count=0;
    if(!scripts||!scripts->state||!count)return 0;s=scripts->state;
    if(s->reference_count>capacity)return 0;
    for(i=0;i<s->reference_count;++i) {
        if(groups)groups[i]=s->reference_groups[i];
        if(handles)handles[i]=s->reference_handles[i];
    }
    *count=s->reference_count;return 1;
}
