/* Isolated actual item_new/weapon_new constructor parts. This does not invoke
   object_new, create attachments, reconnect to the map or spawn a biped. */
#include "cseries.h"
#include "memory/data.h"
#include "items/items.h"
#include "items/weapons.h"
#include "items/weapon_definitions.h"
#include "items/projectile_definitions.h"
#include "effects/contrail_definitions.h"
#include "engine_tags.h"
#include "cache/cache_files.h"
extern boolean tag_index_is_group(long,long);
#include "native_object_tags.h"
#include "native_actor_assets.h"
#include "mapped_file.h"
#include "objects/object_definitions.h"
#include "models/model_definitions.h"
#include "models/model_animation_definitions.h"
#include "physics/collision_model_definitions.h"
#include "retail_object_spawn.h"
#undef malloc
#undef free
#undef memcpy
#include <stdio.h>
#include <stdlib.h>
extern int mac_constructor_storage_start(void),mac_constructor_storage_stop(void),mac_constructor_time_start(long);
extern long mac_constructor_storage_new(long),mac_constructor_storage_used(void);
extern void mac_constructor_storage_delete(long),mac_constructor_storage_compact(void),mac_constructor_time_stop(void);
extern unsigned long mac_engine_service_errors;
static unsigned checks,failures;
#define CHECK(x) do{++checks;if(!(x)){++failures;fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);}}while(0)
static void verify(long index,const struct mac_spawn_weapon*expected,long time)
{
    struct weapon_datum*w=weapon_get(index);unsigned i;
    CHECK(w->definition_index==(long)expected->object.handle);
    CHECK(TEST_FLAG(w->object.flags,_object_dynamic_lighting_recompute_bit));CHECK(TEST_FLAG(w->object.flags,_object_static_lighting_recompute_bit));
    CHECK(w->item.last_owned_time==time);CHECK(w->item.ignore_object_index==NONE);
    CHECK(w->weapon.state==0);CHECK(w->weapon.overheated_effect_index==NONE);
    for(i=0;i<expected->magazine_count;++i){CHECK(w->weapon.magazines[i].rounds_loaded==expected->magazines[i].initial_loaded);CHECK(w->weapon.magazines[i].rounds_total==expected->magazines[i].initial_spare);}
    for(i=0;i<expected->trigger_count;++i){CHECK(w->weapon.triggers[i].charging_effect_index==NONE);CHECK(w->weapon.triggers[i].idle_ticks==127);}
}
static void reject_dependency(const struct mac_retail_cache*c,uint32_t handle,size_t offset,unsigned char value)
{
    struct mac_retail_tag t;struct mac_actor_assets rejected;const char*error;
    unsigned char*raw,saved[4];
    CHECK(mac_retail_tag_get(c,handle&65535,&t));
    raw=(unsigned char*)mac_retail_tag_span(c,t.address,(uint32_t)offset+4);
    memcpy(saved,raw+offset,4);raw[offset]=value;raw[offset+1]=raw[offset+2]=raw[offset+3]=0;
    CHECK(!mac_actor_assets_load(c,&rejected,&error));
    CHECK(!rejected.tags.state&&!rejected.published_count);
    memcpy(raw+offset,saved,4);
}
int main(int argc,char**argv)
{
    struct mac_retail_cache c;struct mac_retail_object_spawn spawn;struct mac_actor_assets assets={0};
    FILE*f;long length,index[4],replacement;unsigned char*bytes;const char*error;unsigned i;
#ifdef MAC_ACTOR_ENHANCED
    const unsigned char *pixels;size_t pixel_length;struct mac_mapped_file pixel_map={0};
    if(argc!=3)return 2;
    if(!mac_mapped_file_open(argv[2],0x40000000U,&pixel_map,&error))return 2;
    pixels=pixel_map.data;pixel_length=pixel_map.size;
#else
    if(argc!=2)return 2;
#endif
    f=fopen(argv[1],"rb");if(!f)return 2;
    if(fseek(f,0,SEEK_END)||(length=ftell(f))<2048||length>0x30000000L||fseek(f,0,SEEK_SET)){fclose(f);return 2;}
    bytes=malloc((size_t)length);if(!bytes){fclose(f);return 2;}
    if(fread(bytes,1,(size_t)length,f)!=(size_t)length){fclose(f);free(bytes);return 2;}fclose(f);
    if(!mac_retail_open(bytes,(size_t)length,&c,&error)||!mac_retail_object_spawn_decode(&c,&spawn,&error)){
        fprintf(stderr,"constructor setup: %s\n",error);free(bytes);return 1;}
    {struct mac_retail_tag t;unsigned char*raw,saved[4];struct mac_actor_assets rejected;
        CHECK(mac_retail_tag_get(&c,spawn.weapons[spawn.weapon_count-1].object.refs[1].handle&65535,&t));
        raw=(unsigned char*)mac_retail_tag_span(&c,t.address,128);memcpy(saved,raw+116,4);raw[116]=255;raw[117]=255;raw[118]=255;raw[119]=127;
        CHECK(!mac_actor_assets_load(&c,&rejected,&error));CHECK(!rejected.tags.state&&!rejected.published_count);memcpy(raw+116,saved,4);
    }
    reject_dependency(&c,UINT32_C(0xe2a0012c),offsetof(struct contrail_definition,states),17);
    reject_dependency(&c,UINT32_C(0xe3750201),offsetof(struct projectile_definition,projectile.material_responses),34);
#ifdef MAC_ACTOR_ENHANCED
    {struct mac_actor_assets rejected;
        CHECK(!mac_actor_assets_load_with_bitmaps(&c,pixels,1,&rejected,&error));
        CHECK(!rejected.tags.state&&!rejected.published_count&&!rejected.bitmap_count);
    }
    if(!mac_actor_assets_load_with_bitmaps(&c,pixels,(size_t)pixel_length,&assets,&error))
#else
    if(!mac_actor_assets_load(&c,&assets,&error))
#endif
    {fprintf(stderr,"actor assets: %s\n",error);free(bytes);return 1;}
    CHECK(assets.biped==spawn.biped.handle&&assets.weapon_count==spawn.weapon_count);
#ifdef MAC_ACTOR_ENHANCED
    /* Each of two weapons contributes a first-person bitmap tag. */
    CHECK(assets.published_count==22+assets.bitmap_count);CHECK(assets.bitmap_count==1);
    mac_mapped_file_close(&pixel_map);pixels=NULL;CHECK(!pixel_map.data&&!pixel_map.size);
    CHECK(mac_actor_assets_bind_texture_cache(&assets));
    CHECK(!mac_actor_assets_bind_texture_cache(&assets));
    CHECK(mac_native_bitmap_cache_requests()==0);
#else
    /* The complete retail dependency walk now includes the real first-person
       model/graph and biped dialogue definitions, in addition to world roots. */
    CHECK(assets.published_count==22);
#endif
    free(bytes);bytes=NULL;
    if(!mac_engine_tags_bind(&assets.tags)||!mac_constructor_storage_start()||!mac_constructor_time_start(123)){fprintf(stderr,"native constructor services unavailable\n");return 1;}
    {struct tag_iterator it;long h;unsigned count=0;
        tag_iterator_new(&it,NONE);
        while((h=tag_iterator_next(&it))!=NONE){CHECK(tag_get_group_tag(h)!=NONE);++count;}
        CHECK(count==assets.published_count);CHECK(tag_iterator_next(&it)==NONE);
        tag_iterator_new(&it,'obje');count=0;
        while((h=tag_iterator_next(&it))!=NONE){CHECK(tag_index_is_group(h,'obje'));++count;}
        CHECK(count==5);
        tag_iterator_new(&it,'mode');count=0;
        while((h=tag_iterator_next(&it))!=NONE){CHECK(tag_index_is_group(h,'mode'));CHECK(!tag_index_is_group(h,'mod2'));++count;}
        /* Three shared world models plus one first-person model per weapon. */
        CHECK(count==5);
        CHECK(!tag_index_is_group((long)(assets.biped^65536U),'bipd'));
        it.absolute_index=-1;CHECK(tag_iterator_next(&it)==NONE);
    }
#ifndef MAC_ACTOR_ENHANCED
    /* weapon_new reaches the contrail's real bitmap tag; this target has no
       external bitmaps.map resource buffer. Keep this probe asset-only rather
       than inventing a bitmap or publishing an incomplete native tag. The
       enhanced probe below exercises weapon_new when given real resource data. */
    mac_constructor_storage_stop();mac_constructor_time_stop();
    mac_engine_tags_unbind();mac_actor_assets_free(&assets);free(bytes);
    printf("integrated actor asset dependencies: checks=%u failures=%u; real biped/weapon roots, dialogue, first-person models and malformed refs; constructors deferred until actual bitmap resources are supplied\n",checks,failures);
    return failures?1:0;
#endif
    {struct projectile_definition*p=projectile_definition_get((long)UINT32_C(0xe29f012b));
        struct object_attachment_definition*attachment=p->object.attachments.address;
        struct contrail_definition*trail;
        CHECK(p->projectile.material_responses.count==33);CHECK(p->object.attachments.count==1);
        CHECK(attachment->type.group_tag=='cont');
        trail=contrail_definition_get(attachment->type.index);CHECK(trail->states.count==2);
        CHECK((uint32_t)trail->bitmap.index==UINT32_C(0xe2a1012d));
        p=projectile_definition_get((long)UINT32_C(0xe3750201));
        CHECK(p->projectile.material_responses.count==33);CHECK(p->object.attachments.count==0);
    }
    weapons_initialize();weapons_initialize_for_new_map();
    CHECK(mac_engine_service_errors==0);
    for(i=0;i<=assets.weapon_count;++i){long h=(long)(i?assets.weapons[i-1]:assets.biped);struct object_definition*o=object_definition_get(h);struct model*m;struct collision_model*coll;struct animation_graph*g;
        CHECK(o!=NULL);CHECK(o->object.model.group_tag=='mode');m=model_definition_get(o->object.model.index);coll=collision_model_definition_get(o->object.collision_model.index);g=o->object.animation_graph.index==NONE?NULL:animation_graph_definition_get(o->object.animation_graph.index);
        {long j;struct object_attachment_definition*at=o->object.attachments.address;for(j=0;j<o->object.attachments.count;++j)if(at[j].type.index!=NONE){CHECK(at[j].type.group_tag=='ligh');CHECK(tag_get('ligh',at[j].type.index)!=NULL);}}
        CHECK(m&&coll);CHECK(m->nodes.count==coll->nodes.count);if(g){CHECK(m->nodes.count==g->nodes.count);CHECK(g->animations.count>0);}else CHECK(i&&o->object.animation_graph.index==NONE);
    }
    printf("integrated actor assets: %lu roots, %lu owned bytes; biped/model/collision/animation and %lu starting weapons; source map released\n",(unsigned long)assets.published_count,(unsigned long)mac_native_tags_owned_bytes(&assets.tags),(unsigned long)assets.weapon_count);
    for(i=0;i<spawn.weapon_count;++i){index[i]=mac_constructor_storage_new((long)spawn.weapons[i].object.handle);CHECK(index[i]!=NONE);if(index[i]!=NONE){CHECK(item_new(index[i]));CHECK(weapon_new(index[i]));verify(index[i],&spawn.weapons[i],123);}}
#ifdef MAC_ACTOR_ENHANCED
    CHECK(mac_native_bitmap_cache_requests()==1);
#endif
    CHECK(mac_constructor_storage_used()>0);
    if(spawn.weapon_count && spawn.weapons[0].magazine_count && spawn.weapons[0].magazines[0].loaded_max>1){
        struct weapon_definition*d=weapon_definition_get((long)spawn.weapons[0].object.handle);
        struct weapon_magazine_definition*m=TAG_BLOCK_GET_ELEMENT(&d->weapon.magazines,0,struct weapon_magazine_definition);
        short saved=m->rounds_total_initial;
        long low;
        m->rounds_total_initial=1;low=mac_constructor_storage_new((long)spawn.weapons[0].object.handle);CHECK(low!=NONE);
        if(low!=NONE){struct weapon_datum*w;CHECK(item_new(low));CHECK(weapon_new(low));w=weapon_get(low);CHECK(w->weapon.magazines[0].rounds_loaded==1);CHECK(w->weapon.magazines[0].rounds_total==0);mac_constructor_storage_delete(low);}
        m->rounds_total_initial=saved;
    }
    if(spawn.weapon_count>=2 && index[0]!=NONE && index[1]!=NONE){
        mac_constructor_storage_delete(index[0]);CHECK(object_header_try_and_get(index[0])==NULL);
        mac_constructor_storage_compact();verify(index[1],&spawn.weapons[1],123);
        replacement=mac_constructor_storage_new((long)spawn.weapons[0].object.handle);CHECK(replacement!=NONE&&replacement!=index[0]);
        if(replacement!=NONE){CHECK(item_new(replacement));CHECK(weapon_new(replacement));verify(replacement,&spawn.weapons[0],123);mac_constructor_storage_delete(replacement);}index[0]=NONE;
    }
    for(i=0;i<spawn.weapon_count;++i)if(index[i]!=NONE)mac_constructor_storage_delete(index[i]);
    CHECK(mac_constructor_storage_used()==0);CHECK(mac_constructor_storage_stop());mac_constructor_time_stop();mac_engine_tags_unbind();mac_actor_assets_free(&assets);free(bytes);
#ifdef MAC_ACTOR_ENHANCED
    CHECK(mac_native_bitmap_cache_requests()==3);
    printf("enhanced weapon_new: actual trail cache requests=%lu; resident CPU texture, no GPU upload\n",mac_native_bitmap_cache_requests());
#endif
    CHECK(mac_engine_service_errors==0);printf("integrated actor assets and actual item_new/weapon_new: checks=%u failures=%u; real datum/pool allocation, compaction, salt reuse and game_time; no object_new or biped\n",checks,failures);return failures?1:0;
}
