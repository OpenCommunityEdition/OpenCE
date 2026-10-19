/* Two complete native v7 session ownership cycles inside the real native
   physical-memory, game-state, game startup, and shutdown lifecycle. */
#include "cseries.h"
#include "cache/cache_files.h"
#include "cache/texture_cache.h"
#include "engine_texture_lifecycle.h"
#include "engine_device_state.h"
#include "engine_arena.h"
#include "engine_physical_memory.h"
#include "engine_session.h"
#include "engine_cache_files.h"
#include "engine_synchronous_precache.h"
#include "xbox_map.h"
#include "native_object_tags.h"
#include "engine_game_runtime.h"
#include "engine_game_variant.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "scenario/scenario_definitions.h"
#include "game/game_engine_list.h"
#include "camera/director.h"
#include "camera/observer.h"
#include "camera/camera_scripting.h"
#include "physics/collision_usage.h"
#include "physics/collision_model_definitions.h"
#include "game/game_globals.h"
#include "scenario/scenario.h"
#include "tag_files/tag_files.h"
#include "math/real_math.h"
#include "cache/physical_memory_map.h"
#include "saved games/game_state.h"
#include "rasterizer/xbox/rasterizer_xbox.h"
#include "input/input.h"
#include "input/input_abstraction.h"
#include "game/player_control_runtime.h"
#define MAC_INPUT_NO_CARBON
#include "mac_input.h"
#include "engine_input.h"
#include "mac_movie.h" /* port: port/mac/mac_movie.c */
#include "game/players.h"
#include "cutscene/cinematics.h"
#include "game/local_players.h"
#include "game/player_queues_new.h"
#include "objects/objects.h"
#include "objects/object_definitions.h"
#include "units/units.h"
#include "items/weapons.h"
#include "items/projectiles.h"
#include "main/main.h"
#include "sound/sound_manager.h"
#ifdef MAC_SESSION_GPU_RENDER
#include "mac_sound_convert.h"
#endif
#ifdef MAC_SESSION_GPU_RENDER
#include "rasterizer/rasterizer.h"
#include "mac_dsound_audio_stats.h"
#include "native_resource_memory.h"
#include "engine_device_factory.h"
#include "mac_session_interactive.h"
#include "mac_gpu_textures.h"
#include "engine_text_gl.h"
#include "render/render.h"
#include "render/render_cameras.h"
#include "engine_menu.h"
#include "engine_network_test.h" /* port: the menus' network games (port/mac/engine_network_test.c) */
#include "mac_gpu_info.h"
#include "mac_hud_hires.h"
/* port/linux/src/port_config.c */
const char *config_string(const char *name);
int config_boolean(const char *name);
long config_integer(const char *name);
short players_port_local_player_count(void); /* port: source/game/players.c */
void port_mods_startup(void); /* port/linux/src/port_mods.c */
/* port/mac/engine_files.c */
int mac_engine_files_data_root(char *root,size_t capacity);
#include <sys/stat.h>
#include "mac_loading.h" /* port: the loading screen below (port/mac/mac_loading.c) */
#endif

int mac_texture_lifecycle_bound_probe(void);
int mac_tag_compat_probe(const struct mac_engine_session *session);
/* Source sound_manager.c public function is absent from its header. */
boolean sound_is_active(void);

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks,failures;
/* the map is Blood Gulch, whose start sees the shot fly (other maps' checks
   expect less of the scene) */
static int probe_map_is_blood_gulch=1;
/* HALO_MAC_CAMPAIGN=1: a campaign level, started with no game engine (the
   multiplayer checks that need Slayer's Blood Gulch are skipped) */
static int probe_campaign;
/* port: the session's map is the title screen's (ui.map: scenario type main
   menu): no player unit and no start point; the scripts place the camera */
static int probe_title;
/* HALO_MAC_MENU=1 (engine_menu.h): the menus start the games. The first
   session is the menu's background map; a game the menu chooses is a session
   of its own, and the pause menu's leave comes back to the menu's. */
static int probe_menu_mode;
static int probe_menu_phase;
static unsigned probe_request;
int mac_engine_main_credits_pending(int clear); /* port: tools/mac_main_game_render_view.py */
static int probe_refused;
static void probe_refuse(const char *what);
static unsigned probe_cycles;
static unsigned probe_menu_sessions;
/* whether a session only spawns the player (a visible game; the menu's
   games, and its background after the first, are not walked, looked
   around and shot as the regression runs are) */
#define probe_spawn_only() (mac_player_play_mode||(probe_menu_mode&&(!probe_menu_phase||probe_menu_sessions>1)))
extern struct data_array *hs_thread_data;
static int mac_player_play_mode;
static void (*volatile mac_full_game_tick)(void)=game_tick;
#ifdef MAC_SESSION_GPU_RENDER
static int mac_gpu_render_enabled;
static int mac_gpu_visible;
static unsigned mac_render_frames=1;
static int mac_render_seed_enabled;
static unsigned long mac_render_seed;
extern void mac_engine_main_game_render(double time_delta_since_tick_sec);
extern void render_interpolation_frame_begin(void);
extern void render_interpolation_frame_end(void);

static int mac_render_seed_parse(const char *text,unsigned long *seed)
{
    unsigned long value=0;
    if(!text||!*text||!seed)return 0;
    while(*text) {
        unsigned long digit;
        if(*text<'0'||*text>'9')return 0;
        digit=(unsigned long)(*text-'0');
        if(value>(0xffffffffUL-digit)/10UL)return 0;
        value=value*10UL+digit;
        ++text;
    }
    *seed=value;
    return 1;
}
static int mac_env_is_one(const char *name)
{
    const char *value=getenv(name);
    return value&&value[0]=='1'&&value[1]=='\0';
}
#endif
#define CHECK(value) do { ++checks; if(!(value)) { \
    ++failures; fprintf(stderr,"FAIL %s at line %d\n",#value,__LINE__); \
} } while(0)

#ifdef MAC_SESSION_GPU_RENDER
static int resolve_font_atlas(void *context,const void *resource,unsigned char sampler,
    GLuint *name,GLenum *target,unsigned *levels,float scale[4],const char **error)
{
    const D3DBaseTexture *texture=(const D3DBaseTexture *)resource;
    int result;unsigned int atlas_name=0;
    (void)context;
    result=mac_text_gl_resolve(texture->Data,&atlas_name,scale);
    /* The atlas's placeholder bitmap is swizzled, as the game's bitmaps are:
       the text vertex shader normalizes its coordinates by 1/256 already, so
       the pixel shader must not scale them again (a linear one would). The
       scale is the same for every byte order: floats, not packed pixels. */
    if(result==1&&!mac_gpu_textures_resource_is_linear(resource))scale[0]=scale[1]=1.0f;
    if(result==0) {
        /* (the menus' pictures, when the menus are up: HALO_MAC_MENU=1) */
        result=mac_engine_menu_art_resolve(resource,sampler,&atlas_name,scale);
        if(result<0){if(error)*error="native menu picture upload or sampler failed";return -1;}
    }
    *name=(GLuint)atlas_name;
    if(result==1 && sampler!=MAC_GPU_TEXTURE_SAMPLER_2D)result=-1;
    if(result<0){if(error)*error="native font atlas upload or sampler failed";return -1;}
    if(result){*target=GL_TEXTURE_2D;*levels=1;}
    return result;
}
struct original_render_call { unsigned calls; };
static int render_original_game_frame(void *opaque,const struct mac_input_frame *input,
    unsigned width,unsigned height)
{
    struct original_render_call *call=(struct original_render_call *)opaque;
    (void)input;
    if(width!=640||height!=480)return 0;
    /* Match the original main loop's interpolation bracket and render body. */
    render_interpolation_frame_begin();
    mac_engine_main_game_render(1.0/30.0);
    render_interpolation_frame_end();
    main_present_frame();
    ++call->calls;
    return 1;
}

