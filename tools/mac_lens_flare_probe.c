#include "cseries.h"
#include "native_object_tags.h"
#include "native_lens_flare_layout.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#undef malloc
#undef free
#undef memcpy
#undef memset
static unsigned checks,failures;
#define CHECK(x) do {++checks;if(!(x)){++failures;fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);}} while(0)
static uint32_t read32(const unsigned char*p){return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static int visit(void *context,uint32_t group,uint32_t handle){unsigned *n=context;(void)group;(void)handle;++*n;return 1;}
int main(int argc,char **argv)
{
    FILE *file;long length;unsigned char *bytes,*disk=NULL,saved[4];
    struct mac_retail_cache cache;struct mac_retail_tag tag;struct mac_native_object_tag object,retained;
    const char *error=NULL;uint32_t i,handle=0,count=0,lens_count=0,digest=2166136261U;unsigned refs=0;int saw_reflections=0;
    memset(&retained,0,sizeof(retained));
    if(argc!=2||!(file=fopen(argv[1],"rb")))return 2;
    if(fseek(file,0,SEEK_END)||(length=ftell(file))<2048||length>0x40000000||fseek(file,0,SEEK_SET))return 2;
    bytes=malloc(length);if(!bytes||fread(bytes,1,length,file)!=(size_t)length)return 2;fclose(file);
    if(!mac_retail_open(bytes,length,&cache,&error))return 2;
    for(i=0;i<cache.tag_count;++i) {
        if(!mac_retail_tag_get(&cache,i,&tag))return 2;
        if(tag.group!=UINT32_C(0x6c656e73))continue;
        ++lens_count;
        if(!mac_native_lens_flare_decode(&cache,tag.handle,&object,&error)) {
            fprintf(stderr,"lens decode %08lx: %s\n",(unsigned long)tag.handle,error?error:"failed");return 1;
        }
        {
            struct lens_flare_definition *lens=object.root;unsigned found=0;
            CHECK(sizeof(*lens)==0xf0&&lens->reflections.count>=0&&lens->reflections.count<=32);
            CHECK((uint32_t)lens->primary_map.group_tag==UINT32_C(0x6269746d)||lens->primary_map.index==NONE);
            CHECK(mac_native_object_visit_references(&object,visit,&found));refs+=found;
            if(lens->reflections.count) {
                struct lens_flare_reflection *reflection=lens->reflections.address;
                CHECK(reflection!=NULL);CHECK(reflection[0].bitmap_index>=-1);
                if(!handle){handle=tag.handle;count=(uint32_t)lens->reflections.count;disk=(unsigned char *)mac_retail_tag_span(&cache,tag.address,0xf0);retained=object;memset(&object,0,sizeof(object));}
                saw_reflections=1;
            }
            digest^=object.field_digest;
        }
        mac_native_object_tag_free(&object);
    }
    CHECK(handle!=0&&saw_reflections&&refs>0);
    if(handle&&disk) {
        unsigned char *block=disk+0xc4;
        memcpy(saved,block,4);block[0]=33;block[1]=block[2]=block[3]=0;
        CHECK(!mac_native_lens_flare_decode(&cache,handle,&object,&error));CHECK(error!=NULL);
        memcpy(block,saved,4);memcpy(saved,block+4,4);memset(block+4,0xff,4);
        CHECK(!mac_native_lens_flare_decode(&cache,handle,&object,&error));CHECK(error!=NULL);
        memcpy(block+4,saved,4);
    }
    free(bytes);
    if(handle) {
        struct lens_flare_definition *lens=retained.root;
        CHECK(lens!=NULL);if(lens){CHECK((uint32_t)lens->reflections.count==count);CHECK(retained.field_digest!=0);
            CHECK(lens->primary_map.name!=NULL&&lens->primary_map.name[0]!=0);
            CHECK(((struct lens_flare_reflection *)lens->reflections.address)[0].radius_lower_bound==
                ((struct lens_flare_reflection *)lens->reflections.address)[0].radius_lower_bound);}
        mac_native_object_tag_free(&retained);
    }
    printf("native lens roots=%lu first_reflections=%lu refs=%u digest=%08lx checks=%u failures=%u\n",
        (unsigned long)lens_count,(unsigned long)count,refs,(unsigned long)digest,checks,failures);
    return failures?1:0;
}
