#include "cseries.h"
#include "native_interface_assets.h"
#include "native_hud_messages.h"
#include "native_grenade_hud_layout.h"
#include "native_looping_sound_tags.h"
#include "native_sound_tags.h"
#include "sound/sound_definitions.h"
#include "interface/unit_hud_interface_definition.h"
#include "interface/weapon_hud_interface_definition.h"
#include "interface/hud_definitions.h"
#include "native_hud_component_layout.h"
#include <stdlib.h>
#include <string.h>
#undef calloc
#undef free
#undef memset

struct interface_sound_queue {
    uint32_t *handles,*groups;
    unsigned count,capacity;
    const struct mac_retail_cache *cache;
    const void *bitmaps,*sounds;
    size_t bitmap_bytes,sound_bytes;
    struct mac_actor_assets *assets;
    const char *failure;
};
static int interface_reference(void *opaque,uint32_t group,uint32_t handle)
{
    struct interface_sound_queue *q=opaque;uint32_t loaded;unsigned i;
    /* These families are already converted and owned by the root pass. */
    if(group==UINT32_C(0x666f6e74)||group==UINT32_C(0x75737472)||
       group==UINT32_C(0x68756467)||group==UINT32_C(0x68756423)||
       group==UINT32_C(0x756e6869)||group==UINT32_C(0x77706869)||
       group==UINT32_C(0x67726869)) {
        loaded=mac_native_tags_group(&q->assets->tags,handle);
        if(loaded!=group){q->failure="HUD reference is not present in the owned interface registry";return 0;}
        return 1;
    }
    if(group==UINT32_C(0x6269746d)) {
        if(!q->bitmaps||!q->bitmap_bytes) {q->failure="HUD bitmap dependency requires bitmaps.map";return 0;}
        if(!mac_actor_assets_load_bitmap(q->cache,q->bitmaps,q->bitmap_bytes,q->assets,handle,&q->failure))return 0;
        return 1;
    }
    if(group!=UINT32_C(0x736e6421)&&group!=UINT32_C(0x6c736e64)&&
       group!=UINT32_C(0x686d7420)) {
        q->failure="HUD dependency has an unsupported tag family";
        return 0;
    }
    loaded=mac_native_tags_group(&q->assets->tags,handle);
    if(loaded!=UINT32_MAX) {
        if(loaded!=group){q->failure="HUD sound dependency handle family mismatch";return 0;}
    }
    for(i=0;i<q->count;++i)if(q->handles[i]==handle) {
        if(q->groups[i]!=group){q->failure="HUD sound dependency duplicate family mismatch";return 0;}
        return 1;
    }
    if(q->count>=q->capacity){q->failure="HUD sound dependency count exceeds tag table";return 0;}
    q->handles[q->count]=handle;q->groups[q->count]=group;++q->count;return 1;
}
static int interface_root_references(struct interface_sound_queue *q,uint32_t group)
{
    uint32_t cursor=0,handle;size_t size;
    if(group==UINT32_C(0x68756467))size=sizeof(struct hud_globals_definition);
    else if(group==UINT32_C(0x68756423))size=sizeof(struct hud_number_definition);
    else if(group==UINT32_C(0x756e6869))size=sizeof(struct unit_hud_interface_definition);
    else if(group==UINT32_C(0x67726869))size=sizeof(struct grenade_hud_interface_definition);
    else size=sizeof(struct weapon_hud_interface_definition);
    while((handle=mac_native_tags_next(&q->assets->tags,group,&cursor))!=UINT32_MAX) {
        const void *root=mac_native_tags_get(&q->assets->tags,group,handle,size,&q->failure);
        struct mac_native_object_tag view;
        if(!root)return 0;
        memset(&view,0,sizeof(view));view.root=(void *)root;view.handle=handle;
        view.group=group;view.root_bytes=size;
        if(!mac_native_object_visit_references(&view,interface_reference,q))return 0;
    }
    return 1;
}
static int interface_sound_dependencies(struct interface_sound_queue *q)
{
    unsigned cursor;
    for(cursor=0;cursor<q->count;++cursor) {
        uint32_t handle=q->handles[cursor],group=q->groups[cursor],loaded;
        const char *why=NULL;struct mac_native_object_tag tag;const void *root;
        memset(&tag,0,sizeof(tag));
        loaded=mac_native_tags_group(&q->assets->tags,handle);
        if(loaded!=UINT32_MAX) {
            if(loaded!=group){q->failure="HUD sound dependency family mismatch";return 0;}
            root=mac_native_tags_get(&q->assets->tags,group,handle,
                group==UINT32_C(0x736e6421)?sizeof(struct sound_definition):
                group==UINT32_C(0x6c736e64)?sizeof(struct looping_sound_definition):0x80,&why);
            if(!root){q->failure=why?why:"HUD sound dependency root is unavailable";return 0;}
            tag.root=(void *)root;tag.root_bytes=
                group==UINT32_C(0x736e6421)?sizeof(struct sound_definition):
                group==UINT32_C(0x6c736e64)?sizeof(struct looping_sound_definition):0x80;
            tag.group=group;tag.handle=handle;
        } else {
            if(group==UINT32_C(0x736e6421)) {
                if(!q->sounds||!q->sound_bytes){q->failure="HUD sound dependency requires sounds.map";return 0;}
                if(!mac_native_sound_decode(q->cache,handle,q->sounds,q->sound_bytes,&tag,&why)) {
                    q->failure=why?why:"HUD sound decode failed";return 0;
                }
            } else if(group==UINT32_C(0x6c736e64)) {
                if(!mac_native_looping_sound_decode(q->cache,handle,&tag,&why)) {
                    q->failure=why?why:"HUD looping-sound decode failed";return 0;
                }
            } else if(!mac_native_hud_message_text_decode(q->cache,handle,&tag,&why)) {
                q->failure=why?why:"HUD message-text decode failed";return 0;
            }
            if(!mac_native_tags_publish(&q->assets->tags,handle,group,tag.root,tag.root_bytes,
                tag.bytes_owned,tag.owner,
                group==UINT32_C(0x686d7420)?mac_native_hud_message_text_owner_free:mac_native_object_owner_free,&why)) {
                if(group==UINT32_C(0x686d7420))mac_native_hud_message_text_tag_free(&tag);
                else mac_native_object_tag_free(&tag);
                q->failure=why?why:"HUD sound publication failed";return 0;
            }
            ++q->assets->published_count;
        }
        if(group!=UINT32_C(0x686d7420)&&
           !mac_native_object_visit_references(&tag,interface_reference,q))return 0;
    }
    return 1;
}
int mac_native_interface_assets_load(const struct mac_retail_cache *cache,
    const void *bitmaps,size_t bitmap_bytes,const void *sounds,size_t sound_bytes,
    struct mac_actor_assets *assets,const char **error)
{
    struct interface_sound_queue q;
    if(error)*error=NULL;
    if(!cache||!assets||!assets->tags.state){if(error)*error="native interface registry is absent";return 0;}
    if(!mac_native_interface_assets_load_roots(cache,&assets->tags,error))return 0;
    memset(&q,0,sizeof(q));q.capacity=cache->tag_count;q.cache=cache;q.bitmaps=bitmaps;
    q.bitmap_bytes=bitmap_bytes;q.sounds=sounds;q.sound_bytes=sound_bytes;q.assets=assets;
    if(!q.capacity||q.capacity>65536U){if(error)*error="HUD dependency table count invalid";return 0;}
    q.handles=calloc(q.capacity,sizeof(*q.handles));q.groups=calloc(q.capacity,sizeof(*q.groups));
    if(!q.handles||!q.groups){free(q.handles);free(q.groups);if(error)*error="HUD dependency allocation failed";return 0;}
    if(!interface_root_references(&q,UINT32_C(0x68756467))||
       !interface_root_references(&q,UINT32_C(0x68756423))||
       !interface_root_references(&q,UINT32_C(0x756e6869))||
       !interface_root_references(&q,UINT32_C(0x77706869))||
       !interface_root_references(&q,UINT32_C(0x67726869))||
       !interface_sound_dependencies(&q)) {
        if(error)*error=q.failure?q.failure:"HUD dependency closure failed";
        free(q.handles);free(q.groups);return 0;
    }
    free(q.handles);free(q.groups);return 1;
}