static void run_original_game_render(unsigned cycle)
{
    const struct mac_device_state *device;
    struct original_render_call call;
    const unsigned int *pixels;
    unsigned long pitch,x,y,nonblack=0;
    unsigned long presents;
    unsigned long long digest=UINT64_C(14695981039346656037);
    const char *capture=getenv("HALO_MAC_RENDER_CAPTURE");
    FILE *file;

    device=mac_device_state_get();
    CHECK(device&&device->gpu&&device->gpu_width==640&&device->gpu_height==480);
    if(!device||!device->gpu)return;
    presents=device->gpu_present_count;
    memset(&call,0,sizeof(call));
    /* HALO_MAC_MENU=1 (diagnostic): the PC version's main menu over the map */
    if(probe_menu_mode) {
        CHECK(mac_engine_menu_start("game map",probe_menu_phase));
    }
    if(mac_gpu_visible||mac_render_frames>1||probe_menu_mode) {
        struct mac_session_interactive_stats stats;
        unsigned frames=mac_render_frames;
        memset(&stats,0,sizeof(stats));
        /* (the menu waits for the player: a script or a request ends its run) */
        if(probe_menu_mode&&probe_menu_phase&&!mac_gpu_visible) {
            const char *cap=getenv("HALO_MAC_MENU_MAX_FRAMES");
            frames=cap&&atoi(cap)>0?(unsigned)atoi(cap):600;
        }
        CHECK(mac_session_interactive_run(frames,mac_gpu_visible,&stats));
        probe_request=stats.request;
        call.calls=stats.frames;
        if(mac_player_play_mode && mac_gpu_visible && stats.quit_requested && !stats.frames)
            return;
        CHECK(stats.ticks>0);
        if(frames&&!stats.request&&!stats.quit_requested)CHECK(stats.frames==frames);
        /* A hidden regression run must remain in gameplay: a successfully
           rendered postgame screen does not validate a sustained session. */
        /* port: nor one whose network game ended (engine_network_test.c) */
        if(!mac_gpu_visible&&!mac_engine_network_test_game_over()) {
            CHECK(!game_engine_showing_postgame());
            /* port: a campaign cinematic (d40's) can take the player's unit away */
            CHECK(probe_title || player_control_get_unit_index(0)!=NONE || (probe_campaign && cinematic_in_progress()));
        }
        fprintf(stderr,"native rendered loop: frames=%u ticks=%u quit=%u\n",
            stats.frames,stats.ticks,stats.quit_requested);
        /* port: a network game that ended or lost its host stopped the run
           before a frame was drawn; there is no picture to check */
        if(mac_engine_network_test_game_over()) {
            fprintf(stderr,"native rendered loop: the network test's game is over\n");
            return;
        }
    } else {
        mac_device_gpu_request_readback();
        CHECK(mac_device_gpu_frame(render_original_game_frame,&call));
    }
    device=mac_device_state_get();
    fprintf(stderr,"native world visibility: cluster=%d rendered_clusters=%d surfaces=%ld\n",
        render.cluster_index,render.rendered_cluster_count,render.environment_surface_count);
    /* port: a campaign cinematic (a10's, in space) may show no bsp surface */
    if(!mac_gpu_visible)CHECK(probe_title || render.environment_surface_count>0 || (probe_campaign && cinematic_in_progress()));
    CHECK(call.calls>0&&device&&device->gpu_present_count==presents+call.calls);
    if(!device||!call.calls)return;
    pixels=(const unsigned int *)PLATFORM_PHYSICAL_TO_VIRTUAL(device->back_buffer.Data);
    pitch=(device->gpu_width*4+D3DTEXTURE_PITCH_ALIGNMENT-1)&~(unsigned long)(D3DTEXTURE_PITCH_ALIGNMENT-1);
    if(!pixels||!pitch) { CHECK(FALSE);return; }
    for(y=0;y<device->gpu_height;++y)for(x=0;x<device->gpu_width;++x) {
        unsigned int pixel=pixels[(pitch/4)*y+x];
        unsigned char red=(unsigned char)(pixel>>16);
        unsigned char green=(unsigned char)(pixel>>8);
        unsigned char blue=(unsigned char)pixel;
        digest^=red;digest*=UINT64_C(1099511628211);
        digest^=green;digest*=UINT64_C(1099511628211);
        digest^=blue;digest*=UINT64_C(1099511628211);
        if(red||green||blue)++nonblack;
    }
    /* (a visible game ends whenever its window closes: a fade or a cutscene's cut may be black) */
    CHECK(mac_gpu_visible||(digest!=0&&nonblack>0));
    fprintf(stderr,"original render cycle %u: presents=%lu nonblack_pixels=%lu digest=%016llx\n",
        cycle,device->gpu_present_count-presents,nonblack,digest);
    /* port: the frame is written only when HALO_MAC_RENDER_CAPTURE asks for it (it was always written, as
       mac_game_render_cycle1.ppm in the working directory: 900 KB in the app's folder in normal play) */
    file=(capture&&*capture)?fopen(capture,"wb"):NULL;
    CHECK(file!=NULL||!capture||!*capture);
    if(!file)return;
    fprintf(file,"P6\n%lu %lu\n255\n",device->gpu_width,device->gpu_height);
    for(y=0;y<device->gpu_height;++y)for(x=0;x<device->gpu_width;++x) {
        unsigned int pixel=pixels[(pitch/4)*y+x];
        unsigned char rgb[3];
        rgb[0]=(unsigned char)(pixel>>16);rgb[1]=(unsigned char)(pixel>>8);rgb[2]=(unsigned char)pixel;
        if(fwrite(rgb,1,sizeof(rgb),file)!=sizeof(rgb))failures++;
    }
    if(fclose(file)!=0)++failures;
}
#endif

struct scope_context {
    struct mac_engine_session *session;
    unsigned cycle;
};

static void update_native_camera(void)
{
    /* The original main loop updates these services after game_time_update. */
    collision_log_continue_period(TRUE);
    director_update(1.0f/30.0f);
    observer_update(1.0f/30.0f);
    collision_log_end_period();
    game_engine_update_non_deterministic(1.0f/30.0f);
}

static void run_native_look(void)
{
    struct mac_input input;
    struct mac_input_frame frame;
    const struct observer_result *camera=observer_get_camera(0);
    real_vector3d before;
    unsigned step;
    CHECK(camera!=NULL);
    if(!camera)return;
    before=camera->forward;
    mac_input_init(&input);mac_input_focus(&input,1);
    for(step=0;step<12;++step) {
        long previous_time=game_time_get();
        mac_input_motion(&input,4.0f,0.0f);
        mac_input_snapshot(&input,&frame);mac_engine_input_publish(&frame);
        input_frame_begin();input_update();input_abstraction_update();
        CHECK(player_control_update(1.0f/30.0f)==FALSE);
        game_time_update(1.0f/30.0f);update_native_camera();input_frame_end();
        CHECK(game_time_get()==previous_time+1);
    }
    camera=observer_get_camera(0);
    CHECK(camera!=NULL);
    if(camera) {
        real dx=camera->forward.i-before.i,dy=camera->forward.j-before.j;
        real dz=camera->forward.k-before.k;
        CHECK(dx*dx+dy*dy+dz*dz>0.000001f);
        CHECK(camera->location.cluster_index!=NONE);
        fprintf(stderr,"native mouse look: forward=(%.9g,%.9g,%.9g)->(%.9g,%.9g,%.9g)\n",
            before.i,before.j,before.k,camera->forward.i,camera->forward.j,camera->forward.k);
    }
    mac_engine_input_publish(NULL);input_update();
}

static void run_native_firing(long unit_index)
{
    struct mac_input input;
    struct mac_input_frame frame;
    struct unit_datum *unit=unit_get(unit_index);
    short slot=unit->unit.current_weapon_index;
    long weapon_index,before_fired;
    unsigned step,owned_projectile_frames=0;
    CHECK(slot>=0 && slot<MAXIMUM_WEAPONS_PER_UNIT);
    if(slot<0 || slot>=MAXIMUM_WEAPONS_PER_UNIT)return;
    weapon_index=unit->unit.weapon_object_indices[slot];
    CHECK(weapon_index!=NONE);
    if(weapon_index==NONE)return;
    before_fired=weapon_get(weapon_index)->weapon.game_time_last_fired;
    mac_input_init(&input);mac_input_focus(&input,1);
    /* Use the actual selected loadout, including charge/release weapons.
       Observe projectiles each frame because real collision may retire them. */
    for(step=0;step<80;++step) {
        struct object_iterator iterator;
        long previous_time=game_time_get();
        int owned=0;
        mac_input_button(&input,1,step>=10 && step<70);
        mac_input_snapshot(&input,&frame);mac_engine_input_publish(&frame);
        input_frame_begin();input_update();input_abstraction_update();
        CHECK(player_control_update(1.0f/30.0f)==FALSE);
        game_time_update(1.0f/30.0f);update_native_camera();input_frame_end();
        CHECK(game_time_get()==previous_time+1);
        object_iterator_new(&iterator,_object_mask_projectile,0);
        while(object_iterator_next(&iterator))
            if(projectile_get(iterator.index)->projectile.ignore_object_index==unit_index)owned=1;
        if(owned)++owned_projectile_frames;
    }
    CHECK(weapon_get(weapon_index)->weapon.game_time_last_fired>before_fired);
    /* (on other maps the start may face a wall the shot hits at once) */
    CHECK(owned_projectile_frames>0 || !probe_map_is_blood_gulch);
    fprintf(stderr,"native mouse firing: weapon=%08lx fired_time=%ld->%ld projectile_frames=%u\n",
        (unsigned long)weapon_index,before_fired,
        weapon_get(weapon_index)->weapon.game_time_last_fired,owned_projectile_frames);
    mac_engine_input_publish(NULL);input_update();
}

