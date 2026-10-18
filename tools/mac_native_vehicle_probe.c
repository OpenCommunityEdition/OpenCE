#include "cseries.h"
#include "native_vehicle_layout.h"
#include "objects/object_types.h"
#include "native_object_tags.h"
#include "native_scenario_objects.h"
#include "native_physics_tags.h"
#include "native_physics_layout.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#undef malloc
#undef free
#undef memcpy
#undef memset
#undef memcmp

static unsigned checks,failures;
#define CHECK(x) do { ++checks; if(!(x)){++failures;fprintf(stderr,"FAIL %s\n",#x);} } while(0)

struct reference_summary { unsigned count,model_count,vehicle_effects;uint32_t digest; };
static int visit_reference(void *context,uint32_t group,uint32_t handle)
{
    struct reference_summary *s=(struct reference_summary *)context;
    ++s->count;s->digest=(s->digest^group)*UINT32_C(16777619);
    s->digest=(s->digest^handle)*UINT32_C(16777619);
    if(group==UINT32_C(0x6d6f6432))++s->model_count;
    if(group==UINT32_C(0x666f6f74) || group==UINT32_C(0x65666665) || group==UINT32_C(0x736e6421))++s->vehicle_effects;
    return 1;
}

static uint32_t read32(const unsigned char *p)
{ return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static void write32(unsigned char *p,uint32_t v)
{ p[0]=(unsigned char)v;p[1]=(unsigned char)(v>>8);p[2]=(unsigned char)(v>>16);p[3]=(unsigned char)(v>>24); }
static void write16(unsigned char *p,uint16_t v)
{ p[0]=(unsigned char)v;p[1]=(unsigned char)(v>>8); }

static void rejected_mutation(const struct mac_retail_cache *cache,uint32_t handle,
    unsigned char *field,unsigned size,uint32_t value,const char *label)
{
    struct mac_native_object_tag out,zero;
    unsigned char saved[4];const char *error=NULL;int accepted;
    if(size>sizeof(saved)){CHECK(0);return;}
    memcpy(saved,field,size);
    if(size==2)write16(field,(uint16_t)value);else write32(field,value);
    memset(&out,0xa5,sizeof(out));memset(&zero,0,sizeof(zero));
    accepted=mac_native_vehicle_decode(cache,handle,&out,&error);
    ++checks;
    if(accepted || memcmp(&out,&zero,sizeof(out))) {
        ++failures;fprintf(stderr,"FAIL %s (accepted=%d error=%s)\n",label,accepted,error?error:"none");
    }
    mac_native_object_tag_free(&out);memcpy(field,saved,size);
}

static void vehicle_mutations(const struct mac_retail_cache *cache,uint32_t handle)
{
    struct mac_retail_tag tag;unsigned char *disk;
    if(!mac_retail_tag_get(cache,handle&65535,&tag)){CHECK(0);return;}
    disk=(unsigned char *)mac_retail_tag_span(cache,tag.address,sizeof(struct vehicle_definition));
    if(!disk){CHECK(0);return;}
    rejected_mutation(cache,handle,disk,2,0xffff,"wrong tag definition type rejected");
    rejected_mutation(cache,handle,disk+offsetof(struct vehicle_definition,vehicle_type),2,0xffff,
        "vehicle type enum rejected");
    rejected_mutation(cache,handle,disk+offsetof(struct vehicle_definition,function_modes),2,37,
        "vehicle function-mode enum rejected");
    rejected_mutation(cache,handle,disk+offsetof(struct vehicle_definition,unit.object.model)+12,4,
        read32(disk+offsetof(struct vehicle_definition,unit.object.model)+12)^UINT32_C(0x00010000),
        "vehicle model salted handle rejected");
    rejected_mutation(cache,handle,disk+offsetof(struct vehicle_definition,unit.unit.seats),4,
        UINT32_MAX,"vehicle seat block count rejected");
}

static void physics_rejected_mutation(const struct mac_retail_cache *cache,uint32_t handle,
    unsigned char *field,unsigned size,uint32_t value,const char *label)
{
    struct mac_native_physics_tag out,zero;unsigned char saved[4];const char *error=NULL;int accepted;
    if(size>sizeof(saved)){CHECK(0);return;}
    memcpy(saved,field,size);
    if(size==2)write16(field,(uint16_t)value);else write32(field,value);
    memset(&out,0xa5,sizeof(out));memset(&zero,0,sizeof(zero));
    accepted=mac_native_physics_decode(cache,handle,&out,&error);
    ++checks;
    if(accepted || memcmp(&out,&zero,sizeof(out))) {
        ++failures;fprintf(stderr,"FAIL %s (accepted=%d error=%s)\n",label,accepted,error?error:"none");
    }
    mac_native_physics_tag_free(&out);memcpy(field,saved,size);
}

static void physics_mutations(const struct mac_retail_cache *cache,uint32_t handle)
{
    struct mac_retail_tag tag;unsigned char *root,*mass_points;
    if(!mac_retail_tag_get(cache,handle&65535,&tag)){CHECK(0);return;}
    root=(unsigned char *)mac_retail_tag_span(cache,tag.address,sizeof(struct physics_definition));
    if(!root){CHECK(0);return;}
    physics_rejected_mutation(cache,handle,root,4,UINT32_C(0x7f800000),"non-finite physics scalar rejected");
    physics_rejected_mutation(cache,handle,root+offsetof(struct physics_definition,powered_mass_points),4,
        MAXIMUM_POWERED_MASS_POINTS_PER_PHYSICS+1,"oversized powered-mass-point block rejected");
    physics_rejected_mutation(cache,handle,root+offsetof(struct physics_definition,mass_points),4,
        UINT32_MAX,"oversized physics mass-point block rejected");
    mass_points=(unsigned char *)mac_retail_tag_span(cache,
        read32(root+offsetof(struct physics_definition,mass_points)+4),sizeof(struct mass_point_definition));
    if(!mass_points){CHECK(0);return;}
    physics_rejected_mutation(cache,handle,mass_points+offsetof(struct mass_point_definition,friction_type),2,
        UINT16_MAX,"invalid mass-point friction enum rejected");
    physics_rejected_mutation(cache,handle,mass_points+offsetof(struct mass_point_definition,powered_mass_point_index),2,
        UINT16_C(0x7fff),"invalid powered-mass-point index rejected");
}

int main(int argc,char **argv)
{
    FILE *file;long length;unsigned char *bytes;struct mac_retail_cache cache;
    struct mac_native_scenario_objects scenario;struct mac_native_object_tag roots[5];
    struct mac_native_physics_tag warthog_physics;
    struct reference_summary refs[5];const char *error=NULL;unsigned i;
    if(argc!=2)return 2;
    file=fopen(argv[1],"rb");if(!file)return 2;
    if(fseek(file,0,SEEK_END)||(length=ftell(file))<2048||length>0x30000000L||fseek(file,0,SEEK_SET)) { fclose(file);return 2; }
    bytes=(unsigned char *)malloc((size_t)length);if(!bytes){fclose(file);return 2;}
    if(fread(bytes,1,(size_t)length,file)!=(size_t)length){free(bytes);fclose(file);return 2;}fclose(file);
    if(!mac_retail_open(bytes,(size_t)length,&cache,&error)) {
        fprintf(stderr,"map: %s\n",error?error:"invalid");free(bytes);return 1;
    }
    memset(&scenario,0,sizeof(scenario));memset(roots,0,sizeof(roots));memset(refs,0,sizeof(refs));memset(&warthog_physics,0,sizeof(warthog_physics));
    CHECK(mac_native_scenario_objects_decode(&cache,&scenario,&error));
    if(!scenario.owner){free(bytes);return 1;}
    CHECK(scenario.families[MAC_SCENARIO_VEHICLE].placement_count==28);
    CHECK(scenario.families[MAC_SCENARIO_VEHICLE].palette_count==5);
    for(i=0;i<5;++i) {
        const struct mac_native_scenario_palette *palette=&scenario.families[MAC_SCENARIO_VEHICLE].palette[i];
        struct vehicle_definition *vehicle;unsigned mode;long j;
        struct mac_retail_tag raw_tag;const unsigned char *raw_vehicle=NULL;
        CHECK(palette->group_tag==UINT32_C(0x76656869));
        CHECK(palette->handle!=UINT32_MAX && palette->name && palette->name_length>0);
        if(palette->handle==UINT32_MAX || !palette->name)continue;
        if(!mac_native_vehicle_decode(&cache,palette->handle,&roots[i],&error)) {
            fprintf(stderr,"vehicle %s (%08lx): %s\n",palette->name,(unsigned long)palette->handle,error?error:"decode failed");
            ++failures;continue;
        }
        vehicle=(struct vehicle_definition *)roots[i].root;
        if(mac_retail_tag_get(&cache,palette->handle&65535,&raw_tag))
            raw_vehicle=mac_retail_tag_span(&cache,raw_tag.address,sizeof(*vehicle));
        CHECK(raw_vehicle!=NULL);
        if(raw_vehicle) {
            CHECK(!memcmp(vehicle->unknown324,raw_vehicle+offsetof(struct vehicle_definition,unknown324),sizeof(vehicle->unknown324)));
            CHECK(!memcmp(vehicle->unknown368,raw_vehicle+offsetof(struct vehicle_definition,unknown368),sizeof(vehicle->unknown368)));
        }
        CHECK(roots[i].group==UINT32_C(0x76656869));
        CHECK(roots[i].root_bytes==sizeof(*vehicle));
        CHECK(vehicle->unit.object.type==_object_type_vehicle);
        CHECK(vehicle->vehicle_type>=0 && vehicle->vehicle_type<7);
        for(mode=0;mode<4;++mode)CHECK(vehicle->function_modes[mode]>=0 && vehicle->function_modes[mode]<37);
        CHECK(vehicle->suspension_sound.group_tag==UINT32_C(0x736e6421));
        CHECK(vehicle->crash_sound.group_tag==UINT32_C(0x736e6421));
        CHECK(vehicle->material_effects.group_tag==UINT32_C(0x666f6f74));
        CHECK(vehicle->effect.group_tag==UINT32_C(0x65666665));
        CHECK(vehicle->suspension_sound.index==NONE || vehicle->suspension_sound.name!=NULL);
        CHECK(vehicle->crash_sound.index==NONE || vehicle->crash_sound.name!=NULL);
        CHECK(vehicle->material_effects.index==NONE || vehicle->material_effects.name!=NULL);
        CHECK(vehicle->effect.index==NONE || vehicle->effect.name!=NULL);
        CHECK(mac_native_object_visit_references(&roots[i],visit_reference,&refs[i]));
        CHECK(refs[i].count>0 && refs[i].model_count==1);
        printf("vehicle root %u: %s handle=%08lx type=%d mode=%d refs=%u digest=%08lx bytes=%lu\n",
            i,palette->name,(unsigned long)palette->handle,vehicle->vehicle_type,vehicle->function_modes[0],
            refs[i].count,(unsigned long)refs[i].digest,(unsigned long)roots[i].bytes_owned);
        printf("  model=%08lx collision=%08lx animation=%08lx physics=%08lx seats=%ld initial_weapons=%ld attachments=%ld\n",
            (unsigned long)vehicle->unit.object.model.index,(unsigned long)vehicle->unit.object.collision_model.index,
            (unsigned long)vehicle->unit.object.animation_graph.index,(unsigned long)vehicle->unit.object.physics.index,
            vehicle->unit.unit.seats.count,vehicle->unit.unit.initial_weapons.count,vehicle->unit.object.attachments.count);
        for(j=0;j<vehicle->unit.unit.initial_weapons.count;++j) {
            struct tag_reference *weapon=(struct tag_reference *)((unsigned char *)vehicle->unit.unit.initial_weapons.address+j*36);
            printf("  initial weapon[%ld]=%08lx group=%08lx %s\n",j,(unsigned long)weapon->index,
                (unsigned long)weapon->group_tag,weapon->name?weapon->name:"(no name)");
        }
        for(j=0;j<vehicle->unit.unit.seats.count;++j) {
            struct unit_seat *seat=(struct unit_seat *)vehicle->unit.unit.seats.address+j;
            printf("  seat[%ld]=%s built-in=%08lx %s\n",j,seat->label,(unsigned long)seat->built_in_actor_reference.index,
                seat->built_in_actor_reference.name?seat->built_in_actor_reference.name:"(no name)");
        }
        for(j=0;j<vehicle->unit.object.attachments.count;++j) {
            struct object_attachment_definition *attachment=(struct object_attachment_definition *)vehicle->unit.object.attachments.address+j;
            printf("  attachment[%ld]=%08lx group=%08lx %s\n",j,(unsigned long)attachment->type.index,
                (unsigned long)attachment->type.group_tag,
                attachment->type.name?attachment->type.name:"(no name)");
        }
    }
    if(roots[0].root)vehicle_mutations(&cache,roots[0].handle);
    if(roots[0].root) {
        struct vehicle_definition *warthog=(struct vehicle_definition *)roots[0].root;
        struct physics_definition *physics;
        if(!mac_native_physics_decode(&cache,(uint32_t)warthog->unit.object.physics.index,&warthog_physics,&error)) {
            fprintf(stderr,"Warthog physics: %s\n",error?error:"decode failed");++failures;
        } else {
            physics=(struct physics_definition *)warthog_physics.root;
            CHECK(physics->mass>0.f);
            CHECK(physics->inertial_matrix.count==2);
            CHECK(physics->powered_mass_points.count<=MAXIMUM_POWERED_MASS_POINTS_PER_PHYSICS);
            CHECK(physics->mass_points.count>0 && physics->mass_points.count<=MAXIMUM_MASS_POINTS_PER_PHYSICS);
            printf("Warthog physics %08lx: mass=%.3f inertia=%ld powered=%ld points=%ld bytes=%lu\n",
                (unsigned long)warthog_physics.handle,physics->mass,physics->inertial_matrix.count,
                physics->powered_mass_points.count,physics->mass_points.count,(unsigned long)warthog_physics.bytes_owned);
            physics_mutations(&cache,warthog_physics.handle);
        }
    }
    /* Decoded roots own all nested data and reference names. They must remain
       usable after the memory-mapped cache backing store is released. */
    free(bytes);bytes=NULL;
    for(i=0;i<5;++i)if(roots[i].root) {
        struct reference_summary after={0,0,0,0};
        struct vehicle_definition *vehicle=(struct vehicle_definition *)roots[i].root;
        CHECK(vehicle->unit.object.model.name && vehicle->unit.object.model.name[0]);
        CHECK(mac_native_object_visit_references(&roots[i],visit_reference,&after));
        CHECK(after.count==refs[i].count && after.digest==refs[i].digest);
        mac_native_object_tag_free(&roots[i]);
    }
    if(warthog_physics.root) {
        struct physics_definition *physics=(struct physics_definition *)warthog_physics.root;
        CHECK(physics->mass>0.f && physics->mass_points.count>0);
        mac_native_physics_tag_free(&warthog_physics);
    }
    mac_native_scenario_objects_free(&scenario);
    printf("native vehicle checks=%u failures=%u\n",checks,failures);
    return failures?1:0;
}
