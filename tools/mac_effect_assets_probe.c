#include "cseries.h"
#include "native_effect_assets.h"
#include "native_effect_layout.h"
#include "game/game_globals.h"
#include "objects/damage_effect_definitions.h"
#include "effects/particle_system_definitions.h"
#include "effects/contrail_definitions.h"
#include "bitmaps/bitmap_group.h"
#include "native_object_tags.h"
#include "mapped_file.h"
#undef strcmp
#include <stdio.h>
#include <string.h>
static unsigned checks,failures;
#define CHECK(x) do{++checks;if(!(x)){++failures;fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);}}while(0)
static int loaded(void *context,uint32_t group,uint32_t handle)
{
    const struct mac_actor_assets *assets=context;
    CHECK(mac_native_tags_get(&assets->tags,group,handle,1,NULL)!=NULL);return 1;
}
int main(int argc,char **argv)
{
    static const char *names[]={"weapons\\rocket launcher\\effects\\exhaust","weapons\\rocket launcher\\effects\\rocket exhaust","weapons\\rocket launcher\\exhaust","weapons\\assault rifle\\bullet","characters\\cyborg\\melee"};
    struct mac_mapped_file map={0},bitmaps={0},sounds={0};struct mac_retail_cache cache;
    struct mac_actor_assets assets={0};const char *error=NULL;unsigned n;uint32_t handles[5]={UINT32_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX},digest=2166136261UL;int ok=0;
    if(argc!=4)return 2;
    if(!mac_mapped_file_open(argv[1],0x30000000,&map,&error)||
       !mac_mapped_file_open(argv[2],0x30000000,&bitmaps,&error)||
       !mac_mapped_file_open(argv[3],0x30000000,&sounds,&error)||
       !mac_retail_open(map.data,map.size,&cache,&error)||
       !mac_native_tags_create(&cache,&assets.tags,&error))goto done;
    CHECK(!mac_native_effect_assets_load(&cache,bitmaps.data,bitmaps.size,sounds.data,sounds.size,&assets,UINT32_MAX,&error));
    for(n=0;n<5;++n) {
        uint32_t i;size_t before;
        for(i=0;i<cache.tag_count;++i) {
            struct mac_retail_tag tag;
            if(!mac_retail_tag_get(&cache,i,&tag))goto done;
            if(!strcmp(tag.name,names[n]) && (n<3 || tag.group==UINT32_C(0x6a707421))){handles[n]=tag.handle;break;}
        }
        CHECK(handles[n]!=UINT32_MAX);if(handles[n]==UINT32_MAX)goto done;
        if(!mac_native_effect_assets_load(&cache,bitmaps.data,bitmaps.size,sounds.data,sounds.size,&assets,handles[n],&error)) {
            fprintf(stderr,"effect root %s: %s\n",names[n],error?error:"failed");goto done;
        }
        before=mac_native_tags_owned_bytes(&assets.tags);
        CHECK(mac_native_effect_assets_load(&cache,bitmaps.data,bitmaps.size,sounds.data,sounds.size,&assets,handles[n],&error));
        CHECK(before==mac_native_tags_owned_bytes(&assets.tags));
    }
    {
        uint32_t biped=UINT32_MAX;
        for(n=0;n<cache.tag_count;++n) {
            struct mac_retail_tag tag;
            if(!mac_retail_tag_get(&cache,n,&tag))goto done;
            if(tag.group==UINT32_C(0x62697064) && !strcmp(tag.name,"globals\\cyborg")) {
                biped=tag.handle;break;
            }
        }
        CHECK(biped!=UINT32_MAX);
        if(biped==UINT32_MAX || !mac_actor_assets_load_biped_with_resources(&cache,
            bitmaps.data,bitmaps.size,sounds.data,sounds.size,&assets,biped,&error)) {
            fprintf(stderr,"biped effect closure: %s\n",error?error:"missing globals\\cyborg");goto done;
        }
        {
            uint32_t foot=UINT32_MAX;
            CHECK(mac_native_tags_get(&assets.tags,UINT32_C(0x62697064),biped,1,&error)!=NULL);
            for(n=0;n<cache.tag_count;++n) {
                struct mac_retail_tag metadata;
                if(!mac_retail_tag_get(&cache,n,&metadata))goto done;
                if(metadata.group==UINT32_C(0x666f6f74) &&
                   mac_native_tags_get(&assets.tags,metadata.group,metadata.handle,1,NULL)) {
                    foot=metadata.handle;break;
                }
            }
            CHECK(foot!=UINT32_MAX);
        }
    }
    {
        struct particle_system_definition *system=(void *)mac_native_tags_get(&assets.tags,UINT32_C(0x7063746c),handles[1],sizeof(*system),&error);
        struct particle_system_type *types=system?system->types.address:NULL;
        CHECK(system&&system->types.count>0);
        if(types && types[0].particle_states.count) {
            struct particle_system_type_particle_state *state=types[0].particle_states.address;
            short saved=state->sequence_index;unsigned long flags=types[0].flags;
            state->sequence_index=32767;types[0].flags&=~(1UL<<_particle_system_type_disabled_bit);
            CHECK(!mac_native_effect_assets_load(&cache,bitmaps.data,bitmaps.size,sounds.data,sounds.size,&assets,handles[1],&error));
            state->sequence_index=saved;types[0].flags=flags;
            CHECK(mac_native_effect_assets_load(&cache,bitmaps.data,bitmaps.size,sounds.data,sounds.size,&assets,handles[1],&error));
        }
    }
    for(n=0;n<cache.tag_count;++n) {
        struct mac_retail_tag metadata;struct mac_native_object_tag tag={0};
        if(!mac_retail_tag_get(&cache,n,&metadata))goto done;
        if(metadata.group!=UINT32_C(0x666f6f74))continue;
        if(!mac_native_effect_assets_load(&cache,bitmaps.data,bitmaps.size,sounds.data,sounds.size,&assets,metadata.handle,&error)) {
            fprintf(stderr,"material effect root %s: %s\n",metadata.name,error?error:"failed");goto done;
        }
        tag.handle=metadata.handle;tag.group=metadata.group;tag.root_bytes=12;
        tag.root=(void *)mac_native_tags_get(&assets.tags,tag.group,tag.handle,12,&error);
        CHECK(tag.root!=NULL);CHECK(mac_native_object_visit_references(&tag,loaded,&assets));
    }
    {
        unsigned projectiles=0;
        for(n=0;n<cache.tag_count;++n) {
            struct mac_retail_tag metadata;struct mac_native_object_tag tag={0};
            if(!mac_retail_tag_get(&cache,n,&metadata))goto done;
            if(metadata.group!=UINT32_C(0x70726f6a) || strcmp(metadata.name,"weapons\\assault rifle\\bullet"))continue;
            if(!mac_native_object_effect_assets_load(&cache,bitmaps.data,bitmaps.size,sounds.data,sounds.size,&assets,metadata.handle,&error))goto done;
            CHECK(mac_native_projectile_decode(&cache,metadata.handle,&tag,&error));
            CHECK(mac_native_object_visit_references(&tag,loaded,&assets));
            mac_native_object_tag_free(&tag);++projectiles;
        }
        CHECK(projectiles==1);
    }
    mac_mapped_file_close(&map);mac_mapped_file_close(&bitmaps);mac_mapped_file_close(&sounds);
    for(n=0;n<assets.bitmap_count;++n) {
        struct mac_native_bitmap_tag *bitmap=assets.bitmaps+n;
        struct bitmap_data *images=bitmap->root->bitmaps.address;unsigned i;
        for(i=0;i<bitmap->image_count;++i) {
            unsigned mip;
            for(mip=0;mip<=(unsigned)images[i].mipmap_count;++mip) {
                uint32_t w,h;size_t bytes,p;const unsigned char *rgba=mac_native_bitmap_rgba(bitmap->owner,i,mip,&w,&h,&bytes);
                CHECK(rgba!=NULL);if(!rgba)continue;CHECK(bytes==w*h*4);
                for(p=0;p<bytes;++p)digest=(digest^rgba[p])*16777619UL;
                if(!mip && images[i].format==8) {
                    const uint16_t *native=images[i].base_address;
                    for(p=0;p<w*h;++p) {
                        CHECK((native[p]&31)==rgba[p*4+2]>>3);
                        CHECK(((native[p]>>5)&31)==rgba[p*4+1]>>3);
                        CHECK(((native[p]>>10)&31)==rgba[p*4]>>3);
                        CHECK((native[p]>>15)==(rgba[p*4+3]?1:0));
                    }
                }
                if(!mip && (images[i].format==10 || images[i].format==11)) {
                    const uint32_t *native=images[i].base_address;
                    for(p=0;p<w*h;++p) {
                        uint32_t expected=(uint32_t)rgba[p*4]<<16|(uint32_t)rgba[p*4+1]<<8|rgba[p*4+2];
                        CHECK((native[p]&0xffffff)==expected);
                        if(images[i].format==11)CHECK(native[p]>>24==rgba[p*4+3]);
                    }
                }
            }
        }
    }
    for(n=0;n<5;++n) {
        struct mac_native_object_tag tag={0};
        tag.handle=handles[n];tag.group=mac_native_tags_group(&assets.tags,handles[n]);
        tag.root_bytes=n>=3?sizeof(struct damage_effect_definition):n==2?sizeof(struct contrail_definition):n?sizeof(struct particle_system_definition):sizeof(struct effect_definition);
        tag.root=(void *)mac_native_tags_get(&assets.tags,tag.group,handles[n],tag.root_bytes,&error);
        CHECK(tag.root!=NULL);CHECK(mac_native_object_visit_references(&tag,loaded,&assets));
    }
    printf("native rocket effect assets: %u checks, %u failures; published=%lu bitmaps=%u bytes=%lu pixels=%08lx\n",checks,failures,
        (unsigned long)assets.published_count,assets.bitmap_count,(unsigned long)mac_native_tags_owned_bytes(&assets.tags),(unsigned long)digest);
    ok=!failures;
done:
    if(!ok)fprintf(stderr,"effect assets incomplete: %s\n",error?error:"test failure");
    mac_actor_assets_free(&assets);mac_mapped_file_close(&map);mac_mapped_file_close(&bitmaps);mac_mapped_file_close(&sounds);return ok?0:1;
}
