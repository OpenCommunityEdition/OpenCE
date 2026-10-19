#include "cseries.h"
#include "engine_session.h"
#include "native_scenario_scripts.h"
#include "engine_tags.h"
#include "engine_texture_lifecycle.h"
#include "engine_synchronous_precache.h"
#include "engine_scenario_transition.h"
#include "native_interface_assets.h"
#include "native_environment_assets.h"
#include "native_globals_tags.h"
#include "native_sky_assets.h"
#include "native_effect_assets.h"
#include "native_model_tags.h"
#include "native_world_vertex_buffers.h"
#include "engine_device_state.h"
#include "native_decal_assets.h"
#include "native_animation_tags.h"
#include "models/model_animation_definitions.h"
#include "models/model_definitions.h"
#include "game/game_globals.h"
#include "scenario/scenario_definitions.h"
#include "units/unit_definitions.h"
#include "items/weapon_definitions.h"
#include "structures/structure_bsp_definitions.h"
#include "native_hud_messages.h"
#include "native_resource_memory.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
/* port: the loading screen's progress (port/mac/native_tags.c) */
extern void (*mac_loading_progress_hook)(float);
extern void (*mac_loading_pump_hook)(void);
#define LOADING_PUMP() do { if(mac_loading_pump_hook)mac_loading_pump_hook(); } while(0)
#define LOADING_PROGRESS(fraction) do { if(mac_loading_progress_hook)mac_loading_progress_hook(fraction); } while(0)



static int load_placed_object_assets(struct mac_engine_session *session,
    const char **error)
{
    static const uint32_t families[]={MAC_SCENARIO_SCENERY,
        MAC_SCENARIO_SOUND_SCENERY,MAC_SCENARIO_VEHICLE,MAC_SCENARIO_LIGHT_FIXTURE,
        MAC_SCENARIO_MACHINE,MAC_SCENARIO_CONTROL,MAC_SCENARIO_BIPED,MAC_SCENARIO_WEAPON};
    const struct mac_retail_cache *cache=&session->files.retail;
    unsigned f;
    uint32_t count,i,handle,decal_handles[256],decal_count;
    struct mac_engine_world_info info;
    if(!mac_engine_world_info(&session->world,&info)) {
        if(error)*error="native scenario sky count is unavailable";
        return 0;
    }
    if(!mac_engine_world_decal_handles(&session->world,decal_handles,256,&decal_count)) {
        if(error)*error="native scenario decal palette is unavailable";
        return 0;
    }
    for(i=0;i<decal_count;++i)
        if(!mac_native_decal_assets_load(cache,session->files.bitmaps.data,
            session->files.bitmaps.size,&session->assets,&decal_handles[i],1,error))return 0;
    for(i=0;i<info.sky_references;++i) {
        if(!mac_engine_world_sky_handle(&session->world,i,&handle)) {
            if(error)*error="native scenario sky reference is unavailable";
            return 0;
        }
        if(handle!=UINT32_MAX && !mac_native_sky_assets_load(cache,
            session->files.bitmaps.data,session->files.bitmaps.size,
            &session->assets,handle,error))return 0;
    }
    for(f=0;f<sizeof(families)/sizeof(families[0]);++f) {
        if(!mac_engine_world_object_palette_count(&session->world,families[f],&count)) {
            if(error)*error="native scenario object palette count is unavailable";
            return 0;
        }
        for(i=0;i<count;++i) {
            if(!mac_engine_world_object_palette_handle(&session->world,families[f],i,&handle)) {
                if(error)*error="native scenario object palette entry is unavailable";
                return 0;
            }
            if(handle==UINT32_MAX)continue;
            if(families[f]==MAC_SCENARIO_BIPED) {
                /* campaign bipeds (the multiplayer one is loaded already) */
                if(!mac_actor_assets_load_placed_biped(cache,session->files.bitmaps.data,
                    session->files.bitmaps.size,session->files.sounds.data,
                    session->files.sounds.size,&session->assets,handle,error)) {
                    struct mac_retail_tag metadata;
                    fprintf(stderr,"native placed biped palette %u: %08lx (%s): %s\n",
                        (unsigned)i,(unsigned long)handle,
                        mac_retail_tag_get(cache,handle&65535,&metadata)?metadata.name:"?",
                        error&&*error?*error:"no reason given");
                    return 0;
                }
            } else if(families[f]==MAC_SCENARIO_WEAPON) {
                if(!mac_actor_assets_load_weapon_with_resources(cache,session->files.bitmaps.data,
                    session->files.bitmaps.size,session->files.sounds.data,
                    session->files.sounds.size,&session->assets,handle,error)) {
                    struct mac_retail_tag metadata;
                    fprintf(stderr,"native placed weapon palette %u: %08lx (%s): %s\n",
                        (unsigned)i,(unsigned long)handle,
                        mac_retail_tag_get(cache,handle&65535,&metadata)?metadata.name:"?",
                        error&&*error?*error:"no reason given");
                    return 0;
                }
            } else if(families[f]!=MAC_SCENARIO_VEHICLE) {
                int loaded;
                /* its effects first: attachments_load requires attached
                   effects and particle systems to be loaded already */
                if(!mac_native_object_effect_assets_load(cache,session->files.bitmaps.data,
                    session->files.bitmaps.size,session->files.sounds.data,
                    session->files.sounds.size,&session->assets,handle,error)) {
                    struct mac_retail_tag metadata;
                    fprintf(stderr,"native placed object family %u palette %u effects: %08lx (%s): %s\n",
                        (unsigned)families[f],(unsigned)i,(unsigned long)handle,
                        mac_retail_tag_get(cache,handle&65535,&metadata)?metadata.name:"?",
                        error&&*error?*error:"no reason given");
                    return 0;
                }
                switch(families[f]) {
                case MAC_SCENARIO_SCENERY:loaded=mac_actor_assets_load_scenery(cache,&session->assets,handle,error);break;
                case MAC_SCENARIO_SOUND_SCENERY:loaded=mac_actor_assets_load_sound_scenery(cache,&session->assets,handle,error);break;
                case MAC_SCENARIO_LIGHT_FIXTURE:loaded=mac_actor_assets_load_light_fixture(cache,&session->assets,handle,error);break;
                case MAC_SCENARIO_MACHINE:loaded=mac_actor_assets_load_machine(cache,&session->assets,handle,error);break;
                default:loaded=mac_actor_assets_load_control(cache,&session->assets,handle,error);break;
                }
                if(!loaded) {
                    struct mac_retail_tag metadata;
                    fprintf(stderr,"native placed object family %u palette %u: %08lx (%s): %s\n",
                        (unsigned)families[f],(unsigned)i,(unsigned long)handle,
                        mac_retail_tag_get(cache,handle&65535,&metadata)?metadata.name:"?",
                        error&&*error?*error:"no reason given");
                    return 0;
                }
                /* Attached lights publish without their lens flares and
                   bitmaps; the effect walk resolves those and any widgets
                   (again: a published light's own references). */
                if(!mac_native_object_effect_assets_load(cache,session->files.bitmaps.data,
                    session->files.bitmaps.size,session->files.sounds.data,
                    session->files.sounds.size,&session->assets,handle,error))return 0;
            } else if(!mac_actor_assets_load_vehicle_with_resources(cache,
                session->files.bitmaps.data,session->files.bitmaps.size,
                session->files.sounds.data,session->files.sounds.size,
                &session->assets,handle,error))return 0;
        }
    }
    return mac_actor_assets_load_world_sounds(cache,&session->world,
        session->files.sounds.data,session->files.sounds.size,&session->assets,error);
}

