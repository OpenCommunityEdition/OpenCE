/* Owned material impact tables, checked against the retail byte stream. */
#include "cseries.h"
#include "effects/material_effect_definitions.h"
#include "native_object_tags.h"
#include "mapped_file.h"
#undef malloc
#undef calloc
#undef free
#undef memcpy
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,failures,references;
#define CHECK(x) do{++checks;if(!(x)){++failures;fprintf(stderr,"FAIL material effects %d %s\n",__LINE__,#x);}}while(0)
static uint32_t le32(const unsigned char *p)
{return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static int visit(void *unused,uint32_t group,uint32_t handle)
{(void)unused;CHECK(group==UINT32_C(0x65666665)||group==UINT32_C(0x736e6421));CHECK(handle!=UINT32_MAX);++references;return 1;}
int main(int argc,char **argv)
{
    struct mac_mapped_file map={0};struct mac_retail_cache cache;const char *error=NULL;
    struct mac_native_object_tag *owned;unsigned i,count=0;uint32_t digest=2166136261UL;
    if(argc!=2 || !mac_mapped_file_open(argv[1],0x30000000,&map,&error) || !mac_retail_open(map.data,map.size,&cache,&error))return 2;
    owned=calloc(cache.tag_count,sizeof(*owned));if(!owned)return 2;
    for(i=0;i<cache.tag_count;++i) {
        struct mac_retail_tag tag;struct material_effects_definition *root;
        const unsigned char *disk,*rows;long j;
        if(!mac_retail_tag_get(&cache,i,&tag)||tag.group!=UINT32_C(0x666f6f74))continue;
        if(!mac_native_material_effects_decode(&cache,tag.handle,owned+count,&error)) {
            fprintf(stderr,"material effects %s: %s\n",tag.name,error?error:"decode failed");++failures;continue;
        }
        root=owned[count].root;disk=mac_retail_tag_span(&cache,tag.address,12);
        CHECK(root->effects.count==(long)le32(disk));
        rows=mac_retail_tag_span(&cache,le32(disk+4),(size_t)root->effects.count*28);
        CHECK(!root->effects.count||rows);
        for(j=0;rows&&j<root->effects.count;++j) {
            struct material_effect *effect=(struct material_effect *)root->effects.address+j;
            const unsigned char *materials;long k;
            CHECK(effect->materials.count==(long)le32(rows+j*28));
            materials=mac_retail_tag_span(&cache,le32(rows+j*28+4),(size_t)effect->materials.count*48);
            CHECK(!effect->materials.count||materials);
            for(k=0;materials&&k<effect->materials.count;++k) {
                struct material_effect_material *m=(struct material_effect_material *)effect->materials.address+k;
                CHECK((uint32_t)m->effect.index==le32(materials+k*48+12));
                CHECK((uint32_t)m->sound.index==le32(materials+k*48+28));
            }
        }
        if(count==0) {
            unsigned char *copy=malloc(map.size);struct mac_retail_cache malformed;
            unsigned offsets[3];unsigned n;
            CHECK(copy!=NULL);
            if(copy) {
                memcpy(copy,map.data,map.size);CHECK(mac_retail_open(copy,map.size,&malformed,&error));
                offsets[0]=(unsigned)(disk-cache.file);offsets[1]=offsets[0]+4;
                offsets[2]=rows?(unsigned)(rows-cache.file):offsets[0];
                for(n=0;n<3;++n) {
                    struct mac_native_object_tag rejected={0};unsigned char saved[4];
                    uint32_t bad=n==0?14:(n==1?UINT32_MAX:34);unsigned k;
                    memcpy(saved,copy+offsets[n],4);
                    for(k=0;k<4;++k)copy[offsets[n]+k]=(unsigned char)(bad>>(8*k));
                    CHECK(!mac_native_material_effects_decode(&malformed,tag.handle,&rejected,&error));
                    CHECK(!rejected.root&&!rejected.owner);mac_native_object_tag_free(&rejected);
                    memcpy(copy+offsets[n],saved,4);
                }
                free(copy);
            }
        }
        digest=(digest^owned[count].field_digest)*16777619UL;++count;
    }
    mac_mapped_file_close(&map);CHECK(count>0);
    for(i=0;i<count;++i){CHECK(mac_native_object_visit_references(owned+i,visit,NULL));mac_native_object_tag_free(owned+i);}free(owned);
    printf("native material effects: %u checks %u failures tags=%u references=%u digest=%08lx\n",checks,failures,count,references,(unsigned long)digest);
    return failures?1:0;
}
