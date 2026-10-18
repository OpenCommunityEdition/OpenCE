#include "cseries.h"
#include "units/biped_definitions.h"
#include "items/weapon_definitions.h"
#include "native_object_tags.h"
#include "native_tags.h"
#include "engine_tags.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#undef malloc
#undef free
#undef memcpy
#undef memset
#undef strcmp
#undef memcmp
static unsigned mutation_checks,mutation_failures;
struct reference_inventory { unsigned count,required; uint32_t digest; };
static int inventory_reference(void *context,uint32_t group,uint32_t handle)
{
    struct reference_inventory *r=(struct reference_inventory *)context;
    ++r->count;r->digest=(r->digest^group)*UINT32_C(16777619);
    r->digest=(r->digest^handle)*UINT32_C(16777619);
    if(handle==UINT32_C(0xe2080094))r->required|=1;
    if(handle==UINT32_C(0xe211009d))r->required|=2;
    if(handle==UINT32_C(0xe22900b5))r->required|=4;
    if(handle==UINT32_C(0xe25e00ea))r->required|=8;
    if(handle==UINT32_C(0xe34d01d9))r->required|=16;
    return 1;
}
static int stop_reference(void *context,uint32_t group,uint32_t handle)
{ unsigned *count=(unsigned *)context;(void)group;(void)handle;++*count;return 0; }
static uint32_t read32(const unsigned char *p)
{ return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static void reject_mutation(const struct mac_retail_cache *cache,uint32_t handle,int weapon,
    unsigned char *field,unsigned size,uint32_t value)
{
    struct mac_native_object_tag result,zero;const char *error;
    unsigned char saved[32];unsigned i;int ok;
    memcpy(saved,field,size);
    for(i=0;i<size;++i)field[i]=size==32?'X':(unsigned char)(value>>(8*i));
    memset(&result,0xa5,sizeof(result));memset(&zero,0,sizeof(zero));
    ok=weapon?mac_native_weapon_decode(cache,handle,&result,&error):mac_native_biped_decode(cache,handle,&result,&error);
    ++mutation_checks;if(ok || memcmp(&result,&zero,sizeof(result))) { ++mutation_failures;fprintf(stderr,"mutation accepted: handle=%08lx value=%08lx size=%u\n",(unsigned long)handle,(unsigned long)value,size); }
    mac_native_object_tag_free(&result);memcpy(field,saved,size);
}
static void mutations(const struct mac_retail_cache *cache,uint32_t handle,int weapon)
{
    struct mac_retail_tag t;unsigned char *p,*m,*tr;
    struct mac_native_object_tag decoded;struct weapon_definition *native;
    unsigned char saved[8];const char *error;int ok;
    if(!mac_retail_tag_get(cache,handle&65535,&t)){++mutation_failures;return;}
    p=(unsigned char *)mac_retail_tag_span(cache,t.address,weapon?0x508:0x4f4);
    if(!p){++mutation_failures;return;}
    reject_mutation(cache,handle,weapon,p+4,4,UINT32_C(0x7fc00000));
    reject_mutation(cache,handle,weapon,p+0x34,4,0);
    reject_mutation(cache,handle,weapon,p+0x140,4,9);
    if(read32(p+0x140))reject_mutation(cache,handle,weapon,p+0x144,4,UINT32_MAX);
    if(weapon) {
        reject_mutation(cache,handle,1,p+0x4c0,4,1);
        memcpy(saved,p+0x4c4,8);memset(p+0x4c4,255,8);
        ok=mac_native_weapon_decode(cache,handle,&decoded,&error);
        native=(struct weapon_definition *)decoded.root;++mutation_checks;
        if(!ok || native->weapon.unused_block.count || native->weapon.unused_block.address || native->weapon.unused_block.definition)++mutation_failures;
        mac_native_object_tag_free(&decoded);memcpy(p+0x4c4,saved,8);
        reject_mutation(cache,handle,1,p+0x4f0,4,3);
        reject_mutation(cache,handle,1,p+0x4fc,4,3);
        m=(unsigned char *)mac_retail_tag_span(cache,read32(p+0x4f4),112);
        tr=(unsigned char *)mac_retail_tag_span(cache,read32(p+0x500),276);
        if(m)reject_mutation(cache,handle,1,m+6,2,65535);
        if(tr)reject_mutation(cache,handle,1,tr+32,2,2);
    } else {
        reject_mutation(cache,handle,0,p+0x2d8,4,5);
        reject_mutation(cache,handle,0,p+0x2dc,4,UINT32_MAX);
        reject_mutation(cache,handle,0,p+0x4e8,4,3);
        reject_mutation(cache,handle,0,p+0x400,4,UINT32_C(0x7f800000));
    }
}
int main(int argc,char **argv)
{
    FILE *f;long length;unsigned char *bytes;
    struct reference_inventory before={0,0,0},after={0,0,0};unsigned stopped=0;
    struct mac_retail_cache cache;struct mac_retail_tag tag;struct mac_native_object_tag owned;
    struct mac_native_tags registry;
    struct mac_native_object_tag weapon_owned;
    struct weapon_definition *weapon;
    uint32_t weapon_handle;
    struct biped_definition *biped;const char *error;uint32_t i,h=0;unsigned checks=0,failures=0,scenery_count=0,sound_scenery_count=0;
    if(argc!=2)return 2;
    f=fopen(argv[1],"rb");if(!f)return 2;
    if(fseek(f,0,SEEK_END) || (length=ftell(f))<2048 || length>0x30000000L || fseek(f,0,SEEK_SET)){fclose(f);return 2;}
    bytes=(unsigned char *)malloc((size_t)length);if(!bytes){fclose(f);return 2;}
    if(fread(bytes,1,(size_t)length,f)!=(size_t)length){free(bytes);fclose(f);return 2;}fclose(f);
    if(!mac_retail_open(bytes,(size_t)length,&cache,&error)){fprintf(stderr,"map: %s\n",error);free(bytes);return 1;}
    for(i=0;i<cache.tag_count;++i)if(mac_retail_tag_get(&cache,i,&tag) && tag.group==UINT32_C(0x62697064) &&
        !strcmp(tag.name,"characters\\cyborg_mp\\cyborg_mp")){h=tag.handle;break;}
    if(!h || !mac_native_biped_decode(&cache,h,&owned,&error)){fprintf(stderr,"native biped: %s\n",h?error:"missing");free(bytes);return 1;}
    if(!mac_native_tags_create(&cache,&registry,&error)) { fprintf(stderr,"registry: %s\n",error);mac_native_object_tag_free(&owned);free(bytes);return 1; }
    if(!mac_native_tags_publish(&registry,owned.handle,owned.group,owned.root,owned.root_bytes,owned.bytes_owned,
        owned.owner,mac_native_object_owner_free,&error)) {
        fprintf(stderr,"publish: %s\n",error);mac_native_tags_free(&registry);mac_native_object_tag_free(&owned);free(bytes);return 1;
    }
    owned.owner=NULL;
    if(!mac_engine_tags_bind(&registry)) { mac_native_tags_free(&registry);free(bytes);return 1; }
    biped=biped_definition_get((long)h);
#define CHECK(test) do { ++checks;if(!(test)){++failures;fprintf(stderr,"FAIL %s\n",#test);} }while(0)
    CHECK(mac_native_object_visit_references(&owned,inventory_reference,&before));
    CHECK(before.required==31);
    CHECK(!mac_native_object_visit_references(&owned,stop_reference,&stopped));CHECK(stopped==1);
    CHECK(biped->object.type==0);CHECK(biped->object.model.index==(long)UINT32_C(0xe2080094));
    CHECK(biped->object.animation_graph.index==(long)UINT32_C(0xe211009d));
    CHECK(biped->object.attachments.count==1);CHECK(biped->object.functions.count==2);
    CHECK(biped->object.change_colors.count==2);CHECK(biped->unit.huds.count==2);
    CHECK(biped->unit.dialogue_variants.count==1);CHECK(biped->unit.initial_weapons.count==2);
    CHECK(biped->biped.contact_points.count==2);CHECK(biped->biped.collision_radius>.19f && biped->biped.collision_radius<.21f);
    CHECK(((struct unit_dialogue_variant *)biped->unit.dialogue_variants.address)[0].dialogue_index==(long)UINT32_C(0xe25b00e7));
    CHECK(tag_get(OBJECT_DEFINITION_TAG,(long)h)==biped);
    CHECK(tag_get(UNIT_DEFINITION_TAG,(long)h)==biped);
    CHECK(!strcmp(tag_get_name((long)h),"characters\\cyborg_mp\\cyborg_mp"));
    CHECK(mac_native_tags_get(&registry,UINT32_C(0x6d6f6432),(uint32_t)biped->object.model.index,1,&error)==NULL);
    CHECK(mac_native_tags_get(&registry,UINT32_C(0x62697064),h^UINT32_C(0x10000),1,&error)==NULL);
    mutations(&cache,h,0);
    for(i=0;i<(uint32_t)biped->unit.initial_weapons.count;++i) {
        struct tag_reference *r=(struct tag_reference *)((unsigned char *)biped->unit.initial_weapons.address+i*36);
        weapon_handle=(uint32_t)r->index;
        if(weapon_handle==UINT32_MAX)continue;
        if(!mac_native_weapon_decode(&cache,weapon_handle,&weapon_owned,&error)) {
            fprintf(stderr,"native weapon %08lx: %s\n",(unsigned long)weapon_handle,error);
            mac_engine_tags_unbind();mac_native_tags_free(&registry);free(bytes);return 1;
        }
        if(!mac_native_tags_publish(&registry,weapon_owned.handle,weapon_owned.group,weapon_owned.root,
            weapon_owned.root_bytes,weapon_owned.bytes_owned,weapon_owned.owner,mac_native_object_owner_free,&error)) {
            mac_native_object_tag_free(&weapon_owned);mac_engine_tags_unbind();mac_native_tags_free(&registry);free(bytes);return 1;
        }
        weapon_owned.owner=NULL;weapon=weapon_definition_get((long)weapon_handle);
        CHECK(weapon->object.type==2);CHECK(weapon->weapon.magazines.count==1);CHECK(weapon->weapon.triggers.count>=1);
        CHECK(tag_get(ITEM_DEFINITION_TAG,(long)weapon_handle)==weapon);CHECK(tag_get(OBJECT_DEFINITION_TAG,(long)weapon_handle)==weapon);
        printf("native weapon published: handle=%08lx root_bytes=%lu owned_bytes=%lu values=%lu digest=%08lx\n",
            (unsigned long)weapon_handle,(unsigned long)weapon_owned.root_bytes,(unsigned long)weapon_owned.bytes_owned,
            (unsigned long)weapon_owned.converted_values,(unsigned long)weapon_owned.field_digest);
        mutations(&cache,weapon_handle,1);mac_native_object_tag_free(&weapon_owned);
    }
    for(i=0;i<cache.tag_count;++i) {
        struct mac_native_object_tag scenery,wrong;
        struct object_definition *definition;
        struct reference_inventory refs={0,0,0};
        int sound,ok;
        if(!mac_retail_tag_get(&cache,i,&tag) ||
           (tag.group!=UINT32_C(0x7363656e) && tag.group!=UINT32_C(0x73736365)))continue;
        sound=tag.group==UINT32_C(0x73736365);
        if(sound)++sound_scenery_count;else ++scenery_count;
        ok=sound?mac_native_sound_scenery_decode(&cache,tag.handle,&scenery,&error):
            mac_native_scenery_decode(&cache,tag.handle,&scenery,&error);
        CHECK(ok);
        if(!ok){fprintf(stderr,"scenery %s: %s\n",tag.name,error);continue;}
        definition=(struct object_definition *)scenery.root;
        CHECK(scenery.root_bytes==sizeof(*definition));
        CHECK(definition->object.type==(sound?11:6));
        CHECK(mac_native_object_visit_references(&scenery,inventory_reference,&refs));
        CHECK(!(sound?mac_native_sound_scenery_decode(&cache,tag.handle^UINT32_C(0x10000),&wrong,&error):
            mac_native_scenery_decode(&cache,tag.handle^UINT32_C(0x10000),&wrong,&error)));
        mac_native_object_tag_free(&wrong);
        printf("native scenery: %s handle=%08lx references=%u digest=%08lx\n",tag.name,
            (unsigned long)tag.handle,refs.count,(unsigned long)scenery.field_digest);
        mac_native_object_tag_free(&scenery);
    }
    CHECK(scenery_count==19);CHECK(sound_scenery_count==1);
    /* Ownership must not depend on the original LE map surviving. */
    free(bytes);
    CHECK(mac_native_object_visit_references(&owned,inventory_reference,&after));
    CHECK(before.count==after.count && before.digest==after.digest);
    printf("native biped direct references=%u digest=%08lx; transitive closure not yet ready\n",before.count,(unsigned long)before.digest);
    CHECK(biped->unit.initial_weapons.count==2);CHECK(biped->biped.collision_radius>.19f);
    CHECK(!strcmp(biped->object.model.name,"characters\\cyborg\\cyborg"));
    CHECK(((struct object_attachment_definition *)biped->object.attachments.address)[0].type.index!=(long)UINT32_MAX);
    printf("native biped published: root_bytes=%lu owned_bytes=%lu values=%lu digest=%08lx checks=%u failures=%u\n",
        (unsigned long)owned.root_bytes,(unsigned long)owned.bytes_owned,(unsigned long)owned.converted_values,
        (unsigned long)owned.field_digest,checks,failures);
    mac_engine_tags_unbind();mac_native_tags_free(&registry);
    mac_native_object_tag_free(&owned);mac_native_object_tag_free(&owned);
    printf("native object rejection checks=%u failures=%u\n",mutation_checks,mutation_failures);
    return failures || mutation_failures?1:0;
}