/* The tags a campaign level's scripts name (hs_references): sounds, effects,
   damage, animations and the objects scripts create. */
static int load_script_reference_assets(struct mac_engine_session *session,const char **error)
{
    const struct mac_retail_cache *cache=&session->files.retail;
    uint32_t *groups,*handles,count=0,i;
    int ok=1;
    groups=(uint32_t *)calloc(4096,sizeof(*groups));
    handles=(uint32_t *)calloc(4096,sizeof(*handles));
    if(!groups || !handles) {
        free(groups);free(handles);
        if(error)*error="native script reference allocation";return 0;
    }
    if(!mac_engine_world_script_references(&session->world,groups,handles,4096,&count)) {
        free(groups);free(handles);
        if(error)*error="native script references are unavailable";return 0;
    }
    /* the weapons the starting profiles hand out */
    {
        uint32_t weapons[16],n=mac_engine_world_profile_weapons(&session->world,weapons,16);
        for(i=0;i<n;++i)
            if(!mac_actor_assets_load_weapon_with_resources(cache,session->files.bitmaps.data,
                session->files.bitmaps.size,session->files.sounds.data,session->files.sounds.size,
                &session->assets,weapons[i],error)) {
                free(groups);free(handles);return 0;
            }
    }
    for(i=0;ok && i<count;++i) {
        uint32_t group=groups[i],handle=handles[i];
        const void *bitmaps=session->files.bitmaps.data,*sounds=session->files.sounds.data;
        size_t bitmap_bytes=session->files.bitmaps.size,sound_bytes=session->files.sounds.size;
        struct mac_native_tags *tags=&session->assets.tags;
        if(handle==UINT32_MAX)continue;
        switch(group) {
        case 0x736e6421: case 0x6c736e64: case 0x6a707421: case 0x65666665:
        case 0x70726f6a: case 0x6c696768: case 0x7063746c:
            ok=mac_native_effect_assets_load(cache,bitmaps,bitmap_bytes,sounds,sound_bytes,
                &session->assets,handle,error);break;
        case 0x616e7472:
            ok=mac_actor_assets_load_animation(cache,&session->assets,handle,error);break;
        case 0x62697064:
            ok=mac_actor_assets_load_placed_biped(cache,bitmaps,bitmap_bytes,sounds,sound_bytes,
                &session->assets,handle,error);break;
        case 0x76656869:
            ok=mac_actor_assets_load_vehicle_with_resources(cache,bitmaps,bitmap_bytes,
                sounds,sound_bytes,&session->assets,handle,error);break;
        case 0x77656170:
            ok=mac_actor_assets_load_weapon_with_resources(cache,bitmaps,bitmap_bytes,
                sounds,sound_bytes,&session->assets,handle,error);break;
        case 0x65716970:
            ok=mac_actor_assets_load_equipment_with_sounds(cache,sounds,sound_bytes,
                &session->assets,handle,error);break;
        case 0x61637476:
            /* port: a script names an actor variant (c10's captain, the monitors) */
            ok=mac_actor_assets_load_actor_variant(cache,bitmaps,bitmap_bytes,sounds,sound_bytes,
                &session->assets,handle,error);break;
        case 0x7363656e: case 0x6d616368:
            ok=mac_native_tags_group(tags,handle)==group ||
                (mac_native_object_effect_assets_load(cache,bitmaps,bitmap_bytes,sounds,
                    sound_bytes,&session->assets,handle,error) &&
                 (group==0x7363656e?mac_actor_assets_load_scenery(cache,&session->assets,handle,error):
                  mac_actor_assets_load_machine(cache,&session->assets,handle,error)));
            break;
        default:
            if(error)*error="a script names a tag family that is not supported";ok=0;break;
        }
        if(!ok) {
            struct mac_retail_tag metadata;
            fprintf(stderr,"native script reference %u: %08lx (%s) group %08lx: %s\n",(unsigned)i,
                (unsigned long)handle,mac_retail_tag_get(cache,handle&65535,&metadata)?metadata.name:"?",
                (unsigned long)group,error&&*error?*error:"no reason given");
        }
    }
    free(groups);free(handles);
    return ok;
}