static void run_player_ticks(void)
{
    struct mac_input native_input;
    struct mac_input_frame native_frame;
    struct player_datum *player;
    real_point3d start;
    unsigned tick,moving_ticks=0;
    long player_index,unit_index=NONE;
    /* port: a game's local players (split screen: co-op, the menus' joined players), each on its controller */
    player_index=probe_title?player_new(0,NONE,0,NULL):mac_engine_menu_create_local_players();
    CHECK(player_index!=NONE);
    if(player_index==NONE)return;
    if(probe_title)local_player_set_player_index(0,player_index);
    if(probe_title) {
        /* port: as main_load_ui_scenario does after main_new_map: the player
           has no unit, and the scripts' camera is the view */
        game_time_start();
        game_initial_pulse();
        director_script_camera(TRUE);
        scripted_camera_set(0,0,NONE);
        return;
    }
    game_time_start();
    game_initial_pulse();
    mac_input_init(&native_input);
    mac_input_focus(&native_input,1);
    /* Feed native key snapshots through input abstraction and player control.
       The engine chooses a start and creates/arms its player; no unit is
       manually created, attached, teleported or advanced. */
    for(tick=0;tick<180 && moving_ticks<30 &&
        (!probe_spawn_only() || unit_index==NONE);++tick) {
        long previous_time=game_time_get();
        mac_input_key(&native_input,13,!probe_spawn_only() && unit_index!=NONE); /* ANSI W */
        mac_input_snapshot(&native_input,&native_frame);
        mac_engine_input_publish(&native_frame);
        input_frame_begin();input_update();input_abstraction_update();
        CHECK(player_control_update(1.0f/30.0f)==FALSE);
        /* Player control queues the action; the original clock schedules it. */
        game_time_update(1.0f/30.0f);
        update_native_camera();
        input_frame_end();
        CHECK(game_time_get()==previous_time+1);
        player=player_get(player_index);
        if(unit_index==NONE && player->unit_index!=NONE) {
            unit_index=player->unit_index;
            start=object_get(unit_index)->object.position;
            fprintf(stderr,"stage: engine spawned player unit %08lx at tick %u\n",
                (unsigned long)unit_index,tick+1);
        } else if(unit_index!=NONE) {
            CHECK(player->unit_index==unit_index);
            if(player->unit_index!=unit_index)break;
            ++moving_ticks;
        }
    }
    mac_engine_input_publish(NULL);
    input_update();
    CHECK(unit_index!=NONE);
    /* port: debug.mac_local_players (HALO_MAC_LOCAL_PLAYERS): a split screen start has all its players, each with
       a unit (a campaign level's opening cinematic may not have given them theirs yet) */
    if(!probe_title && config_integer("debug.mac_local_players")>1) {
        long wanted=config_integer("debug.mac_local_players"),present=0,with_units=0;
        short controller;
        if(wanted>4)wanted=4;
        if(probe_campaign && wanted>2)wanted=2;
        for(controller=0;controller<4;++controller)
            if(local_player_get_player_index(controller)!=NONE) {
                ++present;
                if(player_control_get_unit_index(controller)!=NONE)++with_units;
            }
        fprintf(stderr,"split screen: %ld local players wanted, %ld present, %ld with units, %d in the game\n",
            wanted,present,with_units,(int)players_port_local_player_count());
        CHECK(present==wanted);
        CHECK(players_port_local_player_count()==wanted);
        CHECK(with_units==wanted || (probe_campaign && cinematic_in_progress()));
    }
    if(probe_spawn_only())return;
    if(probe_campaign) {
        /* (the walk, look and shot checks belong to the multiplayer start) */
        static const char *const type_names[]={"biped","vehicle","weapon","equipment","garbage",
            "projectile","scenery","machine","control","light_fixture","placeholder","sound_scenery"};
        unsigned counts[12],t;
        struct object_iterator iterator;
        const struct scenario *scenario=global_scenario_get();
        memset(counts,0,sizeof(counts));
        object_iterator_new(&iterator,_object_mask_all,0);
        while(object_iterator_next(&iterator)) {
            short type=object_get(iterator.index)->object.type;
            CHECK(type>=0 && type<12);
            if(type>=0 && type<12)++counts[type];
        }
        fprintf(stderr,"campaign objects:");
        for(t=0;t<12;++t)if(counts[t])fprintf(stderr," %s=%u",type_names[t],counts[t]);
        fprintf(stderr,"\ncampaign scenario placements: bipeds=%ld vehicles=%ld weapons=%ld equipment=%ld "
            "scenery=%ld machines=%ld controls=%ld light_fixtures=%ld sound_scenery=%ld\n",
            scenario->bipeds.count,scenario->vehicles.count,scenario->weapons.count,scenario->equipment.count,
            scenario->scenery.count,scenario->machines.count,scenario->controls.count,
            scenario->light_fixtures.count,scenario->sound_scenery.count);
        fprintf(stderr,"campaign scenario blocks: scripts=%ld globals=%ld trigger_volumes=%ld cutscene_flags=%ld "
            "camera_points=%ld encounters=%ld command_lists=%ld recorded_animations=%ld\n",
            scenario->hs_scripts.count,scenario->hs_globals.count,scenario->trigger_volumes.count,
            scenario->cutscene_flags.count,scenario->cutscene_camera_points.count,scenario->ai_encounters.count,
            scenario->ai_command_lists.count,scenario->recorded_animations.count);
        {
            struct data_iterator threads;unsigned running=0;
            data_iterator_new(&threads,hs_thread_data);
            while(data_iterator_next(&threads))++running;
            fprintf(stderr,"campaign scripts: %u threads running\n",running);
            CHECK(running>0);
        }
        /* every placement the difficulty allows must have made an object */
        /* port: vehicles (a30's are made by scripts) and weapons may be none at the start */
        CHECK(counts[0]>0 && counts[6]>0);
        if(unit_index!=NONE)
            fprintf(stderr,"campaign: player unit %08lx at (%.9g,%.9g,%.9g) after %u ticks\n",
                (unsigned long)unit_index,object_get(unit_index)->object.position.x,
                object_get(unit_index)->object.position.y,object_get(unit_index)->object.position.z,tick);
        return;
    }
    CHECK(moving_ticks==30);
    if(unit_index!=NONE && moving_ticks==30) {
        const struct object_datum *unit=object_get(unit_index);
        real dx=unit->object.position.x-start.x,dy=unit->object.position.y-start.y;
        CHECK(unit->object.location.cluster_index!=NONE);
        CHECK(dx*dx+dy*dy>0.0001f);
        fprintf(stderr,"native input/clock frames: %u frames, %u movement ticks, delta=(%.9g,%.9g)\n",
            tick,moving_ticks,dx,dy);
        {
            const struct observer_result *camera=observer_get_camera(0);
            CHECK(camera!=NULL);
            if(camera) {
                real length=camera->forward.i*camera->forward.i+
                    camera->forward.j*camera->forward.j+camera->forward.k*camera->forward.k;
                CHECK(camera->location.cluster_index!=NONE);
                CHECK(camera->field_of_view>0.f && camera->field_of_view<3.141593f);
                CHECK(length>0.99f && length<1.01f);
                CHECK(director_get_perspective(0)==_director_perspective_first_person);
                fprintf(stderr,"native observer: position=(%.9g,%.9g,%.9g) fov=%.9g cluster=%d\n",
                    camera->position.x,camera->position.y,camera->position.z,
                    camera->field_of_view,camera->location.cluster_index);
            }
        }
        run_native_look();
        run_native_firing(unit_index);
    }
}

