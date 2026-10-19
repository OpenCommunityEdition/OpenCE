/* Native engine tag entry points. This registry has no raw-cache fallback. */
#include "cseries.h"
#include "tag_files/tag_groups.h"
#include "tag_files/tag_files.h"
#include "engine_tags.h"
#include "cache/cache_files.h"
#include "engine_world.h"
#include "structures/structure_bsp_definitions.h"
#include "mac_flags.h"
#include "engine_cache_files.h"
#include <stdio.h>
#include <stdlib.h>
static struct mac_native_tags *active_tags;
/* (HALO_MAC_PERF_TAGS) tag_get's answers kept: the registry's entries are
   336 bytes each, and looking one up twice a call (once to resolve the
   handle, once to fetch the root) was most of the cost of the game's many
   tag_get calls. Discarded when the registry's epoch changes. */
extern unsigned long mac_native_tags_epoch;
#define TAG_MEMO_SLOTS 512U
static struct tag_memo_slot { unsigned long group,handle;const void *root; } tag_memo[TAG_MEMO_SLOTS];
static unsigned long tag_memo_epoch;
static const void *tag_memo_state;
static int tag_memo_enabled=-1;
/* port: an Xbox (version 5) map's tags are the engine's own (cache_files.c,
   as scenario_tags_load reads them in place); the cache's definitions, which
   tools/mac_cache_files_native_view.py renames, answer for them */
extern int mac_cache_files_xbox_flag; /* port: engine_cache_files.c; read without a call, as tag_get is called most of all */
#define XBOX() (mac_cache_files_xbox_flag)
void *tag_get_retail_legacy(long group,long handle);
char *tag_get_name_retail_legacy(long handle);
unsigned long tag_get_group_tag_retail_legacy(long handle);
long tag_loaded_retail_legacy(long group,const char *name);
boolean tag_index_is_group_retail_legacy(long handle,long group);
void tag_iterator_new_retail_legacy(struct tag_iterator *iterator,long group);
long tag_iterator_next_retail_legacy(struct tag_iterator *iterator);
/* cache_files.c accepts an unsalted absolute index only in the nonnegative
   short range. A salted handle must match exactly; never mask arbitrary high
   bits into an index. */
static uint32_t resolve_engine_handle(uint32_t handle)
{
    if(handle==UINT32_MAX)return UINT32_MAX;
    if(mac_native_tags_name(active_tags,handle))return handle;
    if(handle<=0x7fffU) {
        uint32_t published=mac_native_tags_handle_at_index(active_tags,handle);
        if(published!=UINT32_MAX)return published;
    }
    return handle;
}
int mac_engine_tags_bind(struct mac_native_tags *tags)
{
    if(active_tags || !tags || !tags->state)return 0;
    active_tags=tags;return 1;
}
void mac_engine_tags_unbind(void) { active_tags=NULL; }
struct mac_native_tags *mac_engine_tags_active(void) { return active_tags; }
int mac_engine_tags_checksum(uint32_t *out)
{ return mac_native_tags_checksum(active_tags,out); }
uint32_t mac_engine_tags_find_metadata(uint32_t group,const char *name)
{ return mac_native_tags_find_metadata(active_tags,group,name); }
const char *mac_engine_tags_name(uint32_t handle)
{ return mac_native_tags_name(active_tags,resolve_engine_handle(handle)); }
void *tag_get(long group,long handle)
{
    const char *error;
    if(XBOX())return tag_get_retail_legacy(group,handle);
    /* port: a structure bsp is the native world's, not a native tag (the
       scenario's BSP switch asks for it by the reference's tag index) */
    if(group==STRUCTURE_BSP_TAG) {
        struct structure_bsp *bsp=mac_engine_world_structure_for_handle(handle);
        if(bsp)return bsp;
    }
    uint32_t resolved;
    const void *root;
    struct tag_memo_slot *memo=NULL;
    if(tag_memo_enabled<0)tag_memo_enabled=mac_perf_flag("HALO_MAC_PERF_TAGS");
    if(tag_memo_enabled&&active_tags) {
        if(tag_memo_epoch!=mac_native_tags_epoch||tag_memo_state!=active_tags->state) {
            memset(tag_memo,0,sizeof(tag_memo));
            tag_memo_epoch=mac_native_tags_epoch;tag_memo_state=active_tags->state;
        }
        memo=&tag_memo[(((unsigned long)handle*2654435761UL)>>7^(unsigned long)group)%TAG_MEMO_SLOTS];
        if(memo->root&&memo->handle==(unsigned long)handle&&memo->group==(unsigned long)group)
            return (void *)memo->root;
    }
    resolved=resolve_engine_handle((uint32_t)handle);
    root=mac_native_tags_get(active_tags,(uint32_t)group,resolved,1,&error);
    if(root&&memo){memo->group=(unsigned long)group;memo->handle=(unsigned long)handle;memo->root=root;}
    if(!root) {
        const char *name=mac_native_tags_name(active_tags,resolved);
        fprintf(stderr,"native tag %08lx group %08lx (%s): %s\n",
            (unsigned long)handle,(unsigned long)group,name?name:"unknown handle",error);
        abort();
    }
    return (void *)root;
}
char *tag_get_name(long handle)
{
    if(XBOX())return tag_get_name_retail_legacy(handle);
    uint32_t resolved=resolve_engine_handle((uint32_t)handle);
    const char *name=mac_native_tags_name(active_tags,resolved);
    if(!name) { fprintf(stderr,"invalid native tag name handle %08lx\n",(unsigned long)handle);abort(); }
    return (char *)name;
}

unsigned long tag_get_group_tag(long handle)
{
    if(XBOX())return tag_get_group_tag_retail_legacy(handle);
    uint32_t group=mac_native_tags_group(active_tags,resolve_engine_handle((uint32_t)handle));
    if(group==UINT32_MAX) { fprintf(stderr,"unloaded native tag group %08lx\n",(unsigned long)handle);abort(); }
    return (unsigned long)group;
}
long tag_loaded(long group,const char *name)
{ if(XBOX())return tag_loaded_retail_legacy(group,name);
  return (long)mac_native_tags_find(active_tags,(uint32_t)group,name); }

boolean tag_index_is_group(long handle,long group)
{
    if(XBOX())return tag_index_is_group_retail_legacy(handle,group);
    uint32_t actual=mac_native_tags_group(active_tags,resolve_engine_handle((uint32_t)handle));
    return mac_native_tag_group_matches(actual,(uint32_t)group)?TRUE:FALSE;
}
void tag_iterator_new(struct tag_iterator *iterator,long group)
{
    if(XBOX()){tag_iterator_new_retail_legacy(iterator,group);return;}
    memset(iterator,0,sizeof(*iterator));
    iterator->group_tag=group;
}
long tag_iterator_next(struct tag_iterator *iterator)
{
    if(XBOX())return tag_iterator_next_retail_legacy(iterator);
    uint32_t cursor,result;
    if(!iterator || iterator->absolute_index<0)return NONE;
    cursor=(uint32_t)iterator->absolute_index;
    result=mac_native_tags_next(active_tags,(uint32_t)iterator->group_tag,&cursor);
    iterator->absolute_index=(long)cursor;
    return (long)result;
}