/* The tags the AI blocks name: the actor variants the palette lists (with
   their actors, units, weapons and equipment), the animation graphs the
   command lists play and the dialogue of the conversations. */
static int load_ai_assets(struct mac_engine_session *session,const char **error)
{
    const struct mac_retail_cache *cache=&session->files.retail;
    uint32_t *groups,*handles,count,total=0,i;
    int ok=1;
    groups=(uint32_t *)calloc(4096,sizeof(*groups));
    handles=(uint32_t *)calloc(4096,sizeof(*handles));
    if(!groups || !handles) {
        free(groups);free(handles);
        if(error)*error="native ai reference allocation";return 0;
    }
    count=mac_engine_world_ai_references(&session->world,groups,handles,4096,&total);
    if(total>count) {
        free(groups);free(handles);
        if(error)*error="native ai references exceed the capacity";return 0;
    }
    for(i=0;ok && i<count;++i) {
        const void *bitmaps=session->files.bitmaps.data,*sounds=session->files.sounds.data;
        size_t bitmap_bytes=session->files.bitmaps.size,sound_bytes=session->files.sounds.size;
        switch(groups[i]) {
        case 0x61637476:
            ok=mac_actor_assets_load_actor_variant(cache,bitmaps,bitmap_bytes,sounds,sound_bytes,
                &session->assets,handles[i],error);break;
        case 0x616e7472:
            ok=mac_actor_assets_load_animation(cache,&session->assets,handles[i],error);break;
        default:
            ok=mac_native_effect_assets_load(cache,bitmaps,bitmap_bytes,sounds,sound_bytes,
                &session->assets,handles[i],error);break;
        }
        if(!ok) {
            struct mac_retail_tag metadata;
            fprintf(stderr,"native ai reference %u: %08lx (%s) group %08lx: %s\n",(unsigned)i,
                (unsigned long)handles[i],mac_retail_tag_get(cache,handles[i]&65535,&metadata)?metadata.name:"?",
                (unsigned long)groups[i],error&&*error?*error:"no reason given");
        }
    }
    if(ok)fprintf(stderr,"campaign ai assets: %u tags loaded for the AI blocks\n",(unsigned)count);
    free(groups);free(handles);
    return ok;
}

/* The actors units are built with (a seat's built-in actor: the Pelican's gun
   turret): the game places them when the unit exists (ai_place_pending_mounted_weapons),
   so their variants are loaded with the units that name them (a loaded variant's
   unit can name more). */
static int load_seat_actors(struct mac_engine_session *session,const char **error)
{
    const struct mac_retail_cache *cache=&session->files.retail;
    int pass;
    for(pass=0;pass<4;++pass) {
        uint32_t units[1024],variants[64],unit_count=0,variant_count=0,cursor=0,handle,i;
        int more=0;
        while(unit_count<1024 && (handle=mac_native_tags_next(&session->assets.tags,UNIT_DEFINITION_TAG,&cursor))!=UINT32_MAX)
            units[unit_count++]=handle;
        for(i=0;i<unit_count;++i) {
            const struct unit_definition *unit=(const struct unit_definition *)mac_native_tags_get(
                &session->assets.tags,mac_native_tags_group(&session->assets.tags,units[i]),units[i],sizeof(*unit),NULL);
            long seat;
            if(!unit || unit->unit.seats.count<0 || unit->unit.seats.count>64 ||
               (unit->unit.seats.count && !unit->unit.seats.address))continue;
            for(seat=0;seat<unit->unit.seats.count;++seat) {
                const struct unit_seat *s=(const struct unit_seat *)unit->unit.seats.address+seat;
                uint32_t actor=(uint32_t)s->built_in_actor_reference.index,k;
                int known=0;
                if(actor==UINT32_MAX || mac_native_tags_group(&session->assets.tags,actor)!=UINT32_MAX)continue;
                for(k=0;k<variant_count;++k)if(variants[k]==actor)known=1;
                if(!known && variant_count<64)variants[variant_count++]=actor;
            }
        }
        /* port: a weapon's AI firing parameters name an actor variant the
           actors firing it read (actor_combat_get_firing_variant_definition) */
        cursor=0;
        while((handle=mac_native_tags_next(&session->assets.tags,UINT32_C(0x77656170),&cursor))!=UINT32_MAX) {
            const struct weapon_definition *weapon=(const struct weapon_definition *)mac_native_tags_get(
                &session->assets.tags,UINT32_C(0x77656170),handle,sizeof(*weapon),NULL);
            uint32_t actor,k;
            int known=0;
            if(!weapon)continue;
            actor=(uint32_t)weapon->weapon.ai_firing_parameters.index;
            if(actor==UINT32_MAX || mac_native_tags_group(&session->assets.tags,actor)!=UINT32_MAX)continue;
            for(k=0;k<variant_count;++k)if(variants[k]==actor)known=1;
            if(!known && variant_count<64)variants[variant_count++]=actor;
        }
        for(i=0;i<variant_count;++i) {
            if(!mac_actor_assets_load_actor_variant(cache,session->files.bitmaps.data,session->files.bitmaps.size,
                session->files.sounds.data,session->files.sounds.size,&session->assets,variants[i],error))return 0;
            fprintf(stderr,"native session: built-in seat actor %08lx loaded\n",(unsigned long)variants[i]);
            more=1;
        }
        if(!more)break;
    }
    return 1;
}

