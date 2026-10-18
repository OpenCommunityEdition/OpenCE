/* Damage storage and both vibration channels from the real retail cache. */
#include "cseries.h"
#include "game/game_globals.h"
#include "objects/damage_effect_definitions.h"
#include "native_object_tags.h"
#undef calloc
#undef malloc
#undef free
#undef memcpy
#undef strcmp
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,failures,refs;
#define CHECK(x) do{++checks;if(!(x)){++failures;fprintf(stderr,"FAIL damage %d %s\n",__LINE__,#x);}}while(0)
static uint32_t u32(const unsigned char*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static int visit(void *unused,uint32_t group,uint32_t handle)
{(void)unused;CHECK(group==UINT32_C(0x736e6421));CHECK(handle!=UINT32_MAX);++refs;return 1;}
int main(int argc,char **argv)
{
    FILE *file;long bytes;unsigned char *raw;struct mac_retail_cache cache;
    struct mac_native_object_tag *tags;struct mac_retail_tag metadata;const char *error=NULL;
    unsigned i,count=0,continuous=0;uint32_t digest=2166136261UL;
    printf("native damage sizes: root=%lu damage=%lu sound=%lu breaking=%lu flash=%lu vibrate=%lu impulse=%lu shake=%lu\n",(unsigned long)sizeof(struct damage_effect_definition),(unsigned long)offsetof(struct damage_effect_definition,damage),(unsigned long)offsetof(struct damage_effect_definition,sound),(unsigned long)offsetof(struct damage_effect_definition,breaking_effect),(unsigned long)offsetof(struct damage_effect_definition,screen_flash),(unsigned long)offsetof(struct damage_effect_definition,vibrate),(unsigned long)offsetof(struct damage_effect_definition,camera_impulse),(unsigned long)offsetof(struct damage_effect_definition,camera_shake));
    if(argc!=2)return 2;file=fopen(argv[1],"rb");if(!file)return 2;
    if(fseek(file,0,SEEK_END)||(bytes=ftell(file))<2048||bytes>0x30000000||fseek(file,0,SEEK_SET))return 2;
    raw=malloc(bytes);if(!raw||fread(raw,1,bytes,file)!=(size_t)bytes)return 2;fclose(file);
    if(!mac_retail_open(raw,bytes,&cache,&error))return 1;
    tags=calloc(cache.tag_count,sizeof(*tags));if(!tags)return 2;
    for(i=0;i<cache.tag_count;++i) {
        int (*decode)(const struct mac_retail_cache *,uint32_t,struct mac_native_object_tag *,const char **);
        unsigned char *disk;unsigned m;
        if(!mac_retail_tag_get(&cache,i,&metadata))continue;
        if(metadata.group==UINT32_C(0x6a707421))decode=mac_native_damage_effect_decode;
        else if(metadata.group==UINT32_C(0x63646d67)){decode=mac_native_continuous_damage_decode;++continuous;}
        else continue;
        if(!decode(&cache,metadata.handle,tags+count,&error)) {
            fprintf(stderr,"damage %s: %s\n",metadata.name,error?error:"failed");++failures;continue;
        }
        disk=(unsigned char *)mac_retail_tag_span(&cache,metadata.address,tags[count].root_bytes);
        if(metadata.group==UINT32_C(0x6a707421)) {
            struct damage_effect_definition *d=tags[count].root;uint32_t bits;
            const unsigned offsets[]={120,454,472,276};
            if(!strcmp(metadata.name,"weapons\\assault rifle\\bullet")) {
                CHECK(d->damage.category==2);CHECK(d->damage.damage_minimum==10.f);
                CHECK(d->damage.damage_lower_bound==10.f&&d->damage.damage_upper_bound==10.f);
                CHECK(d->vibrate.vibrate_frequencies[1].fade_function==4);
            }
            for(m=0;m<2;++m) {
                memcpy(&bits,&d->vibrate.vibrate_frequencies[m].frequency,4);
                CHECK(bits==u32(disk+92+m*20));
            }
            for(m=0;m<4;++m) {
                unsigned char saved[16];struct mac_native_object_tag invalid={0};unsigned j,n=m==3?16:4;
                uint32_t value=m==2?UINT32_C(0x7f800000):(m==3?UINT32_C(0x6269746d):32767);
                memcpy(saved,disk+offsets[m],n);for(j=0;j<4;++j)disk[offsets[m]+j]=(unsigned char)(value>>(j*8));
                if(m==3)for(j=0;j<4;++j)disk[offsets[m]+12+j]=(unsigned char)(metadata.handle>>(j*8));
                CHECK(!decode(&cache,metadata.handle,&invalid,&error));CHECK(!invalid.root&&!invalid.owner);
                mac_native_object_tag_free(&invalid);memcpy(disk+offsets[m],saved,n);
            }
        }
        digest=(digest^tags[count].field_digest)*16777619UL;++count;
    }
    free(raw);CHECK(count>0);
    for(i=0;i<count;++i){CHECK(mac_native_object_visit_references(tags+i,visit,NULL));mac_native_object_tag_free(tags+i);}free(tags);
    printf("native damage storage: %u checks %u failures; tags=%u continuous=%u references=%u digest=%08lx\n",checks,failures,count,continuous,refs,(unsigned long)digest);
    return failures?1:0;
}
