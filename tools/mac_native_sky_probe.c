#include "cseries.h"
#include "math/real_math.h"
#include "tag_files/tag_files.h"
#include "tag_files/tag_groups.h"
#include "scenario/sky_definitions.h"
#include "native_sky_tags.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../port/mac/retail_cache.c"
#include "../port/mac/native_tags.c"
#include "../port/mac/native_sky_tags.c"
#undef memcpy
#undef memset
#undef strcmp
#undef strncpy
void *csmemcpy(void *out,const void *in,unsigned long bytes) { return memcpy(out,in,bytes); }
void *csmemset(void *out,long value,unsigned long bytes) { return memset(out,(int)value,bytes); }
long csstrcmp(const char *a,const char *b) { return strcmp(a,b); }
char *csstrncpy(char *out,const char *in,unsigned long count) { return strncpy(out,in,count); }
void debug_free(void *pointer,const char *file,long line)
{ (void)file;(void)line;free(pointer); }
static unsigned checks,failures;
#define CHECK(x) do{++checks;if(!(x)){++failures;fprintf(stderr,"FAIL sky line %d: %s\n",__LINE__,#x);}}while(0)
static uint32_t probe_u32(const unsigned char *p)
{ return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static void reject(struct mac_retail_cache *cache,uint32_t handle,unsigned char *field,unsigned bytes,uint32_t value)
{
    struct mac_native_sky_tag sky={0};const char *error;unsigned i;unsigned char saved[4];
    for(i=0;i<bytes;++i){saved[i]=field[i];field[i]=(unsigned char)(value>>(8*i));}
    CHECK(!mac_native_sky_decode(cache,handle,&sky,&error));
    CHECK(!sky.root&&!sky.owner&&!sky.bytes_owned);for(i=0;i<bytes;++i)field[i]=saved[i];
}
int main(int argc,char **argv)
{
    FILE *file;long length;unsigned char *bytes,*scenario,*sky_root,*light;struct mac_retail_cache cache;
    struct mac_native_sky_tag sky={0},after_map={0};struct mac_native_tags registry={0};
    const char *error;uint32_t sky_handle,sky_address,sky_count,i,bits;struct sky *native;struct tag_reference *model,*fog;
    struct mac_retail_tag model_tag,lens_tag;char expected_model_name[256];
    if(argc!=2)return 2;file=fopen(argv[1],"rb");if(!file)return 2;
    if(fseek(file,0,SEEK_END)||(length=ftell(file))<2048||length>0x30000000L||fseek(file,0,SEEK_SET)){fclose(file);return 2;}
    bytes=malloc((size_t)length);if(!bytes){fclose(file);return 2;}
    if(fread(bytes,1,(size_t)length,file)!=(size_t)length){fclose(file);free(bytes);return 2;}fclose(file);
    if(!mac_retail_open(bytes,(size_t)length,&cache,&error)){fprintf(stderr,"cache: %s\n",error);free(bytes);return 1;}
    scenario=(unsigned char *)cache.tags+cache.scenario_offset;
    sky_count=probe_u32(scenario+0x30);sky_address=probe_u32(scenario+0x34);
    if(sky_count!=1){fprintf(stderr,"expected one BG sky, got %lu\n",(unsigned long)sky_count);free(bytes);return 1;}
    sky_handle=probe_u32(mac_retail_tag_span(&cache,sky_address,16)+12);
    sky_root=(unsigned char *)mac_retail_tag_span(&cache,probe_u32(cache.tags+cache.instances_offset+(sky_handle&65535)*32+20),0xd0);
    if(!sky_root||!mac_native_sky_decode(&cache,sky_handle,&sky,&error)){fprintf(stderr,"sky decode: %s\n",error);free(bytes);return 1;}
    CHECK(sky.handle==sky_handle&&sky.root_bytes==0xd0&&sky.bytes_owned>=0xd0);
    CHECK(mac_native_tags_create(&cache,&registry,&error));
    CHECK(mac_native_sky_publish(&cache,&registry,sky_handle,&error));
    CHECK(mac_native_tags_get(&registry,UINT32_C(0x736b7920),sky_handle,0xd0,&error)!=NULL);
    CHECK(mac_native_tags_owned_bytes(&registry)>0);
    native=(struct sky *)sky.root;model=(struct tag_reference *)sky.root;fog=&native->indoor_fog_screen;
    CHECK(model->index==(long)probe_u32(sky_root+12));
    CHECK((uint32_t)fog->index==probe_u32(sky_root+0xa4));
    for(i=0;i<2;++i) {
        struct sky_atmospheric_fog *f=i?&native->indoor_fog:&native->outdoor_fog;
        const unsigned char *raw=sky_root+(i?0x78:0x58);unsigned j;
        for(j=0;j<3;++j){memcpy(&bits,&f->color.n[j],4);CHECK(bits==probe_u32(raw+j*4));}
        memcpy(&bits,&f->maximum_density,4);CHECK(bits==probe_u32(raw+20));
        memcpy(&bits,&f->start_distance,4);CHECK(bits==probe_u32(raw+24));
        memcpy(&bits,&f->opaque_distance,4);CHECK(bits==probe_u32(raw+28));
    }
    CHECK(native->indoor_fog_screen.index==NONE);
    CHECK(((struct tag_block *)((unsigned char *)sky.root+0xc4))->count==1);
    light=(unsigned char *)mac_retail_tag_span(&cache,probe_u32(sky_root+0xc8),0x74);
    CHECK(light!=NULL);
    if(light) {
        struct tag_block *lights=(struct tag_block *)((unsigned char *)sky.root+0xc4);
        struct tag_reference *lens=(struct tag_reference *)lights->address;
        CHECK((uint32_t)lens->index==probe_u32(light+12));
        CHECK(lens->group_tag==probe_u32(light));
        CHECK(lens->index!=NONE);
        CHECK(mac_retail_tag_get(&cache,(uint32_t)lens->index&65535,&lens_tag));
        CHECK(lens_tag.handle==(uint32_t)lens->index&&lens_tag.group==UINT32_C(0x6c656e73));
        fprintf(stderr,"sky lens asset: handle=%08lx name=%s\n",(unsigned long)lens_tag.handle,lens_tag.name);
        CHECK(!strcmp((char *)lights->address+16,""));
        memcpy(&bits,(unsigned char *)lights->address+0x68,4);CHECK(bits==probe_u32(light+0x68));
        memcpy(&bits,(unsigned char *)lights->address+0x6c,4);CHECK(bits==probe_u32(light+0x6c));
    }
    reject(&cache,sky_handle,sky_root+0x58,4,UINT32_C(0x7fc00000));
    reject(&cache,sky_handle,sky_root+0xac,4,9);
    reject(&cache,sky_handle,light,4,UINT32_C(0x62697064));
    reject(&cache,sky_handle,light+12,4,UINT32_C(0x12345678));
    reject(&cache,sky_handle,light+0x68,4,UINT32_C(0x7f800000));
    CHECK(mac_retail_tag_get(&cache,probe_u32(sky_root+12)&65535,&model_tag));
    strncpy(expected_model_name,model_tag.name,sizeof(expected_model_name)-1);expected_model_name[sizeof(expected_model_name)-1]=0;
    CHECK(mac_native_sky_decode(&cache,sky_handle,&after_map,&error));
    mac_native_sky_tag_free(&sky);mac_native_tags_free(&registry);free(bytes);bytes=NULL;
    native=(struct sky *)after_map.root;
    CHECK(native->outdoor_fog.start_distance==3.f&&native->outdoor_fog.opaque_distance==100.f);
    CHECK(native->indoor_fog.maximum_density==1.f&&native->indoor_fog.opaque_distance==120.f);
    CHECK(native->indoor_fog_screen.name==NULL&&(uint32_t)native->indoor_fog_screen.index==UINT32_MAX);
    CHECK(((struct tag_reference *)after_map.root)->name!=NULL);
    CHECK(!strcmp(((struct tag_reference *)after_map.root)->name,expected_model_name));
    mac_native_sky_tag_free(&after_map);
    printf("native sky payload: checks=%u failures=%u handle=%08lx\n",checks,failures,(unsigned long)sky_handle);
    return failures?1:0;
}
