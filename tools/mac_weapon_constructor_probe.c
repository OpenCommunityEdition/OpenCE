/* Isolated actual item_new/weapon_new constructor parts. This does not invoke
   object_new, create attachments, reconnect to the map or spawn a biped. */
#include "cseries.h"
#include "memory/data.h"
#include "items/items.h"
#include "items/weapons.h"
#include "items/weapon_definitions.h"
#include "engine_tags.h"
#include "native_object_tags.h"
#include "retail_object_spawn.h"
#undef malloc
#undef free
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
int main(int argc,char**argv)
{
    struct mac_retail_cache c;struct mac_retail_object_spawn spawn;struct mac_native_tags registry={0};struct mac_native_object_tag tag;
    FILE*f;long length,index[4],replacement;unsigned char*bytes;const char*error;unsigned i;
    if(argc!=2)return 2;f=fopen(argv[1],"rb");if(!f)return 2;
    if(fseek(f,0,SEEK_END)||(length=ftell(f))<2048||length>0x30000000L||fseek(f,0,SEEK_SET)){fclose(f);return 2;}
    bytes=malloc((size_t)length);if(!bytes){fclose(f);return 2;}
    if(fread(bytes,1,(size_t)length,f)!=(size_t)length){fclose(f);free(bytes);return 2;}fclose(f);
    if(!mac_retail_open(bytes,(size_t)length,&c,&error)||!mac_retail_object_spawn_decode(&c,&spawn,&error)||!mac_native_tags_create(&c,&registry,&error)){
        fprintf(stderr,"constructor setup: %s\n",error);free(bytes);return 1;}
    for(i=0;i<spawn.weapon_count;++i){
        if(!mac_native_weapon_decode(&c,spawn.weapons[i].object.handle,&tag,&error)){fprintf(stderr,"native weapon: %s\n",error);mac_native_tags_free(&registry);free(bytes);return 1;}
        if(!mac_native_tags_publish(&registry,tag.handle,tag.group,tag.root,tag.root_bytes,tag.bytes_owned,tag.owner,mac_native_object_owner_free,&error)){
            fprintf(stderr,"weapon publish: %s\n",error);mac_native_object_tag_free(&tag);mac_native_tags_free(&registry);free(bytes);return 1;}
    }
    if(!mac_engine_tags_bind(&registry)||!mac_constructor_storage_start()||!mac_constructor_time_start(123)){fprintf(stderr,"native constructor services unavailable\n");return 1;}
    for(i=0;i<spawn.weapon_count;++i){index[i]=mac_constructor_storage_new((long)spawn.weapons[i].object.handle);CHECK(index[i]!=NONE);if(index[i]!=NONE){CHECK(item_new(index[i]));CHECK(weapon_new(index[i]));verify(index[i],&spawn.weapons[i],123);}}
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
    CHECK(mac_constructor_storage_used()==0);CHECK(mac_constructor_storage_stop());mac_constructor_time_stop();mac_engine_tags_unbind();mac_native_tags_free(&registry);free(bytes);
    CHECK(mac_engine_service_errors==0);printf("actual stock item_new/weapon_new: checks=%u failures=%u; real datum/pool allocation, compaction, salt reuse and game_time; no object_new or biped\n",checks,failures);return failures?1:0;
}
