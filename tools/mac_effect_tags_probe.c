#include "cseries.h"
#include "real_math.h"
#include "native_effect_layout.h"
#include "effects/particle_system_definitions.h"
#include "native_object_tags.h"
#undef malloc
#undef free
#undef memcpy
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,failures,references;
#define CHECK(x) do{++checks;if(!(x)){++failures;fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);}}while(0)
static uint32_t read32(const unsigned char *p)
{return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static int reference(void *context,uint32_t group,uint32_t handle)
{(void)context;CHECK(group!=UINT32_MAX&&group!=0);CHECK(handle!=UINT32_MAX);++references;return 1;}
static int decode(const struct mac_retail_cache *c,const struct mac_retail_tag *tag,struct mac_native_object_tag *out,const char **error)
{return tag->group==UINT32_C(0x70617274)?mac_native_particle_decode(c,tag->handle,out,error):tag->group==UINT32_C(0x65666665)?mac_native_effect_decode(c,tag->handle,out,error):mac_native_particle_system_decode(c,tag->handle,out,error);}
static void reject(const struct mac_retail_cache *c,const struct mac_retail_tag *tag,unsigned char *field,uint32_t value)
{
    struct mac_native_object_tag invalid={0};const char *error;unsigned char saved[4];unsigned i;
    memcpy(saved,field,4);for(i=0;i<4;++i)field[i]=(unsigned char)(value>>(8*i));
    CHECK(!decode(c,tag,&invalid,&error));CHECK(!invalid.root&&!invalid.owner);
    mac_native_object_tag_free(&invalid);memcpy(field,saved,4);
}
int main(int argc,char **argv)
{
    FILE *f;long length;unsigned char *raw;struct mac_retail_cache cache;struct mac_retail_tag tag;
    struct mac_native_object_tag *owned;const char *error;unsigned i,count=0,effects=0,systems=0,particles=0;uint32_t digest=2166136261UL;
    if(argc!=2)return 2;f=fopen(argv[1],"rb");if(!f)return 2;
    if(fseek(f,0,SEEK_END)||(length=ftell(f))<2048||length>0x30000000L||fseek(f,0,SEEK_SET)){fclose(f);return 2;}
    raw=malloc(length);if(!raw){fclose(f);return 2;}
    if(fread(raw,1,length,f)!=(size_t)length){free(raw);fclose(f);return 2;}fclose(f);
    if(!mac_retail_open(raw,length,&cache,&error)){free(raw);return 1;}
    owned=calloc(cache.tag_count,sizeof(*owned));if(!owned){free(raw);return 2;}
    for(i=0;i<cache.tag_count;++i) {
        unsigned char *disk;unsigned block_offset,limit;int ok;
        if(!mac_retail_tag_get(&cache,i,&tag)||(tag.group!=UINT32_C(0x65666665)&&tag.group!=UINT32_C(0x7063746c)&&tag.group!=UINT32_C(0x70617274)))continue;
        ok=decode(&cache,&tag,owned+count,&error);CHECK(ok);
        if(!ok){fprintf(stderr,"effect tag %08lx %s: %s\n",(unsigned long)tag.handle,tag.name,error?error:"failed");continue;}
        disk=(unsigned char *)mac_retail_tag_span(&cache,tag.address,(uint32_t)owned[count].root_bytes);CHECK(disk!=NULL);
        if(tag.group==UINT32_C(0x70617274)) {
            struct particle_definition *particle=owned[count].root;uint32_t bits;
            memcpy(&bits,&particle->radius_lower_bound,4);CHECK(bits==read32(disk+0x74));
            CHECK((uint32_t)particle->bitmap.index==read32(disk+16));
            reject(&cache,&tag,disk+0x74,0x7fc00000);
            reject(&cache,&tag,disk+4,0x736e6421);
            reject(&cache,&tag,disk+offsetof(struct particle_definition,sprite_orientation),0x7fff);
            digest=(digest^owned[count].field_digest)*16777619UL;++count;++particles;continue;
        }
        if(tag.group==UINT32_C(0x65666665)) {
            struct effect_definition *effect=owned[count].root;uint32_t bits;
            struct effect_event_definition *events=effect->events.address;long e,p;
            block_offset=offsetof(struct effect_definition,events);limit=32;
            CHECK((uint32_t)effect->events.count==read32(disk+block_offset));
            CHECK((uint32_t)effect->locations.count==read32(disk+offsetof(struct effect_definition,locations)));
            memcpy(&bits,&effect->runtime_danger_radius,4);CHECK(bits==read32(disk+8));
            reject(&cache,&tag,disk+4,0x7fff7fff);
            for(e=0;e<effect->events.count;++e) {
                struct effect_part_definition *parts=events[e].parts.address;
                for(p=0;p<events[e].parts.count;++p) {
                    struct mac_retail_tag target;
                    if(parts[p].reference.index==NONE)continue;
                    CHECK(mac_retail_tag_get(&cache,(uint32_t)parts[p].reference.index&65535,&target));
                    CHECK(parts[p].runtime_base_class_tag==target.group ||
                          parts[p].runtime_base_class_tag==target.parents[0] ||
                          parts[p].runtime_base_class_tag==target.parents[1]);
                }
            }
            reject(&cache,&tag,disk+8,0x7fc00000);++effects;
        } else {
            struct particle_system_definition *system=owned[count].root;
            block_offset=offsetof(struct particle_system_definition,types);limit=4;
            CHECK((uint32_t)system->types.count==read32(disk+block_offset));
            CHECK((uint32_t)system->physics_constants.count==read32(disk+offsetof(struct particle_system_definition,physics_constants)));++systems;
            reject(&cache,&tag,disk+offsetof(struct particle_system_definition,system_update_physics),0x7fff);
        }
        reject(&cache,&tag,disk+block_offset,limit+1);
        if(read32(disk+block_offset))reject(&cache,&tag,disk+block_offset+4,0xffffffff);
        digest=(digest^owned[count].field_digest)*16777619UL;++count;
    }
    free(raw);CHECK(effects>0&&systems>0&&particles>0);
    for(i=0;i<count;++i){CHECK(mac_native_object_visit_references(owned+i,reference,NULL));mac_native_object_tag_free(owned+i);}
    free(owned);
    printf("native effect storage: %u checks, %u failures; effects=%u systems=%u particles=%u refs=%u digest=%08lx\n",checks,failures,effects,systems,particles,references,(unsigned long)digest);
    return failures?1:0;
}
