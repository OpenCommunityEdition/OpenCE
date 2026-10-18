/* Opt-in checks around one genuine scenario-placed vehicle. */
#include "cseries.h"
#include "objects/objects.h"
#include "objects/object_types.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "units/vehicle_datum.h"
#include "units/vehicle_definitions.h"
#include "units/unit_definitions.h"
#include "objects/widgets/antenna.h"
#include "mac_vehicle_runtime_probe.h"

static int check(int condition,const char *name)
{
    if(!condition)fprintf(stderr,"vehicle runtime FAIL: %s\n",name);
    return condition;
}

static int finite_real(real value)
{
    return value==value && value<=REAL_MAX && value>=-REAL_MAX;
}

int mac_vehicle_runtime_place(uint32_t expected_definition,long *object_index)
{
    struct scenario *scenario=global_scenario_get();
    struct scenario_object_datum *record=NULL;
    struct scenario_object_palette_entry *entry;
    struct vehicle_datum *vehicle;
    long i,index;
    unsigned before,antenna_before;
    int ok=1;
    if(!check(scenario!=NULL,"scenario is loaded") || !check(object_index!=NULL,"output index exists"))return 0;
    *object_index=NONE;
    ok&=check(scenario->vehicle_palette.count>0,"vehicle palette is nonempty");
    ok&=check(scenario->vehicles.count>0,"scenario has vehicle placements");
    ok&=check(expected_definition==UINT32_C(0xe3d40260),
        "Blood Gulch vehicle palette zero is the loaded Warthog root e3d40260");
    if(!ok)return 0;
    entry=(struct scenario_object_palette_entry *)TAG_BLOCK_GET_ELEMENT(
        &scenario->vehicle_palette,0,struct scenario_object_palette_entry);
    ok&=check(entry->reference.index==(long)expected_definition,
        "palette entry zero matches the preloaded native Warthog definition");
    if(!ok)return 0;
    for(i=0;i<scenario->vehicles.count;++i){
        struct scenario_object_datum *candidate=(struct scenario_object_datum *)
            ((byte *)scenario->vehicles.address+i*0x78U);
        if(candidate->palette_entry_index==0){record=candidate;break;}
    }
    ok&=check(record!=NULL,"map contains an actual placement for vehicle palette entry zero");
    if(!ok)return 0;
    before=object_header_data->actual_count;
    antenna_before=antenna_data?antenna_data->actual_count:0;
    fprintf(stderr,"vehicle runtime: scenario placement=%ld palette=%d position=(%.9g,%.9g,%.9g) rotation=(%.9g,%.9g,%.9g)\n",
        i,record->palette_entry_index,record->position.x,record->position.y,record->position.z,
        record->rotation.pitch,record->rotation.roll,record->rotation.yaw);
    index=object_new_from_scenario(record,&scenario->vehicle_palette);
    ok&=check(index!=NONE,"genuine object_new_from_scenario succeeds");
    if(index==NONE)return 0;
    vehicle=vehicle_datum_get(index);
    ok&=check(vehicle->definition_index==(long)expected_definition,"created vehicle definition matches palette handle");
    ok&=check(vehicle->object.type==_object_type_vehicle,"created object has vehicle runtime type");
    ok&=check(object_header_data->actual_count>=before+1,
        "constructor added the requested vehicle to the live object set");
    ok&=check(antenna_data && antenna_data->actual_count==antenna_before+1,
        "vehicle widget lifecycle creates its source-defined antenna datum");
    ok&=check(vehicle->object.location.cluster_index!=NONE,"placed vehicle resolves to a world cluster");
    {
        struct unit_definition *definition=vehicle_definition_get((long)expected_definition);
        real expected_z=record->position.z+definition->object.bounding_radius*0.5f;
        ok&=check(vehicle->object.position.x==record->position.x &&
            vehicle->object.position.y==record->position.y && vehicle->object.position.z==expected_z,
            "created vehicle position includes vehicle_new's source-defined half-radius height");
    }
    *object_index=index;
    fprintf(stderr,"vehicle runtime: created=%08lx type=%d definition=%08lx cluster=%d position=(%.9g,%.9g,%.9g) speed=%.9g ground_ticks=%u objects=%u\n",
        index,vehicle->object.type,(unsigned long)vehicle->definition_index,
        vehicle->object.location.cluster_index,vehicle->object.position.x,
        vehicle->object.position.y,vehicle->object.position.z,vehicle->vehicle.speed,
        vehicle->vehicle.on_ground_ticks,object_header_data->actual_count);
    return ok;
}

int mac_vehicle_runtime_after_update(long object_index,uint32_t expected_definition)
{
    struct vehicle_datum *vehicle=vehicle_datum_try_and_get(object_index);
    int ok=1;
    ok&=check(vehicle!=NULL,"vehicle survives the real objects_update loop");
    if(!vehicle)return 0;
    ok&=check(vehicle->definition_index==(long)expected_definition,"updated vehicle retains its definition");
    ok&=check(vehicle->object.type==_object_type_vehicle,"updated datum remains a vehicle");
    ok&=check(vehicle->object.location.cluster_index!=NONE,"updated vehicle remains connected to a world cluster");
    ok&=check(finite_real(vehicle->object.position.x) && finite_real(vehicle->object.position.y) &&
        finite_real(vehicle->object.position.z) && finite_real(vehicle->vehicle.speed),
        "real vehicle update leaves finite transform and speed");
    fprintf(stderr,"vehicle runtime: after objects_update handle=%08lx position=(%.9g,%.9g,%.9g) velocity=(%.9g,%.9g,%.9g) speed=%.9g slide=%.9g turn=%.9g wheel=%.9g ground_ticks=%u\n",
        object_index,vehicle->object.position.x,vehicle->object.position.y,vehicle->object.position.z,
        vehicle->object.translational_velocity.i,vehicle->object.translational_velocity.j,
        vehicle->object.translational_velocity.k,vehicle->vehicle.speed,vehicle->vehicle.slide,
        vehicle->vehicle.turn,vehicle->vehicle.wheel,vehicle->vehicle.on_ground_ticks);
    return ok;
}
