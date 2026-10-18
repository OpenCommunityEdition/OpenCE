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

static unsigned checks,failures;
#define CHECK(x) do { ++checks; if(!(x)){++failures;fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);} } while(0)

struct reference_counts { unsigned bitmaps,other; };
static int count_reference(void *context,uint32_t group,uint32_t handle)
{
    struct reference_counts *counts=(struct reference_counts *)context;
    if(group==UINT32_C(0x6269746d) && handle!=UINT32_MAX)++counts->bitmaps;
    else ++counts->other;
    return 1;
}
static int stop_reference(void *context,uint32_t group,uint32_t handle)
{ unsigned *count=(unsigned *)context;(void)group;(void)handle;++*count;return 0; }

static int decode_shader(const struct mac_retail_cache *cache,uint32_t handle,
    uint32_t group,struct mac_native_object_tag *out,const char **error)
{
    switch(group) {
    case UINT32_C(0x736f736f):return mac_native_model_shader_decode(cache,handle,out,error);
    case UINT32_C(0x73676c61):return mac_native_glass_shader_decode(cache,handle,out,error);
    case UINT32_C(0x736d6574):return mac_native_meter_shader_decode(cache,handle,out,error);
    default:return 0;
    }
}

static void reject_mutation(const struct mac_retail_cache *cache,uint32_t handle,
    unsigned char *field,size_t length,const void *value,size_t value_length)
{
    unsigned char saved[4];struct mac_native_object_tag out,zero;const char *error;
    if(value_length>sizeof(saved)||value_length!=length){++failures;return;}
    memcpy(saved,field,length);memcpy(field,value,length);
    memset(&out,0xa5,sizeof(out));memset(&zero,0,sizeof(zero));
    CHECK(!mac_native_model_shader_decode(cache,handle,&out,&error));
    CHECK(!memcmp(&out,&zero,sizeof(out)));
    mac_native_object_tag_free(&out);memcpy(field,saved,length);
}

int main(int argc,char **argv)
{
    FILE *file;long length;unsigned char *bytes;struct mac_retail_cache cache;
    struct mac_retail_tag metadata;struct mac_native_object_tag tag;const char *error;
    uint32_t i,known=UINT32_C(0xe1b60042);unsigned model=0,glass=0,meter=0;
    struct reference_counts references={0,0};unsigned stopped=0,known_bitmaps=0;
    unsigned char bad_enum[2]={3,0};
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
    CHECK(mac_retail_tag_get(&cache,known&UINT32_C(0xffff),&metadata));
    CHECK(metadata.handle==known&&metadata.group==UINT32_C(0x736f736f));
    if(!mac_native_model_shader_decode(&cache,known,&tag,&error)){
        fprintf(stderr,"model shader %08lx: %s\n",(unsigned long)known,error?error:"invalid");
        free(bytes);return 1;
    }
    CHECK(tag.group==UINT32_C(0x736f736f));
    CHECK(((struct shader_model_definition *)tag.root)->shader.base.type==4);
    CHECK(mac_native_object_visit_references(&tag,count_reference,&references));
    CHECK(references.bitmaps>0&&references.other==0);
    known_bitmaps=references.bitmaps;
    CHECK(!mac_native_object_visit_references(&tag,stop_reference,&stopped));CHECK(stopped==1);
    {
        unsigned char *disk=(unsigned char *)mac_retail_tag_span(&cache,metadata.address,0x1b8);
        if(!disk){CHECK(0);}
        else {
            reject_mutation(&cache,known,disk+0xd4,2,bad_enum,sizeof(bad_enum));
            {
                unsigned char bad_group[4]={'s','o','s','o'};
                reject_mutation(&cache,known,disk+0xa4,4,bad_group,sizeof(bad_group));
            }
        }
    }
    mac_native_object_tag_free(&tag);
    for(i=0;i<cache.tag_count;++i)if(mac_retail_tag_get(&cache,i,&metadata)) {
        uint32_t group=metadata.group;
        if(group!=UINT32_C(0x736f736f)&&group!=UINT32_C(0x73676c61)&&group!=UINT32_C(0x736d6574))continue;
        memset(&tag,0,sizeof(tag));
        if(!decode_shader(&cache,metadata.handle,group,&tag,&error)) {
            fprintf(stderr,"shader %08lx %c%c%c%c %s: %s\n",(unsigned long)metadata.handle,
                (int)(group>>24),(int)(group>>16),(int)(group>>8),(int)group,metadata.name,
                error?error:"invalid");++failures;continue;
        }
        ++checks;
        if(group==UINT32_C(0x736f736f))++model;
        else if(group==UINT32_C(0x73676c61))++glass;
        else ++meter;
        memset(&references,0,sizeof(references));
        CHECK(mac_native_object_visit_references(&tag,count_reference,&references));
        CHECK(references.other==0);
        mac_native_object_tag_free(&tag);
    }
    CHECK(model>0&&glass>0&&meter>0);
    printf("native shader roots: soso=%u sgla=%u smet=%u; e1b60042 bitmap references=%u; checks=%u failures=%u\n",
        model,glass,meter,known_bitmaps,checks,failures);
    free(bytes);return failures?1:0;
}