static int check_published_sound(void *opaque,uint32_t group,uint32_t handle)
{
    const struct mac_engine_session *session=opaque;
    CHECK(group==UINT32_C(0x736e6421));
    CHECK(mac_native_tags_get(&session->assets.tags,group,handle,1,NULL)!=NULL);
    return 1;
}
static void check_dialogue_dependencies(const struct mac_engine_session *session)
{
    uint32_t cursor=0,handle;unsigned count=0;
    while((handle=mac_native_tags_next(&session->assets.tags,UINT32_C(0x75646c67),&cursor))!=UINT32_MAX) {
        struct mac_native_object_tag dialogue;
        memset(&dialogue,0,sizeof(dialogue));
        dialogue.handle=handle;dialogue.group=UINT32_C(0x75646c67);
        dialogue.root_bytes=0x1010; /* Complete native dialogue schema extent. */
        dialogue.root=(void *)mac_native_tags_get(&session->assets.tags,dialogue.group,
            handle,dialogue.root_bytes,NULL);
        CHECK(mac_native_object_visit_references(&dialogue,check_published_sound,(void *)session));
        ++count;
    }
    CHECK(count>0);
}

static void check_collision_effect_dependencies(const struct mac_engine_session *session)
{
    const struct object_definition *biped=mac_native_tags_get(&session->assets.tags,
        UINT32_C(0x62697064),session->assets.biped,sizeof(*biped),NULL);
    const struct collision_model *model;
    const struct tag_reference *references[8];unsigned i;
    CHECK(biped!=NULL);if(!biped)return;
    model=mac_native_tags_get(&session->assets.tags,UINT32_C(0x636f6c6c),
        (uint32_t)biped->object.collision_model.index,sizeof(*model),NULL);
    CHECK(model!=NULL);if(!model)return;
    references[0]=&model->resistance.localized_damage_effect;
    references[1]=&model->resistance.area_damage_effect;
    references[2]=&model->resistance.body_damaged_effect;
    references[3]=&model->resistance.body_depleted_effect;
    references[4]=&model->resistance.body_destroyed_effect;
    references[5]=&model->resistance.shield_damaged_effect;
    references[6]=&model->resistance.shield_depleted_effect;
    references[7]=&model->resistance.shield_recharging_effect;
    for(i=0;i<8;++i)if(references[i]->index!=NONE)
        CHECK(mac_native_tags_get(&session->assets.tags,(uint32_t)references[i]->group_tag,
            (uint32_t)references[i]->index,1,NULL)!=NULL);
}

/* port: the loading screen. The Xbox game draws progress_bar.c's blurring fade and sweep of light (and
   plays its chant) while a map loads (game.c: main_pregame_render, main_present_frame, until the load
   is done), and the Linux and Windows ports run that same code. The Mac's session load is a number of
   synchronous steps, so the same screen (port/mac/mac_loading.c, in OpenGL: the progress bar's own
   drawing needs Direct3D textures and immediate draws the Mac's device does not make) is drawn from
   them: at each step's progress, and from the tags made (native_tags.c's pump, at most each
   LOADING_FRAME_MS). Each frame goes through the window (mac_device_gpu_frame), which handles the
   window's events: the window is not left unanswering while a map loads. */
extern void (*mac_loading_pump_hook)(void);
extern void (*mac_loading_progress_hook)(float);
#define LOADING_FRAME_MS 50UL
#ifdef MAC_SESSION_GPU_RENDER
static struct { int active,stopping; float target; unsigned long last; unsigned frames,shots; } probe_loading;

#define probe_loading_milliseconds() ((unsigned long)system_milliseconds())

/* the compressed monk chant of progress_bar.c (its data, a global of the game), four voices at
   other pitches whose volumes swell and fade across the progress, as progress_bar_render sets them */
extern byte compressed_monk_tone[18396];
#define LOADING_VOICES 4
static IDirectSoundBuffer *probe_loading_voices[LOADING_VOICES];

static void probe_loading_chant_start(void)
{
    static const long pitches[LOADING_VOICES]={-4681,-6386,-3999,-8773};
    DSBUFFERDESC description;
    XBOXADPCMWAVEFORMAT format;
    long voice;
    memset(&format,0,sizeof(format));
    format.wfx.wFormatTag=WAVE_FORMAT_XBOX_ADPCM;
    format.wfx.wBitsPerSample=4;
    format.wfx.nChannels=1;
    format.wfx.nBlockAlign=36;
    format.wfx.nSamplesPerSec=22050;
    format.wfx.nAvgBytesPerSec=12384;
    format.wfx.cbSize=2;
    format.wSamplesPerBlock=64;
    memset(&description,0,sizeof(description));
    description.dwSize=sizeof(description);
    description.dwFlags=DSBCAPS_LOCDEFER|DSBCAPS_CTRLPOSITIONNOTIFY;
    description.dwBufferBytes=0;
    description.lpwfxFormat=&format.wfx;
    for(voice=0;voice<LOADING_VOICES;++voice) {
        if(DirectSoundCreateBuffer(&description,&probe_loading_voices[voice])<0) {
            probe_loading_voices[voice]=NULL;
            fprintf(stderr,"loading: no chant (voice %ld could not be made)\n",voice);
            return;
        }
        IDirectSoundBuffer_SetBufferData(probe_loading_voices[voice],compressed_monk_tone,sizeof(compressed_monk_tone));
        IDirectSoundBuffer_SetLoopRegion(probe_loading_voices[voice],0,sizeof(compressed_monk_tone));
        IDirectSoundBuffer_SetCurrentPosition(probe_loading_voices[voice],0);
        IDirectSoundBuffer_SetVolume(probe_loading_voices[voice],-5000);
        IDirectSoundBuffer_SetPitch(probe_loading_voices[voice],pitches[voice]);
        IDirectSoundBuffer_Play(probe_loading_voices[voice],0,0,DSBPLAY_LOOPING);
    }
    fprintf(stderr,"loading: the chant is playing (%d voices)\n",LOADING_VOICES);
}

/* the voices' volumes for the drawn progress (progress_bar_render's) */
static void probe_loading_chant_volume(float progress)
{
    static const float ranges[LOADING_VOICES][2]={{0.0f,1.0f},{0.4f,1.0f},{0.5f,1.0f},{0.55f,1.0f}};
    static const float volumes[LOADING_VOICES]={3500.0f,4500.0f,3500.0f,4500.0f};
    long voice;
    for(voice=0;voice<LOADING_VOICES;++voice) {
        float t=0.0f;
        if(progress>=ranges[voice][0]&&progress<=ranges[voice][1])
            t=(progress-ranges[voice][0])/(ranges[voice][1]-ranges[voice][0]);
        if(probe_loading_voices[voice])
            IDirectSoundBuffer_SetVolume(probe_loading_voices[voice],
                (long)((float)pow(sin(t*3.1415),0.3)*volumes[voice]-5000.0f));
    }
}

static void probe_loading_chant_stop(void)
{
    long voice;
    for(voice=0;voice<LOADING_VOICES;++voice)
        if(probe_loading_voices[voice]) {
            IDirectSoundBuffer_Stop(probe_loading_voices[voice]);
            IDirectSoundBuffer_Release(probe_loading_voices[voice]);
            probe_loading_voices[voice]=NULL;
        }
}

/* HALO_MAC_LOADING_SHOTS=<prefix>[:every] (diagnostic): a loading frame written as a PPM every that many (default 15) */
static int probe_loading_frame(void *opaque,const struct mac_input_frame *input,unsigned width,unsigned height)
{
    const char *shots=getenv("HALO_MAC_LOADING_SHOTS");
    const char *every_text=shots?strchr(shots,':'):NULL;
    unsigned every=every_text&&atoi(every_text+1)>0?(unsigned)atoi(every_text+1):15;
    char path[512];
    int shot=shots&&*shots&&(probe_loading.frames%every==0||probe_loading.stopping);
    (void)opaque;(void)input;
    if(shot) {
        int length=every_text?(int)(every_text-shots):(int)strlen(shots);
        snprintf(path,sizeof(path),"%.*s-%u.ppm",length,shots,++probe_loading.shots);
        fprintf(stderr,"loading: frame %u (progress %.2f) written to %s\n",probe_loading.frames,(double)probe_loading.target,path);
    }
    mac_loading_draw(width,height,probe_loading.target,shot?path:NULL);
    probe_loading_chant_volume(mac_loading_progress());
    if(probe_loading.stopping)mac_loading_stop();
    ++probe_loading.frames;
    return 1;
}

static void probe_loading_draw(void)
{
    probe_loading.last=probe_loading_milliseconds();
    mac_device_gpu_frame(probe_loading_frame,NULL);
}

static void probe_loading_pump(void)
{
    if(probe_loading.active&&probe_loading_milliseconds()-probe_loading.last>=LOADING_FRAME_MS)
        probe_loading_draw();
}

static void probe_loading_progress(float fraction)
{
    if(!probe_loading.active)return;
    if(fraction>probe_loading.target)probe_loading.target=fraction;
    probe_loading_draw();
}

