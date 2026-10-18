/* Actual object lifecycle and optional full constructor test. --spawn enables
   genuine biped plus recursive starting-weapon creation in the native world.
   --player adds a real local player record, unit association and detach.
   --player-queue additionally exercises actual queued player actions. */
#include "cseries.h"
#include "objects/objects.h"
#include "memory/data.h"
#include "units/units.h"
#include "units/bipeds.h"
#include "items/weapons.h"
#include "game/players.h"
#include "game/local_players.h"
#include "interface/ui_widget.h"
#include "interface/player_ui.h"
#include "interface/event_manager.h"
#include "ai/ai.h"
#include "cutscene/cinematics.h"
#include "physics/breakable_surfaces.h"
#include "physics/collision_usage.h"
#include "interface/hud_unit.h"
#include "structures/structures.h"
#include "effects/decals.h"
#include "native_decal_assets.h"
#include "native_scenario_objects.h"
#include "sound/game_sound.h"
#include "render/render.h"
#include "structures/cluster_partitions.h"
#include "scenario/scenario_definitions.h"
#include "engine_arena.h"
#include "engine_game_runtime.h"
#include "engine_device_state.h"
#ifdef MAC_MODEL_GPU
#include "native_model_gpu_probe.h"
#endif
#include "interface/first_person_weapons.h"
#include "rasterizer/xbox/rasterizer_xbox.h"
#include "engine_world.h"
#include "engine_tags.h"
#include "native_actor_assets.h"
#include "mapped_file.h"
#include "native_globals_tags.h"
#include "native_hud_tags.h"
#ifdef MAC_VEHICLE_RUNTIME
#include "mac_vehicle_runtime_probe.h"
#endif
#include "interface/hud.h"
#include "interface/hud_messaging.h"
#include "interface/interface.h"
#include "game/game_globals.h"
#include "scenario/scenario.h"
#include "structures/structure_bsp_definitions.h"
#ifdef MAC_NATIVE_INPUT
#include "input/input.h"
#include "input/input_abstraction.h"
#include "game/player_control_runtime.h"
#define MAC_INPUT_NO_CARBON
#include "mac_input.h"
#include "engine_input.h"
#endif
#ifdef MAC_RUNTIME_VIEW
#define MAC_INPUT_NO_CARBON
#include "mac_runtime_view.h"
struct mac_runtime_view *mac_probe_runtime_view;
#endif
#undef memcpy
#undef memset
extern struct data_array *light_data;
extern struct data_array *global_decal_data;
extern struct cluster_partition collideable_object_cluster_partition;
extern struct cluster_partition noncollideable_object_cluster_partition;
static unsigned checks,failures;
struct runtime_context {int spawn,player,queue,control,apply,movement,object_update,decal_only,scenery,all_scenery,sound_lifecycle,vehicle,native_input,sky_assets,model_gpu;struct hud_globals_definition *hud;struct game_globals *globals;struct mac_actor_assets *assets;uint32_t vehicle_definition;float point[3];short cluster;};
int mac_constructor_time_start(long);
void mac_constructor_time_stop(void);
int mac_player_queue_probe(long player_index);
int mac_hud_runtime_probe(long weapon);
int mac_scenery_runtime_probe(void);
#ifdef MAC_SKY_ASSETS
int mac_sky_assets_runtime_probe(const struct mac_retail_cache *,const void *,size_t,struct mac_engine_world *,struct mac_actor_assets *);
#endif
#ifdef MAC_PLAYER_QUEUE_APPLY
int mac_player_queue_apply_probe(long player_index);
int mac_player_movement_probe(long player_index);
int mac_object_update_probe(long player_index);
#endif
#define CHECK(x) do{++checks;if(!(x)){++failures;fprintf(stderr,"FAIL %s line %d\n",#x,__LINE__);}}while(0)
static unsigned read_word(const unsigned char*p){return p[0]|(unsigned)p[1]<<8|(unsigned)p[2]<<16|(unsigned)p[3]<<24;}
static int run(void *context)
{
    unsigned map;
    struct runtime_context *c=context;
    struct game_globals *previous_globals=global_game_globals;
    struct hud_globals_definition *previous_hud=hud_globals;
    if(c->sound_lifecycle)game_sound_initialize();
    if(c->apply){hud_globals=c->hud;hud_messaging_initialize();}
    if(c->control)global_game_globals=c->globals;
#ifdef MAC_NATIVE_INPUT
    if(c->native_input){
        CHECK(input_initialize());
        ui_widgets_initialize();
        event_manager_initialize();
        input_abstraction_initialize();
        player_ui_initialize();
    }
#endif
    if(c->spawn){ai_initialize();players_initialize();render_objects_initialize();CHECK(mac_constructor_time_start(123));}
    if(c->movement){CHECK(mac_game_core_start());cinematic_initialize();breakable_surfaces_initialize();collision_log_initialize();}
    if(c->object_update){structure_decals_initialize();hud_initialize_unit_interface();decals_initialize();first_person_weapons_initialize();}
    fprintf(stderr,"runtime: objects_initialize\n");objects_initialize();
    CHECK(object_header_data!=NULL);CHECK(!object_header_data->valid);
    CHECK(object_header_data->actual_count==0);
    for(map=0;map<2;++map){
        fprintf(stderr,"runtime: objects_initialize_for_new_map %u\n",map);
        if(c->spawn){players_initialize_for_new_map();render_objects_initialize_for_new_map();}
        if(c->control){
            unsigned local;struct game_globals_player_control *constants=c->globals->player_control.address;
            player_control_initialize_for_new_map();
            CHECK(c->globals->player_control.count==1);CHECK(constants->look_function.count==6);
            CHECK(constants->look_default_pitch_rate==60.0f);CHECK(constants->look_default_yaw_rate==120.0f);
            for(local=0;local<MAXIMUM_NUMBER_OF_LOCAL_PLAYERS;++local){
                CHECK(player_control_get_unit_index((short)local)==NONE);
                CHECK(player_look_pitch_rate[local]==constants->look_default_pitch_rate);
                CHECK(player_look_yaw_rate[local]==constants->look_default_yaw_rate);
            }
        }
        if(c->apply){hud_messaging_initialize_for_new_map();CHECK(mac_hud_runtime_probe((long)c->assets->weapons[0]));}
        if(c->movement){cinematic_initialize_for_new_map();breakable_surfaces_initialize_for_new_map();}
        if(c->object_update){first_person_weapons_initialize_for_new_map();structure_decals_initialize_for_new_map();hud_initialize_unit_interface_for_new_map();decals_initialize_for_new_map();CHECK(global_decal_data->actual_count==0);}
        if(c->sound_lifecycle)game_sound_initialize_for_new_map();
        objects_initialize_for_new_map();
        CHECK(object_header_data->valid);CHECK(object_header_data->actual_count==0);
        CHECK(collideable_object_cluster_partition.data_reference_data->valid);
        CHECK(noncollideable_object_cluster_partition.cluster_reference_data->valid);
        if(c->spawn){
            struct object_placement_data placement;long biped,player=NONE;long weapon_indices[2]={NONE,NONE};unsigned slot,weapons=0;
            object_placement_data_new(&placement,(long)c->assets->biped,NONE);
            memcpy(&placement.position,c->point,sizeof(placement.position));
            fprintf(stderr,"runtime: full object_new biped map %u\n",map);
            biped=object_new(&placement);CHECK(biped!=NONE);
            if(biped!=NONE){
                struct unit_datum *unit=unit_get(biped);
                CHECK(unit->definition_index==(long)c->assets->biped);
                for(slot=0;slot<MAXIMUM_WEAPONS_PER_UNIT;++slot){
                    long w=unit->unit.weapon_object_indices[slot];
                    if(w!=NONE){struct weapon_datum *weapon=weapon_get(w);if(weapons<2)weapon_indices[weapons]=w;++weapons;
                        CHECK(weapon->definition_index==(long)c->assets->weapons[0] || weapon->definition_index==(long)c->assets->weapons[1]);}
                }
                CHECK(weapons==2);CHECK(object_header_data->actual_count==3);
#ifdef MAC_MODEL_GPU
                if(c->model_gpu){unsigned draw;const char *gpu_error=NULL;
                    struct mac_native_model_gpu_result result;
                    CHECK(mac_native_model_gpu_probe_object(biped,&result,&gpu_error));
                    CHECK(result.parts>0&&result.vertices>0&&result.pixels>0);
                    fprintf(stderr,"runtime model GPU biped: parts=%u vertices=%u pixels=%u error=%s\n",result.parts,result.vertices,result.pixels,gpu_error?gpu_error:"none");
                    for(draw=0;draw<2;++draw){gpu_error=NULL;memset(&result,0,sizeof(result));
                        CHECK(weapon_indices[draw]!=NONE&&mac_native_model_gpu_probe_object(weapon_indices[draw],&result,&gpu_error));
                        CHECK(result.parts>0&&result.vertices>0&&result.pixels>0);
                        fprintf(stderr,"runtime model GPU weapon%u: parts=%u vertices=%u pixels=%u error=%s\n",draw,result.parts,result.vertices,result.pixels,gpu_error?gpu_error:"none");
                    }
                }
#endif
                CHECK(biped_get(biped)->biped.jump_recovery_timer==127);
                CHECK(biped_get(biped)->biped.support_surface_index==NONE);
                CHECK(unit->object.location.cluster_index==c->cluster);
                CHECK(light_data->actual_count>0);
                fprintf(stderr,"runtime: biped=%08lx weapons=%u objects=%d cluster=%d lights=%d\n",biped,weapons,object_header_data->actual_count,unit->object.location.cluster_index,light_data->actual_count);
                if(c->player){
                    long vehicle=NONE;
                    fprintf(stderr,"runtime: player_new local machine 0\n");
                    player=player_new(0,NONE,0,NULL);CHECK(player!=NONE);
                    if(player!=NONE){
                        CHECK(player_data->actual_count==1);
                        CHECK(machine_get_player_list(0)[0]==player);
                        CHECK(player_get(player)->unit_index==NONE);
                        local_player_set_player_index(0,player);
                        if(c->native_input)player_ui_set_single_player_local_player_controller(0,0);
                        CHECK(local_player_get_player_index(0)==player);
                        CHECK(player_get(player)->local_player_index==0);
                        fprintf(stderr,"runtime: associate player=%08lx biped=%08lx\n",player,biped);
                        players_set_local_player_unit(0,biped);
                        CHECK(player_get(player)->unit_index==biped);
                        CHECK(unit->unit.player_index==player);
                        CHECK(player_control_get_unit_index(0)==biped);
                        CHECK(TEST_FLAG(unit->unit.flags,_unit_actively_controlled_bit));
                        CHECK(TEST_FLAG(unit->unit.flags,_unit_controllable_bit));
#ifdef MAC_VEHICLE_RUNTIME
                        if(c->vehicle)CHECK(mac_vehicle_runtime_place(c->vehicle_definition,&vehicle));
#endif
                        if(c->queue){
                            main_menu_active(TRUE);CHECK(main_menu_is_active());
                            main_menu_active(FALSE);CHECK(!main_menu_is_active());
#ifdef MAC_PLAYER_QUEUE_APPLY
                            if(c->apply){
                                int applied=mac_player_queue_apply_probe(player);CHECK(applied);
                                if(applied&&c->decal_only) {
                                    unsigned long empty[64],visible[64];short clusters=global_structure_bsp_get()->clusters.count;
                                    memset(empty,0,sizeof(empty));CHECK(clusters<=2048);
                                    memcpy(visible,players_get_combined_pvs(),BIT_VECTOR_SIZE_IN_BYTES(clusters));
                                    structure_decals_update(empty,visible,clusters);
                                    printf("actual decal lifecycle: created=%d\n",global_decal_data->actual_count);
                                    CHECK(global_decal_data->actual_count==8);
                                    structure_decals_update(visible,empty,clusters);
                                    CHECK(global_decal_data->actual_count==0);
                                }
                                else if(applied&&c->object_update){
                                    int updated=mac_object_update_probe(player);
                                    CHECK(updated);
#ifdef MAC_VEHICLE_RUNTIME
                                    if(updated&&c->vehicle)CHECK(mac_vehicle_runtime_after_update(vehicle,c->vehicle_definition));
#endif
                                }
                                else if(applied&&c->movement)CHECK(mac_player_movement_probe(player));
                            }else
#endif
                            CHECK(mac_player_queue_probe(player));
                        }
                        if(c->all_scenery)CHECK(mac_scenery_runtime_probe());
                        else if(c->scenery) {
                            struct scenario *scenario=global_scenario_get();
                            struct scenario_object_datum *record=NULL;
                            struct object_datum *object;
                            long n,scenery=NONE;
                            for(n=0;n<scenario->scenery.count;++n) {
                                struct scenario_object_datum *candidate=(struct scenario_object_datum *)
                                    ((byte *)scenario->scenery.address+n*0x48);
                                if(candidate->palette_entry_index==0){record=candidate;break;}
                            }
                            CHECK(record!=NULL);
                            if(record)scenery=object_new_from_scenario(record,&scenario->scenery_palette);
                            CHECK(scenery!=NONE);
                            if(scenery!=NONE) {
                                object=object_get(scenery);
                                CHECK(object->definition_index==(long)UINT32_C(0xe1860012));
                                CHECK(object->object.type==6);
                                CHECK(object->object.location.cluster_index!=NONE);
                                CHECK(object->object.position.x==record->position.x);
                                CHECK(object->object.position.y==record->position.y);
                                CHECK(object->object.position.z==record->position.z);
                                fprintf(stderr,"actual scenario scenery: handle=%08lx cluster=%d position=(%g,%g,%g)\n",
                                    scenery,object->object.location.cluster_index,object->object.position.x,
                                    object->object.position.y,object->object.position.z);
                                object_delete_immediately(scenery);
                                CHECK(object_header_try_and_get(scenery)==NULL);
                            }
                        }
#ifdef MAC_VEHICLE_RUNTIME
                        if(vehicle!=NONE){object_delete_immediately(vehicle);CHECK(object_header_try_and_get(vehicle)==NULL);}
#endif
                        players_set_local_player_unit(0,NONE);
                        CHECK(player_get(player)->unit_index==NONE);
                        CHECK(unit->unit.player_index==NONE);
                        CHECK(player_control_get_unit_index(0)==NONE);
                        CHECK(!TEST_FLAG(unit->unit.flags,_unit_actively_controlled_bit));
                        local_player_set_player_index(0,NONE);
                        CHECK(local_player_get_player_index(0)==NONE);
                        machine_remove_player(player);CHECK(machine_get_player_list(0)[0]==NONE);
                        player_delete(player);CHECK(player_data->actual_count==0);
                    }
                }
                /* Ordinary object_delete defers reclamation until a game tick. */
                object_delete_immediately(biped);CHECK(object_header_try_and_get(biped)==NULL);
                /* Inventory objects may not yet be attached to skeleton nodes
                   before the first tick. Delete remaining real objects too. */
                {long remaining;unsigned limit=0;
                    while((remaining=data_next_index(object_header_data,NONE))!=NONE && limit++<16)
                        object_delete_immediately(remaining);
                }
                CHECK(object_header_data->actual_count==0);CHECK(light_data->actual_count==0);
            }
        }
        if(c->apply)hud_messaging_dispose_from_old_map();
        if(c->movement){breakable_surfaces_dispose_from_old_map();cinematic_dispose_from_old_map();}
        if(c->object_update){first_person_weapons_dispose_from_old_map();hud_dispose_unit_interface_from_old_map();structure_decals_dispose_from_old_map();decals_dispose_from_old_map();}
        objects_dispose_from_old_map();
        if(c->sound_lifecycle)game_sound_dispose_from_old_map();
        CHECK(!object_header_data->valid);
        if(c->spawn){players_dispose_from_old_map();render_objects_dispose_from_old_map();}
    }
    if(c->movement){breakable_surfaces_dispose();cinematic_dispose();CHECK(mac_game_core_stop());}
    if(c->object_update){first_person_weapons_dispose();hud_dispose_unit_interface();structure_decals_dispose();decals_dispose();}
    objects_dispose();CHECK(object_header_data==NULL);
    if(c->spawn){ai_dispose();players_dispose();render_objects_dispose();mac_constructor_time_stop();}
#ifdef MAC_NATIVE_INPUT
    if(c->native_input){
        ui_widgets_dispose();
        event_manager_dispose();
        input_abstraction_dispose();
        player_ui_dispose();
    }
#endif
    if(c->apply){hud_messaging_dispose();hud_globals=previous_hud;}
    if(c->sound_lifecycle)game_sound_dispose();
    global_game_globals=previous_globals;
#ifdef MAC_NATIVE_INPUT
    if(c->native_input)input_dispose();
#endif
    return failures==0;
}
int main(int argc,char**argv)
{
    struct mac_mapped_file map={0},pixels={0},sounds={0};struct mac_retail_cache cache;
    struct mac_actor_assets assets={0};struct mac_engine_world world={0};const char*error=NULL;
#ifdef MAC_RUNTIME_VIEW
    struct mac_runtime_view view={0};
#endif
    struct runtime_context context;float facing;struct mac_engine_location location;
    if(argc!=3 && argc!=4 && argc!=5)return 2;
    memset(&context,0,sizeof(context));
#ifdef MAC_NATIVE_INPUT
    context.native_input=getenv("HALO_MAC_NATIVE_INPUT")!=NULL;
#endif
#ifdef MAC_PLAYER_QUEUE_APPLY
    context.decal_only=argc==4 && !strcmp(argv[3],"--decal-lifecycle");
    context.vehicle=(argc==4 || argc==5) && !strcmp(argv[3],"--vehicle-object-update");
    context.object_update=context.decal_only || context.vehicle || (argc==4 && !strcmp(argv[3],"--object-update"));
    context.movement=context.object_update || (argc==4 && !strcmp(argv[3],"--player-movement"));
    context.apply=context.movement || (argc==4 && !strcmp(argv[3],"--player-apply"));
#endif
    context.sky_assets=argc==4 && !strcmp(argv[3],"--sky-assets");
#ifdef MAC_MODEL_GPU
    context.model_gpu=argc==4 && !strcmp(argv[3],"--model-gpu");
    if(context.model_gpu)context.spawn=1;
#endif
    context.control=context.apply || (argc==4 && !strcmp(argv[3],"--player-control"));
    context.queue=context.control || (argc==4 && !strcmp(argv[3],"--player-queue"));
    context.player=context.queue || (argc==4 && !strcmp(argv[3],"--player"));
    context.spawn=context.player || context.model_gpu || (argc==4 && !strcmp(argv[3],"--spawn"));
    if((argc==4 || argc==5) && !context.spawn && !context.vehicle && !context.sky_assets)return 2;
    if(argc==5 && !context.vehicle)return 2;
    context.scenery=context.object_update && getenv("HALO_MAC_SCENERY")!=NULL;
    context.all_scenery=context.object_update && getenv("HALO_MAC_SCENERY_ALL")!=NULL;
    context.sound_lifecycle=context.all_scenery || context.vehicle;
    context.assets=&assets;
    if(!mac_mapped_file_open(argv[1],0x30000000,&map,&error)||
       !mac_mapped_file_open(argv[2],0x40000000,&pixels,&error)||
       (context.vehicle && (argc!=5 || !mac_mapped_file_open(argv[4],0x20000000,&sounds,&error))) ||
       !mac_retail_open(map.data,map.size,&cache,&error)){fprintf(stderr,"runtime input: %s\n",error);return 1;}
    /* The location-only world has no device-group conversion. Refuse maps
       carrying groups rather than silently inventing an empty group list. */
    CHECK(offsetof(struct scenario,device_groups)+12<=cache.tag_size-cache.scenario_offset);
    if(failures)return 1;
    if(read_word(cache.tags+cache.scenario_offset+offsetof(struct scenario,device_groups))){fprintf(stderr,"runtime: unsupported nonempty scenario device groups\n");return 1;}
    CHECK(mac_engine_arena_start());if(failures)return 1;
#ifdef MAC_MODEL_GPU
    if(context.model_gpu){CHECK(mac_device_gpu_start(64,48,0,"OpenCE actual model indexed draw"));
        global_d3d_device=mac_device_gpu_identity();}
    else
#endif
    {CHECK(mac_device_headless_start(640,480));global_d3d_device=mac_device_headless_identity();}
    CHECK(global_d3d_device!=NULL);
    if(!mac_actor_assets_load_with_bitmaps(&cache,pixels.data,pixels.size,&assets,&error)||
       !mac_engine_world_create(&cache,0,&world,&error)||
       !mac_engine_world_enable_graph(&world,&cache,&error)){fprintf(stderr,"runtime native data: %s\n",error);return 1;}
#ifdef MAC_SKY_ASSETS
    if(context.sky_assets && !mac_sky_assets_runtime_probe(&cache,pixels.data,pixels.size,&world,&assets)) {
        fprintf(stderr,"runtime sky assets probe failed\n");return 1;
    }
#else
    if(context.sky_assets)return 2;
#endif
#ifdef MAC_VEHICLE_RUNTIME
    if(context.vehicle &&
       (!mac_engine_world_object_palette_handle(&world,MAC_SCENARIO_VEHICLE,0,&context.vehicle_definition) ||
        !mac_actor_assets_load_vehicle_with_resources(&cache,pixels.data,pixels.size,
            sounds.data,sounds.size,&assets,context.vehicle_definition,&error))) {
        fprintf(stderr,"runtime Warthog assets: %s\n",error?error:"vehicle palette entry zero");return 1;
    }
    if(context.vehicle && !mac_actor_assets_load_world_sounds(&cache,&world,
        sounds.data,sounds.size,&assets,&error)) {
        fprintf(stderr,"runtime BSP sound assets: %s\n",error?error:"world sound palette");return 1;
    }
#endif
    if(context.scenery && !mac_actor_assets_load_scenery(&cache,&assets,UINT32_C(0xe1860012),&error)) {
        fprintf(stderr,"runtime scenery assets: %s\n",error?error:"failed");return 1;
    }
    if(context.all_scenery) {
        unsigned i,family;
        for(family=0;family<2;++family)for(i=0;i<(family?1U:19U);++i) {
            uint32_t handle;
            if(!mac_engine_world_object_palette_handle(&world,family?MAC_SCENARIO_SOUND_SCENERY:MAC_SCENARIO_SCENERY,i,&handle) ||
               !(family?mac_actor_assets_load_sound_scenery(&cache,&assets,handle,&error):
                   mac_actor_assets_load_scenery(&cache,&assets,handle,&error))) {
                fprintf(stderr,"runtime scenery palette %u:%u: %s\n",family,i,error?error:"lookup");return 1;
            }
        }
    }
    if(context.object_update && !mac_engine_world_enable_surfaces(&world,&cache,&error)) {
        fprintf(stderr,"runtime surface conversion: %s\n",error?error:"failed");return 1;
    }
    {
        struct mac_engine_world_info info;
        CHECK(mac_engine_world_info(&world,&info));
        fprintf(stderr,"runtime world: materials=%u fog=%u/%u/%u decals=%u palette=%u decal_clusters=%u\n",info.collision_materials,info.fog_planes,info.fog_regions,info.fog_palette,info.runtime_decals,info.scenario_decal_palette,info.clusters_with_decals);
        if(context.object_update && !mac_engine_world_object_update_supported(&world)){
            fprintf(stderr,"runtime: object update refused; native fog/decal dependencies remain unconverted\n");
            mac_mapped_file_close(&pixels);mac_mapped_file_close(&map);
            mac_engine_world_free(&world);mac_actor_assets_free(&assets);global_d3d_device=NULL;mac_device_state_stop();
            mac_engine_arena_stop();return 3;
        }
    }
    if(context.object_update){
        uint32_t lightmap=mac_engine_world_lightmap_handle(&world);
        if(lightmap!=UINT32_MAX && !mac_actor_assets_load_bitmap(&cache,pixels.data,pixels.size,&assets,lightmap,&error)) {
            fprintf(stderr,"runtime lightmap assets: %s\n",error?error:"bitmap decode");return 1;
        }
        fprintf(stderr,"runtime lightmap handle=%08lx bitmaps=%u\n",(unsigned long)lightmap,assets.bitmap_count);
    }
    if(context.object_update){
        uint32_t handles[64];unsigned count;
        if(!mac_engine_world_decal_handles(&world,handles,64,&count) ||
           !mac_native_decal_assets_load(&cache,pixels.data,pixels.size,&assets,handles,count,&error)){
            fprintf(stderr,"runtime decal assets: %s\n",error?error:"palette access");return 1;
        }
        fprintf(stderr,"runtime decal assets: palette=%u bitmaps=%u published=%u\n",count,assets.bitmap_count,assets.published_count);
    }
    if(context.control){
        uint32_t index,handle=0;struct mac_retail_tag tag;struct mac_native_object_tag globals;
        for(index=0;index<cache.tag_count;++index)
            if(mac_retail_tag_get(&cache,index,&tag)&&tag.group==UINT32_C(0x6d617467)){handle=tag.handle;break;}
        if(!handle||!mac_native_globals_decode(&cache,handle,&globals,&error)){
            fprintf(stderr,"runtime globals: %s\n",handle?error:"missing matg");return 1;
        }
        if(!mac_native_tags_publish(&assets.tags,globals.handle,globals.group,globals.root,globals.root_bytes,
            globals.bytes_owned,globals.owner,mac_native_object_owner_free,&error)){
            mac_native_object_tag_free(&globals);fprintf(stderr,"runtime globals publish: %s\n",error);return 1;
        }
        globals.owner=NULL;context.globals=globals.root;
    }
    if(context.apply){
        struct game_globals_interface_tag_references *refs=context.globals->interface_tag_references.address;
        struct mac_native_object_tag hud;
        uint32_t handle=(uint32_t)refs->interface_tag_references[_interface_hud_globals].index;
        if(!mac_native_hud_globals_decode(&cache,handle,&hud,&error)){
            fprintf(stderr,"runtime HUD: %s\n",error);return 1;
        }
        if(!mac_native_tags_publish(&assets.tags,hud.handle,hud.group,hud.root,hud.root_bytes,
            hud.bytes_owned,hud.owner,mac_native_object_owner_free,&error)){
            mac_native_object_tag_free(&hud);fprintf(stderr,"runtime HUD publish: %s\n",error);return 1;
        }
        hud.owner=NULL;context.hud=hud.root;
    }
    mac_mapped_file_close(&pixels);
#ifdef MAC_RUNTIME_VIEW
    if(context.object_update) {
        CHECK(mac_runtime_view_open(&view,&cache,0,argv[2],640,480,MAC_RUNTIME_VIEW_HIDDEN,
            "OpenCE real unit view diagnostic",&error));
        if(failures){fprintf(stderr,"runtime view: %s\n",error?error:"failed");return 1;}
        CHECK(mac_runtime_view_is_accelerated(&view));
        CHECK(!mac_runtime_view_is_visible(&view));
        mac_probe_runtime_view=&view;
    }
#else
    mac_mapped_file_close(&map);
#endif
    CHECK(mac_engine_tags_bind(&assets.tags));CHECK(mac_actor_assets_bind_texture_cache(&assets));
    CHECK(mac_engine_world_spawn(&world,0,context.point,&facing,&location));
    context.cluster=location.cluster;
    CHECK(mac_engine_world_scope(&world,run,&context));
    mac_mapped_file_close(&sounds);
#ifdef MAC_RUNTIME_VIEW
    mac_probe_runtime_view=NULL;mac_runtime_view_close(&view);mac_mapped_file_close(&map);
#endif
    global_d3d_device=NULL;mac_device_state_stop();
    mac_actor_assets_unbind_texture_cache(&assets);mac_engine_tags_unbind();
    mac_engine_world_free(&world);mac_actor_assets_free(&assets);
    CHECK(mac_engine_arena_stop());
    printf("actual object runtime: %u checks %u failures; spawn=%d player=%d queue=%d control=%d apply=%d movement=%d objects=%d\n",checks,failures,context.spawn,context.player,context.queue,context.control,context.apply,context.movement,context.object_update);
    return failures?1:0;
}