static int load_item_and_environment_assets(struct mac_engine_session *session,
    const char **error)
{
    const struct mac_retail_cache *cache=&session->files.retail;
    uint32_t i;
    if(!session->files.bitmaps.data || !session->files.sounds.data) {
        if(error)*error="native session requires bitmaps.map and sounds.map for item assets";
        return 0;
    }
    for(i=0;i<cache->tag_count;++i) {
        struct mac_retail_tag tag;
        LOADING_PUMP();
        if(!mac_retail_tag_get(cache,i,&tag)) {
            if(error)*error="native item tag table is invalid";
            return 0;
        }
        if(tag.group==UINT32_C(0x69746d63)) {
            if(!mac_actor_assets_load_item_collection_with_resources(cache,
                session->files.bitmaps.data,session->files.bitmaps.size,
                session->files.sounds.data,session->files.sounds.size,
                &session->assets,tag.handle,error))return 0;
        } else if(tag.group==UINT32_C(0x65716970)) {
            if(!mac_actor_assets_load_equipment_with_sounds(cache,
                session->files.sounds.data,session->files.sounds.size,
                &session->assets,tag.handle,error))return 0;
        } else if(tag.group==UINT32_C(0x76656869)) {
            /* every vehicle the map holds, as for items: a map may hold
               ones its palette does not list (Longest's Warthog, which the
               game asks for when it starts) */
            if(!mac_actor_assets_load_vehicle_with_resources(cache,
                session->files.bitmaps.data,session->files.bitmaps.size,
                session->files.sounds.data,session->files.sounds.size,
                &session->assets,tag.handle,error))return 0;
        }
    }
    if(!mac_native_environment_lighting_load(cache,&session->world,
        session->files.bitmaps.data,session->files.bitmaps.size,
        &session->assets,error))return 0;
    {
        /* the planar fog the BSP's regions name, with its screen bitmap and
           sounds (the water maps') */
        uint32_t fogs[256],fog_count;
        if(!mac_engine_world_fog_handles(&session->world,fogs,256,&fog_count)) {
            if(error)*error="native fog palette";return 0;
        }
        for(i=0;i<fog_count;++i)
            if(fogs[i]!=UINT32_MAX && !mac_native_effect_assets_load(cache,session->files.bitmaps.data,
                session->files.bitmaps.size,session->files.sounds.data,session->files.sounds.size,
                &session->assets,fogs[i],error))return 0;
    }
    return 1;
}

/* The assets of the scenario's other structure BSPs (scenario_switch_structure_bsp):
   their materials' shaders and bitmaps, lightmaps, planar fog, and sounds. Each is
   loaded once however many BSPs name it. */
static int load_other_bsp_assets(struct mac_engine_session *session,const char **error)
{
    const struct mac_retail_cache *cache=&session->files.retail;
    uint32_t index,count=mac_engine_world_bsp_count(&session->world);
    for(index=0;index<count;++index) {
        struct mac_engine_world other;
        LOADING_PUMP();
        uint32_t fogs[256],fog_count,i;
        if(!mac_engine_world_other_bsp(&session->world,index,&other))continue;
        if(!mac_native_environment_lighting_load(cache,&other,
            session->files.bitmaps.data,session->files.bitmaps.size,&session->assets,error))return 0;
        if(!mac_engine_world_fog_handles(&other,fogs,256,&fog_count)) {
            if(error)*error="native fog palette (other BSP)";return 0;
        }
        for(i=0;i<fog_count;++i)
            if(fogs[i]!=UINT32_MAX && !mac_native_effect_assets_load(cache,session->files.bitmaps.data,
                session->files.bitmaps.size,session->files.sounds.data,session->files.sounds.size,
                &session->assets,fogs[i],error))return 0;
        if(!mac_actor_assets_load_world_sounds(cache,&other,session->files.sounds.data,
            session->files.sounds.size,&session->assets,error))return 0;
        fprintf(stderr,"native session: BSP %u assets loaded\n",(unsigned)index);
    }
    return 1;
}

/* A campaign scenario names its own HUD message text; the interface walk
   loads only the ones the HUD globals name. */
static int load_scenario_hud_messages(struct mac_engine_session *session,const char **error)
{
    uint32_t handle;
    struct mac_native_object_tag tag;
    const char *why=NULL;
    if(!mac_engine_world_scenario_hud_messages(&session->world,&handle)) {
        if(error)*error="native scenario HUD messages are unavailable";
        return 0;
    }
    if(handle==UINT32_MAX || mac_native_tags_group(&session->assets.tags,handle)!=UINT32_MAX)return 1;
    memset(&tag,0,sizeof(tag));
    if(!mac_native_hud_message_text_decode(&session->files.retail,handle,&tag,&why)) {
        if(error)*error=why?why:"native scenario HUD message decode failed";
        return 0;
    }
    if(!mac_native_tags_publish(&session->assets.tags,handle,UINT32_C(0x686d7420),tag.root,
        tag.root_bytes,tag.bytes_owned,tag.owner,mac_native_hud_message_text_owner_free,&why)) {
        mac_native_hud_message_text_tag_free(&tag);
        if(error)*error=why?why:"native scenario HUD message publication failed";
        return 0;
    }
    ++session->assets.published_count;
    return 1;
}

