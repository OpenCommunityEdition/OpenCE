/* Bounded execution of the unchanged biped callback or full objects_update.
   This is not a game_tick: callers first apply a genuine queued
   action and publish the real native BSP, animation, collision and matg tags. */
#include "cseries.h"
#include "game/game.h"
#include "game/game_globals.h"
#include "game/players.h"
#include "game/player_queues_new.h"
#include "units/units.h"
#include "units/bipeds.h"
#include "objects/objects.h"
#include "objects/object_lights.h"
#include "scenario/scenario.h"
#include "structures/structure_bsp_definitions.h"
#include "physics/collision_usage.h"
#include "physics/collisions.h"
#include "memory/data.h"
#ifdef MAC_NATIVE_INPUT
#include "input/input.h"
#include "input/input_abstraction.h"
#include "game/player_control_runtime.h"
#define MAC_INPUT_NO_CARBON
#include "mac_input.h"
#include "engine_input.h"
#endif
extern struct data_array *global_decal_data;
#include <float.h>
#ifdef MAC_RUNTIME_VIEW
#define MAC_INPUT_NO_CARBON
#include "mac_runtime_view.h"
#include "mac_live_object_view.h"
extern struct mac_runtime_view *mac_probe_runtime_view;
static int draw_unit_view(long unit_index)
{
    real_point3d position;real_vector3d aim;real_euler_angles2d angles;
    struct mac_bsp_camera camera;struct mac_runtime_view_frame frame;
    const char *error=NULL;
    unit_get_camera_position(unit_index,&position);
    unit_get_aiming_vector(unit_index,&aim);
    euler_angles2d_from_vector3d(&angles,&aim);
    camera.position[0]=position.x;camera.position[1]=position.y;camera.position[2]=position.z;
    camera.yaw=angles.yaw;camera.pitch=angles.pitch;
    if(getenv("HALO_MAC_TERRAIN_OBJECT_OVERLAY")) {
        struct mac_live_object_view_result result;
        if(!mac_live_object_view_draw(mac_probe_runtime_view,unit_index,&camera,&result,&error)) {
            fprintf(stderr,"actual terrain-camera object overlay: %s\n",error?error:"draw failed");return 0;
        }
        printf("terrain-camera live model geometry: biped=%u parts/%u vertices/%u textured/%u unsupported_base/%u unsupported_shader_semantics; weapon=%u parts/%u vertices/%u textured/%u unsupported_base/%u unsupported_shader_semantics; pixel_digest=%08lx->%08lx; diagnostic base-texture renderer, not Xbox shader parity\n",
            result.biped_parts,result.biped_vertices,result.biped_textured_parts,
            result.biped_unsupported_shaders,result.biped_unsupported_semantics,
            result.weapon_parts,result.weapon_vertices,result.weapon_textured_parts,
            result.weapon_unsupported_shaders,result.weapon_unsupported_semantics,
            (unsigned long)result.terrain_digest,(unsigned long)result.overlay_digest);
        return 1;
    }
    if(!mac_runtime_view_draw(mac_probe_runtime_view,&camera,&frame,&error)) {
        fprintf(stderr,"actual unit view: %s\n",error?error:"frame failed");return 0;
    }
    if(frame.frame_number==1 || frame.frame_number%120==0)
        printf("actual unit view: frame=%u pixels=%u renderer=%s\n",frame.frame_number,
            frame.scene_pixels,mac_runtime_view_renderer(mac_probe_runtime_view));
    return frame.scene_pixels>32 && !frame.input.held && !frame.input.pressed && !frame.input.released;
}
#endif
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; fprintf(stderr,"movement FAIL %s line %d\n",#x,__LINE__); } } while(0)
static int finite_value(real value) { return value>=-FLT_MAX && value<=FLT_MAX; }
static int movement_probe(long player_index,int full_objects)
{
    long unit_index=player_get(player_index)->unit_index;
    struct biped_datum *biped;
    real_point3d initial;
    long initial_time=game_time_get();
    short collision_depth=global_current_collision_user_depth;
    unsigned checks=0,failures=0,step,steps=8;
    const char *requested=full_objects?getenv("HALO_MAC_OBJECT_STEPS"):NULL;
    real dx=0.f,dy=0.f,dz=0.f,maximum_z;
    int queued=full_objects && getenv("HALO_MAC_QUEUED_MOVEMENT")!=NULL;
    short old_connection=game_connection();
    struct player_action action;
#ifdef MAC_NATIVE_INPUT
    int native_input=full_objects && getenv("HALO_MAC_NATIVE_INPUT")!=NULL;
    struct mac_input native_state;
    struct mac_input_frame native_frame;
    if(native_input)queued=0;
#endif
    if(requested && *requested) {
        char *end;unsigned long parsed=strtoul(requested,&end,10);
        if(*end || parsed<8 || parsed>120) {
            fprintf(stderr,"HALO_MAC_OBJECT_STEPS must be between 8 and 120\n");return 0;
        }
        steps=(unsigned)parsed;
    }
    CHECK(unit_index!=NONE);CHECK(collision_depth>0);if(failures)return 0;
    biped=biped_get(unit_index);
    CHECK(biped->unit.player_index==player_index);
    CHECK(biped->object.parent_object_index==NONE);
    CHECK(unit_controllable(unit_index));
    CHECK(biped->unit.control_flags==0 && biped->unit.primary_trigger==0.f);
    CHECK(biped->unit.throttle.i*biped->unit.throttle.i+biped->unit.throttle.j*biped->unit.throttle.j>0.01f);
    CHECK(!TEST_FLAG(biped->biped.flags,_biped_no_collision_bit));
    CHECK(!TEST_FLAG(biped->unit.flags,_unit_suspended_bit));
    if(failures)return 0;
    {
        struct structure_bsp *structure=global_structure_bsp_get();
        struct structure_collision_material *materials=structure->collision_materials.address;
        struct structure_runtime_decal *decals=structure->runtime_decals.address;
        CHECK(structure->runtime_decals.count==8);
        /* Independent little-endian inspection of the installed Blood Gulch
           map establishes these exact binary32 values and signed angles. */
        CHECK(decals[0].position.x==99.30181884765625f);
        CHECK(decals[0].position.y==-155.87684631347656f);
        CHECK(decals[0].position.z==2.344130039215088f);
        CHECK(decals[0].palette_index==3 && decals[0].yaw==-120 && decals[0].pitch==42);
        CHECK(structure->collision_materials.count==11);
        CHECK(materials[2].shader.name!=NULL);
        if(materials[2].shader.name)CHECK(!strcmp(materials[2].shader.name,"levels\\b30\\shaders\\metal panels generic ext clean"));
    }
    initial=biped->object.position;maximum_z=initial.z;
 #ifdef MAC_NATIVE_INPUT
    if(native_input) {
        if(steps<60){fprintf(stderr,"native input jump probe needs at least 60 steps\n");return 0;}
        game_connection_set(_game_connection_local);
        if(!update_server_new()){game_connection_set(old_connection);return 0;}
        update_server_start();update_queues_distributed_reset();
        mac_input_init(&native_state);mac_input_focus(&native_state,1);
    }
 #endif
    if(queued) {
        if(steps<60){fprintf(stderr,"queued jump probe needs at least 60 steps\n");return 0;}
        game_connection_set(_game_connection_local);
        if(!update_server_new()){game_connection_set(old_connection);return 0;}
        update_server_start();update_queues_distributed_reset();
        csmemset(&action,0,sizeof(action));
        action.desired_weapon_index=NONE;action.desired_grenade_index=NONE;action.desired_zoom_level=NONE;
        action.desired_facing.yaw=0.75f;action.desired_facing.pitch=-0.125f;
        action.throttle.i=0.375f;action.throttle.j=-0.125f;
    }
    if(full_objects) {
        real_rgb_color illumination;
        lights_illumination_at_point(&initial,&biped->object.location,&illumination);
        CHECK(finite_value(illumination.red) && finite_value(illumination.green) && finite_value(illumination.blue));
        CHECK(illumination.red>=0.f && illumination.red<=1.f && illumination.green>=0.f && illumination.green<=1.f && illumination.blue>=0.f && illumination.blue<=1.f);
        printf("actual spawn illumination=(%.8g,%.8g,%.8g)\n",illumination.red,illumination.green,illumination.blue);
    }
    for(step=0;step<steps && !failures;++step){
        /* Timestamp the callback steps without claiming the rest of the
           engine simulation ran. Queued mode delivers actual per-step actions; otherwise controls
           remain the initial applied action. */
        game_time_set_distributed(initial_time+(long)step+1);
 #ifdef MAC_NATIVE_INPUT
        if(native_input) {
            mac_input_key(&native_state,13,1); /* ANSI W */
            mac_input_key(&native_state,0,1);  /* ANSI A */
            mac_input_key(&native_state,49,step==15); /* Space: real jump control */
            mac_input_snapshot(&native_state,&native_frame);
            mac_engine_input_publish(&native_frame);
            input_frame_begin();input_update();input_abstraction_update();
            CHECK(player_control_update(1.f/30.f)==FALSE);
            update_client_local_ticks(1);players_update_before_game();
            CHECK(biped->unit.throttle.i>0.69f && biped->unit.throttle.i<0.72f);
            CHECK(biped->unit.throttle.j>0.69f && biped->unit.throttle.j<0.72f);
            if(step==15)CHECK(TEST_FLAG(biped->unit.control_flags,_unit_control_jump_bit));
            input_frame_end();
        } else
 #endif
        if(queued) {
            action.control_flags=step==15?FLAG(_unit_control_jump_bit):0;
            update_client_queue_push();update_client_queue(&action);update_client_local_ticks(1);
            players_update_before_game();
            CHECK(biped->unit.throttle.i==action.throttle.i && biped->unit.throttle.j==action.throttle.j);
        }
        fprintf(stderr,"movement: actual %s step %u\n",full_objects?"objects_update":"biped_update",step+1);
        if(full_objects){objects_update();CHECK(global_decal_data!=NULL);
            if(global_decal_data){printf("actual object update decals=%d\n",global_decal_data->actual_count);CHECK(global_decal_data->actual_count>0);}}
        else CHECK(biped_update(unit_index));
#ifdef MAC_RUNTIME_VIEW
        if(mac_probe_runtime_view)CHECK(draw_unit_view(unit_index));
#endif
        CHECK(global_current_collision_user_depth==collision_depth);
        if(steps==8)CHECK(biped->biped.support_surface_index>=0);
        else CHECK(biped->biped.support_surface_index>=NONE &&
            biped->biped.support_surface_index<global_structure_bsp_get()->surfaces.count);
        CHECK(finite_value(biped->object.position.x) && finite_value(biped->object.position.y) && finite_value(biped->object.position.z));
        CHECK(finite_value(biped->object.translational_velocity.i) && finite_value(biped->object.translational_velocity.j) && finite_value(biped->object.translational_velocity.k));
        CHECK(biped->object.location.cluster_index>=0 && biped->object.location.cluster_index<global_structure_bsp_get()->clusters.count);
        if(biped->object.position.z>maximum_z)maximum_z=biped->object.position.z;
        dx=biped->object.position.x-initial.x;dy=biped->object.position.y-initial.y;dz=biped->object.position.z-initial.z;
        CHECK(dx*dx+dy*dy+dz*dz<100.f);
        printf("biped callback step=%u position=(%.8g,%.8g,%.8g) velocity=(%.8g,%.8g,%.8g) cluster=%d support=%ld\n",step+1,
            biped->object.position.x,biped->object.position.y,biped->object.position.z,
            biped->object.translational_velocity.i,biped->object.translational_velocity.j,biped->object.translational_velocity.k,
            biped->object.location.cluster_index,biped->biped.support_surface_index);
    }
    {
        struct collision_result hit;
        real_point3d origin=biped->object.position;
        real_vector3d down;
        origin.z+=0.5f;down.i=0.f;down.j=0.f;down.k=-2.f;
        CHECK(collision_test_vector(FLAG(_collision_test_structure_bit),&origin,&down,unit_index,&hit));
        /* Blood Gulch spawn zero stands on a metal base surface. */
        if(steps==8) {
            CHECK(hit.material_type==_material_thick_metal);
            CHECK(hit.surface_index==1412);
        } else CHECK(hit.surface_index>=0 && hit.surface_index<global_structure_bsp_get()->surfaces.count);
        printf("actual ground material: type=%d surface=%ld\n",hit.material_type,hit.surface_index);
    }
    {
        real_point3d camera;real_vector3d aim;real_euler_angles2d angles;
        unit_get_camera_position(unit_index,&camera);
        unit_get_aiming_vector(unit_index,&aim);
        CHECK(finite_value(camera.x) && finite_value(camera.y) && finite_value(camera.z));
        CHECK(finite_value(aim.i) && finite_value(aim.j) && finite_value(aim.k));
        CHECK(aim.i*aim.i+aim.j*aim.j+aim.k*aim.k>0.5f);
        euler_angles2d_from_vector3d(&angles,&aim);
        CHECK(finite_value(angles.yaw) && finite_value(angles.pitch));
        printf("actual unit camera: position=(%.9g,%.9g,%.9g) yaw=%.9g pitch=%.9g\n",
            camera.x,camera.y,camera.z,angles.yaw,angles.pitch);
    }
 #ifdef MAC_NATIVE_INPUT
    if(native_input) {
        CHECK(maximum_z>initial.z+0.2f);
        printf("native input jump maximum height=%.8g initial=%.8g\n",maximum_z,initial.z);
        update_server_delete();game_connection_set(old_connection);
        mac_engine_input_publish(NULL);
    }
 #endif
    if(queued) {
        CHECK(maximum_z>initial.z+0.2f);
        printf("actual queued jump maximum height=%.8g initial=%.8g\n",maximum_z,initial.z);
        update_server_delete();game_connection_set(old_connection);
    }
    CHECK(dx*dx+dy*dy>0.00000001f);
    game_time_set_distributed(initial_time);
    printf("actual %s movement: %u checks %u failures steps=%u delta=(%.8g,%.8g,%.8g); no full game tick\n",full_objects?"object update":"biped callback",checks,failures,step,dx,dy,dz);
    return failures==0;
}

int mac_player_movement_probe(long player_index){return movement_probe(player_index,0);}
int mac_object_update_probe(long player_index){return movement_probe(player_index,1);}
