#include "cseries.h"
#include "sound/sound_definitions.h"
#include "sound/sound_environment_definitions.h"
#include "native_sound_tags.h"
#include "engine_cache_files.h"
#include "cache/cache_files.h"
#ifdef MAC_SOUND_CACHE_LIFECYCLE
#include "cache/sound_cache.h"
#include "cache/physical_memory_map.h"
#include "engine_physical_memory.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#undef malloc
#undef free
#undef memcpy
#undef memset
#undef memcmp
static unsigned checks,failures;
#define CHECK(x) do{++checks;if(!(x)){++failures;fprintf(stderr,"FAIL %s line %d\n",#x,__LINE__);}}while(0)
static uint32_t read32(const unsigned char *p)
{return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static int refs(void *c,uint32_t group,uint32_t handle)
{(void)c;CHECK(group==UINT32_C(0x736e6421));CHECK(handle!=UINT32_MAX);return 1;}
int main(int argc,char **argv)
{
    FILE *f;long length;unsigned char *bytes,*external=NULL;size_t external_bytes=0;
    struct mac_cache_files files={0};
    struct mac_retail_cache cache;struct mac_retail_tag tag;
    struct mac_native_object_tag *tags;unsigned count=0,perms=0;uint32_t i;
    const char *error=NULL;
    typedef char sound_root_size[sizeof(struct sound_definition)==0xa4?1:-1];
    typedef char sound_range_size[sizeof(struct sound_pitch_range)==0x48?1:-1];
    typedef char sound_permutation_size[sizeof(struct sound_permutation)==0x7c?1:-1];
    if(argc!=2&&argc!=3)return 2;
    if(argc==3) {
        f=fopen(argv[2],"rb");if(!f)return 2;
        if(fseek(f,0,SEEK_END)||(length=ftell(f))<0||length>0x30000000L||fseek(f,0,SEEK_SET)){fclose(f);return 2;}
        external_bytes=(size_t)length;external=malloc(external_bytes);if(!external){fclose(f);return 2;}
        if(fread(external,1,external_bytes,f)!=external_bytes){fclose(f);free(external);return 2;}fclose(f);
    }
    f=fopen(argv[1],"rb");if(!f)return 2;
    if(fseek(f,0,SEEK_END)||(length=ftell(f))<2048||length>0x30000000L||fseek(f,0,SEEK_SET)){fclose(f);return 2;}
    bytes=malloc(length);if(!bytes){fclose(f);return 2;}
    if(fread(bytes,1,length,f)!=(size_t)length){fclose(f);free(bytes);return 2;}fclose(f);
    if(!mac_retail_open(bytes,length,&cache,&error)){free(bytes);return 1;}
    if(!mac_cache_files_open(argv[1],NULL,argc==3?argv[2]:NULL,&files,&error)||!mac_cache_files_bind(&files)){free(bytes);free(external);return 2;}
#ifdef MAC_SOUND_CACHE_LIFECYCLE
    CHECK(mac_engine_physical_memory_start());if(failures)return 2;
    physical_memory_allocate();physical_memory_verify();sound_cache_new();sound_cache_open();
#endif
    tags=calloc(cache.tag_count,sizeof(*tags));if(!tags){free(bytes);return 2;}
    for(i=0;i<cache.tag_count;++i) {
        struct sound_definition *sound;struct mac_native_object_tag rejected;
        unsigned char *disk,saved[4];long r,p;int ok;
        if(!mac_retail_tag_get(&cache,i,&tag))continue;
        if(tag.group==UINT32_C(0x736e6465)) {
            struct mac_native_object_tag environment={0};
            struct sound_environment_definition *definition;
            unsigned char *raw=(unsigned char *)mac_retail_tag_span(&cache,tag.address,0x48);
            uint32_t bits;
            CHECK(mac_native_sound_environment_decode(&cache,tag.handle,&environment,&error));
            if(environment.root) {
                definition=environment.root;memcpy(&bits,&definition->decay_time,4);
                CHECK(bits==read32(raw+20));
                CHECK(definition->priority==(short)(raw[4]|raw[5]<<8));
                printf("native sound environment: handle=%08lx digest=%08lx\n",(unsigned long)tag.handle,(unsigned long)environment.field_digest);
                mac_native_object_tag_free(&environment);
            }
            memcpy(saved,raw+20,4);raw[20]=0;raw[21]=0;raw[22]=0xc0;raw[23]=0x7f;
            CHECK(!mac_native_sound_environment_decode(&cache,tag.handle,&environment,&error));
            CHECK(environment.root==NULL);memcpy(raw+20,saved,4);
            continue;
        }
        if(tag.group!=UINT32_C(0x736e6421))continue;
        ok=mac_native_sound_decode(&cache,tag.handle,external,external_bytes,tags+count,&error);CHECK(ok);
        if(!ok){fprintf(stderr,"sound %s: %s\n",tag.name,error);continue;}
        sound=tags[count].root;
        CHECK(sound->scripting_sound_index==NONE);CHECK(sound->promotion_counter==0);
        for(r=0;r<sound->pitch_ranges.count;++r) {
            struct sound_pitch_range *range=(struct sound_pitch_range *)sound->pitch_ranges.address+r;
            CHECK(range->previous_permutation_index==NONE);
            for(p=0;p<range->permutations.count;++p) {
                struct sound_permutation *perm=(struct sound_permutation *)range->permutations.address+p;
                CHECK(perm->unknown0==NONE);CHECK(perm->unknown2==tag.handle);
                CHECK(!perm->samples.size||perm->samples.address!=NULL);++perms;
#ifdef MAC_SOUND_CACHE_LIFECYCLE
                if(perm->samples.size) {
                    sound_cache_sound_new(tag.handle,perm);
                    CHECK(_sound_cache_sound_request(perm,TRUE,TRUE,TRUE));
                    CHECK(perm->unknown0!=NONE&&perm->unknown1!=0);
                    if(perm->unknown1)CHECK(!memcmp((void *)perm->unknown1,perm->samples.address,(size_t)perm->samples.size));
                    CHECK(_sound_cache_sound_request(perm,FALSE,FALSE,FALSE));
                    sound_cache_sound_hardware_lock(perm);sound_cache_sound_hardware_unlock(perm);
                    sound_cache_sound_finished(perm);sound_cache_sound_delete(perm);
                    CHECK(perm->unknown0==NONE&&perm->unknown1==0);
                }
#endif
                if(perm->samples.size) {
                    void *readback=malloc((size_t)perm->samples.size);boolean done=FALSE;
                    CHECK(readback!=NULL);
                    if(readback) {
                        CHECK(cache_file_read(tag.handle,perm->samples.file_offset,perm->samples.size,readback,&done,TRUE)==0&&done);
                        CHECK(!memcmp(readback,perm->samples.address,(size_t)perm->samples.size));free(readback);
                    }
                }
            }
        }
        disk=(unsigned char *)mac_retail_tag_span(&cache,tag.address,0xa4);
        if(count==0) {
            unsigned char *range_disk=(unsigned char *)mac_retail_tag_span(&cache,read32(disk+0x9c),0x48);
            unsigned char *perm_disk=range_disk?(unsigned char *)mac_retail_tag_span(&cache,read32(range_disk+0x40),0x7c):NULL;
            CHECK(perm_disk!=NULL);
            if(perm_disk) {
                unsigned char original[20];unsigned at;
                memcpy(original,perm_disk+0x40,20);
                CHECK(!mac_native_sound_decode(&cache,tag.handle,NULL,0,&rejected,&error));
                CHECK(!rejected.root&&!rejected.owner);
                for(at=0;at<3;++at) {
                    memset(perm_disk+0x40+4*at,255,4);
                    CHECK(!mac_native_sound_decode(&cache,tag.handle,external,external_bytes,&rejected,&error));
                    CHECK(!rejected.root&&!rejected.owner);memcpy(perm_disk+0x40,original,20);
                }
            }
        }
        memcpy(saved,disk+0x98,4);memset(disk+0x98,255,4);
        CHECK(!mac_native_sound_decode(&cache,tag.handle,external,external_bytes,&rejected,&error));
        CHECK(!rejected.root&&!rejected.owner);memcpy(disk+0x98,saved,4);
        CHECK(!mac_native_sound_decode(&cache,tag.handle^UINT32_C(0x10000),NULL,0,&rejected,&error));
        printf("sound %s digest=%08lx\n",tag.name,(unsigned long)tags[count].field_digest);++count;
    }
#ifdef MAC_SOUND_CACHE_LIFECYCLE
    sound_cache_idle();sound_cache_flush();sound_cache_close();sound_cache_delete();
    physical_memory_free();CHECK(mac_engine_physical_memory_allocation_count()==0);
    mac_engine_physical_memory_stop();
#endif
    mac_cache_files_close(&files);free(bytes);free(external);CHECK(count>0);CHECK(perms>0);
    for(i=0;i<count;++i){CHECK(mac_native_object_visit_references(tags+i,refs,NULL));mac_native_object_tag_free(tags+i);}
    free(tags);printf("native sounds=%u permutations=%u checks=%u failures=%u\n",count,perms,checks,failures);
    return failures?1:0;
}