static int load_global_weapon_assets(struct mac_engine_session *session,const char **error)
{
    uint32_t handle=mac_native_tags_find(&session->assets.tags,GAME_GLOBALS_TAG,
        "globals\\globals");
    const struct game_globals *globals=mac_native_tags_get(&session->assets.tags,
        GAME_GLOBALS_TAG,handle,sizeof(*globals),error);
    const struct tag_reference *weapons;
    long i;
    if(!globals)return 0;
    weapons=globals->weapon_list.address;
    /* The original game-engine resource prediction visits this list, including
       weapons absent from the map's item collections. Own its real roots too. */
    for(i=0;i<globals->weapon_list.count;++i) {
        struct mac_retail_tag metadata;
        handle=(uint32_t)weapons[i].index;
        if(handle==UINT32_MAX)continue;
        if(!mac_retail_tag_get(&session->files.retail,handle&65535,&metadata) ||
           metadata.handle!=handle) {
            if(error)*error="native globals weapon metadata";return 0;
        }
        if(metadata.group==UINT32_C(0x77656170)) {
            if(!mac_actor_assets_load_weapon_with_resources(&session->files.retail,
                session->files.bitmaps.data,session->files.bitmaps.size,
                session->files.sounds.data,session->files.sounds.size,
                &session->assets,handle,error))return 0;
        } else if(metadata.group==UINT32_C(0x65716970)) {
            if(!mac_actor_assets_load_equipment_with_sounds(&session->files.retail,
                session->files.sounds.data,session->files.sounds.size,
                &session->assets,handle,error))return 0;
        } else {
            if(error)*error="native globals weapon family unsupported";return 0;
        }
    }
    return 1;
}

struct session_resource_context {
    struct mac_engine_session *session;
    const char **error;
};
static int load_animation_sound_reference(void *opaque,uint32_t group,uint32_t handle)
{
    struct session_resource_context *context=opaque;
    struct mac_engine_session *session=context->session;
    if(group!=UINT32_C(0x736e6421)) {
        if(context->error)*context->error="native animation references a non-sound family";
        return 0;
    }
    return mac_native_effect_assets_load(&session->files.retail,
        session->files.bitmaps.data,session->files.bitmaps.size,
        session->files.sounds.data,session->files.sounds.size,
        &session->assets,handle,context->error);
}
static int load_animation_sound_assets(struct mac_engine_session *session,const char **error)
{
    uint32_t cursor=0,handle;
    struct session_resource_context context;
    context.session=session;context.error=error;
    while((handle=mac_native_tags_next(&session->assets.tags,
        ANIMATION_GRAPH_TAG,&cursor))!=UINT32_MAX) {
        const struct animation_graph *graph=mac_native_tags_get(&session->assets.tags,
            ANIMATION_GRAPH_TAG,handle,sizeof(*graph),error);
        if(!graph || !mac_native_animation_visit_sounds(graph,
            load_animation_sound_reference,&context))return 0;
    }
    return 1;
}
static int load_model_shader_reference(void *opaque,uint32_t group,uint32_t handle)
{
    struct session_resource_context *context=opaque;
    struct mac_engine_session *session=context->session;
    if(!mac_native_tag_group_matches(group,UINT32_C(0x73686472))) {
        if(context->error)*context->error="native model references a non-shader family";
        return 0;
    }
    return mac_native_effect_assets_load(&session->files.retail,
        session->files.bitmaps.data,session->files.bitmaps.size,
        session->files.sounds.data,session->files.sounds.size,
        &session->assets,handle,context->error);
}
static int load_model_shader_assets(struct mac_engine_session *session,const char **error)
{
    uint32_t cursor=0,handle;
    struct session_resource_context context;
    context.session=session;context.error=error;
    while((handle=mac_native_tags_next(&session->assets.tags,
        UINT32_C(0x6d6f6465),&cursor))!=UINT32_MAX) {
        const struct model *model=mac_native_tags_get(&session->assets.tags,
            UINT32_C(0x6d6f6465),handle,sizeof(*model),error);
        if(!model || !mac_native_model_visit_shaders(model,
            load_model_shader_reference,&context))return 0;
    }
    /* BSP shaders may have been published by the lighting owner with only
       their base map loaded. Resolve their complete reference graph too. */
    cursor=0;
    while((handle=mac_native_tags_next(&session->assets.tags,
        UINT32_C(0x73686472),&cursor))!=UINT32_MAX) {
        uint32_t group=mac_native_tags_group(&session->assets.tags,handle);
        if(!load_model_shader_reference(&context,group,handle))return 0;
    }
    return 1;
}

static int load_global_resource_reference(void *opaque,uint32_t group,uint32_t handle)
{
    struct mac_engine_session *session=opaque;
    const char *error=NULL;
    /* First-person hands are a globals model, separate from each weapon's
       model. Load them before shader closure and first-person remapping. */
    if(group==UINT32_C(0x6d6f6432) || group==UINT32_C(0x6d6f6465))
        return mac_actor_assets_load_model(&session->files.retail,
            &session->assets,handle,&error);
    /* The grenades' projectiles: no weapon fires them, so only here. */
    if(group==UINT32_C(0x70726f6a))
        return mac_actor_assets_load_projectile_with_resources(&session->files.retail,
            session->files.bitmaps.data,session->files.bitmaps.size,
            session->files.sounds.data,session->files.sounds.size,
            &session->assets,handle,&error);
    if(group==UINT32_C(0x6269746d))
        return mac_actor_assets_load_bitmap(&session->files.retail,
            session->files.bitmaps.data,session->files.bitmaps.size,
            &session->assets,handle,&error);
    /* port: the King of the Hill's hill shader (a shader of any kind), which
       game_engine_king.c draws the hill's boundary with */
    switch(group) {
    case 0:case 0x73686472:case 0x73636869:case 0x73636578:case 0x73676c61:case 0x73776174:
    case 0x73706c61:case 0x736d6574:case 0x736f736f:case 0x73656e76: {
        struct mac_retail_tag shader;
        if(handle==UINT32_MAX || !mac_retail_tag_get(&session->files.retail,handle&65535,&shader) ||
           shader.handle!=handle)return 1;
        switch(shader.group) {
        case 0x73636869:case 0x73636578:case 0x73676c61:case 0x73776174:case 0x73706c61:
        case 0x736d6574:case 0x736f736f:case 0x73656e76:
            return mac_native_effect_assets_load(&session->files.retail,
                session->files.bitmaps.data,session->files.bitmaps.size,
                session->files.sounds.data,session->files.sounds.size,
                &session->assets,handle,&error);
        default:break;
        }
        break;
    }
    default:break;
    }
    /* Include falling/contact damage and night-vision effects in the same
       bounded dependency closure as globals sounds. HUD tags retain their
       dedicated owner below. */
    if(group!=UINT32_C(0x736e6421) && group!=UINT32_C(0x6a707421) &&
       group!=UINT32_C(0x65666665) &&
       /* the default unit camera track, for units without their own */
       group!=UINT32_C(0x7472616b))return 1;
    return mac_native_effect_assets_load(&session->files.retail,
        session->files.bitmaps.data,session->files.bitmaps.size,
        session->files.sounds.data,session->files.sounds.size,
        &session->assets,handle,&error);
}

