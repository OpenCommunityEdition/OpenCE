#include "cseries.h"
#include "text/text_group.h"
#include "text/font_group.h"
#include "native_interface_assets.h"
#include "native_object_tags.h"
#include "interface/unit_hud_interface_definition.h"
#include "interface/weapon_hud_interface_definition.h"
#include "native_hud_component_layout.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#undef malloc
#undef free
#undef memcpy
#undef memset
#undef strlen
#undef memcmp
static unsigned checks,failures;
#define CHECK(x) do {++checks;if(!(x)){++failures;fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);}}while(0)
static uint32_t rd(const unsigned char*p){return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint16_t r16(const unsigned char*p){return (uint16_t)(p[0]|(uint16_t)p[1]<<8);}
static uint32_t hash_word(uint32_t h,uint32_t v){return (h^v)*16777619U;}
static uint32_t hash_bytes(uint32_t h,const unsigned char*p,size_t n)
{size_t i;for(i=0;i<n;++i)h=hash_word(h,p[i]);return h;}
static int count_reference(void *context,uint32_t group,uint32_t handle)
{unsigned *n=context;(void)group;(void)handle;++*n;return 1;}
static int visit_owned_root(const void *root,uint32_t handle,uint32_t group,size_t size,unsigned *count)
{
    struct mac_native_object_tag view;
    memset(&view,0,sizeof(view));view.root=(void *)root;view.handle=handle;view.group=group;view.root_bytes=size;
    return mac_native_object_visit_references(&view,count_reference,count);
}
struct probe_font_glyph { word character;short character_width,bitmap_width,bitmap_height;
 short bitmap_origin_x,bitmap_origin_y,hardware_character_index;word pad;long pixels_offset; };
typedef char probe_font_glyph_size[sizeof(struct probe_font_glyph)==FONT_CHARACTER_SIZE?1:-1];
static uint32_t font_hash(const struct font_header *f)
{
    uint32_t h=2166136261U;long i,j;const struct font_character_table *t=f->character_tables.address;
    const struct probe_font_glyph *g=f->characters.address;
    h=hash_word(h,f->flags);h=hash_word(h,(uint16_t)f->ascending_height);
    h=hash_word(h,(uint16_t)f->descending_height);h=hash_word(h,(uint16_t)f->leading_height);
    h=hash_word(h,(uint16_t)f->leading_width);
    for(i=0;i<9;++i)h=hash_word(h,(uint32_t)f->pad[i]);
    for(i=0;i<NUMBER_OF_TEXT_STYLES;++i){const struct tag_reference*r=&f->style_fonts[i];
        h=hash_word(h,(uint32_t)r->group_tag);h=hash_word(h,(uint32_t)r->name_length);
        h=hash_word(h,(uint32_t)r->index);if(r->name)h=hash_bytes(h,(const unsigned char*)r->name,strlen(r->name)+1);}
    h=hash_word(h,(uint32_t)f->character_tables.count);
    for(i=0;i<f->character_tables.count;++i){h=hash_word(h,(uint32_t)t[i].character_indices.count);
        for(j=0;j<t[i].character_indices.count;++j)h=hash_word(h,(uint16_t)((short*)t[i].character_indices.address)[j]);}
    h=hash_word(h,(uint32_t)f->characters.count);
    for(i=0;i<f->characters.count;++i){h=hash_word(h,g[i].character);h=hash_word(h,(uint16_t)g[i].character_width);
        h=hash_word(h,(uint16_t)g[i].bitmap_width);h=hash_word(h,(uint16_t)g[i].bitmap_height);
        h=hash_word(h,(uint16_t)g[i].bitmap_origin_x);h=hash_word(h,(uint16_t)g[i].bitmap_origin_y);
        h=hash_word(h,(uint16_t)g[i].hardware_character_index);h=hash_word(h,g[i].pad);
        h=hash_word(h,(uint32_t)g[i].pixels_offset);}
    h=hash_word(h,(uint32_t)f->pixels.size);
    if(f->pixels.address)h=hash_bytes(h,f->pixels.address,(size_t)f->pixels.size);
    return h;
}
static void reject_word(struct mac_retail_cache *cache,unsigned char *word,uint32_t value)
{
    struct mac_native_tags tags={0};unsigned char saved[4];unsigned i;const char *error=NULL;
    memcpy(saved,word,4);for(i=0;i<4;++i)word[i]=(unsigned char)(value>>(i*8));
    CHECK(mac_native_tags_create(cache,&tags,&error));
    {int rejected=!mac_native_interface_assets_load_roots(cache,&tags,&error);CHECK(rejected);
        CHECK(error!=NULL);if(!rejected)fprintf(stderr,"accepted malformed font/interface word offset=%lu value=%08lx\n",
            (unsigned long)(word-cache->file),(unsigned long)value);}
    mac_native_tags_free(&tags);memcpy(word,saved,4);
}
int main(int argc,char **argv)
{
    FILE *f;long length;unsigned char *bytes,*first=NULL,*first_entry=NULL,*wide_entry=NULL;
    struct mac_retail_cache cache;struct mac_native_tags tags={0};
    unsigned char *font_root=NULL,*font_table=NULL,*font_indices=NULL,*font_glyph=NULL;
    const char *error=NULL;uint32_t i,j,k,lists=0,strings=0,fonts=0,digest=2166136261U,owned_digest=2166136261U;
    uint32_t font_digest=2166136261U,owned_font_digest=2166136261U;
    uint32_t hud_numbers=0,unit_huds=0,weapon_huds=0;unsigned hud_refs=0;
    if(argc!=2||!(f=fopen(argv[1],"rb")))return 2;
    if(fseek(f,0,SEEK_END)||(length=ftell(f))<2048||fseek(f,0,SEEK_SET))return 2;
    bytes=malloc(length);if(!bytes||fread(bytes,1,length,f)!=(size_t)length)return 2;fclose(f);
    if(!mac_retail_open(bytes,length,&cache,&error)||!mac_native_tags_create(&cache,&tags,&error))return 2;
    if(!mac_native_interface_assets_load_roots(&cache,&tags,&error)){fprintf(stderr,"load: %s\n",error);return 1;}
    for(i=0;i<cache.tag_count;++i) {
        struct mac_retail_tag tag;const struct string_list *list;
        const struct string_list_entry *entry;const unsigned char *root,*disk;
        mac_retail_tag_get(&cache,i,&tag);
        if(tag.group!=STRING_LIST_TAG&&tag.group!=UNICODE_STRING_LIST_TAG)continue;
        ++lists;root=mac_retail_tag_span(&cache,tag.address,12);if(!first)first=(unsigned char *)root;
        list=mac_native_tags_get(&tags,tag.group,tag.handle,sizeof(*list),&error);
        CHECK(list!=NULL);if(!list)return 1;
        CHECK(list->strings.count==(long)rd(root));entry=list->strings.address;
        disk=mac_retail_tag_span(&cache,rd(root+4),list->strings.count*20);
        if(list->strings.count&&!first_entry)first_entry=(unsigned char *)disk;
        if(list->strings.count&&tag.group==UNICODE_STRING_LIST_TAG&&!wide_entry)wide_entry=(unsigned char *)disk;
        for(j=0;j<(uint32_t)list->strings.count;++j) {
            uint32_t size=rd(disk+j*20);const unsigned char *text=mac_retail_tag_span(&cache,rd(disk+j*20+12),size);
            CHECK(entry[j].string.size==(long)size);++strings;
            if(tag.group==UNICODE_STRING_LIST_TAG) {
                const uint16_t *native=entry[j].string.address;
                for(k=0;k<size/2;++k){uint16_t expected=k+1==size/2?0:text[k*2]|(uint16_t)text[k*2+1]<<8;CHECK(native[k]==expected);digest=(digest^native[k])*16777619U;}
            } else {
                const unsigned char *native=entry[j].string.address;
                for(k=0;k<size;++k){CHECK(native[k]==(k+1==size?0:text[k]));digest=(digest^native[k])*16777619U;}
            }
        }
    }
    for(i=0;i<cache.tag_count;++i) {
        struct mac_retail_tag tag;const void *root=NULL;size_t bytes_expected=0;
        if(!mac_retail_tag_get(&cache,i,&tag))continue;
        if(tag.group==UINT32_C(0x68756423)){++hud_numbers;bytes_expected=sizeof(struct hud_number_definition);}
        else if(tag.group==UINT32_C(0x756e6869)){++unit_huds;bytes_expected=sizeof(struct unit_hud_interface_definition);}
        else if(tag.group==UINT32_C(0x77706869)){++weapon_huds;bytes_expected=sizeof(struct weapon_hud_interface_definition);}
        else continue;
        root=mac_native_tags_get(&tags,tag.group,tag.handle,bytes_expected,&error);CHECK(root!=NULL);
        CHECK(visit_owned_root(root,tag.handle,tag.group,bytes_expected,&hud_refs));
    }
    CHECK(hud_numbers>0&&unit_huds>0&&weapon_huds>0&&hud_refs>0);
    for(i=0;i<cache.tag_count;++i) {
        struct mac_retail_tag tag;const unsigned char *root,*td,*gd,*pd;
        const struct font_header *font;const struct font_character_table *tables;
        const struct probe_font_glyph *glyphs;uint32_t tc,gc,pc,a,b;
        mac_retail_tag_get(&cache,i,&tag);if(tag.group!=UINT32_C(0x666f6e74))continue;
        ++fonts;root=mac_retail_tag_span(&cache,tag.address,0x9c);
        font=mac_native_tags_get(&tags,tag.group,tag.handle,sizeof(*font),&error);CHECK(root&&font);
        if(!root||!font)continue;
        if(!font_root)font_root=(unsigned char *)root;
        tc=rd(root+48);gc=rd(root+124);pc=rd(root+136);
        CHECK(font->character_tables.count==(long)tc&&font->characters.count==(long)gc&&font->pixels.size==(long)pc);
        td=tc?mac_retail_tag_span(&cache,rd(root+52),tc*12):NULL;
        gd=gc?mac_retail_tag_span(&cache,rd(root+128),gc*FONT_CHARACTER_SIZE):NULL;
        pd=pc?mac_retail_tag_span(&cache,rd(root+148),pc):NULL;
        tables=font->character_tables.address;glyphs=font->characters.address;
        for(a=0;a<tc;++a){const unsigned char*d=td+a*12;uint32_t n=rd(d);const unsigned char*ix=n?mac_retail_tag_span(&cache,rd(d+4),n*2):NULL;
            CHECK(tables[a].character_indices.count==(long)n);if(n&&!font_indices)font_indices=(unsigned char *)ix;
            if(n&&!font_table)font_table=(unsigned char *)d;
            for(b=0;b<n;++b)CHECK(((short *)tables[a].character_indices.address)[b]==(short)r16(ix+b*2));}
        for(a=0;a<gc;++a){const unsigned char*d=gd+a*FONT_CHARACTER_SIZE;const struct probe_font_glyph*g=glyphs+a;
            if(!font_glyph&&(short)r16(d+4)>0&&(short)r16(d+6)>0)font_glyph=(unsigned char *)d;
            CHECK(g->character==r16(d)&&g->character_width==(short)r16(d+2));
            CHECK(g->bitmap_width==(short)r16(d+4)&&g->bitmap_height==(short)r16(d+6));
            CHECK(g->bitmap_origin_x==(short)r16(d+8)&&g->bitmap_origin_y==(short)r16(d+10));
            CHECK(g->hardware_character_index==NONE&&g->pad==0&&(uint32_t)g->pixels_offset==rd(d+16));}
        if(pc)CHECK(memcmp(font->pixels.address,pd,pc)==0);
        for(a=0;a<NUMBER_OF_TEXT_STYLES;++a){const struct tag_reference*r=&font->style_fonts[a];const unsigned char*d=root+60+a*16;
            CHECK((uint32_t)r->group_tag==rd(d)&&(uint32_t)r->name_length==rd(d+8)&&(uint32_t)r->index==rd(d+12));
            if(r->name)CHECK(strlen(r->name)<=255);}
        font_digest=hash_word(font_digest,tag.handle);font_digest=hash_word(font_digest,font_hash(font));
    }
    CHECK(lists>0&&strings>0&&fonts>0);mac_native_tags_free(&tags);
    CHECK(first&&first_entry&&wide_entry);
    reject_word(&cache,first,UINT32_MAX);
    reject_word(&cache,first+4,0);
    reject_word(&cache,first_entry,UINT32_MAX);
    reject_word(&cache,first_entry+12,0);
    reject_word(&cache,wide_entry,3);
    CHECK(font_root&&font_table&&font_indices&&font_glyph);
    reject_word(&cache,font_root+48,257);
    reject_word(&cache,font_root+136,0x800001);
    reject_word(&cache,font_table,257);
    reject_word(&cache,font_glyph+16,0x7fffffff);
    if(font_indices)reject_word(&cache,font_indices,0x00007fff);
    CHECK(mac_native_tags_create(&cache,&tags,&error));
    CHECK(mac_native_interface_assets_load_roots(&cache,&tags,&error));
    free(bytes);
    i=0;
    {
        uint32_t group;
        for(group=0;group<3;++group) {
            uint32_t wanted=group==0?UINT32_C(0x68756423):group==1?UINT32_C(0x756e6869):UINT32_C(0x77706869),cursor=0,handle;
            size_t size=group==0?sizeof(struct hud_number_definition):group==1?sizeof(struct unit_hud_interface_definition):sizeof(struct weapon_hud_interface_definition);
            while((handle=mac_native_tags_next(&tags,wanted,&cursor))!=UINT32_MAX) {
                const void *root=mac_native_tags_get(&tags,wanted,handle,size,&error);unsigned refs=0;
                CHECK(root!=NULL);CHECK(visit_owned_root(root,handle,wanted,size,&refs));
            }
        }
    }
    while((j=mac_native_tags_next(&tags,UINT32_MAX,&i))!=UINT32_MAX) {
        uint32_t group=mac_native_tags_group(&tags,j),row;
        const struct string_list *list;
        const struct string_list_entry *entry;
        if(group!=STRING_LIST_TAG&&group!=UNICODE_STRING_LIST_TAG)continue;
        list=mac_native_tags_get(&tags,group,j,sizeof(*list),&error);
        entry=list->strings.address;
        for(row=0;row<(uint32_t)list->strings.count;++row) {
            for(k=0;k<(uint32_t)entry[row].string.size/(group==UNICODE_STRING_LIST_TAG?2:1);++k) {
                uint32_t value=group==UNICODE_STRING_LIST_TAG?
                    ((uint16_t *)entry[row].string.address)[k]:((unsigned char *)entry[row].string.address)[k];
                owned_digest=(owned_digest^value)*16777619U;
            }
        }
    }
    i=0;
    while((j=mac_native_tags_next(&tags,UINT32_C(0x666f6e74),&i))!=UINT32_MAX) {
        const struct font_header *font=mac_native_tags_get(&tags,UINT32_C(0x666f6e74),j,sizeof(*font),&error);
        CHECK(font!=NULL);if(font){owned_font_digest=hash_word(owned_font_digest,j);
            owned_font_digest=hash_word(owned_font_digest,font_hash(font));}
    }
    CHECK(owned_digest==digest);
    CHECK(fonts>0&&font_digest==owned_font_digest);
    CHECK(mac_native_tags_owned_bytes(&tags)>0);mac_native_tags_free(&tags);CHECK(!tags.state);
    printf("native interface assets lists=%lu strings=%lu fonts=%lu HUD=%lu/%lu/%lu refs=%u digest=%08lx font_digest=%08lx checks=%u failures=%u\n",
        (unsigned long)lists,(unsigned long)strings,(unsigned long)fonts,(unsigned long)hud_numbers,
        (unsigned long)unit_huds,(unsigned long)weapon_huds,hud_refs,(unsigned long)digest,
        (unsigned long)font_digest,checks,failures);
    return failures?1:0;
}
