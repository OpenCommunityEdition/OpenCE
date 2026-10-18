#include "cseries.h"
#include "text/font_group.h"
#include "native_font_tags.h"
#include <stdlib.h>
#include <string.h>
#undef malloc
#undef free
#undef memcpy
#undef memset

enum { FONT_ROOT_BYTES=0x9c, FONT_TABLE_LIMIT=256, FONT_INDEX_LIMIT=256,
    FONT_CHARACTER_LIMIT=32767, FONT_PIXEL_LIMIT=0x800000,
    FONT_OWNER_LIMIT=16*1024*1024, FONT_GLYPH_W=128, FONT_GLYPH_H=128,
    FONT_GLYPH_BORDER=2 };
struct native_font_character {
    word character; short character_width,bitmap_width,bitmap_height;
    short bitmap_origin_x,bitmap_origin_y,hardware_character_index;
    word pad; long pixels_offset;
};
struct native_font_owner {
    size_t bytes;
    void *allocations[FONT_TABLE_LIMIT*2+16];
    unsigned allocation_count;
};
typedef char font_header_witness[sizeof(struct font_header)==FONT_ROOT_BYTES?1:-1];
typedef char font_character_witness[sizeof(struct native_font_character)==FONT_CHARACTER_SIZE?1:-1];
typedef char font_table_witness[sizeof(struct font_character_table)==12?1:-1];
typedef char font_pixel_offset_witness[offsetof(struct font_header,pixels)==0x88?1:-1];

