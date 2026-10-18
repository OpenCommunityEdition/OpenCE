/* Native equivalents for the two cache tag helpers retained by source users.
   There is deliberately no raw-cache fallback. */
#include "cseries.h"
#include "tag_files/tag_groups.h"
#include "cache/cache_files.h"
#include "engine_tags.h"
#include "engine_cache_files.h"
/* port: an Xbox map's own (cache_files.c, renamed by the native view) */
unsigned long tag_groups_checksum_retail_legacy(void);
void tag_reference_set_retail_legacy(struct tag_reference *reference,unsigned long group_tag,const char *name);
#include <stdio.h>
#include <stdlib.h>

unsigned long tag_groups_checksum(void)
{
    if(mac_cache_files_xbox_active())return tag_groups_checksum_retail_legacy();
    uint32_t checksum;
    if(!mac_engine_tags_checksum(&checksum)) {
        fprintf(stderr,"native tag registry: checksum requested outside a bound cache\n");
        abort();
    }
    return (unsigned long)checksum;
}

void tag_reference_set(struct tag_reference *reference,
    unsigned long group_tag,const char *name)
{
    uint32_t handle;
    const char *owned_name;
    size_t length;
    if(mac_cache_files_xbox_active()){tag_reference_set_retail_legacy(reference,group_tag,name);return;}
    if(!reference || !name || group_tag>UINT32_MAX) {
        fprintf(stderr,"native tag registry: invalid tag_reference_set arguments\n");
        abort();
    }
    for(length=0;length<=255 && name[length];++length) { }
    if(!length || length>255) {
        fprintf(stderr,"native tag registry: tag reference name is empty or too long\n");
        abort();
    }
    handle=mac_engine_tags_find_metadata((uint32_t)group_tag,name);
    owned_name=handle==UINT32_MAX?NULL:mac_engine_tags_name(handle);
    if(!owned_name) {
        fprintf(stderr,"native tag registry: reference target is absent from the bound map\n");
        abort();
    }
    reference->group_tag=group_tag;
    reference->name=(char *)owned_name;
    reference->name_length=(long)length;
    reference->index=(long)handle;
}