static int load_game_globals(const struct mac_retail_cache *cache,
    struct mac_engine_session *session,const char **error)
{
    uint32_t i;
    for(i=0;i<cache->tag_count;++i) {
        struct mac_retail_tag metadata;
        struct mac_native_object_tag globals;
        const char *why=NULL;
        if(!mac_retail_tag_get(cache,i,&metadata)) {
            if(error)*error="native matg metadata is invalid";
            return 0;
        }
        if(metadata.group!=GAME_GLOBALS_TAG || strcmp(metadata.name,"globals\\globals"))continue;
        if(mac_native_tags_group(&session->assets.tags,metadata.handle)==GAME_GLOBALS_TAG)
            return 1;
        if(!mac_native_globals_decode(cache,metadata.handle,&globals,&why)) {
            if(error)*error=why?why:"native matg conversion failed";
            return 0;
        }
        if(!mac_native_object_visit_references(&globals,load_global_resource_reference,session)) {
            mac_native_object_tag_free(&globals);
            if(error)*error="native game globals resource dependency failed";
            return 0;
        }
        if(!mac_native_tags_publish(&session->assets.tags,metadata.handle,GAME_GLOBALS_TAG,
            globals.root,globals.root_bytes,globals.bytes_owned,globals.owner,
            mac_native_object_owner_free,&why)) {
            mac_native_object_tag_free(&globals);
            if(error)*error=why?why:"native matg publication failed";
            return 0;
        }
        return 1;
    }
    if(error)*error="retail cache has no globals\\globals matg";
    return 0;
}

/* port: cache_files.c's own, renamed by tools/mac_cache_files_native_view.py */
boolean scenario_structure_bsp_load_retail_legacy(struct scenario_structure_bsp_reference *reference);
void scenario_structure_bsp_unload_retail_legacy(struct scenario_structure_bsp_reference *reference);

/* the vertex buffers of every BSP of the world (whichever is loaded) */
static void dispose_bsp_buffers(struct mac_engine_world *world)
{
    uint32_t index,count=mac_engine_world_bsp_count(world);
    struct mac_engine_world other;
    mac_native_world_vertex_buffers_dispose(mac_engine_world_structure(world));
    for(index=0;index<count;++index)
        if(mac_engine_world_other_bsp(world,index,&other))
            mac_native_world_vertex_buffers_dispose(mac_engine_world_structure(&other));
}

/* scenario_structure_bsp_load and _unload (the cache_files view, for a BSP switch):
   the BSP's vertex buffers are made when it becomes the loaded one and let go of
   when another does (the GPU is up when device state has one). */
boolean mac_engine_structure_bsp_load(const struct scenario_structure_bsp_reference *reference)
{
    /* port: an Xbox map's bsp is read to its place in the tag cache, with its
       vertex buffers, by the engine's own loader (cache_files.c) */
    if(mac_cache_files_xbox_active()) {
        boolean loaded=scenario_structure_bsp_load_retail_legacy((struct scenario_structure_bsp_reference *)reference);
        if(loaded)mac_native_world_vertex_buffers_certify_xbox(
            (struct structure_bsp *)tag_get(STRUCTURE_BSP_TAG,reference->structure_bsp.index));
        return loaded;
    }
    struct structure_bsp *bsp=mac_engine_world_structure_for_reference(reference,1);
    if(!bsp)return FALSE;
    if(mac_device_state_get() && mac_device_state_get()->gpu &&
       !mac_native_world_vertex_buffers_prepare(bsp)) {
        fprintf(stderr,"native BSP: vertex buffers could not be made\n");
        return FALSE;
    }
    if(!structure_bsp_port_verify(bsp))
        fprintf(stderr,"native BSP %d: collision bsps %ld clusters %ld surfaces %ld cluster data %ld sound data %ld\n",
            (int)reference->structure_bsp.index,(long)bsp->collision_bsp.count,(long)bsp->clusters.count,
            (long)bsp->surfaces.count,(long)bsp->cluster_data.size,(long)bsp->sound_cluster_data.size);
    fprintf(stderr,"native BSP %d loaded\n",(int)reference->structure_bsp.index);
    return TRUE;
}
void mac_engine_structure_bsp_unload(const struct scenario_structure_bsp_reference *reference)
{
    if(mac_cache_files_xbox_active()) {
        mac_native_world_vertex_buffers_release_xbox(
            (struct structure_bsp *)tag_get(STRUCTURE_BSP_TAG,reference->structure_bsp.index));
        scenario_structure_bsp_unload_retail_legacy((struct scenario_structure_bsp_reference *)reference);
        return;
    }
    struct structure_bsp *bsp=mac_engine_world_structure_for_reference(reference,0);
    if(!bsp)return;
    mac_native_world_vertex_buffers_dispose(bsp);
    mac_device_bsp_decode_cache_clear();
    fprintf(stderr,"native BSP %d unloaded\n",(int)reference->structure_bsp.index);
}