static uint32_t r32(const unsigned char *p)
{return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint16_t r16(const unsigned char *p)
{return (uint16_t)(p[0]|(uint16_t)p[1]<<8);}
static int32_t ri32(const unsigned char *p)
{uint32_t v=r32(p);int32_t s;memcpy(&s,&v,4);return s;}
static void *owner_alloc(struct native_font_owner *o,size_t bytes)
{
    void *p;if(!bytes)return NULL;
    if(bytes>FONT_OWNER_LIMIT-o->bytes || o->allocation_count>=FONT_TABLE_LIMIT*2+16)return NULL;
    p=calloc(1,bytes);if(!p)return NULL;
    o->allocations[o->allocation_count++]=p;o->bytes+=bytes;return p;
}
void mac_native_font_owner_free(void *v)
{
    struct native_font_owner *o=(struct native_font_owner *)v;unsigned i;
    if(!o)return;for(i=0;i<o->allocation_count;++i)free(o->allocations[i]);free(o);
}
static int decode_ref(const struct mac_retail_cache *cache,struct mac_native_tags *tags,
    struct native_font_owner *owner,const unsigned char *p,struct tag_reference *out)
{
    uint32_t group=r32(p),address=r32(p+4),length=r32(p+8),handle=r32(p+12);
    const unsigned char *name;size_t avail;const unsigned char *end;char *copy;
    struct mac_retail_tag target;
    memset(out,0,sizeof(*out));out->group_tag=group;out->name_length=(long)length;
    memcpy(&out->index,&handle,4);
    if(handle==UINT32_MAX)return (group==UINT32_MAX||group==UINT32_C(0x666f6e74)) && length==0 && address==0;
    if(group!=UINT32_C(0x666f6e74) || length>255 ||
       !mac_retail_tag_get(cache,handle&65535,&target) || target.handle!=handle || target.group!=group ||
       mac_native_tags_find_metadata(tags,group,target.name)!=handle)
        return 0;
    if(!address)return length==0;
    name=mac_retail_tag_span(cache,address,1);if(!name)return 0;
    avail=cache->tag_size-(address-MAC_RETAIL_BASE);if(avail>256)avail=256;
    end=(const unsigned char *)memchr(name,0,avail);if(!end)return 0;
    if(length && (size_t)(end-name)!=length)return 0;
    copy=owner_alloc(owner,(size_t)(end-name)+1);if(!copy)return 0;
    memcpy(copy,name,(size_t)(end-name)+1);out->name=copy;
    return 1;
}
int mac_native_font_decode(const struct mac_retail_cache *cache,
    struct mac_native_tags *tags,uint32_t handle,void **root_out,size_t *root_bytes,
    size_t *owned_bytes,void **owner_out,const char **error)
{
    struct mac_retail_tag tag;struct native_font_owner *owner=NULL;
    struct font_header *root;const unsigned char *p,*tables,*indices,*glyphs,*pixels;
    uint32_t table_count,glyph_count,pixel_bytes,i,j;size_t total;
    if(error)*error="native font tag";
    if(root_out)*root_out=NULL;if(root_bytes)*root_bytes=0;
    if(owned_bytes)*owned_bytes=0;if(owner_out)*owner_out=NULL;
    if(!cache||!tags||!root_out||!root_bytes||!owned_bytes||!owner_out||
       !mac_retail_tag_get(cache,handle&65535,&tag)||tag.handle!=handle||tag.group!=UINT32_C(0x666f6e74))return 0;
    p=mac_retail_tag_span(cache,tag.address,FONT_ROOT_BYTES);if(!p)return 0;
    table_count=r32(p+48);glyph_count=r32(p+124);pixel_bytes=r32(p+136);
    if(table_count>FONT_TABLE_LIMIT||glyph_count>FONT_CHARACTER_LIMIT||pixel_bytes>FONT_PIXEL_LIMIT)return 0;
    tables=table_count?mac_retail_tag_span(cache,r32(p+52),table_count*12):NULL;
    glyphs=glyph_count?mac_retail_tag_span(cache,r32(p+128),glyph_count*FONT_CHARACTER_SIZE):NULL;
    pixels=pixel_bytes?mac_retail_tag_span(cache,r32(p+148),pixel_bytes):NULL;
    if((table_count&&!tables)||(glyph_count&&!glyphs)||(pixel_bytes&&!pixels))return 0;
    total=sizeof(*owner)+sizeof(*root)+(size_t)table_count*sizeof(struct font_character_table)+
        (size_t)glyph_count*sizeof(struct native_font_character)+pixel_bytes;
    if(total>FONT_OWNER_LIMIT)return 0;
    owner=(struct native_font_owner *)calloc(1,sizeof(*owner));if(!owner)return 0;owner->bytes=sizeof(*owner);
    root=owner_alloc(owner,sizeof(*root));if(!root)goto fail;
    /* Native fixed fields have the same layout; convert endian-sensitive
       scalars explicitly and install owned pointers for every variable field. */
    root->flags=r32(p);root->ascending_height=(short)r16(p+4);root->descending_height=(short)r16(p+6);
    root->leading_height=(short)r16(p+8);root->leading_width=(short)r16(p+10);
    for(i=0;i<9;++i)root->pad[i]=ri32(p+12+i*4);
    root->character_tables.count=(long)table_count;
    root->characters.count=(long)glyph_count;
    root->pixels.size=(long)pixel_bytes;
    root->pixels.pad=(long)r32(p+140);root->pixels.file_offset=(long)r32(p+144);
    root->character_tables.address=owner_alloc(owner,(size_t)table_count*sizeof(struct font_character_table));
    root->characters.address=owner_alloc(owner,(size_t)glyph_count*sizeof(struct native_font_character));
    root->pixels.address=owner_alloc(owner,pixel_bytes);
    if((table_count&&!root->character_tables.address)||(glyph_count&&!root->characters.address)||
       (pixel_bytes&&!root->pixels.address))goto fail;
    if(pixel_bytes)memcpy(root->pixels.address,pixels,pixel_bytes);
    for(i=0;i<NUMBER_OF_TEXT_STYLES;++i)
        if(!decode_ref(cache,tags,owner,p+60+i*16,&root->style_fonts[i]))goto fail;
    for(i=0;i<table_count;++i) {
        const unsigned char *d=tables+i*12;struct font_character_table *t=(struct font_character_table *)root->character_tables.address+i;
        uint32_t n=r32(d);const unsigned char *entries;
        if(n>FONT_INDEX_LIMIT)goto fail;
        entries=n?mac_retail_tag_span(cache,r32(d+4),n*2):NULL;if(n&&!entries)goto fail;
        t->character_indices.count=(long)n;
        t->character_indices.address=owner_alloc(owner,(size_t)n*sizeof(short));
        if(n&&!t->character_indices.address)goto fail;
        for(j=0;j<n;++j) {
            short index=(short)r16(entries+j*2);
            if(index!=NONE && (index<0 || (uint32_t)index>=glyph_count))goto fail;
            ((short *)t->character_indices.address)[j]=index;
        }
    }
    for(i=0;i<glyph_count;++i) {
        const unsigned char *d=glyphs+i*FONT_CHARACTER_SIZE;
        struct native_font_character *g=(struct native_font_character *)root->characters.address+i;
        int32_t offset=ri32(d+16);int16_t w=(int16_t)r16(d+4),h=(int16_t)r16(d+6);
        uint32_t image_bytes;
        g->character=r16(d);g->character_width=(short)r16(d+2);g->bitmap_width=w;g->bitmap_height=h;
        g->bitmap_origin_x=(short)r16(d+8);g->bitmap_origin_y=(short)r16(d+10);
        /* This index and pad belong to rasterizer_text's runtime cache. */
        g->hardware_character_index=NONE;g->pad=0;g->pixels_offset=offset;
        /* port: ui.map's gamespy font has a glyph of width -5 (no bitmap) */
        if(w< -FONT_GLYPH_W||h< -FONT_GLYPH_H||w>FONT_GLYPH_W||h>FONT_GLYPH_H)goto fail;
        if(w>0&&h>0) {
            image_bytes=(uint32_t)w*(uint32_t)h;
            if(offset<0||(uint32_t)offset>pixel_bytes||image_bytes>pixel_bytes-(uint32_t)offset)goto fail;
        }
    }
    root->character_tables.definition=NULL;root->characters.definition=NULL;root->pixels.definition=NULL;
    owner->bytes+=0;*root_out=root;*root_bytes=sizeof(*root);*owned_bytes=owner->bytes;*owner_out=owner;
    if(error)*error=NULL;return 1;
fail:
    mac_native_font_owner_free(owner);if(error)*error="native font has malformed block, reference, glyph or pixel range";return 0;
}