/* the loading screen up: a game chosen, or the way back to the menu. Its first picture is the
   last frame of the menu (the readback the run's last frame asked for) */
static void probe_loading_begin(void)
{
    const struct mac_device_state *device=mac_device_state_get();
    const unsigned *pixels=NULL;
    unsigned long pitch=0;
    if(probe_loading.active||!device||!device->gpu)return;
    {
        unsigned shots=probe_loading.shots;
        memset(&probe_loading,0,sizeof(probe_loading));
        probe_loading.shots=shots;
    }
    if(device->back_buffer.Data) {
        pixels=(const unsigned *)PLATFORM_PHYSICAL_TO_VIRTUAL(device->back_buffer.Data);
        pitch=(device->gpu_width*4+D3DTEXTURE_PITCH_ALIGNMENT-1)&~(unsigned long)(D3DTEXTURE_PITCH_ALIGNMENT-1);
    }
    mac_loading_start(pixels,(unsigned)device->gpu_width,(unsigned)device->gpu_height,(unsigned)(pitch/4));
    probe_loading.active=1;
    probe_loading.target=0.02f;
    probe_loading_chant_start();
    mac_loading_pump_hook=probe_loading_pump;
    mac_loading_progress_hook=probe_loading_progress;
    mac_xbox_map_pump=probe_loading_pump;
    fprintf(stderr,"loading: screen up for %s\n",probe_menu_phase?"the menu":mac_engine_menu_pending_map());
    probe_loading_draw();
}

/* the map is loaded and started: one last frame at the end of the progress, then the screen goes */
static void probe_loading_end(void)
{
    if(!probe_loading.active)return;
    probe_loading.target=1.0f;
    probe_loading.stopping=1;
    probe_loading_draw();
    mac_loading_pump_hook=NULL;
    mac_loading_progress_hook=NULL;
    mac_xbox_map_pump=NULL;
    probe_loading_chant_stop();
    probe_loading.active=0;
    fprintf(stderr,"loading: screen down after %u frames\n",probe_loading.frames);
}
#else
#define probe_loading_begin()
#define probe_loading_end()
#endif

/* One map's whole lifecycle on the session that owns its scenario: the game
   engine chosen, the map loaded and started, the player ticked and the game
   drawn, then ended and unloaded (a second time for the second cycle). The
   native v7 session and an Xbox map's engine-loaded one both run it.
   0: the session's checks stop; 1: the session is done; 2: go on. */
static int run_map_lifecycle(const char *scenario_name,unsigned cycle)
{
    const char *map_error=NULL;
    int loaded,initialized;
    CHECK(scenario_name!=NULL && *scenario_name);
    if(scenario_name && *scenario_name) {
        game_connection_set(_game_connection_local);
        /* The selected engine persists across map unloads, as in main's
           repeated-map path. Only initial startup selects the variant. */
        if(probe_menu_mode) {
            /* (a campaign level has no game engine, a multiplayer map Slayer's;
               the one the last session left is let go when the kind changes) */
            if(probe_campaign||probe_title) {
                if(game_engine)game_engine_dispose();
                CHECK(mac_engine_game_variant_select_campaign(&map_error));
            } else {
                /* (port: the gametype the menus chose; the engine the last game left is let go) */
                if(game_engine)game_engine_dispose();
                CHECK(mac_engine_game_variant_select_multiplayer(&map_error));
            }
        } else if(probe_campaign) {
            if(cycle==1)
                CHECK(mac_engine_game_variant_select_campaign(&map_error));
            else
                CHECK(!game_engine);
        } else if(cycle==1)
            CHECK(mac_engine_game_variant_select_slayer(&map_error));
        else
            CHECK(game_engine && game_engine->type==game_engine_slayer);
        if(map_error)fprintf(stderr,"variant selection: %s\n",map_error);
        if(failures)return 0;
        fprintf(stderr,"stage: game_load native map %s\n",scenario_name);
        if(mac_loading_progress_hook)mac_loading_progress_hook(0.94f);
        loaded=mac_game_load_native_map(scenario_name);
        /* port: main_setup_connection's options.difficulty = global_difficulty_level (the level won, or
           the saved game's, which the menus set with main_set_difficulty) */
        if(loaded&&probe_menu_mode)game_difficulty_level_set(main_get_difficulty());
        if(probe_menu_mode&&!loaded) {
            fprintf(stderr,"menus: %s cannot be loaded; back to the menu\n",scenario_name);
            probe_refuse("it cannot be loaded");game_unload();
            return 1;
        }
        CHECK(loaded);
        if(!loaded)return 0;
        CHECK(game_map_loaded());
        CHECK(global_game_globals!=NULL);
        CHECK(scenario_globals->structure_bsp_index==global_structure_bsp_index);
        fprintf(stderr,"stage: game_initialize_for_new_map\n");
        if(mac_loading_progress_hook)mac_loading_progress_hook(0.97f);
        initialized=mac_engine_game_variant_initialize_map(&map_error);
        if(probe_menu_mode&&!initialized) {
            fprintf(stderr,"menus: %s cannot be started (%s); back to the menu\n",scenario_name,
                map_error?map_error:"initialization failed");
            probe_refuse(map_error?map_error:"it cannot be started");game_dispose_from_old_map();game_unload();
            return 1;
        }
        CHECK(initialized);
        if(map_error)fprintf(stderr,"map initialization: %s\n",map_error);
        if(initialized) {
            fprintf(stderr,"stage: local player and full game ticks\n");
            run_player_ticks();
            probe_loading_end();
#ifdef MAC_SESSION_GPU_RENDER
            if(mac_gpu_render_enabled&&(cycle==1||probe_menu_mode))
                run_original_game_render(cycle);
            /* (HALO_MAC_MENU=1: the menus go before the map does) */
            mac_engine_menu_stop();
#endif
            /* port: the gametype that ran and its goal objects */
            if(probe_menu_mode&&game_engine)mac_engine_game_variant_report();
            CHECK(game_in_progress());
            fprintf(stderr,"stage: game_dispose_from_old_map\n");
            game_dispose_from_old_map();
        }
        /* Exercise authentic game.c unload while the session still owns
           the converted scenario, tags, and BSP it is releasing. */
        game_unload();
        CHECK(!game_map_loaded());
        CHECK(global_game_globals==NULL);
        CHECK(scenario_globals->structure_bsp_index==NONE);
    }
    return 2;
}