int mac_engine_session_open(const char *map_path,const char *bitmaps_path,
    const char *sounds_path,uint32_t spawn_index,struct mac_engine_session *out,
    const char **error)
{
    struct mac_engine_session session;
    const struct mac_retail_cache *cache;
    const char *why=NULL;

    if(error)*error="native session output or already-owned state";
    if(!out || out->files.map.data || out->assets.tags.state || out->world.state || out->bound)
        return 0;
    memset(&session,0,sizeof(session));
    LOADING_PROGRESS(0.04f);
    if(!mac_cache_files_open(map_path,bitmaps_path,sounds_path,&session.files,&why))
        goto fail;
    LOADING_PROGRESS(0.08f);
    cache=&session.files.retail;
    /* port: the title screen's map (scenario type 2, main menu) has no player
       start points; it gets one at the origin in BSP 0 to open by */
    {
        const unsigned char *root=cache->scenario_offset<=UINT32_MAX-MAC_RETAIL_BASE?
            mac_retail_tag_span(cache,MAC_RETAIL_BASE+cache->scenario_offset,0x3e):NULL;
        session.main_menu=root && root[60]==2 && root[61]==0;
    }
    if(session.main_menu) {
        memset(session.spawns,0,sizeof(session.spawns[0]));
        session.spawns[0].bsp=0;session.spawn_count=1;
        if(!cache->bsp_count){why="the title map has no structure bsp";goto fail;}
    } else
    if(!mac_retail_spawns(cache,session.spawns,&session.spawn_count,&why) ||
       spawn_index>=session.spawn_count) {
        if(!why)why="selected retail spawn is outside the map";
        goto fail;
    }
    session.spawn_index=spawn_index;
    if(session.spawns[spawn_index].bsp<0 ||
       (uint32_t)session.spawns[spawn_index].bsp>=cache->bsp_count) {
        why="selected retail spawn has no valid BSP";
        goto fail;
    }
    session.bsp_index=(uint32_t)session.spawns[spawn_index].bsp;
    /* port: a campaign level starts in the bsp its scripts switch to first
       (b40, c40, d40); the port does not switch bsps while playing yet */
    {
        uint32_t start;
        if(mac_native_scenario_scripts_start_bsp(cache,&start) && start!=session.bsp_index) {
            fprintf(stderr,"campaign: the level's scripts start in structure bsp %u (its start points are in %u)\n",
                (unsigned)start,(unsigned)session.bsp_index);
            session.bsp_index=start;
        }
    }
    if(session.main_menu) {
        if(!mac_actor_assets_load_empty(cache,&session.assets,&why))goto fail;
    } else if(bitmaps_path) {
        if(!mac_actor_assets_load_with_bitmaps(cache,session.files.bitmaps.data,
            session.files.bitmaps.size,&session.assets,&why))goto fail;
    } else if(!mac_actor_assets_load(cache,&session.assets,&why))goto fail;
    /* port: the title map's globals name no player biped; nothing spawns a player unit */
    if(!session.main_menu && !mac_actor_assets_load_biped_with_resources(cache,
        session.files.bitmaps.data,session.files.bitmaps.size,
        session.files.sounds.data,session.files.sounds.size,
        &session.assets,session.assets.biped,&why))goto fail;
    if(!load_game_globals(cache,&session,&why) ||
       !mac_native_interface_assets_load(cache,session.files.bitmaps.data,
           session.files.bitmaps.size,session.files.sounds.data,session.files.sounds.size,
           &session.assets,&why))goto fail;
    /* (each step named, for one that fails without saying why) */
#define SESSION_STEP(call,name) do { if(!(call)) { if(!why)why=(name);goto fail; } } while(0)
    LOADING_PROGRESS(0.12f);
    SESSION_STEP(mac_engine_world_create(cache,session.bsp_index,&session.world,&why),"world creation");
    SESSION_STEP(mac_engine_world_enable_graph(&session.world,cache,&why),"world cluster graph");
    SESSION_STEP(mac_engine_world_enable_surfaces(&session.world,cache,&why),"world surfaces");
    LOADING_PROGRESS(0.2f);
    SESSION_STEP(mac_engine_world_create_other_bsps(&session.world,cache,&why),"other structure BSPs");
    LOADING_PROGRESS(0.3f);
    /* HALO_MAC_AI_ASSET_CHECK (diagnostic): the AI's tags are loaded first,
       and a failure reported, for a level whose other assets are refused */
    if(getenv("HALO_MAC_AI_ASSET_CHECK")) {
        const char *ai_why=NULL;
        int ai_ok=load_ai_assets(&session,&ai_why);
        fprintf(stderr,"campaign ai asset check: %s%s%s\n",ai_ok?"loaded":"refused: ",ai_ok?"":(ai_why?ai_why:"no reason"),"");
    }
    SESSION_STEP(load_item_and_environment_assets(&session,&why),"item and environment assets");
    LOADING_PROGRESS(0.4f);
    SESSION_STEP(load_placed_object_assets(&session,&why),"placed object assets");
    LOADING_PROGRESS(0.55f);
    SESSION_STEP(load_other_bsp_assets(&session,&why),"other structure BSP assets");
    LOADING_PROGRESS(0.7f);
    SESSION_STEP(load_scenario_hud_messages(&session,&why),"scenario HUD messages");
    SESSION_STEP(load_script_reference_assets(&session,&why),"script reference assets");
    SESSION_STEP(load_ai_assets(&session,&why),"ai assets");
    LOADING_PROGRESS(0.78f);
    SESSION_STEP(load_global_weapon_assets(&session,&why),"global weapon assets");
    SESSION_STEP(load_seat_actors(&session,&why),"seat actor assets");
    SESSION_STEP(load_animation_sound_assets(&session,&why),"animation sound assets");
    LOADING_PROGRESS(0.85f);
    SESSION_STEP(load_model_shader_assets(&session,&why),"model shader assets");
    /* port: the title music, whose sounds the closure below loads */
    if(session.main_menu)
        SESSION_STEP(mac_actor_assets_load_title_music(cache,&session.assets,&why),"title music");
    SESSION_STEP(mac_actor_assets_load_looping_sound_dependencies(cache,session.files.sounds.data,
        session.files.sounds.size,&session.assets,&why),"looping sound dependencies");
#undef SESSION_STEP
    LOADING_PROGRESS(0.9f);
    if(mac_device_state_get() && mac_device_state_get()->gpu &&
       !mac_native_world_vertex_buffers_prepare(mac_engine_world_structure(&session.world))) {
        why="native BSP vertex buffer allocation";
        {
            unsigned long bitmap_bytes=0;unsigned b;
            for(b=0;b<session.assets.bitmap_count;++b)bitmap_bytes+=session.assets.bitmaps[b].bytes_owned;
            fprintf(stderr,"native contiguous memory: %lu allocations, %lu bytes; %u bitmaps, %lu bytes\n",
                mac_resource_allocation_count(),mac_resource_allocation_bytes(),
                session.assets.bitmap_count,bitmap_bytes);
            fprintf(stderr,"native contiguous window mapped: %lu bytes\n",mac_resource_window_mapped_bytes());
        }
        goto fail;
    }
    *out=session;
    if(error)*error=NULL;
    return 1;
fail:
    if(error)*error=why?why:"native session initialization failed";
    fprintf(stderr,"native session load failed before teardown: %s\n",
        why?why:"unknown dependency");
    mac_device_bsp_decode_cache_clear();
    dispose_bsp_buffers(&session.world);
    mac_engine_world_free(&session.world);
    mac_actor_assets_free(&session.assets);
    mac_cache_files_close(&session.files);
    return 0;
}

