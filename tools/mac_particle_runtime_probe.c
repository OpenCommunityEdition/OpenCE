/* Actual particle-system creation/update against owned Blood Gulch resources. */
#include "cseries.h"
#include "objects/objects.h"
#include "objects/object_types.h"
#include "objects/damage.h"
#include "items/projectiles.h"
#include "items/weapons.h"
#include "units/units.h"
#include "game/game.h"
#include "game/player_queues_new.h"
#include "interface/first_person_weapons.h"
#include "effects/player_effects.h"
#include "game/player_rumble.h"
#include "game/game_allegiance.h"
#include "input/input.h"
#include "interface/player_ui.h"
#include "effects/particle_systems.h"
#include "effects/effects.h"
#include "effects/particles.h"
#include "effects/contrails.h"
#include "effects/decals.h"
#include "game/players.h"
#include "game/local_players.h"
#include "game/player_control_runtime.h"
#include "game/game_globals.h"
#include "ai/ai.h"
#include "render/render.h"
#include "scenario/scenario.h"
#include "interface/hud.h"
#include "interface/hud_messaging.h"
#include "native_globals_tags.h"
#include "native_hud_tags.h"
#include "effects/particle_system_definitions.h"
#include "memory/data.h"
#include "physics/breakable_surfaces.h"
#include "physics/collision_usage.h"
#include "engine_world.h"
#include "engine_arena.h"
#include "engine_game_runtime.h"
#include "engine_device_state.h"
#include "rasterizer/xbox/rasterizer_xbox.h"
#include "engine_tags.h"
#include "native_actor_assets.h"
#include "native_effect_assets.h"
#include "native_environment_assets.h"
#include "mapped_file.h"
#undef memset
#undef strcmp
#include <stdio.h>
#include <string.h>
int mac_constructor_time_start(long);
void mac_constructor_time_stop(void);
int mac_player_queue_apply_probe(long);
extern struct data_array *global_decal_data;
static unsigned checks,failures;
#define CHECK(x) do{++checks;if(!(x)){++failures;fprintf(stderr,"FAIL particle runtime %d %s\n",__LINE__,#x);}}while(0)
static void apply_player_action(struct player_action *action,long tick)
{
    game_time_set_distributed(tick);
    update_client_queue_push();update_client_queue(action);update_client_local_ticks(1);
    players_update_before_game();
}
struct context { uint32_t handle,effect,biped,damage,projectile,weapon;real_point3d point;
    struct game_globals *globals;struct hud_globals_definition *hud; };
