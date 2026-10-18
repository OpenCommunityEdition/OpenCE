#include "cseries.h"
#include "native_shader_layout.h"
#include "native_object_tags.h"
#include "mapped_file.h"
#undef calloc
#undef malloc
#undef free
#undef memcpy
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,failures,refs;
#define CHECK(x) do{++checks;if(!(x)){++failures;fprintf(stderr,"FAIL environment %d %s\n",__LINE__,#x);}}while(0)
static uint32_t u32(const unsigned char*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static float le_float(const unsigned char*p){uint32_t bits=u32(p);float value;memcpy(&value,&bits,4);return value;}
static int visit(void *unused,uint32_t group,uint32_t handle)
{(void)unused;CHECK(group==UINT32_C(0x6269746d)||group==UINT32_C(0x6c656e73)||group==UINT32_C(0x73686472));CHECK(handle!=UINT32_MAX);++refs;return 1;}
int main(int argc,char **argv)
{
    FILE *file;long bytes;unsigned char *raw;struct mac_retail_cache cache;
    struct mac_native_object_tag *tags;struct mac_retail_tag metadata;const char *error=NULL;
    unsigned i,count=0;uint32_t digest=2166136261UL;
    if(argc!=2)return 2;file=fopen(argv[1],"rb");if(!file)return 2;
    if(fseek(file,0,SEEK_END)||(bytes=ftell(file))<2048||bytes>0x30000000||fseek(file,0,SEEK_SET))return 2;
    raw=malloc(bytes);if(!raw||fread(raw,1,bytes,file)!=(size_t)bytes)return 2;fclose(file);
    if(!mac_retail_open(raw,bytes,&cache,&error))return 1;
    tags=calloc(cache.tag_count,sizeof(*tags));if(!tags)return 2;
    for(i=0;i<cache.tag_count;++i) {
        struct shader_environment_definition *shader;unsigned char *disk;uint32_t bits;unsigned m;
        static const unsigned offsets[]={offsetof(struct shader_environment_definition,type),offsetof(struct shader_environment_definition,u_animation_function),offsetof(struct shader_environment_definition,base_map)};
        if(!mac_retail_tag_get(&cache,i,&metadata))continue;
        if(metadata.group==UINT32_C(0x73636869) || metadata.group==UINT32_C(0x73636578)) {
            struct shader_transparent_chicago_definition *chicago;
            struct mac_native_object_tag invalid={0};unsigned char saved[2];unsigned offset;
            if(!mac_native_chicago_shader_decode(&cache,metadata.handle,tags+count,&error)) {
                fprintf(stderr,"Chicago %s: %s\n",metadata.name,error?error:"failed");++failures;continue;
            }
            chicago=tags[count].root;CHECK(chicago->shader.base.type==6);
            offset=offsetof(struct shader_transparent_chicago_definition,transparent)+offsetof(struct shader_transparent,first_map_type);
            disk=(unsigned char *)mac_retail_tag_span(&cache,metadata.address,tags[count].root_bytes);
            CHECK(chicago->transparent.maps.count==(long)u32(disk+offsetof(struct shader_transparent_chicago_definition,transparent)+offsetof(struct shader_transparent,maps)));
            memcpy(saved,disk+offset,2);disk[offset]=255;disk[offset+1]=127;
            CHECK(!mac_native_chicago_shader_decode(&cache,metadata.handle,&invalid,&error));
            CHECK(!invalid.root&&!invalid.owner);memcpy(disk+offset,saved,2);mac_native_object_tag_free(&invalid);
            digest=(digest^tags[count].field_digest)*16777619UL;++count;continue;
        }
        if(metadata.group!=UINT32_C(0x73656e76))continue;
        if(!mac_native_environment_shader_decode(&cache,metadata.handle,tags+count,&error)) {
            fprintf(stderr,"environment %s: %s\n",metadata.name,error?error:"failed");++failures;continue;
        }
        shader=tags[count].root;disk=(unsigned char *)mac_retail_tag_span(&cache,metadata.address,0x344);
        CHECK(shader->shader.base.type==3);CHECK((uint32_t)shader->base_map.index==u32(disk+0x94));
        memcpy(&bits,&shader->primary_detail_map_scale,4);CHECK(bits==u32(disk+0xb4));
        CHECK(shader->specular_brightness==le_float(disk+0x290));
        CHECK(shader->specular_view_perpendicular_color.red==le_float(disk+0x2a8));
        CHECK(shader->specular_view_perpendicular_color.green==le_float(disk+0x2ac));
        CHECK(shader->specular_view_perpendicular_color.blue==le_float(disk+0x2b0));
        CHECK(shader->specular_view_parallel_color.red==le_float(disk+0x2b4));
        CHECK(shader->specular_view_parallel_color.green==le_float(disk+0x2b8));
        CHECK(shader->specular_view_parallel_color.blue==le_float(disk+0x2bc));
        CHECK(shader->reflection_lightmap_brightness_scale==le_float(disk+0x2d4));
        CHECK(shader->reflection_view_perpendicular_brightness==le_float(disk+0x2f4));
        CHECK(shader->reflection_view_parallel_brightness==le_float(disk+0x2f8));
        for(m=0;m<3;++m) {
            unsigned char saved[4];struct mac_native_object_tag invalid={0};uint32_t value=m==2?UINT32_C(0x736e6421):32767;unsigned j;
            memcpy(saved,disk+offsets[m],4);for(j=0;j<4;++j)disk[offsets[m]+j]=(unsigned char)(value>>(j*8));
            CHECK(!mac_native_environment_shader_decode(&cache,metadata.handle,&invalid,&error));
            CHECK(!invalid.root&&!invalid.owner);mac_native_object_tag_free(&invalid);memcpy(disk+offsets[m],saved,4);
        }
        digest=(digest^tags[count].field_digest)*16777619UL;++count;
    }
    free(raw);CHECK(count>0);
    for(i=0;i<count;++i){CHECK(mac_native_object_visit_references(tags+i,visit,NULL));mac_native_object_tag_free(tags+i);}free(tags);
    printf("native environment shader storage: %u checks %u failures; shaders=%u references=%u digest=%08lx\n",checks,failures,count,refs,(unsigned long)digest);
    return failures?1:0;
}