int mac_engine_session_bind(struct mac_engine_session *session)
{
    if(!session || session->bound || !session->files.map.data ||
       !session->assets.tags.state)return 0;
    if(!mac_cache_files_bind(&session->files))return 0;
    if(!mac_engine_tags_bind(&session->assets.tags)) {
        mac_cache_files_unbind(&session->files);
        return 0;
    }
    if(session->assets.bitmap_count && !mac_actor_assets_bind_texture_cache(&session->assets)) {
        mac_engine_tags_unbind();
        mac_cache_files_unbind(&session->files);
        return 0;
    }
    if(session->assets.bitmap_count && !mac_engine_texture_lifecycle_bind()) {
        mac_actor_assets_unbind_texture_cache(&session->assets);
        mac_engine_tags_unbind();
        mac_cache_files_unbind(&session->files);
        return 0;
    }
    if(!mac_engine_precache_bind(&session->files.retail)) {
        if(session->assets.bitmap_count)mac_engine_texture_lifecycle_unbind();
        mac_actor_assets_unbind_texture_cache(&session->assets);
        mac_engine_tags_unbind();
        mac_cache_files_unbind(&session->files);
        return 0;
    }
    session->bound=1;
    return 1;
}

int mac_engine_session_unbind(struct mac_engine_session *session)
{
    if(!session || !session->bound)return 0;
    /* Preserve the complete binding if its owner scope cannot be released.
       The cache-scope release is the only fallible unbind operation. */
    if(!mac_cache_files_unbind(&session->files))return 0;
    if(!mac_engine_precache_unbind(&session->files.retail))return 0;
    if(session->assets.bitmap_count && !mac_engine_texture_lifecycle_unbind())return 0;
    mac_actor_assets_unbind_texture_cache(&session->assets);
    mac_engine_tags_unbind();
    session->bound=0;
    return 1;
}

int mac_engine_session_scope(const struct mac_engine_session *session,
    int (*operation)(void *),void *context)
{
    if(!session || !session->bound || !session->world.state || !operation)return 0;
    return mac_engine_world_scope(&session->world,operation,context);
}

const struct mac_retail_cache *mac_engine_session_cache(const struct mac_engine_session *session)
{
    return session && session->files.map.data ? &session->files.retail : NULL;
}

int mac_engine_session_spawn(const struct mac_engine_session *session,
    struct mac_retail_spawn *spawn)
{
    if(!session || !spawn || session->spawn_index>=session->spawn_count)return 0;
    *spawn=session->spawns[session->spawn_index];
    return 1;
}

void mac_engine_session_close(struct mac_engine_session *session)
{
    if(!session)return;
    if(session->bound && !mac_engine_session_unbind(session))return;
    mac_device_bsp_decode_cache_clear();
    dispose_bsp_buffers(&session->world);
    mac_engine_world_free(&session->world);
    /* port: what mac_engine_scenario_unload held points into that world */
    mac_engine_scenario_forget();
    mac_actor_assets_free(&session->assets);
    mac_cache_files_close(&session->files);
    memset(session,0,sizeof(*session));
}