static int run(void *opaque)
{
    struct context *c=opaque;unsigned cycle,step,peak=0,effect_peak=0;real_vector3d velocity={0},forward={1.f,0.f,0.f};real_argb_color color;
    real_rgb_color tint={1.f,1.f,1.f};const char *marker="";
    struct game_globals *old_globals=global_game_globals;struct hud_globals_definition *old_hud=hud_globals;
    global_game_globals=c->globals;hud_globals=c->hud;
    memset(&color,0,sizeof(color));color.alpha=1.f;color.red=color.green=color.blue=1.f;
    CHECK(input_initialize());player_ui_initialize();
    random_math_initialize();CHECK(mac_constructor_time_start(123));CHECK(mac_game_core_start());
    game_allegiance_initialize();breakable_surfaces_initialize();collision_log_initialize();
    ai_initialize();render_objects_initialize();hud_messaging_initialize();players_initialize();rumble_initialize();player_effect_initialize();objects_initialize();particles_initialize();particle_systems_initialize();contrails_initialize();decals_initialize();first_person_weapons_initialize();effects_initialize();
    CHECK(particle_systems&&system_particles);if(!particle_systems||!system_particles)return 0;
    for(cycle=0;cycle<2;++cycle) {
        long index,effect,biped,player,cycle_time=game_time_get();struct object_placement_data placement;unsigned cycle_peak=0;struct effects_information information;
        game_allegiance_initialize_for_new_map();breakable_surfaces_initialize_for_new_map();
        players_initialize_for_new_map();rumble_initialize_for_new_map();player_effect_initialize_for_new_map();render_objects_initialize_for_new_map();player_control_initialize_for_new_map();hud_messaging_initialize_for_new_map();objects_initialize_for_new_map();particles_initialize_for_new_map();particle_systems_initialize_for_new_map();contrails_initialize_for_new_map();decals_initialize_for_new_map();first_person_weapons_initialize_for_new_map();effects_initialize_for_new_map();
        index=particle_system_new_unattached((long)c->handle,&c->point,&velocity,&color,1.f);
        CHECK(index!=NONE);
        if(index!=NONE) {
            struct particle_system_datum *system=particle_system_get(index);
            CHECK(system->definition_index==(long)c->handle);CHECK(system->object_index==NONE);
            CHECK(system->location.cluster_index!=NONE);
            CHECK(system->lighting.red>=0.f&&system->lighting.red<100.f);
            CHECK(system->lighting.green>=0.f&&system->lighting.green<100.f);
            CHECK(system->lighting.blue>=0.f&&system->lighting.blue<100.f);
        }
        effect=effect_new_unattached_from_markers((long)c->effect,NONE,&velocity,1,&marker,&c->point,&forward,1.f,1.f,&tint,NULL,FALSE);
        /* No local observer exists in this scope: the real PVS check must
           discard this non-looping effect. Visible emission is a later
           integrated-player test, not inferred from a returned datum index. */
        CHECK(effect!=NONE);effects_information_get(&information);CHECK(information.effect_count==0);
        CHECK(information.location_count==0);
        object_placement_data_new(&placement,(long)c->biped,NONE);
        placement.position=c->point;placement.position.z-=1.f;
        biped=object_new(&placement);CHECK(biped!=NONE);
        player=player_new(0,NONE,0,NULL);CHECK(player!=NONE);
        if(biped==NONE || player==NONE)return 0;
        local_player_set_player_index(0,player);players_set_local_player_unit(0,biped);
        CHECK(mac_player_queue_apply_probe(player));
        effect=effect_new_unattached_from_markers((long)c->effect,NONE,&velocity,1,&marker,&c->point,&forward,1.f,1.f,&tint,NULL,FALSE);
        CHECK(effect!=NONE);effects_information_get(&information);
        /* This effect can finish during its zero-time creation update; its
           emitted ordinary particles, not a live effect handle, prove emission. */
        CHECK(particle_data->actual_count>0);
        if((unsigned)particle_data->actual_count>effect_peak)effect_peak=(unsigned)particle_data->actual_count;
        {
            struct damage_data damage;struct object_datum *victim=object_get(biped);
            real before_body=victim->object.body_vitality,before_shield=victim->object.shield_vitality;
            damage_data_new(&damage,(long)c->damage);damage.origin=c->point;
            damage.epicenter=victim->object.position;damage.direction=forward;damage.location=victim->object.location;
            object_cause_damage(&damage,biped,NONE,NONE,NONE,NULL);
            CHECK(victim->object.shield_vitality<before_shield);
            CHECK(victim->object.shield_vitality>=0.f);
            CHECK(victim->object.body_vitality==before_body);
            printf("actual rifle damage: shield %.9g -> %.9g body %.9g -> %.9g\n",
                before_shield,victim->object.shield_vitality,before_body,victim->object.body_vitality);
        }
        if(c->projectile!=UINT32_MAX) {
            long projectile;struct object_placement_data shot;
            object_placement_data_new(&shot,(long)c->projectile,biped);
            shot.position=c->point;shot.forward=forward;
            projectile=object_new(&shot);CHECK(projectile!=NONE);
            if(projectile!=NONE) {
                struct projectile_datum *bullet=projectile_get(projectile);
                CHECK(bullet->definition_index==(long)c->projectile);
                CHECK(bullet->object.translational_velocity.i>0.f);
                CHECK(bullet->projectile.ignore_object_index==biped);
                printf("actual rifle projectile: velocity=(%.9g,%.9g,%.9g) cluster=%d\n",
                    bullet->object.translational_velocity.i,bullet->object.translational_velocity.j,
                    bullet->object.translational_velocity.k,bullet->object.location.cluster_index);
                {
                    real_point3d before=bullet->object.position;
                    object_type_update(projectile);
                    CHECK(object_header_try_and_get(projectile)!=NULL);
                    if(object_header_try_and_get(projectile)) {
                        bullet=projectile_get(projectile);
                        CHECK(bullet->object.position.x!=before.x || bullet->object.position.y!=before.y || bullet->object.position.z!=before.z);
                        printf("actual rifle flight: (%.9g,%.9g,%.9g) -> (%.9g,%.9g,%.9g)\n",
                            before.x,before.y,before.z,bullet->object.position.x,bullet->object.position.y,bullet->object.position.z);
                        object_delete_immediately(projectile);
                    }
                }
            }
        }
        if(c->projectile!=UINT32_MAX) {
            long projectile;struct object_placement_data shot;
            object_placement_data_new(&shot,(long)c->projectile,biped);
            shot.position=c->point;shot.forward.i=shot.forward.j=0.f;shot.forward.k=-1.f;
            shot.up.i=1.f;shot.up.j=shot.up.k=0.f;
            projectile=object_new(&shot);CHECK(projectile!=NONE);
            if(projectile!=NONE) {
                object_type_update(projectile);
                CHECK(object_header_try_and_get(projectile)!=NULL);
                CHECK(global_decal_data->actual_count>0);
                if(object_header_try_and_get(projectile)) {
                    struct projectile_datum *bullet=projectile_get(projectile);
                    CHECK(bullet->projectile.hit_material_type!=NONE);
                    printf("actual rifle impact: material=%d action=%d z=%.9g\n",
                        bullet->projectile.hit_material_type,bullet->projectile.action,bullet->object.position.z);
                    object_delete_immediately(projectile);
                }
            }
        }
        if(c->weapon!=UINT32_MAX) {
            short slot;long weapon=NONE;unsigned before_count=(unsigned)object_header_data->actual_count;
            short before_rounds=0;
            for(slot=0;slot<unit_get_weapon_count(biped);++slot) {
                long candidate=unit_inventory_get_weapon(biped,slot);
                if(candidate!=NONE && (uint32_t)object_get(candidate)->definition_index==c->weapon){weapon=candidate;break;}
            }
            CHECK(weapon!=NONE);
            if(weapon!=NONE) {
                struct player_action control;long tick=game_time_get();short connection=game_connection();
                memset(&control,0,sizeof(control));control.desired_weapon_index=slot;
                control.desired_grenade_index=NONE;control.desired_zoom_level=NONE;
                game_connection_set(_game_connection_local);CHECK(update_server_new());
                update_server_start();update_queues_distributed_reset();
                for(step=0;step<60;++step){apply_player_action(&control,++tick);unit_update(biped);weapon_update(weapon);}
                CHECK(weapon_get(weapon)->object.parent_object_index==biped);
                printf("rifle ready: slot=%d current=%d parent=%lx biped=%lx\n",slot,
                    unit_get(biped)->unit.current_weapon_index,weapon_get(weapon)->object.parent_object_index,biped);
                before_rounds=weapon_get(weapon)->weapon.magazines[0].rounds_loaded;
                control.control_flags=FLAG(_unit_control_weapon_primary_trigger_bit);
                control.primary_trigger=1.f;
                for(step=0;step<30;++step) {
                    apply_player_action(&control,++tick);unit_update(biped);weapon_update(weapon);
                }
                control.control_flags=0;control.primary_trigger=0.f;
                apply_player_action(&control,++tick);unit_update(biped);
                {
                    short released_rounds=weapon_get(weapon)->weapon.magazines[0].rounds_loaded;
                    for(step=0;step<10;++step){apply_player_action(&control,++tick);unit_update(biped);weapon_update(weapon);}
                    CHECK(weapon_get(weapon)->weapon.magazines[0].rounds_loaded==released_rounds);
                }
                update_server_delete();game_connection_set(connection);
                CHECK(weapon_get(weapon)->weapon.magazines[0].rounds_loaded<before_rounds);
                CHECK((unsigned)object_header_data->actual_count>before_count);
                printf("actual rifle trigger: rounds %d -> %d objects %u -> %d\n",before_rounds,
                    weapon_get(weapon)->weapon.magazines[0].rounds_loaded,before_count,object_header_data->actual_count);
                {
                    struct object_iterator iterator;unsigned fired=0;
                    object_iterator_new(&iterator,_object_mask_projectile,0);
                    while(object_iterator_next(&iterator)) {
                        struct projectile_datum *bullet=projectile_get(iterator.index);
                        CHECK((uint32_t)bullet->definition_index==c->projectile);
                        CHECK(bullet->projectile.ignore_object_index==biped);++fired;
                        object_delete_immediately(iterator.index);
                    }
                    CHECK(fired==(unsigned)(before_rounds-weapon_get(weapon)->weapon.magazines[0].rounds_loaded));
                    printf("actual rifle trigger projectiles: %u\n",fired);
                }
            }
        }
        for(step=0;step<60;++step) {
            effects_update(1.f/30.f);particles_update(1.f/30.f);contrails_update(1.f/30.f);
            particle_systems_update(1.f/30.f);
            CHECK(particle_systems->actual_count>=0&&system_particles->actual_count>=0);
            if((unsigned)system_particles->actual_count>cycle_peak)cycle_peak=(unsigned)system_particles->actual_count;
            if((unsigned)system_particles->actual_count>peak)peak=(unsigned)system_particles->actual_count;
        }
        CHECK(cycle_peak>0);
        /* Casings outlive the short flight test. Let the real particle
           simulation finish them before the diagnostic arena is released. */
        for(step=0;particle_data->actual_count && step<1800;++step) {
            effects_update(1.f/30.f);particles_update(1.f/30.f);contrails_update(1.f/30.f);
            particle_systems_update(1.f/30.f);
        }
        CHECK(particle_data->actual_count==0);
        printf("actual particle retirement: %u extra steps\n",step);
        players_set_local_player_unit(0,NONE);local_player_set_player_index(0,NONE);
        machine_remove_player(player);player_delete(player);
        object_delete_immediately(biped);
        {long remaining;unsigned limit=0;
            while((remaining=data_next_index(object_header_data,NONE))!=NONE && limit++<(unsigned)object_header_data->maximum_count)
                object_delete_immediately(remaining);
        }
        CHECK(object_header_data->actual_count==0);
        hud_messaging_dispose_from_old_map();render_objects_dispose_from_old_map();
        effects_dispose_from_old_map();first_person_weapons_dispose_from_old_map();decals_dispose_from_old_map();contrails_dispose_from_old_map();
        CHECK(global_decal_data->actual_count==0);
        particle_systems_dispose_from_old_map();particles_dispose_from_old_map();objects_dispose_from_old_map();player_effect_dispose_from_old_map();rumble_dispose_from_old_map();players_dispose_from_old_map();breakable_surfaces_dispose_from_old_map();game_allegiance_dispose_from_old_map();
        CHECK(!particle_systems->valid&&!system_particles->valid);
        CHECK(!particle_systems->actual_count&&!system_particles->actual_count);
        game_time_set_distributed(cycle_time);
    }
    effects_dispose();first_person_weapons_dispose();decals_dispose();contrails_dispose();particle_systems_dispose();particles_dispose();objects_dispose();player_effect_dispose();rumble_dispose();players_dispose();breakable_surfaces_dispose();hud_messaging_dispose();render_objects_dispose();ai_dispose();
    game_allegiance_dispose();CHECK(mac_game_core_stop());mac_constructor_time_stop();random_math_dispose();player_ui_dispose();input_dispose();global_game_globals=old_globals;hud_globals=old_hud;
    printf("actual player-visible effect and particle systems: %u checks %u failures peak=%u effect_particles=%u cycles=2 base_steps=120\n",checks,failures,peak,effect_peak);
    return failures==0;
}
int main(int argc,char **argv)
{
    struct mac_mapped_file map={0},pixels={0},sounds={0};struct mac_retail_cache cache;
    struct mac_actor_assets assets={0};struct mac_engine_world world={0};struct context c;
    struct mac_engine_location location;const char *error=NULL;float facing;uint32_t i;int ok=0;
    if(argc!=3&&argc!=4)return 2;memset(&c,0,sizeof(c));c.handle=c.effect=c.damage=c.projectile=c.weapon=UINT32_MAX;
    if(!mac_mapped_file_open(argv[1],0x30000000,&map,&error)||!mac_mapped_file_open(argv[2],0x30000000,&pixels,&error)||
       !mac_retail_open(map.data,map.size,&cache,&error))goto done;
    if(argc==4&&!mac_mapped_file_open(argv[3],0x30000000,&sounds,&error))goto done;
    if(!mac_engine_arena_start()||!mac_device_headless_start(640,480))goto done;
    global_d3d_device=mac_device_headless_identity();
    if(!mac_actor_assets_load_with_bitmaps(&cache,pixels.data,pixels.size,&assets,&error)||
       !mac_engine_world_create(&cache,0,&world,&error)||!mac_engine_world_enable_graph(&world,&cache,&error)||
       !mac_engine_world_enable_surfaces(&world,&cache,&error)||
       !mac_native_environment_lighting_load(&cache,&world,pixels.data,pixels.size,&assets,&error))goto done;
    for(i=0;i<cache.tag_count;++i) {
        struct mac_retail_tag tag;if(!mac_retail_tag_get(&cache,i,&tag))goto done;
        if(argc==4&&tag.group==UINT32_C(0x77656170)&&!strcmp(tag.name,"weapons\\assault rifle\\assault rifle")){c.weapon=tag.handle;}
        if(argc==4&&tag.group==UINT32_C(0x70726f6a)&&!strcmp(tag.name,"weapons\\assault rifle\\bullet")){c.projectile=tag.handle;}
        if(tag.group==UINT32_C(0x6a707421)&&!strcmp(tag.name,"weapons\\assault rifle\\bullet")){c.damage=tag.handle;}
        if(tag.group==UINT32_C(0x7063746c)&&!strcmp(tag.name,"weapons\\rocket launcher\\effects\\rocket exhaust")){c.handle=tag.handle;}
        if(tag.group==UINT32_C(0x65666665)&&!strcmp(tag.name,"weapons\\rocket launcher\\effects\\exhaust")){c.effect=tag.handle;}
    }
    if(c.damage==UINT32_MAX||c.effect==UINT32_MAX||c.handle==UINT32_MAX||!mac_native_effect_assets_load(&cache,pixels.data,pixels.size,NULL,0,&assets,c.effect,&error) ||
       !mac_native_effect_assets_load(&cache,pixels.data,pixels.size,NULL,0,&assets,c.handle,&error) ||
       !mac_native_effect_assets_load(&cache,pixels.data,pixels.size,NULL,0,&assets,c.damage,&error))goto done;
    if(argc==4 && (c.projectile==UINT32_MAX || !mac_native_object_effect_assets_load(&cache,
        pixels.data,pixels.size,sounds.data,sounds.size,&assets,c.projectile,&error)))goto done;
    if(argc==4 && (c.weapon==UINT32_MAX || !mac_native_object_effect_assets_load(&cache,
        pixels.data,pixels.size,sounds.data,sounds.size,&assets,c.weapon,&error)))goto done;
    c.biped=assets.biped;
    for(i=0;i<cache.tag_count;++i) {
        struct mac_retail_tag tag;struct mac_native_object_tag native={0};
        if(mac_retail_tag_get(&cache,i,&tag) && tag.group==UINT32_C(0x6d617467)) {
            struct game_globals_interface_tag_references *refs;uint32_t handle;
            if(!mac_native_globals_decode(&cache,tag.handle,&native,&error))goto done;
            if(!mac_native_tags_publish(&assets.tags,native.handle,native.group,native.root,native.root_bytes,
                native.bytes_owned,native.owner,mac_native_object_owner_free,&error)){mac_native_object_tag_free(&native);goto done;}
            c.globals=native.root;refs=c.globals->interface_tag_references.address;
            handle=(uint32_t)refs->interface_tag_references[_interface_hud_globals].index;
            if(!mac_native_hud_globals_decode(&cache,handle,&native,&error))goto done;
            if(!mac_native_tags_publish(&assets.tags,native.handle,native.group,native.root,native.root_bytes,
                native.bytes_owned,native.owner,mac_native_object_owner_free,&error)){mac_native_object_tag_free(&native);goto done;}
            c.hud=native.root;break;
        }
    }
    if(!c.globals || !c.hud){error="particle player globals missing";goto done;}
    mac_mapped_file_close(&map);mac_mapped_file_close(&pixels);mac_mapped_file_close(&sounds);
    CHECK(mac_engine_tags_bind(&assets.tags));CHECK(mac_actor_assets_bind_texture_cache(&assets));
    CHECK(mac_engine_world_spawn(&world,0,(float *)&c.point,&facing,&location));c.point.z+=1.f;
    if(!failures)ok=mac_engine_world_scope(&world,run,&c);
done:
    if(!ok)fprintf(stderr,"particle runtime incomplete: %s\n",error?error:"runtime assertion");
    mac_actor_assets_unbind_texture_cache(&assets);mac_engine_tags_unbind();
    mac_engine_world_free(&world);mac_actor_assets_free(&assets);global_d3d_device=NULL;mac_device_state_stop();
    CHECK(mac_engine_arena_stop());mac_mapped_file_close(&map);mac_mapped_file_close(&pixels);mac_mapped_file_close(&sounds);return ok&&!failures?0:1;
}
