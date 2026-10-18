#include "cseries.h"
#include "sound/sound_definitions.h"
#include "native_looping_sound_tags.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#undef malloc
#undef free
#undef memcpy
#undef memset
static unsigned checks,failures;
#define CHECK(x) do{++checks;if(!(x)){++failures;fprintf(stderr,"FAIL %s line %d\n",#x,__LINE__);}}while(0)
static uint32_t read32(const unsigned char *p)
{return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static int reference(void *context,uint32_t group,uint32_t handle)
{
    unsigned *count=context;
    CHECK(group==UINT32_C(0x736e6421)||group==UINT32_C(0x63646d67));
    CHECK(handle!=UINT32_MAX);++*count;return 1;
}
int main(int argc,char **argv)
{
    FILE *file;long length;unsigned char *bytes;
    struct mac_retail_cache cache;struct mac_retail_tag tag;
    struct mac_native_object_tag *tags;unsigned count=0,refs=0;uint32_t i;
    const char *error=NULL;
    if(argc!=2)return 2;file=fopen(argv[1],"rb");if(!file)return 2;
    if(fseek(file,0,SEEK_END)||(length=ftell(file))<2048||length>0x30000000L||fseek(file,0,SEEK_SET)){fclose(file);return 2;}
    bytes=malloc((size_t)length);if(!bytes){fclose(file);return 2;}
    if(fread(bytes,1,(size_t)length,file)!=(size_t)length){fclose(file);free(bytes);return 2;}fclose(file);
    if(!mac_retail_open(bytes,(size_t)length,&cache,&error)){free(bytes);return 1;}
    tags=calloc(cache.tag_count,sizeof(*tags));if(!tags){free(bytes);return 2;}
    for(i=0;i<cache.tag_count;++i) {
        struct looping_sound_definition *sound;
        struct mac_native_object_tag rejected;
        unsigned char *disk,saved[8];uint32_t tracks,details;int ok;
        if(!mac_retail_tag_get(&cache,i,&tag)||tag.group!=UINT32_C(0x6c736e64))continue;
        disk=(unsigned char *)mac_retail_tag_span(&cache,tag.address,0x54);CHECK(disk!=NULL);if(!disk)continue;
        tracks=read32(disk+0x3c);details=read32(disk+0x48);
        ok=mac_native_looping_sound_decode(&cache,tag.handle,tags+count,&error);CHECK(ok);
        if(!ok){fprintf(stderr,"looping sound %s: %s\n",tag.name,error);continue;}
        sound=tags[count].root;CHECK(sound->runtime_scripting_sound_index==NONE);
        CHECK(sound->tracks.count==(long)tracks);CHECK(sound->details.count==(long)details);
        CHECK(tags[count].root_bytes==sizeof(*sound));
        CHECK(!mac_native_looping_sound_decode(&cache,tag.handle^UINT32_C(0x10000),&rejected,&error));
        mac_native_object_tag_free(&rejected);
        memcpy(saved,disk+0x3c,8);memset(disk+0x3c,255,4);
        CHECK(!mac_native_looping_sound_decode(&cache,tag.handle,&rejected,&error));
        CHECK(!rejected.root&&!rejected.owner);mac_native_object_tag_free(&rejected);
        memcpy(disk+0x3c,saved,8);
        if(tracks) {
            memset(disk+0x40,255,4);
            CHECK(!mac_native_looping_sound_decode(&cache,tag.handle,&rejected,&error));
            mac_native_object_tag_free(&rejected);memcpy(disk+0x3c,saved,8);
        }
        printf("looping sound %s tracks=%lu details=%lu digest=%08lx\n",tag.name,
            (unsigned long)tracks,(unsigned long)details,(unsigned long)tags[count].field_digest);
        ++count;
    }
    free(bytes);CHECK(count>0);
    for(i=0;i<count;++i) {
        CHECK(mac_native_object_visit_references(tags+i,reference,&refs));
        mac_native_object_tag_free(tags+i);mac_native_object_tag_free(tags+i);
    }
    CHECK(refs>0);free(tags);
    printf("native looping sounds=%u references=%u checks=%u failures=%u\n",count,refs,checks,failures);
    return failures?1:0;
}