static int run_session_scope(void *opaque)
{
    struct scope_context *context=(struct scope_context *)opaque;
    struct mac_engine_session *session=context->session;
    const struct mac_retail_cache *cache=mac_engine_session_cache(session);
    struct mac_engine_world_info info;
    unsigned char header[16];
    boolean completed=FALSE;
    uint32_t cursor=0,first_tag;
    uint32_t item_collections=0,equipment=0,environments=0,shaders=0;

    CHECK(cache!=NULL);
    fprintf(stderr,"stage: bound session scope cycle %u\n",context->cycle);
    CHECK(mac_engine_world_info(&session->world,&info));
    CHECK(info.clusters>0);
    CHECK(mac_native_tags_owned_bytes(&session->assets.tags)>0);
    if(context->cycle==1&&getenv("HALO_MAC_TAG_REPORT"))
        mac_native_tags_report_unconverted(&session->assets.tags,stderr);
    CHECK(mac_texture_lifecycle_bound_probe());
    CHECK(mac_tag_compat_probe(session));
    /* port: the title map has no player biped or dialogue */
    if(!session->main_menu) {
        check_collision_effect_dependencies(session);
        check_dialogue_dependencies(session);
    }
    CHECK(cache_files_precache_map_loaded(cache->name));
    CHECK(!cache_files_precache_in_progress());
    CHECK(!cache_files_precache_is_copying_map(cache->name));
    CHECK(cache_files_precache_map_begin(cache->name,FALSE));
    CHECK(!cache_files_precache_map_loaded("not-the-open-map"));
    /* (with the menus, a map they name is taken: their game opens it) */
    CHECK(mac_engine_menu_enabled() || !cache_files_precache_map_begin("not-the-open-map",FALSE));
    first_tag=mac_native_tags_next(&session->assets.tags,UINT32_MAX,&cursor);
    CHECK(first_tag!=UINT32_MAX);
    cursor=0;
    while(mac_native_tags_next(&session->assets.tags,UINT32_C(0x69746d63),&cursor)!=UINT32_MAX)
        ++item_collections;
    cursor=0;
    while(mac_native_tags_next(&session->assets.tags,UINT32_C(0x65716970),&cursor)!=UINT32_MAX)
        ++equipment;
    cursor=0;
    while(mac_native_tags_next(&session->assets.tags,UINT32_C(0x73656e76),&cursor)!=UINT32_MAX)
        ++environments;
    cursor=0;
    while(mac_native_tags_next(&session->assets.tags,UINT32_C(0x73686472),&cursor)!=UINT32_MAX)
        ++shaders;
    CHECK(shaders>=environments && shaders>0);
    /* (Blood Gulch has 14 of each; other maps their own, some none) */
    CHECK(item_collections<=4096);
    CHECK(equipment<=4096);
    CHECK(environments>0 || session->main_menu);
    {
        static const uint32_t families[]={MAC_SCENARIO_SCENERY,
            MAC_SCENARIO_SOUND_SCENERY,MAC_SCENARIO_VEHICLE};
        static const uint32_t groups[]={UINT32_C(0x7363656e),
            UINT32_C(0x73736365),UINT32_C(0x76656869)};
        uint32_t f,i,count,handle;
        for(f=0;f<3;++f) {
            CHECK(mac_engine_world_object_palette_count(&session->world,families[f],&count));
            for(i=0;i<count;++i) {
                CHECK(mac_engine_world_object_palette_handle(&session->world,families[f],i,&handle));
                if(handle!=UINT32_MAX)CHECK(mac_native_tags_group(&session->assets.tags,handle)==groups[f]);
            }
        }
        CHECK(!mac_engine_world_object_palette_count(&session->world,MAC_SCENARIO_FAMILY_COUNT,&count));
        CHECK(count==0);
    }

    {
        /* Exercise a complete map lifecycle on each independently owned
           session, including reinitialization after the first unload. */
        int lifecycle=run_map_lifecycle(tag_get_name(global_scenario_index),context->cycle);
        if(lifecycle!=2)return lifecycle;
    }

    /* Use the actual engine-facing synchronous reader while the v7 session
       has bound its validated source. The file header is real game data. */
    CHECK(cache_file_read(NONE,0,sizeof(header),header,&completed,TRUE)==0);
    CHECK(completed==TRUE);
    CHECK(memcmp(header,cache->file,sizeof(header))==0);
    fprintf(stderr,"session cycle %u: item_collections=%u equipment=%u environments=%u shaders=%u bsp=%u spawn=%lu clusters=%u bitmaps=%u\n",
        context->cycle,item_collections,equipment,environments,shaders,session->bsp_index,
        (unsigned long)session->spawn_index,info.clusters,session->assets.bitmap_count);
    return failures==0;
}

/* The map file of the game the menu chose: its scenario path's last part
   (levels\\test\\bloodgulch\\bloodgulch) beside the menu's map. A campaign level
   is one of the retail single-player scenarios. 0 if the file is not there. */
static int probe_choose_map(const char *menu_map,char *out,size_t size)
{
    static const char *const solo[]={"a10","a30","a50","b30","b40","c10","c20","c40","d20","d40"};
    const char *name=mac_engine_menu_pending_map(),*last=name;
    char base[64];
    const char *slash;
    size_t length,i,directory;
    FILE *file;
    for(;*name;++name)if(*name=='\\'||*name=='/')last=name+1;
    length=strlen(last);
    if(!length||length>=sizeof(base))return 0;
    for(i=0;i<length;++i)base[i]=(char)((last[i]>='A'&&last[i]<='Z')?last[i]+32:last[i]);
    base[length]=0;
    slash=strrchr(menu_map,'/');
    directory=slash?(size_t)(slash-menu_map)+1:0;
    if(directory+length+5>size)return 0;
    memcpy(out,menu_map,directory);
    memcpy(out+directory,base,length);
    strcpy(out+directory+length,".map");
    probe_campaign=0;
    for(i=0;i<sizeof(solo)/sizeof(solo[0]);++i)if(!strcmp(base,solo[i]))probe_campaign=1;
    file=fopen(out,"rb");
    if(!file)return 0;
    fclose(file);
    return 1;
}

/* a game the menu could not start: said in the log, and the menu again (this
   build has no widgets of the map's to show the Xbox's error dialogs) */
static void probe_refuse(const char *what)
{
    fprintf(stderr,"menus: %s cannot be started on this Mac (%s)\n",mac_engine_menu_pending_map(),what);
    probe_refused=1;
    /* port: and a network game that was to begin there does not */
    mac_engine_network_begin_failed();
}

/* port: a session on an Xbox (version 5) map: the map is inflated and bound as
   the cache the engine reads (cache_file_read), and the engine's own loader
   (scenario_load: the tags read in place at the console's tag cache address,
   their buffers registered, the first bsp read after them) does the rest,
   as it does on the console and in the Linux port. */
static int probe_run_xbox_session(const char *map,unsigned cycle)
{
    struct mac_cache_files files;
    const char *error=NULL;
    int result;
    memset(&files,0,sizeof(files));
    fprintf(stderr,"stage: xbox session_open cycle %u\n",cycle+1);
    probe_map_is_blood_gulch=strstr(map,"bloodgulch")!=NULL;
    if(mac_loading_progress_hook)mac_loading_progress_hook(0.05f);
    if(!mac_cache_files_open(map,NULL,NULL,&files,&error)) {
        fprintf(stderr,"session open cycle %u: %s\n",cycle+1,error?error:"unknown error");
        if(probe_menu_mode&&!probe_menu_phase) {
            fprintf(stderr,"menus: %s cannot be opened; back to the menu\n",map);
            probe_refuse(error?error:"it cannot be opened");
            return 1;
        }
        ++failures;return 0;
    }
    fprintf(stderr,"xbox map %s (%s): %lu tags, %lu bsps, tag data %lu bytes, %lu vertex buffers\n",
        files.xbox_info.name,files.xbox_info.build,(unsigned long)files.xbox_info.tag_count,
        (unsigned long)files.xbox_info.bsp_count,(unsigned long)files.xbox_info.tag_data_size,
        (unsigned long)files.xbox_info.vertex_buffers);
    if(mac_loading_progress_hook)mac_loading_progress_hook(0.5f);
    probe_title=files.xbox_info.scenario_type==2;
    CHECK(mac_cache_files_bind(&files));
    CHECK(mac_engine_precache_bind(&files.retail));
    result=run_map_lifecycle(files.xbox_info.name,cycle+1);
    CHECK(mac_engine_precache_unbind(&files.retail));
    CHECK(mac_cache_files_unbind(&files));
    mac_cache_files_close(&files);
    if(failures)return 0;
    return result!=0;
}

/* One session: open the map's, bind it, run the scope (the map's whole
   lifecycle, the menu or the game on it), close it. 0 stops the process's run. */
static int probe_run_session(const char *map,const char *bitmaps,const char *sounds,unsigned cycle)
{
    struct mac_engine_session session;
    struct scope_context context;
    struct mac_retail_spawn spawn;
    struct mac_engine_location location;
    float point[3],facing;
    const char *error=NULL;
    /* port: the map this session is on, which a network game's begin is matched to */
    mac_engine_network_session_open(map);
    if(mac_xbox_mode())return probe_run_xbox_session(map,cycle);
    memset(&session,0,sizeof(session));
    context.session=&session;context.cycle=cycle+1;
    fprintf(stderr,"stage: session_open cycle %u\n",cycle+1);
    probe_map_is_blood_gulch=strstr(map,"bloodgulch")!=NULL;
    /* HALO_MAC_SPAWN_INDEX: another start point, to look at other parts
       of a map (diagnostic; the checks are written for start 0) */
    if(!mac_engine_session_open(map,bitmaps,sounds,
        getenv("HALO_MAC_SPAWN_INDEX")?(uint32_t)atoi(getenv("HALO_MAC_SPAWN_INDEX")):0,&session,&error)) {
        fprintf(stderr,"session open cycle %u: %s\n",cycle+1,
            error?error:"unknown error");
        if(probe_menu_mode&&!probe_menu_phase) {
            /* (a map this port cannot open: said, and the menu again) */
            fprintf(stderr,"menus: %s cannot be opened; back to the menu\n",map);
            probe_refuse(error?error:"it cannot be opened");
            return 1;
        }
        ++failures;return 0;
    }
    fprintf(stderr,"session assets: %lu bytes, %lu bitmap owners\n",
        (unsigned long)mac_native_tags_owned_bytes(&session.assets.tags),
        (unsigned long)session.assets.bitmap_count);
    probe_title=session.main_menu;
    CHECK(mac_engine_session_bind(&session));
    CHECK(mac_engine_session_spawn(&session,&spawn));
    CHECK(spawn.bsp==-1 || spawn.bsp==(int16_t)session.bsp_index || probe_campaign);
    if(probe_title)memset(&location,0,sizeof(location));
    else CHECK(mac_engine_world_spawn(&session.world,session.spawn_index,
        point,&facing,&location));
    /* port: a campaign start can lie outside its BSP (b40's and c40's); the
       level's scripts place the player, so only multiplayer requires a cluster */
    if(probe_campaign)
        fprintf(stderr,"campaign start: leaf %ld cluster %d\n",(long)location.leaf,(int)location.cluster);
    else if(!probe_title)CHECK(location.cluster>=0);
    CHECK(mac_engine_session_scope(&session,run_session_scope,&context));
    CHECK(mac_engine_session_unbind(&session));
    mac_engine_session_close(&session);
    CHECK(!session.files.map.data&&!session.assets.tags.state&&
        !session.world.state&&!session.bound);
    if(failures)return 0;
    return 1;
}

