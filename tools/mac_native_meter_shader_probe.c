#include "cseries.h"
#include "native_object_tags.h"
#include "native_shader_layout.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#undef malloc
#undef free
#undef memcpy
#undef memset
#undef memcmp

static unsigned checks,failures,meters,decoded;
#define CHECK(x) do { ++checks; if(!(x)){++failures;fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);} } while(0)

static uint16_t read_le16(const unsigned char *p)
{ return (uint16_t)(p[0]|((uint16_t)p[1]<<8)); }
static uint32_t read_le32(const unsigned char *p)
{ return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24); }

static void check_real(const unsigned char *disk,const unsigned char *native,size_t offset)
{
    uint32_t expected=read_le32(disk+offset),actual;
    memcpy(&actual,native+offset,sizeof(actual));
    CHECK(actual==expected);
}

int main(int argc,char **argv)
{
    FILE *file;long length;unsigned char *bytes;struct mac_retail_cache cache;
    struct mac_retail_tag metadata;struct mac_native_object_tag tag;const char *error;
    uint32_t i;
    if(argc!=2)return 2;
    file=fopen(argv[1],"rb");if(!file)return 2;
    if(fseek(file,0,SEEK_END)||(length=ftell(file))<2048||
       length>0x40000000L||fseek(file,0,SEEK_SET)){fclose(file);return 2;}
    bytes=(unsigned char *)malloc((size_t)length);
    if(!bytes||fread(bytes,1,(size_t)length,file)!=(size_t)length){free(bytes);fclose(file);return 2;}
    fclose(file);
    if(!mac_retail_open(bytes,(size_t)length,&cache,&error)){
        fprintf(stderr,"retail cache: %s\n",error?error:"invalid");free(bytes);return 1;
    }
    for(i=0;i<cache.tag_count;++i)if(mac_retail_tag_get(&cache,i,&metadata)&&
        metadata.group==UINT32_C(0x736d6574)) {
        const unsigned char *disk;const unsigned char *native;
        struct shader_transparent_meter_definition *meter;
        unsigned color,component;size_t colors[]={
            offsetof(struct shader_transparent_meter_definition,gradient_min_color),
            offsetof(struct shader_transparent_meter_definition,gradient_max_color),
            offsetof(struct shader_transparent_meter_definition,background_color),
            offsetof(struct shader_transparent_meter_definition,flash_color),
            offsetof(struct shader_transparent_meter_definition,tint_color)};
        short sources[5];size_t source_offsets[]={
            offsetof(struct shader_transparent_meter_definition,meter_brightness_source),
            offsetof(struct shader_transparent_meter_definition,flash_brightness_source),
            offsetof(struct shader_transparent_meter_definition,value_source),
            offsetof(struct shader_transparent_meter_definition,gradient_source),
            offsetof(struct shader_transparent_meter_definition,flash_extension_source)};
        ++meters;
        if(!mac_native_meter_shader_decode(&cache,metadata.handle,&tag,&error)) {
            fprintf(stderr,"meter %08lx %s: %s\n",(unsigned long)metadata.handle,
                metadata.name,error?error:"invalid");++failures;continue;
        }
        ++decoded;meter=(struct shader_transparent_meter_definition *)tag.root;
        disk=mac_retail_tag_span(&cache,metadata.address,sizeof(*meter));native=(const unsigned char *)meter;
        if(!disk){CHECK(0);mac_native_object_tag_free(&tag);continue;}
        CHECK(meter->shader.base.type==9);
        for(color=0;color<5;++color)for(component=0;component<3;++component)
            check_real(disk,native,colors[color]+component*sizeof(real));
        check_real(disk,native,offsetof(struct shader_transparent_meter_definition,meter_transparency));
        check_real(disk,native,offsetof(struct shader_transparent_meter_definition,background_transparency));
        for(color=0;color<5;++color) {
            uint16_t expected=read_le16(disk+source_offsets[color]);uint16_t actual;
            memcpy(&actual,native+source_offsets[color],sizeof(actual));CHECK(actual==expected);
        }
        {
            uint16_t expected=read_le16(disk+offsetof(struct shader_transparent_meter_definition,flags));
            uint16_t actual;memcpy(&actual,native+offsetof(struct shader_transparent_meter_definition,flags),sizeof(actual));
            CHECK(actual==expected);
        }
        mac_native_object_tag_free(&tag);
    }
    CHECK(meters>0&&decoded==meters);
    printf("native meter shaders: roots=%u decoded=%u checks=%u failures=%u\n",
        meters,decoded,checks,failures);
    free(bytes);return failures?1:0;
}
