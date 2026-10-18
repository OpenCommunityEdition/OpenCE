#include "cseries.h"
#include "native_vehicle_layout.h"
#include "native_physics_layout.h"
#include "native_object_tags.h"
#include "native_scenario_objects.h"
#include "native_actor_assets.h"
#include "native_model_tags.h"
#include "native_collision_tags.h"
#include "native_animation_tags.h"
#include "engine_tags.h"
#include "objects/object_definitions.h"
#include "units/unit_definitions.h"
#include "units/vehicle_definitions.h"
#include "objects/object_types.h"
#include "items/weapon_definitions.h"
#include "models/model_definitions.h"
#include "models/model_animation_definitions.h"
#include "physics/collision_model_definitions.h"
#include "physics/physics_definitions.h"
#include "sound/sound_definitions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#undef malloc
#undef free
#undef memset
#undef memcmp

static unsigned checks,failures;
#define CHECK(x) do{++checks;if(!(x)){++failures;fprintf(stderr,"FAIL %s\n",#x);}}while(0)

int main(int argc,char **argv)
{
    FILE *file;long length;unsigned char *bytes,*bitmap_bytes=NULL;size_t bitmap_size=0;struct mac_retail_cache cache;
    struct mac_native_scenario_objects scenario;struct mac_actor_assets assets;
    const char *error=NULL;uint32_t vehicle_handle,physics_handle,weapon_handle;unsigned i;
    struct vehicle_definition *vehicle;struct physics_definition *physics;struct weapon_definition *weapon;
    struct animation_graph *animation_graph;
    struct object_attachment_definition *attachment;
    if(argc!=2 && argc!=3)return 2;
    if(argc==3) {
        file=fopen(argv[2],"rb");if(!file)return 2;
        if(fseek(file,0,SEEK_END)||(length=ftell(file))<2048||length>0x50000000L||fseek(file,0,SEEK_SET)){fclose(file);return 2;}
        bitmap_bytes=(unsigned char *)malloc((size_t)length);if(!bitmap_bytes){fclose(file);return 2;}
        if(fread(bitmap_bytes,1,(size_t)length,file)!=(size_t)length){free(bitmap_bytes);fclose(file);return 2;}
        fclose(file);bitmap_size=(size_t)length;
    }
    file=fopen(argv[1],"rb");if(!file){free(bitmap_bytes);return 2;}
    if(fseek(file,0,SEEK_END)||(length=ftell(file))<2048||length>0x30000000L||fseek(file,0,SEEK_SET)){free(bitmap_bytes);fclose(file);return 2;}
    bytes=(unsigned char *)malloc((size_t)length);if(!bytes){free(bitmap_bytes);fclose(file);return 2;}
    if(fread(bytes,1,(size_t)length,file)!=(size_t)length){free(bitmap_bytes);free(bytes);fclose(file);return 2;}fclose(file);
    memset(&scenario,0,sizeof(scenario));memset(&assets,0,sizeof(assets));
    if(!mac_retail_open(bytes,(size_t)length,&cache,&error)||
       !mac_native_scenario_objects_decode(&cache,&scenario,&error)) {
        fprintf(stderr,"vehicle asset setup: %s\n",error?error:"invalid map");free(bitmap_bytes);free(bytes);return 1;
    }
    if(scenario.families[MAC_SCENARIO_VEHICLE].palette_count!=5 ||
       strcmp(scenario.families[MAC_SCENARIO_VEHICLE].palette[0].name,"vehicles\\warthog\\mp_warthog")) {
        fprintf(stderr,"unexpected Blood Gulch vehicle palette\n");free(bitmap_bytes);free(bytes);mac_native_scenario_objects_free(&scenario);return 1;
    }
    vehicle_handle=scenario.families[MAC_SCENARIO_VEHICLE].palette[0].handle;
    CHECK(mac_native_tags_create(&cache,&assets.tags,&error));
    if(!assets.tags.state){free(bytes);mac_native_scenario_objects_free(&scenario);return 1;}
    if(!mac_actor_assets_load_vehicle_with_bitmaps(&cache,bitmap_bytes,bitmap_size,&assets,vehicle_handle,&error)) {
        fprintf(stderr,"Warthog assets: %s\n",error?error:"loader failed");
        {
            struct mac_native_object_tag diagnostic;struct object_definition *o;
            if(mac_native_vehicle_decode(&cache,vehicle_handle,&diagnostic,&error)) {
                struct vehicle_definition *v=(struct vehicle_definition *)diagnostic.root;
                struct model *m=(struct model *)mac_native_tags_get(&assets.tags,UINT32_C(0x6d6f6465),(uint32_t)v->unit.object.model.index,sizeof(struct model),&error);
                struct collision_model *c=(struct collision_model *)mac_native_tags_get(&assets.tags,UINT32_C(0x636f6c6c),(uint32_t)v->unit.object.collision_model.index,sizeof(struct collision_model),&error);
                struct animation_graph *g=(struct animation_graph *)mac_native_tags_get(&assets.tags,UINT32_C(0x616e7472),(uint32_t)v->unit.object.animation_graph.index,sizeof(struct animation_graph),&error);
                struct mac_retail_tag graph_tag;uint32_t graph_handle=(uint32_t)v->unit.object.animation_graph.index;
                o=(struct object_definition *)&v->unit.object;
                memset(&graph_tag,0,sizeof(graph_tag));mac_retail_tag_get(&cache,graph_handle&65535,&graph_tag);
                fprintf(stderr,"Warthog graph=%08lx %s counts model/collision/graph=%ld/%ld/%ld clips=%ld first-clip-nodes=%d vehicle-animations=%ld; types=%d/%d\n",(unsigned long)graph_handle,graph_tag.name?graph_tag.name:"?",m?m->nodes.count:-1L,c?c->nodes.count:-1L,g?g->nodes.count:-1L,g?g->animations.count:-1L,(g&&g->animations.count)?((struct animation *)g->animations.address)[0].node_count:-1,g?g->vehicle_animations.count:-1L,o->object.type,_object_type_vehicle);
                if(v->unit.unit.initial_weapons.count) {
                    uint32_t wh=(uint32_t)((struct tag_reference *)v->unit.unit.initial_weapons.address)->index;
                    struct mac_native_object_tag wt;struct mac_retail_tag mt;struct weapon_definition *wd;
                    if(mac_native_weapon_decode(&cache,wh,&wt,&error)) {
                        wd=(struct weapon_definition *)wt.root;memset(&mt,0,sizeof(mt));mac_retail_tag_get(&cache,(uint32_t)wd->object.model.index&65535,&mt);
                        fprintf(stderr,"Warthog weapon=%08lx model=%08lx %s\n",(unsigned long)wh,(unsigned long)wd->object.model.index,mt.name?mt.name:"?");mac_native_object_tag_free(&wt);
                    } else fprintf(stderr,"Warthog weapon decode: %s\n",error?error:"failed");
                }
                mac_native_object_tag_free(&diagnostic);
            }
        }
        free(bitmap_bytes);free(bytes);mac_actor_assets_free(&assets);mac_native_scenario_objects_free(&scenario);return 1;
    }
    free(bitmap_bytes);bitmap_bytes=NULL;
    vehicle=(struct vehicle_definition *)mac_native_tags_get(&assets.tags,UINT32_C(0x76656869),vehicle_handle,sizeof(*vehicle),&error);
    CHECK(vehicle!=NULL);
    if(!vehicle)goto done;
    CHECK(mac_engine_tags_bind(&assets.tags));
    CHECK(vehicle_definition_get((long)vehicle_handle)==&vehicle->unit);
    CHECK(unit_definition_get((long)vehicle_handle)==&vehicle->unit);
    CHECK((void *)object_definition_get((long)vehicle_handle)==(void *)vehicle);
    CHECK(!strcmp(tag_get_name((long)vehicle_handle),"vehicles\\warthog\\mp_warthog"));
    CHECK(vehicle->unit.object.type==_object_type_vehicle);
    CHECK(vehicle->unit.object.model.group_tag==UINT32_C(0x6d6f6465));
    CHECK(mac_native_tags_group(&assets.tags,(uint32_t)vehicle->unit.object.model.index)==UINT32_C(0x6d6f6465));
    CHECK(mac_native_tags_group(&assets.tags,(uint32_t)vehicle->unit.object.collision_model.index)==UINT32_C(0x636f6c6c));
    CHECK(mac_native_tags_group(&assets.tags,(uint32_t)vehicle->unit.object.animation_graph.index)==UINT32_C(0x616e7472));
    animation_graph=(struct animation_graph *)tag_get('antr',(long)vehicle->unit.object.animation_graph.index);
    CHECK(animation_graph!=NULL && animation_graph->vehicle_animations.count==1);
    CHECK(animation_graph!=NULL && animation_graph->nodes.count==0 && animation_graph->object_overlays.count>0 && animation_graph->animations.count==11);
    CHECK(animation_graph!=NULL && ((struct animation *)animation_graph->animations.address)[0].node_count==18);
    physics_handle=(uint32_t)vehicle->unit.object.physics.index;
    physics=(struct physics_definition *)tag_get('phys',(long)physics_handle);
    CHECK(physics!=NULL && physics->mass>0.f);
    CHECK(physics->inertial_matrix.count==2);
    CHECK(physics->powered_mass_points.count==2 && physics->mass_points.count==15);
    CHECK(vehicle->unit.unit.seats.count==3 && vehicle->unit.unit.initial_weapons.count==1);
    for(i=0;i<(unsigned)vehicle->unit.unit.seats.count;++i)
        CHECK(((struct unit_seat *)vehicle->unit.unit.seats.address)[i].built_in_actor_reference.index==NONE);
    weapon_handle=(uint32_t)((struct tag_reference *)vehicle->unit.unit.initial_weapons.address)->index;
    weapon=(struct weapon_definition *)tag_get('weap',(long)weapon_handle);
    CHECK(weapon->object.type==_object_type_weapon);
    CHECK(weapon->object.model.group_tag==UINT32_C(0x6d6f6465));
    CHECK(weapon->object.collision_model.index==NONE || mac_native_tags_group(&assets.tags,(uint32_t)weapon->object.collision_model.index)==UINT32_C(0x636f6c6c));
    CHECK(weapon->object.animation_graph.index==NONE || mac_native_tags_group(&assets.tags,(uint32_t)weapon->object.animation_graph.index)==UINT32_C(0x616e7472));
    attachment=vehicle->unit.object.attachments.address;
    CHECK(vehicle->unit.object.attachments.count==4);
    for(i=0;i<(unsigned)vehicle->unit.object.attachments.count;++i) {
        uint32_t expected=attachment[i].type.group_tag==UINT32_C(0x6c696768)?UINT32_C(0x6c696768):UINT32_C(0x6c736e64);
        if(attachment[i].type.index!=NONE)CHECK(mac_native_tags_group(&assets.tags,(uint32_t)attachment[i].type.index)==expected);
    }
    CHECK(assets.published_count>10);
    free(bytes);bytes=NULL;
    CHECK(mac_native_tags_name(&assets.tags,vehicle_handle)!=NULL);
    CHECK(vehicle->unit.object.model.name!=NULL && vehicle->unit.object.model.name[0]);
    printf("Warthog asset batch: published=%lu owned_bytes=%lu physics=%08lx mass=%.1f points=%ld/%ld; no object_new invoked\n",
        (unsigned long)assets.published_count,(unsigned long)mac_native_tags_owned_bytes(&assets.tags),
        (unsigned long)physics_handle,physics->mass,physics->powered_mass_points.count,physics->mass_points.count);
done:
    mac_engine_tags_unbind();mac_actor_assets_free(&assets);mac_native_scenario_objects_free(&scenario);free(bitmap_bytes);free(bytes);
    printf("vehicle asset checks=%u failures=%u\n",checks,failures);
    return failures?1:0;
}
