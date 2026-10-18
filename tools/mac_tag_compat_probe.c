/* Exercise authentic native replacements for the two retained cache helpers
   inside a real bound v7 session. */
#include "cseries.h"
#include "tag_files/tag_groups.h"
#include "cache/cache_files.h"
#include "engine_session.h"
#include "engine_tags.h"
#include <string.h>

int mac_tag_compat_probe(const struct mac_engine_session *session)
{
    const struct mac_retail_cache *cache;
    struct tag_reference reference;
    const char *name,*owned_name;
    void *full_root,*compact_root;
    uint32_t index,published,group,cursor=0;
    uint32_t handle;
    if(!session)return 0;
    cache=mac_engine_session_cache(session);
    if(!cache || tag_groups_checksum()!=(unsigned long)cache->tag_checksum)return 0;
    handle=cache->scenario_handle;
    name=mac_engine_tags_name(handle);
    if(!name || !*name)return 0;
    /* The scenario root is installed later by game_load. Select an already
       published native root for the compact-index adapter check. */
    published=mac_native_tags_next(&session->assets.tags,UINT32_MAX,&cursor);
    if(published==UINT32_MAX)return 0;
    group=mac_native_tags_group(&session->assets.tags,published);
    index=published&UINT32_C(0xffff);
    if(index>UINT32_C(0x7fff))return 0;
    full_root=tag_get(group,(long)published);
    compact_root=tag_get(group,(long)index);
    if(full_root!=compact_root || tag_get_name((long)published)!=tag_get_name((long)index) ||
       tag_get_group_tag((long)published)!=tag_get_group_tag((long)index) ||
       !tag_index_is_group((long)index,group) ||
       tag_index_is_group((long)index,UINT32_C(0xdeadbeef)) ||
       tag_index_is_group((long)(published^UINT32_C(0x00010000)),group) ||
       tag_index_is_group(NONE,group) ||
       tag_index_is_group(0x8000L,group) ||
       tag_index_is_group((long)(uint32_t)UINT32_C(0xffff8000),group))return 0;
    memset(&reference,0,sizeof(reference));
    tag_reference_set(&reference,UINT32_C(0x73636e72),name);
    owned_name=mac_engine_tags_name((uint32_t)reference.index);
    return reference.group_tag==UINT32_C(0x73636e72) &&
        reference.index==(long)handle && reference.name_length==(long)strlen(name) &&
        owned_name==reference.name && strcmp(owned_name,name)==0;
}