int main(int argc,char **argv)
{
    unsigned cycle;
    int physical_started=0,physical_allocated=0,state_started=0;
    int device_started=0,game_started=0,math_started=0,input_started=0,sound_started=0;
    int textures_started=0,rasterizer_started=0;
#ifdef MAC_SESSION_GPU_RENDER
    /* A double click (Finder adds a -psn_ argument before 10.6) or no
       arguments at all: play the map of mac.map from the data root. */
    static char launch_maps[3][1100];
    static char *launch_argv[6];
    mac_sound_convert_install(); /* Ogg and uncompressed sounds play as Xbox ADPCM */
    mac_hud_hires_install(); /* the high-res HUD (display.high_res_hud), drawn in place of the Xbox maps' */
    /* port: an Xbox map (the maps of the user's own disc image) is one file:
       "map.map [mode]" stands for "map.map - - mode" */
    {
        static char *xbox_argv[6];
        if(argc>=2&&strncmp(argv[1],"-psn_",5)&&mac_xbox_map_path_version(argv[1])) {
            mac_xbox_mode_set(1);
            if(argc<=3) {
                xbox_argv[0]=argv[0];xbox_argv[1]=argv[1];
                xbox_argv[2]=(char *)"-";xbox_argv[3]=(char *)"-";
                xbox_argv[4]=argc==3?argv[2]:NULL;
                argc=argc==3?5:4;argv=xbox_argv;
            }
        }
    }
    if(argc==2&&!strncmp(argv[1],"-psn_",5))argc=1;
    if(argc==1) {
        char root[1024];
        const char *map=config_string("mac.map"),*names[3];
        unsigned index;
        if(!map||!*map)map="bloodgulch";
        names[0]=map;names[1]="bitmaps";names[2]="sounds";
        if(!mac_engine_files_data_root(root,sizeof(root))) {
            fprintf(stderr,"No data root: set paths.data in config.toml to the folder holding Maps\n");
            return 2;
        }
        /* port: the maps of the user's own disc image: <data root>/maps/<map>.map, one file */
        {
            char xbox_path[1100];
            size_t length=strlen(map);
            if(length>=4&&!strcmp(map+length-4,".map"))length-=4;
            snprintf(xbox_path,sizeof(xbox_path),"%s/maps/%.*s.map",root,(int)length,map);
            if(mac_xbox_map_path_version(xbox_path)) {
                mac_xbox_mode_set(1);
                snprintf(launch_maps[0],sizeof(launch_maps[0]),"%s",xbox_path);
                launch_argv[0]=argv[0];launch_argv[1]=launch_maps[0];
                launch_argv[2]=(char *)"-";launch_argv[3]=(char *)"-";
                launch_argv[4]=(char *)(config_boolean("debug.hidden_window")?"--gpu-play-check":"--gpu-play");launch_argv[5]=NULL;
                argv=launch_argv;argc=5;
                goto launch_resolved;
            }
        }
        for(index=0;index<3;++index) {
            struct stat st;
            size_t length=strlen(names[index]);
            if(length>=4&&!strcmp(names[index]+length-4,".map"))length-=4;
            snprintf(launch_maps[index],sizeof(launch_maps[index]),"%s/Maps/%.*s.map",root,(int)length,names[index]);
            if(stat(launch_maps[index],&st)!=0)
                snprintf(launch_maps[index],sizeof(launch_maps[index]),"%s/maps/%.*s.map",root,(int)length,names[index]);
            if(stat(launch_maps[index],&st)!=0) {
                fprintf(stderr,"Missing Halo map: %s (paths.data in config.toml is the folder holding Maps; mac.map is the map)\n",
                    launch_maps[index]);
                return 2;
            }
        }
        launch_argv[0]=argv[0];launch_argv[1]=launch_maps[0];launch_argv[2]=launch_maps[1];
        launch_argv[3]=launch_maps[2];/* (debug.hidden_window: the hidden play check) */
        launch_argv[4]=(char *)(config_boolean("debug.hidden_window")?"--gpu-play-check":"--gpu-play");launch_argv[5]=NULL;
        argv=launch_argv;argc=5;
    launch_resolved:;
    }
    {
        /* the map's name, for what the settings' "auto" picks */
        const char *path=argc>1?argv[1]:"",*slash=strrchr(path,'/');
        mac_gpu_info_set_map(slash?slash+1:path);
    }
    if(argc==5&&strcmp(argv[4],"--gpu-render")==0)mac_gpu_render_enabled=1;
    if(argc==5&&strcmp(argv[4],"--gpu-interactive")==0) {
        mac_gpu_render_enabled=1;mac_gpu_visible=1;mac_render_frames=0;
    }
    if(argc==5 && (strcmp(argv[4],"--gpu-play")==0 ||
        strcmp(argv[4],"--gpu-play-check")==0)) {
        mac_player_play_mode=1;mac_gpu_render_enabled=1;
        mac_gpu_visible=strcmp(argv[4],"--gpu-play")==0;
        mac_render_frames=mac_gpu_visible?0:30;
    }
    {
        const char *seed_value=getenv("HALO_MAC_RENDER_SEED");
        if(seed_value) {
            if(!mac_gpu_render_enabled||mac_gpu_visible||
                !mac_render_seed_parse(seed_value,&mac_render_seed)) {
                fprintf(stderr,
                    "HALO_MAC_RENDER_SEED requires a hidden GPU render and a decimal 32-bit seed\n");
                return 2;
            }
            mac_render_seed_enabled=1;
        }
    }
    if(mac_gpu_render_enabled) {
        const char *value=getenv("HALO_MAC_RENDER_FRAMES");
        if(value&&*value) {
            char *end=NULL;unsigned long frames=strtoul(value,&end,10);
            if(!end||*end||frames>1000000UL||!frames) {
                fprintf(stderr,"HALO_MAC_RENDER_FRAMES must be 1..1000000\n");return 2;
            }
            mac_render_frames=(unsigned)frames;
        }
        if(!mac_engine_device_factory_set_visible(mac_gpu_visible))return 2;
    }
    if(argc!=4&&!(argc==5&&mac_gpu_render_enabled)) {
        fprintf(stderr,"usage: %s bloodgulch.map bitmaps.map sounds.map [--gpu-render|--gpu-interactive|--gpu-play|--gpu-play-check]\n",argv[0]);
        return 2;
    }
#else
    if(argc!=4) {
        fprintf(stderr,"usage: %s bloodgulch.map bitmaps.map sounds.map\n",argv[0]);
        return 2;
    }
#endif
    CHECK(mac_full_game_tick!=NULL);
    probe_campaign=getenv("HALO_MAC_CAMPAIGN") && getenv("HALO_MAC_CAMPAIGN")[0]=='1';
#ifdef MAC_SESSION_GPU_RENDER
    if(mac_gpu_info_campaign())probe_campaign=1;
#endif
    CHECK(mac_engine_physical_memory_start());
    if(failures)goto done;
    physical_started=1;
    physical_memory_allocate();
    physical_allocated=1;
    physical_memory_verify();
    CHECK(mac_engine_physical_memory_allocation_count()==4);
    CHECK(physical_memory_get_game_state_base_address()==
        (void *)HALO_PORT_GAME_STATE_BASE_ADDRESS);
    if(failures)goto done;
    game_state_initialize();
    state_started=1;
    real_math_initialize();
    math_started=1;
#ifdef MAC_SESSION_GPU_RENDER
    if(mac_render_seed_enabled) {
        *get_global_local_random_seed_address()=mac_render_seed;
        fprintf(stderr,"native hidden render seed: %lu\n",mac_render_seed);
    }
#endif
    {
        unsigned long seed=UINT32_C(0x6d617468);
        real_vector3d direction;
        CHECK(seed_random_direction3d(&seed,&direction)!=NULL);
        CHECK(direction.i==direction.i&&direction.j==direction.j&&direction.k==direction.k);
    }
    if(failures)goto done;
#ifdef MAC_SESSION_GPU_RENDER
    if(mac_gpu_render_enabled) {
        /* port: the mods, and whether to play those that say they cannot be played together (a box, platform_ask in
           port/mac/platform.c), before the window opens; a hidden run is not asked and plays them */
        if(mac_gpu_visible)port_mods_startup();
        fprintf(stderr,"stage: original rasterizer_initialize (%s)\n",
            mac_gpu_visible?"visible AGL":"hidden CGL");
        CHECK(rasterizer_initialize());
        if(failures)goto done;
        rasterizer_started=1;
        CHECK(global_d3d_device==mac_device_gpu_identity());
        CHECK(mac_text_gl_start(NULL));
        CHECK(mac_gpu_textures_set_borrowed_resolver(resolve_font_atlas,NULL));
        if(failures)goto done;
        /* port: the retail game's start-up movies (game.intro_movies), before
           the map loads; closing the window during them ends the run */
        if(config_boolean("game.intro_movies")) {
            char root[1024];
            if(mac_engine_files_data_root(root,sizeof(root))&&
                !mac_movie_play_intro(root,mac_device_gpu_frame,mac_gpu_visible)) {
                fprintf(stderr,"stage: window closed during the intro movies\n");
                goto done;
            }
        }
    } else
#endif
    {
        CHECK(mac_device_headless_start(640,480));
        if(failures)goto done;
        device_started=1;
        global_d3d_device=mac_device_headless_identity();
        CHECK(global_d3d_device!=NULL);
        /* The bounded default mode retains the real headless texture service. */
        texture_cache_new();
        textures_started=1;
    }
    CHECK(mac_engine_texture_lifecycle_ready());
    CHECK(input_initialize());
    if(failures)goto done;
    input_started=1;
    fprintf(stderr,"stage: sound_initialize\n");
    sound_initialize();
    sound_started=1;
    CHECK(sound_is_active());
    if(failures)goto done;
#ifdef MAC_SESSION_GPU_RENDER
    /* Looping sounds choose their next permutation from the shared local
       random seed when the real-time mixer finishes a buffer, so a seeded
       capture would otherwise depend on audio timing. HALO_MAC_RENDER_SEED_SOUND=1
       keeps sound for seeded timing comparisons, not image comparisons. */
    if(mac_render_seed_enabled&&!mac_env_is_one("HALO_MAC_RENDER_SEED_SOUND")) {
        sound_enable(FALSE);
        fprintf(stderr,"native hidden render seed: sound manager disabled\n");
    } else if(mac_render_seed_enabled)
        fprintf(stderr,"native hidden render seed: sound kept; captures may vary\n");
#endif
    /* Shell startup creates physical/game state before game_initialize; map
       files are opened afterward, as in the native session frontend. */
    fprintf(stderr,"stage: game_initialize\n");
    game_initialize();
    game_started=1;
    /* main_setup_connection disposes startup's old-map state before the
       first main_new_map. In particular, release its allocated script array
       before map initialization borrows the native cache's syntax storage. */
    fprintf(stderr,"stage: dispose startup map state\n");
    game_dispose_from_old_map();
    probe_menu_mode=mac_engine_menu_enabled();
    {
        /* the map each session opens: the menu's background (argv[1]), then
           the games the menu starts, from the same folder */
        char map_path[1024];
        const char *map=argv[1];
        unsigned total=0;
        probe_menu_phase=probe_menu_mode;
        for(cycle=0;probe_menu_mode||cycle<((mac_player_play_mode)?1U:2U);++cycle) {
            int status;
            probe_request=0;probe_refused=0;
            if(probe_menu_mode&&!probe_menu_phase) {
                if(!probe_choose_map(argv[1],map_path,sizeof(map_path))) {
                    fprintf(stderr,"menus: %s is not one of this folder's maps; back to the menu\n",
                        mac_engine_menu_pending_map());
                    probe_refuse("its map is not here");
                    probe_menu_phase=1;map=argv[1];
                } else map=map_path;
            } else {
                map=argv[1];
                if(probe_menu_mode)probe_campaign=0;
            }
            if(probe_menu_mode&&probe_menu_phase)++probe_menu_sessions;
            status=probe_run_session(map,argv[2],argv[3],cycle);
            probe_loading_end();
            ++total;
            if(!status)goto done;
            if(!probe_menu_mode)continue;
#ifdef MAC_SESSION_GPU_RENDER
            /* port: the campaign won (main_roll_credits): the Mac game's ending movie, then the title */
            if(mac_engine_main_credits_pending(1)&&mac_gpu_render_enabled) {
                char root[1024];
                fprintf(stderr,"campaign: the last level is won; the ending\n");
                if(mac_engine_files_data_root(root,sizeof(root))&&
                    !mac_movie_play_named(root,"ending.mov",mac_device_gpu_frame,mac_gpu_visible)) {
                    fprintf(stderr,"stage: window closed during the ending\n");
                    goto done;
                }
                probe_request=MAC_MENU_REQUEST_MENU;
            }
#endif
            /* what ended the run: a game chosen, the way back, or the end */
            if(probe_request==MAC_MENU_REQUEST_START) {
                /* a game chosen at the title, or the next level of the campaign (a level won:
                   main_won_map_private's ui_set_next_level asks for it, with the same difficulty) */
                probe_menu_phase=0;
                probe_loading_begin();
            } else if(probe_refused||probe_request==MAC_MENU_REQUEST_MENU) {
                /* (the menu's own map refused is the end: nothing to go back to) */
                if(probe_refused&&probe_menu_phase) {
                    ++failures;goto done;
                }
                probe_menu_phase=1;
                probe_loading_begin();
            } else break;
        }
        probe_cycles=total;
    }

done:
#ifdef MAC_SESSION_GPU_RENDER
    if(sound_started&&mac_gpu_render_enabled) {
        struct mac_dsound_audio_stats audio;
        memset(&audio,0,sizeof(audio));
        CHECK(mac_dsound_audio_statistics(&audio));
        fprintf(stderr,"audio snapshot: sound=%d output=%d silent_clock=%d callbacks=%llu frames=%llu nonzero_frames=%llu\n",
            audio.sound_started,audio.output_started,audio.silent_clock_started,
            (unsigned long long)audio.silent_callbacks,
            (unsigned long long)audio.silent_frames,
            (unsigned long long)audio.silent_nonzero_frames);
        fprintf(stderr,"audio mix: calls=%llu frames=%llu total_ms=%.1f max_us=%.0f output_callbacks=%llu voice_mixes=%llu zero_gain_voice_mixes=%llu\n",
            (unsigned long long)audio.mix_calls,(unsigned long long)audio.mix_frames,
            (double)audio.mix_nanoseconds/1000000.0,(double)audio.mix_max_nanoseconds/1000.0,
            (unsigned long long)audio.output.callbacks,
            (unsigned long long)audio.voice_mixes,
            (unsigned long long)audio.voice_zero_gain_mixes);
        fprintf(stderr,"audio openal: active=%d reverb=%d sources=%lu fallbacks=%lu buffers_queued=%lu buffer_failures=%lu sends=%lu obstructions=%lu room_type=%ld\n",
            audio.openal_active,audio.openal_reverb,audio.openal_sources,audio.openal_fallbacks,
            audio.openal_buffers_queued,audio.openal_buffer_failures,
            audio.openal_sends_set,audio.openal_obstructions_set,audio.openal_room_type);
    }
#endif
    if(sound_started)sound_dispose();
    if(game_started)game_dispose();
    else if(state_started)game_state_dispose();
    if(input_started)input_dispose();
#ifdef MAC_SESSION_GPU_RENDER
    if(rasterizer_started) {
        CHECK(mac_gpu_textures_set_borrowed_resolver(NULL,NULL));
        mac_text_dispose();
        rasterizer_dispose();
    }
    else
#endif
    if(textures_started)texture_cache_delete();
    if(physical_allocated) {
        physical_memory_free();
        CHECK(mac_engine_physical_memory_allocation_count()==0);
        CHECK(mac_engine_arena_registrations()==0);
    }
    if(physical_started)mac_engine_physical_memory_stop();
    if(math_started)real_math_dispose();
    if(!rasterizer_started)global_d3d_device=NULL;
    if(device_started)mac_device_state_stop();
    printf("native v7 session lifecycle: %u checks, %u failures, %u cycle(s)\n",
        checks,failures,probe_cycles);
    return failures?1:0;
}
