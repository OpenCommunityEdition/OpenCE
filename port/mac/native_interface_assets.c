#include "cseries.h"
#include "text/text_group.h"
#include "native_interface_assets.h"
#include "native_hud_tags.h"
#include "native_font_tags.h"
#include "native_grenade_hud_layout.h"
#include <stdlib.h>
#include <string.h>
#undef malloc
#undef free
#undef memcpy
#undef memset

typedef char native_text_root_size[sizeof(struct string_list)==12?1:-1];
typedef char native_text_entry_size[sizeof(struct string_list_entry)==20?1:-1];
typedef char native_text_address_offset[offsetof(struct tag_data,address)==12?1:-1];
typedef char native_text_wchar_size[sizeof(wchar_t)==2?1:-1];
static uint32_t read32(const unsigned char *p)
{ return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static int load_strings(const struct mac_retail_cache *cache,
    struct mac_native_tags *tags,const struct mac_retail_tag *tag,const char **error)
{
    const unsigned char *root,*entries,*source;
    struct string_list *list;
    struct string_list_entry *native;
    unsigned char *storage;
    uint32_t count,i,size;
    size_t bytes;
    int wide=tag->group==UNICODE_STRING_LIST_TAG;
    root=mac_retail_tag_span(cache,tag->address,12);
    if(!root)goto malformed;
    count=read32(root);
    if(count>32767)goto malformed;
    entries=count?mac_retail_tag_span(cache,read32(root+4),(size_t)count*20):NULL;
    if(count&&!entries)goto malformed;
    bytes=sizeof(*list)+(size_t)count*sizeof(*native);
    for(i=0;i<count;++i) {
        size=read32(entries+i*20);
        if(size>(wide?65534U:32767U)||(wide&&(size&1)))goto malformed;
        if(size&&!mac_retail_tag_span(cache,read32(entries+i*20+12),size))goto malformed;
        /* A bounded aggregate allocation prevents repeated references from
           expanding a small untrusted map into gigabytes of copied text. */
        if(size>16U*1024U*1024U-bytes)goto malformed;
        bytes+=size;
    }
    list=(struct string_list *)malloc(bytes);
    if(!list){if(error)*error="native interface text allocation failed";return 0;}
    memset(list,0,bytes);
    native=(struct string_list_entry *)(list+1);
    storage=(unsigned char *)(native+count);
    list->strings.count=count;
    list->strings.address=count?native:NULL;
    for(i=0;i<count;++i) {
        uint32_t j;
        size=read32(entries+i*20);
        native[i].string.size=size;
        if(!size)continue;
        source=mac_retail_tag_span(cache,read32(entries+i*20+12),size);
        native[i].string.address=storage;
        if(wide) {
            uint16_t *text=(uint16_t *)storage;
            for(j=0;j<size/2;++j)text[j]=(uint16_t)(source[j*2]|(uint16_t)source[j*2+1]<<8);
            text[size/2-1]=0;
        } else {
            memcpy(storage,source,size);
            storage[size-1]=0;
        }
        storage+=size;
    }
    if(!mac_native_tags_publish(tags,tag->handle,tag->group,list,sizeof(*list),bytes,list,free,error)) {
        free(list);return 0;
    }
    return 1;
malformed:
    if(error)*error="native interface text has invalid count, size or address";
    return 0;
}
/* Halo PC's HUD, as the retail PC maps hold it, drawn by the Xbox's code:
   the conversions port/linux/game/cache_file_formats.c makes to a Custom
   Edition map's HUD (hud_placements_convert, hud_meters_convert), made here
   to the decoded tags, whose layout is the tags' own (offsets from there).
   Halo PC's HUD bitmaps are at twice the Xbox's resolution: an element
   with Halo PC's high resolution scale, or drawing a bitmap with its half
   HUD scale or force high resolution scale flag, is drawn at half its
   scale. And where an Xbox meter has a block of multitexture overlays,
   Halo PC's has a minimum alpha, which read as a block draws dark boxes. */
#define PC_HUD_WIDTH_SCALE 0x04
#define PC_HUD_HEIGHT_SCALE 0x08
#define PC_HUD_SCALING_FLAGS 0x0C
#define PC_HUD_HIGH_RESOLUTION_BIT 2
#define PC_HUD_ELEMENT_BITMAP 0x24
#define PC_HUD_NO_BITMAP (-1)
#define PC_BITMAP_FLAGS 0x06
#define PC_BITMAP_HALF_HUD_SCALE_BIT 4
#define PC_BITMAP_FORCE_HIGH_RESOLUTION_BIT 7
#define PC_TAG_REFERENCE_INDEX 0x0C
#define PC_METER_OVERLAYS 0x58
struct pc_block { long count; void *address; void *definition; };

/* the retail bitmap's flags, read from the map itself */
static int pc_bitmap_halves_scale(const struct mac_retail_cache *cache,long handle)
{
    struct mac_retail_tag tag;
    const unsigned char *bitmap;
    unsigned flags;
    if(handle==-1 || !mac_retail_tag_get(cache,(uint32_t)handle&0xffffU,&tag) ||
       tag.handle!=(uint32_t)handle || tag.group!=UINT32_C(0x6269746d))return 0;
    bitmap=mac_retail_tag_span(cache,tag.address,PC_BITMAP_FLAGS+2);
    if(!bitmap)return 0;
    flags=(unsigned)bitmap[PC_BITMAP_FLAGS]|(unsigned)bitmap[PC_BITMAP_FLAGS+1]<<8;
    return (flags>>PC_BITMAP_HALF_HUD_SCALE_BIT&1U) || (flags>>PC_BITMAP_FORCE_HIGH_RESOLUTION_BIT&1U);
}
static void pc_placement_convert(unsigned char *placement,int bitmap_halves)
{
    short flags;
    float scale;
    memcpy(&flags,placement+PC_HUD_SCALING_FLAGS,sizeof(flags));
    if(!(flags>>PC_HUD_HIGH_RESOLUTION_BIT&1) && !bitmap_halves)return;
    memcpy(&scale,placement+PC_HUD_WIDTH_SCALE,sizeof(scale));scale*=0.5f;
    memcpy(placement+PC_HUD_WIDTH_SCALE,&scale,sizeof(scale));
    memcpy(&scale,placement+PC_HUD_HEIGHT_SCALE,sizeof(scale));scale*=0.5f;
    memcpy(placement+PC_HUD_HEIGHT_SCALE,&scale,sizeof(scale));
    flags=(short)(flags&~(1<<PC_HUD_HIGH_RESOLUTION_BIT));
    memcpy(placement+PC_HUD_SCALING_FLAGS,&flags,sizeof(flags));
}
static void pc_element_convert(const struct mac_retail_cache *cache,unsigned char *element,long bitmap_offset)
{
    long handle=-1;
    if(bitmap_offset!=PC_HUD_NO_BITMAP)
        memcpy(&handle,element+bitmap_offset+PC_TAG_REFERENCE_INDEX,sizeof(handle));
    pc_placement_convert(element,bitmap_offset!=PC_HUD_NO_BITMAP && pc_bitmap_halves_scale(cache,handle));
}
static void pc_block_convert(const struct mac_retail_cache *cache,unsigned char *field,unsigned long element_bytes,
    unsigned long placement_offset,long bitmap_offset,int items_of_halving_bitmap)
{
    struct pc_block block;
    long i;
    memcpy(&block,field,sizeof(block));
    if(block.count<=0 || block.count>1024 || !block.address)return;
    for(i=0;i<block.count;++i) {
        unsigned char *placement=(unsigned char *)block.address+(unsigned long)i*element_bytes+placement_offset;
        if(items_of_halving_bitmap)pc_placement_convert(placement,1);
        else pc_element_convert(cache,placement,bitmap_offset);
    }
}
/* Halo PC draws the part of a meter beyond its fill with the empty colour
   dimming what is behind it (about half), whatever alpha the colour has, which
   in its tags is 0; the Xbox's hud_draw_meter takes the colour's alpha as how
   much it dims (background alpha 255 - alpha), so none. An empty colour with
   no alpha dims by half (Halo PC's, compared with its meters at 800x600). */
#define PC_METER_EMPTY_COLOR 0x40
#define PC_METER_EMPTY_ALPHA 0x80
static void pc_meter_overlays_clear(unsigned char *meter)
{
    uint32_t empty;
    memset(meter+PC_METER_OVERLAYS,0,sizeof(struct pc_block));
    memcpy(&empty,meter+PC_METER_EMPTY_COLOR,sizeof(empty));
    if(!(empty>>24)) {
        empty|=(uint32_t)PC_METER_EMPTY_ALPHA<<24;
        memcpy(meter+PC_METER_EMPTY_COLOR,&empty,sizeof(empty));
    }
}
static void pc_weapon_items_convert(const struct mac_retail_cache *cache,unsigned char *field,unsigned long item_bytes)
{
    struct pc_block block;
    long i;
    memcpy(&block,field,sizeof(block));
    if(block.count<=0 || block.count>1024 || !block.address)return;
    for(i=0;i<block.count;++i) {
        unsigned char *element=(unsigned char *)block.address+(unsigned long)i*0x68;
        long handle;
        memcpy(&handle,element+0x24+PC_TAG_REFERENCE_INDEX,sizeof(handle));
        if(pc_bitmap_halves_scale(cache,handle))
            pc_block_convert(cache,element+0x34,item_bytes,0,PC_HUD_NO_BITMAP,1);
    }
}
static void pc_hud_convert(const struct mac_retail_cache *cache,uint32_t group,void *root)
{
    static const unsigned long unit_elements[]={0x24,0x8C,0xF4,0x17C,0x1E4,0x26C,0x2D4};
    unsigned char *definition=root;
    unsigned i;
    if(!definition)return;
    switch(group) {
    case 0x756e6869: /* unhi */
        for(i=0;i<sizeof(unit_elements)/sizeof(unit_elements[0]);++i)
            pc_element_convert(cache,definition+unit_elements[i],PC_HUD_ELEMENT_BITMAP);
        pc_element_convert(cache,definition+0x35C,PC_HUD_NO_BITMAP);
        pc_block_convert(cache,definition+0x3A4,0x84,0,PC_HUD_ELEMENT_BITMAP,0);
        pc_block_convert(cache,definition+0x3CC,0x144,0x14,PC_HUD_ELEMENT_BITMAP,0);
        pc_block_convert(cache,definition+0x3CC,0x144,0x7C,PC_HUD_ELEMENT_BITMAP,0);
        pc_meter_overlays_clear(definition+0xF4);
        pc_meter_overlays_clear(definition+0x1E4);
        {
            struct pc_block meters;long m;
            memcpy(&meters,definition+0x3CC,sizeof(meters));
            for(m=0;meters.address && m<meters.count && m<1024;++m)
                pc_meter_overlays_clear((unsigned char *)meters.address+(unsigned long)m*0x144+0x7C);
        }
        break;
    case 0x77706869: /* wphi */
        pc_block_convert(cache,definition+0x60,0xB4,0x24,PC_HUD_ELEMENT_BITMAP,0);
        pc_block_convert(cache,definition+0x6C,0xB4,0x24,PC_HUD_ELEMENT_BITMAP,0);
        pc_weapon_items_convert(cache,definition+0x84,0x6C);
        pc_weapon_items_convert(cache,definition+0x90,0x88);
        {
            struct pc_block meters;long m;
            memcpy(&meters,definition+0x6C,sizeof(meters));
            for(m=0;meters.address && m<meters.count && m<1024;++m) {
                pc_meter_overlays_clear((unsigned char *)meters.address+(unsigned long)m*0xB4+0x24);
            }
        }
        break;
    case 0x67726869: /* grhi */
        {
            long handle;
            pc_element_convert(cache,definition+0x24,PC_HUD_ELEMENT_BITMAP);
            pc_element_convert(cache,definition+0x8C,PC_HUD_ELEMENT_BITMAP);
            memcpy(&handle,definition+0x14C+PC_TAG_REFERENCE_INDEX,sizeof(handle));
            if(pc_bitmap_halves_scale(cache,handle))
                pc_block_convert(cache,definition+0x15C,0x88,0,PC_HUD_NO_BITMAP,1);
        }
        break;
    case 0x68756467: /* hudg */
        pc_element_convert(cache,definition+0x24,PC_HUD_NO_BITMAP);
        break;
    default:
        break;
    }
}

int mac_native_interface_assets_load_roots(const struct mac_retail_cache *cache,
    struct mac_native_tags *tags,const char **error)
{
    uint32_t i;
    if(error)*error=NULL;
    if(!cache||!tags||!tags->state){if(error)*error="native interface registry is absent";return 0;}
    for(i=0;i<cache->tag_count;++i) {
        struct mac_retail_tag tag;
        struct mac_native_object_tag hud;
        if(!mac_retail_tag_get(cache,i,&tag)){if(error)*error="native interface tag metadata invalid";return 0;}
        if(mac_native_tags_group(tags,tag.handle)!=UINT32_MAX)continue;
        if(tag.group==STRING_LIST_TAG||tag.group==UNICODE_STRING_LIST_TAG) {
            if(!load_strings(cache,tags,&tag,error))return 0;
        } else if(tag.group==UINT32_C(0x68756467)) {
            if(!mac_native_hud_globals_decode(cache,tag.handle,&hud,error))return 0;
            pc_hud_convert(cache,tag.group,hud.root);
            if(!mac_native_tags_publish(tags,tag.handle,tag.group,hud.root,hud.root_bytes,
                hud.bytes_owned,hud.owner,mac_native_object_owner_free,error)) {
                mac_native_object_tag_free(&hud);return 0;
            }
        } else if(tag.group==UINT32_C(0x666f6e74)) {
            void *root,*owner;size_t root_bytes,owned_bytes;
            if(!mac_native_font_decode(cache,tags,tag.handle,&root,&root_bytes,&owned_bytes,&owner,error))return 0;
            if(!mac_native_tags_publish(tags,tag.handle,tag.group,root,root_bytes,
                owned_bytes,owner,mac_native_font_owner_free,error)) {
                mac_native_font_owner_free(owner);return 0;
            }
        } else if(tag.group==UINT32_C(0x68756423)||tag.group==UINT32_C(0x756e6869)||
                  tag.group==UINT32_C(0x77706869)) {
            int decoded;
            if(tag.group==UINT32_C(0x68756423))decoded=mac_native_hud_number_decode(cache,tag.handle,&hud,error);
            else if(tag.group==UINT32_C(0x756e6869))decoded=mac_native_unit_hud_decode(cache,tag.handle,&hud,error);
            else decoded=mac_native_weapon_hud_decode(cache,tag.handle,&hud,error);
            if(!decoded)return 0;
            pc_hud_convert(cache,tag.group,hud.root);
            if(!mac_native_tags_publish(tags,tag.handle,tag.group,hud.root,hud.root_bytes,
                hud.bytes_owned,hud.owner,mac_native_object_owner_free,error)) {
                mac_native_object_tag_free(&hud);return 0;
            }
        } else if(tag.group==UINT32_C(0x636f6c6f)) {
            /* port: color tables (the HUD timer and debug text draw with one) */
            if(!mac_native_color_table_decode(cache,tag.handle,&hud,error))return 0;
            if(!mac_native_tags_publish(tags,tag.handle,tag.group,hud.root,hud.root_bytes,
                hud.bytes_owned,hud.owner,mac_native_object_owner_free,error)) {
                mac_native_object_tag_free(&hud);return 0;
            }
        } else if(tag.group==UINT32_C(0x67726869)) {
            if(!mac_native_grenade_hud_decode(cache,tag.handle,&hud,error))return 0;
            pc_hud_convert(cache,tag.group,hud.root);
            if(!mac_native_tags_publish(tags,tag.handle,tag.group,hud.root,hud.root_bytes,
                hud.bytes_owned,hud.owner,mac_native_object_owner_free,error)) {
                mac_native_object_tag_free(&hud);return 0;
            }
        }
    }
    return 1;
}

