#include "cseries.h"
#include "native_hud_messages.h"
#include "native_hud_message_layout.h"
#include <stdlib.h>
#include <string.h>
#undef malloc
#undef free
#undef memcpy
#undef memset

#define HUD_MESSAGE_TEXT_GROUP UINT32_C(0x686d7420)
#define HUD_MESSAGE_ELEMENT_LIMIT 8192U
#define HUD_MESSAGE_LIMIT 1024U
#define HUD_MESSAGE_TEXT_LIMIT 131072U

static uint32_t read_u32(const unsigned char *p)
{ return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static uint16_t read_u16(const unsigned char *p)
{ return (uint16_t)(p[0]|(uint16_t)p[1]<<8); }
void mac_native_hud_message_text_owner_free(void *owner)
{ free(owner); }
void mac_native_hud_message_text_tag_free(struct mac_native_object_tag *tag)
{
    if(!tag)return;
    free(tag->owner);
    memset(tag,0,sizeof(*tag));
}

int mac_native_hud_message_text_decode(const struct mac_retail_cache *cache,
    uint32_t handle,struct mac_native_object_tag *out,const char **error)
{
    struct mac_retail_tag tag;
    struct mac_native_hud_message_text *root;
    struct mac_native_hud_state_message_element *elements;
    struct mac_native_hud_state_message *messages;
    unsigned char *owner,*text_native;
    const unsigned char *disk,*element_disk,*message_disk,*text_disk;
    uint32_t element_count,message_count,text_bytes,i;
    size_t total,element_bytes,message_bytes,message_offset;
    const char *failure="native HUD message tag metadata is invalid";
    if(error)*error=NULL;
    if(!cache||!out||out->root||!mac_retail_tag_get(cache,handle&65535U,&tag)||
       tag.handle!=handle||tag.group!=HUD_MESSAGE_TEXT_GROUP)goto invalid;
    disk=mac_retail_tag_span(cache,tag.address,sizeof(*root));
    if(!disk){failure="native HUD message root range is invalid";goto invalid;}
    text_bytes=read_u32(disk);
    element_count=read_u32(disk+20);
    message_count=read_u32(disk+32);
    if(text_bytes>HUD_MESSAGE_TEXT_LIMIT||(text_bytes&1)||
       element_count>HUD_MESSAGE_ELEMENT_LIMIT||message_count>HUD_MESSAGE_LIMIT) {
        failure="native HUD message root count or data size is invalid";goto invalid;
    }
    element_bytes=(size_t)element_count*sizeof(*elements);
    message_bytes=(size_t)message_count*sizeof(*messages);
    element_disk=element_count?mac_retail_tag_span(cache,read_u32(disk+24),element_bytes):NULL;
    message_disk=message_count?mac_retail_tag_span(cache,read_u32(disk+36),message_bytes):NULL;
    text_disk=text_bytes?mac_retail_tag_span(cache,read_u32(disk+12),text_bytes):NULL;
    if((element_count&&!element_disk)||(message_count&&!message_disk)||(text_bytes&&!text_disk)) {
        failure="native HUD message block or text-data range is invalid";goto invalid;
    }
    for(i=0;i<element_count;++i)if(element_disk[i*2]>1) {
        failure="native HUD message element type is invalid";goto invalid;
    }
    for(i=0;i<message_count;++i) {
        const unsigned char *m=message_disk+(size_t)i*sizeof(*messages);
        uint32_t start=read_u16(m+0x22),count=m[0x24],text=read_u16(m+0x20),j;
        if(count>8||start>element_count||count>element_count-start) {
            failure="native HUD message element index range is invalid";goto invalid;
        }
        if(text>text_bytes/2) {failure="native HUD message text index is invalid";goto invalid;}
        if(!text_bytes&&count) {
            failure="native HUD message has elements without text data";goto invalid;
        }
        for(j=0;j<count;++j) {
            const unsigned char *element=element_disk+(size_t)(start+j)*2;
            if(element[0]==0) {
                uint32_t characters=text_bytes/2;
                if(text>=characters||element[1]>characters-text ||
                   read_u16(text_disk+(text+(element[1]?element[1]-1:0))*2)!=0) {
                    failure="native HUD message text run exceeds text data";goto invalid;
                }
                text+=element[1];
            }
        }
    }
    message_offset=(sizeof(*root)+element_bytes+3U)&~(size_t)3U;
    total=message_offset+message_bytes+text_bytes;
    owner=(unsigned char *)malloc(total?total:sizeof(*root));
    if(!owner){if(error)*error="native HUD message allocation failed";return 0;}
    memset(owner,0,total?total:sizeof(*root));
    root=(struct mac_native_hud_message_text *)owner;
    elements=(struct mac_native_hud_state_message_element *)(owner+sizeof(*root));
    messages=(struct mac_native_hud_state_message *)(owner+message_offset);
    text_native=(unsigned char *)messages+message_bytes;
    root->text_data.size=(long)text_bytes;
    root->text_data.address=text_bytes?text_native:NULL;
    root->elements.count=(long)element_count;root->elements.address=element_count?elements:NULL;
    root->messages.count=(long)message_count;root->messages.address=message_count?messages:NULL;
    if(element_count)memcpy(elements,element_disk,element_bytes);
    for(i=0;i<message_count;++i) {
        const unsigned char *m=message_disk+(size_t)i*sizeof(*messages);
        memcpy(messages[i].name,m,32);
        messages[i].text_start_index=read_u16(m+0x20);
        messages[i].element_start_index=read_u16(m+0x22);
        messages[i].element_count=m[0x24];
        memcpy(messages[i].pad25,m+0x25,3);
        /* The trailing longs are storage-only fields and remain opaque. */
        memcpy(messages[i].unused28,m+0x28,sizeof(messages[i].unused28));
    }
    for(i=0;i<text_bytes/2;++i)
        ((uint16_t *)text_native)[i]=read_u16(text_disk+i*2);
    /* The map schema treats this tail as unused; retain it exactly. */
    memcpy(root->unused2C,disk+44,sizeof(root->unused2C));
    out->root=root;out->owner=owner;out->handle=handle;out->group=tag.group;
    out->root_bytes=sizeof(*root);out->bytes_owned=total;out->field_digest=0;
    out->converted_values=element_count+message_count+(text_bytes/2);
    return 1;
invalid:
    if(error)*error=failure;
    return 0;
}
