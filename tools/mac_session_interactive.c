#include "mac_render_timing.h"
/* Opt-in native input, game ticks and original rendering for the Mac port. */
#include "cseries.h"
#include "game/cheats.h"
#include "engine_device_state.h"
#include "engine_input.h"
#include "engine_config.h"
#define MAC_INPUT_NO_CARBON
#include "mac_window.h"
#include "mac_gpu_info.h" /* port: mac.adaptive_detail */
#include "mac_flags.h" /* port: mac_perf_flag */
#include "input/input.h"
#include "input/input_abstraction.h"
#include "game/game.h"
#include "game/game_globals.h"
#include "game/players.h"
#include "game/player_control_runtime.h"
#include "camera/director.h"
#include "effects/player_effects.h"
#include "camera/observer.h"
#include "objects/objects.h"
#include "units/bipeds.h"
#include "units/units.h"
#include "units/unit_definitions.h"
#include "items/weapons.h"
#include "scenario/scenario.h"
#include "physics/collision_usage.h"
#include "game/game_engine.h"
#include "main/main.h"
#include "main/console.h" /* port: the developer console, serviced as main.c's loop does */
#include "interface/terminal.h"
#include "networking/telnet_console.h"
#include "saved games/game_state.h"
#include "saved games/player_profile.h"
#include "interface/player_ui.h"
#include "mac_session_interactive.h"
#include "native_globals_layout.h"
#include "engine_menu.h"
#include "native_resource_memory.h"
#include "ai/ai.h"
#include "camera/camera_scripting.h"
#include "cutscene/cinematics.h"
#include "cutscene/recorded_animations.h"
#include "hs/hs_scenario_definitions.h"
#include "hs/hs.h"
#include "ai/actors.h"
#include "ai/encounters.h"
#include "ai/ai_scenario_definitions.h"
#include "scenario/scenario_definitions.h"

#include "mac_input.h"
#include "interface/hud_messaging.h"
#include "mac_dsound_audio_stats.h"
#include "memory/data.h"
#include "hs/hs.h"
#include "networking/network_game_globals.h"
#include "networking/network_client_manager.h"
#include "bungie_net/network/transport.h"
#include "network_voice.h" /* port: port/linux/game/network_voice.c */
#include "map_share.h" /* port: port/linux/game/map_share.c */
/* port: the automated system link test (port/linux/game/network_test.c),
declared as source/main/main.c declares it */
void network_test_update(boolean main_menu_loaded, real seconds);
/* port: main_globals' own, through the generated view of main.c */
int mac_engine_main_menu_loaded(void);
/* port: port/linux/src/port_config.c's */
int config_boolean(const char *name);
#include "engine_network_test.h"
extern struct data_array *hs_thread_data;
#include "hs/hs_scenario_definitions.h"
#include "effects/particles.h"
#include "effects/particle_systems.h"
#include "effects/contrails.h"
#include "shaders/shader_definitions.h"

#include <stdio.h>
#include <mach/mach_time.h>
#include <time.h>
/* (sys/stat.h clashes with the engine's socket prefix) */
extern int mkdir(const char *path, unsigned short mode);
#include <stdlib.h>
#include <string.h>
#define MAC_INTERACTIVE_TICK (1.0 / 30.0)
#define MAC_INTERACTIVE_MAX_ELAPSED 0.1

extern void mac_engine_main_game_render(double time_delta_since_tick_sec);
extern void render_interpolation_frame_begin(void);
extern void render_interpolation_frame_end(void);
extern unsigned long system_milliseconds(void);

struct mac_interactive_context {
    unsigned max_frames;
    unsigned frames;
    unsigned ticks;
    unsigned long tick_milliseconds;
    unsigned quit_requested;
    unsigned request;
    unsigned failed;
    double accumulator;
    unsigned long last_time;
    unsigned trace_enabled;
    unsigned trace_thresholds_printed;
    unsigned fixed_step;
    unsigned sweep;
    unsigned rng_trace;
    int warm_audio_valid;
    struct mac_dsound_audio_stats warm_audio;
};

extern struct data_array *effect_data;

void mac_engine_main_apply_bsp_switch(void); /* port: mac_main_game_render_view.py */
void mac_engine_main_lifecycle_head(void); /* port: mac_main_game_render_view.py */
int mac_engine_main_new_map_finish(void); /* port: mac_main_game_render_view.py */
void mac_engine_main_save_step(void); /* port: mac_main_game_render_view.py */
static unsigned long mac_interactive_array_hash(const struct data_array *array)
{
    const unsigned char *bytes;
    unsigned long hash = 2166136261UL, length, i;

    if (!array || !array->data || array->maximum_count <= 0 || array->size <= 0)
        return 0;
    bytes = (const unsigned char *)array->data;
    length = (unsigned long)array->maximum_count * (unsigned long)array->size;
    for (i = 0; i < length; ++i)
        hash = ((hash ^ bytes[i]) * 16777619UL) & 0xffffffffUL;
    return hash;
}

/* Hidden determinism diagnostic: both random seeds and the effect arrays
   after each phase, so two runs can be compared line by line. */
static void mac_interactive_trace_rng(const struct mac_interactive_context *context,
    const char *phase)
{
    if (!context || !context->rng_trace)
        return;
    fprintf(stderr,
        "native rng trace: frame=%u tick=%u phase=%s local=%08lx global=%08lx "
        "effects=%d/%08lx particles=%d/%08lx systems=%d/%08lx system_particles=%d/%08lx "
        "contrails=%d/%08lx\n",
        context->frames, context->ticks, phase,
        *get_global_local_random_seed_address() & 0xffffffffUL,
        *get_global_random_seed_address() & 0xffffffffUL,
        effect_data ? effect_data->actual_count : -1, mac_interactive_array_hash(effect_data),
        particle_data ? particle_data->actual_count : -1, mac_interactive_array_hash(particle_data),
        particle_systems ? particle_systems->actual_count : -1, mac_interactive_array_hash(particle_systems),
        system_particles ? system_particles->actual_count : -1, mac_interactive_array_hash(system_particles),
        contrail_data ? contrail_data->actual_count : -1, mac_interactive_array_hash(contrail_data));
}

struct player_control *player_control_get(short local_player_index);

/* HALO_MAC_AI_PLACE (diagnostic): the named encounters (comma separated, or
   "all") are placed on the first frame, as a script's ai_place would. The
   campaign's encounters are made by its scripts as the player reaches them,
   which a hidden run does not do. */
static void mac_interactive_ai_place(void)
{
    const char *list = getenv("HALO_MAC_AI_PLACE");
    struct data_iterator iterator;
    struct encounter_datum *encounter;
    struct scenario *scenario = global_scenario_get();
    unsigned placed = 0;
    if (!list || !scenario)
        return;
    data_iterator_new(&iterator, encounter_data);
    while ((encounter = (struct encounter_datum *)data_iterator_next(&iterator)) != NULL) {
        long absolute = DATUM_INDEX_TO_ABSOLUTE_INDEX(iterator.datum_index);
        const char *name = ((struct encounter_definition *)scenario->ai_encounters.address)[absolute].name;
        const char *at = list;
        size_t length = strlen(name);
        int wanted = !strcmp(list, "all");
        while (!wanted && *at) {
            const char *end = strchr(at, ',');
            size_t part = end ? (size_t)(end - at) : strlen(at);
            if (part == length && !strncmp(at, name, length))
                wanted = 1;
            at += part + (end ? 1 : 0);
        }
        if (wanted) {
            encounter_create(iterator.datum_index, NONE, NONE);
            ++placed;
        }
    }
    fprintf(stderr, "campaign ai: placed %u encounters (%s)\n", placed, list);
}

/* HALO_MAC_AI_VIEW (diagnostic): once there are actors, the player stands a
   few units from the first one (HALO_MAC_AI_VIEW=<distance>, facing it), so
   that a hidden run's capture shows it */
static void mac_interactive_ai_view(unsigned frame)
{
    const char *value = getenv("HALO_MAC_AI_VIEW");
    static real_point3d at;
    static int have_target;
    struct data_iterator iterator;
    struct actor_datum *actor;
    long unit_index = player_control_get_unit_index(0);
    real distance = value ? (real)atof(value) : 0.0f;
    if (!value || !actor_data || unit_index == NONE)
        return;
    /* (the level's opening cutscene takes the camera back each tick, and its
       script puts the player into the dropship) */
    cinematic_stop();
    director_script_camera(FALSE);
    if (!have_target) {
        data_iterator_new(&iterator, actor_data);
        while ((actor = (struct actor_datum *)data_iterator_next(&iterator)) != NULL) {
            struct object_datum *unit = actor->meta.unit_index != NONE ? object_try_and_get(actor->meta.unit_index) : NULL;
            if (unit && unit->object.parent_object_index == NONE) {
                at = unit->object.position;
                at.x -= distance;
                at.z += 0.3f;
                have_target = 1;
                fprintf(stderr, "campaign ai: the player moves to (%.2f,%.2f,%.2f), facing +x, %.1f from actor %08lx\n",
                    at.x, at.y, at.z, distance, (unsigned long)iterator.datum_index);
                break;
            }
        }
    }
    if (have_target && getenv("HALO_MAC_AI_GOD")) {
        struct object_datum *player_object = object_get(unit_index);
        player_object->object.body_vitality = 1.0f;
        player_object->object.shield_vitality = 1.0f;
    }
    if (have_target) {
        real_vector3d forward, up;
        forward.i = 1.0f; forward.j = 0.0f; forward.k = 0.0f;
        up.i = 0.0f; up.j = 0.0f; up.k = 1.0f;
        if (object_get(unit_index)->object.parent_object_index != NONE)
            unit_exit_seat_end(unit_index);
        if (frame >= 1) {
            object_set_position(unit_index, &at, &forward, &up);
            player_control_get(0)->desired_angles.yaw = 0.0f;
            player_control_get(0)->desired_angles.pitch = 0.0f;
        }
    }
}

/* HALO_MAC_AI_LOG (diagnostic): the AI's actors every second of game time,
   and the milliseconds the game's ticks took in that second */
static void mac_interactive_ai_report(unsigned tick, unsigned long tick_milliseconds)
{
    struct data_iterator iterator;
    struct actor_datum *actor;
    unsigned total = 0, active = 0, alive = 0, moved = 0, shown = 0, fighting = 0, seen = 0;
    static real_point3d previous[1024];
    static unsigned char known[1024];
    if (!actor_data)
        return;
    data_iterator_new(&iterator, actor_data);
    while ((actor = (struct actor_datum *)data_iterator_next(&iterator)) != NULL) {
        struct object_datum *unit = actor->meta.unit_index != NONE ? object_try_and_get(actor->meta.unit_index) : NULL;
        ++total;
        if (actor->meta.active)
            ++active;
        if (unit && unit->object.body_vitality > 0.0f)
            ++alive;
        if (actor->state.combat_status >= 3)
            ++fighting;
        if (unit && (iterator.datum_index & 0xffff) < 1024) {
            unsigned slot = (unsigned)(iterator.datum_index & 0xffff);
            if (known[slot]) {
                real dx = unit->object.position.x - previous[slot].x, dy = unit->object.position.y - previous[slot].y;
                if (dx * dx + dy * dy > 0.04f)
                    ++moved;
            }
            previous[slot] = unit->object.position;
            known[slot] = 1;
        }
        if (unit && shown < 6 && ((getenv("HALO_MAC_AI_LOG_INBSP") && unit->object.location.cluster_index >= 0) ||
            (!getenv("HALO_MAC_AI_LOG_INBSP") && ++seen > (getenv("HALO_MAC_AI_LOG_SKIP") ? (unsigned)atoi(getenv("HALO_MAC_AI_LOG_SKIP")) : 0u)))) {
            fprintf(stderr, "  actor %08lx cluster %d type %d mode %d action %d combat %d at (%.2f,%.2f,%.2f) vitality %.2f\n",
                (unsigned long)iterator.datum_index, (int)unit->object.location.cluster_index, actor->meta.type, actor->state.mode, actor->state.action,
                actor->state.combat_status, unit->object.position.x, unit->object.position.y,
                unit->object.position.z, unit->object.body_vitality);
            {
                /* (where the bsp loaded now finds the actor's point, and half a unit above it) */
                struct location below, above;
                real_point3d point = unit->object.position;
                scenario_location_from_point(&below, &point);
                point.z += 0.5f;
                scenario_location_from_point(&above, &point);
                fprintf(stderr, "    bsp %d: cluster at the point %d, 0.5 above %d\n", (int)global_structure_bsp_index,
                    (int)below.cluster_index, (int)above.cluster_index);
            }
            ++shown;
        }
    }
    {
        long player_unit = player_control_get_unit_index(0);
        struct object_datum *player_object = player_unit != NONE ? object_try_and_get(player_unit) : NULL;
        if (player_object)
            fprintf(stderr, "  player at (%.2f,%.2f,%.2f) parent %08lx yaw %.2f\n", player_object->object.position.x,
                player_object->object.position.y, player_object->object.position.z,
                (unsigned long)player_object->object.parent_object_index, player_control_get(0)->desired_angles.yaw);
        else
            fprintf(stderr, "  player has no unit\n");
    }
    fprintf(stderr, "campaign ai: tick %u actors=%u active=%u alive=%u moved=%u fighting=%u tick_ms=%lu\n", tick, total,
        active, alive, moved, fighting, tick_milliseconds);
}

/* HALO_MAC_RADAR_LOG (diagnostic): every second of game time, the local
   player's team, position and facing, and for each live unit: its team, the
   blip type the motion sensor gives it (blip_type_get's rule: the game's
   allegiance table), where it is in the world (ahead/right of the player)
   and where the sensor would put it (the point motion_sensor.c's
   render_blip draws, after the sensor's rotation). HALO_MAC_RADAR_FREE=1
   ends the level's opening cutscene (its camera and hidden HUD) at the
   walk tick, so that the HUD is drawn. */
extern boolean debug_motion_sensor_draw_all_units;

static void mac_interactive_radar_report(unsigned tick)
{
    long player_index = local_player_get_player_index(0);
    long player_unit = player_control_get_unit_index(0);
    struct object_datum *player_object = player_unit != NONE ? object_try_and_get(player_unit) : NULL;
    struct data_iterator iterator;
    struct object_datum *object;
    real_euler_angles2d const *facing = player_control_get_facing_angles(0);
    short team, a, b;
    real yaw, fx, fy, rx, ry;
    unsigned shown = 0;

    if (player_index == NONE || !player_object)
        return;
    team = player_get(player_index)->team_index;
    yaw = facing->yaw;
    fx = (real)cos(yaw); fy = (real)sin(yaw);
    rx = fy; ry = -fx; /* (to the right of facing, in a z-up right-handed world) */
    fprintf(stderr, "radar: perspective %d player dead %d\n", (int)director_get_perspective(0),
        (int)((player_object->object.damage_flags & (1 << _object_dead_bit)) != 0));
    fprintf(stderr, "radar: tick %u player team %d at (%.2f,%.2f,%.2f) yaw %.3f forward (%.2f,%.2f) sensor yaw %.3f\n", tick,
        (int)team, player_object->object.position.x, player_object->object.position.y, player_object->object.position.z,
        yaw, fx, fy, yaw + 1.5707964f);
    if (tick % 300 == 0) {
        fprintf(stderr, "radar: enemy matrix (row team, column other; E enemy, . friend)\n");
        for (a = 0; a < 10; ++a) {
            char row[11];
            for (b = 0; b < 10; ++b)
                row[b] = game_team_is_enemy(a, b) ? 'E' : '.';
            row[10] = 0;
            fprintf(stderr, "radar:   %d %s\n", (int)a, row);
        }
    }
    data_iterator_new(&iterator, object_header_data);
    while (data_iterator_next(&iterator) != NULL) {
        if (!unit_try_and_get(iterator.datum_index) || iterator.datum_index == player_unit)
            continue;
        object = object_try_and_get(iterator.datum_index);
        if (!object)
            continue;
        {
            struct object_datum *unit = object;
            real dx = unit->object.position.x - player_object->object.position.x;
            real dy = unit->object.position.y - player_object->object.position.y;
            real dz = unit->object.position.z - player_object->object.position.z;
            real ahead = dx * fx + dy * fy, right = dx * rx + dy * ry;
            real sensor_yaw = yaw + 1.5707964f, sx, sy;
            boolean enemy = game_team_is_enemy(unit->object.owner_team_index, team);
            /* motion_sensor.c's rotate_vector2d(position, sine(-yaw), cosine(-yaw)) */
            sx = dx * (real)cos(-sensor_yaw) - dy * (real)sin(-sensor_yaw);
            sy = dx * (real)sin(-sensor_yaw) + dy * (real)cos(-sensor_yaw);
            if (dx * dx + dy * dy + dz * dz > 900.0f)
                continue;
            if (shown < 24) {
                fprintf(stderr, "radar:   unit %08lx %s team %d -> %s blip, world d=(%.1f,%.1f,%.1f) ahead %.1f right %.1f, sensor (%.1f,%.1f)\n",
                    (unsigned long)iterator.datum_index, tag_get_name(unit->definition_index),
                    (int)unit->object.owner_team_index, enemy ? "enemy(red)" : "friend(yellow)", dx, dy, dz, ahead, right, sx, sy);
                ++shown;
            }
        }
    }
}

static void mac_interactive_trace_tick(struct mac_interactive_context *context)
{
    long unit_index;
    struct biped_datum *biped;
    struct game_globals *globals;
    struct game_globals_falling_damage *falling;

    if (!context || !context->trace_enabled)
        return;
    if (!context->trace_thresholds_printed) {
        globals = scenario_get_game_globals();
        falling = globals && globals->falling_damage.count > 0 ?
            (struct game_globals_falling_damage *)globals->falling_damage.address : NULL;
        if (falling) {
            fprintf(stderr,
                "native session trace falling: lower=%g upper=%g max_distance=%g "
                "max_fall_velocity=%g min_damage_velocity=%g max_damage_velocity=%g "
                "damage=%ld max_distance_damage=%ld\n",
                falling->falling_distance_lower_bound,
                falling->falling_distance_upper_bound,
                falling->maximum_distance,
                falling->runtime_maximum_falling_velocity,
                falling->runtime_minimum_damage_velocity,
                falling->runtime_maximum_damage_velocity,
                falling->falling_damage.index,
                falling->maximum_distance_damage.index);
        } else {
            fprintf(stderr, "native session trace falling: globals or falling block unavailable\n");
        }
        context->trace_thresholds_printed = 1;
    }

    unit_index = player_control_get_unit_index(0);
    biped = unit_index != NONE ? biped_try_and_get(unit_index) : NULL;
    if (!biped) {
        fprintf(stderr, "native session trace tick=%u unit=NONE\n", context->ticks + 1);
        return;
    }
    fprintf(stderr,
        "native session trace tick=%u unit=%08lx pos=(%.9g,%.9g,%.9g) "
        "vel=(%.9g,%.9g,%.9g) support=%ld ground=%u outside=%u cluster=%d "
        "shield=%.9g/%.9g body=%.9g/%.9g damage=0x%04x airborne=%u\n",
        context->ticks + 1, (unsigned long)unit_index,
        biped->object.position.x, biped->object.position.y, biped->object.position.z,
        biped->object.translational_velocity.i,
        biped->object.translational_velocity.j,
        biped->object.translational_velocity.k,
        biped->biped.support_surface_index,
        TEST_FLAG(biped->object.flags, _object_on_ground_bit),
        TEST_FLAG(biped->object.flags, _object_outside_of_map_bit),
        biped->object.location.cluster_index,
        biped->object.shield_vitality, biped->object.maximum_shield_vitality,
        biped->object.body_vitality, biped->object.maximum_body_vitality,
        biped->object.damage_flags, biped->biped.airborne_ticks);
}

/* HALO_MAC_SIM_DIGEST=1 (diagnostic): after every tick, one line with a
   digest of every object's position, velocity, orientation and vitalities,
   the player's, and the random seed, so that two builds (Mac i386, Mac ppc,
   the Linux build's own code) can be compared tick by tick for identical
   floating point results. The digest hashes bit patterns, never text. */
static unsigned long mac_sim_hash, mac_sim_nodes_hash;
static void mac_sim_hash_word(unsigned long value)
{
    int shift;
    for (shift = 0; shift < 32; shift += 8) {
        mac_sim_hash ^= (value >> shift) & 255UL;
        mac_sim_hash = (mac_sim_hash * 16777619UL) & 0xFFFFFFFFUL;
    }
}
static void mac_sim_hash_real(real value)
{
    union { real f; unsigned long w; } bits;
    bits.w = 0;
    bits.f = value;
    mac_sim_hash_word(bits.w);
}
static void mac_sim_hash_vector(real a, real b, real c)
{
    mac_sim_hash_real(a); mac_sim_hash_real(b); mac_sim_hash_real(c);
}
static void mac_interactive_sim_digest(const struct mac_interactive_context *context)
{
    struct object_iterator objects;
    struct object_datum *object;
    unsigned long count = 0, player_hash = 0;
    long player_unit = player_control_get_unit_index(0);

    if (!getenv("HALO_MAC_SIM_DIGEST"))
        return;
    mac_sim_hash = 2166136261UL;
    mac_sim_nodes_hash = 2166136261UL;
    object_iterator_new(&objects, 0xFFFFFFFFUL, 0);
    while ((object = (struct object_datum *)object_iterator_next(&objects)) != NULL) {
        unsigned long before = mac_sim_hash;
        mac_sim_hash_word((unsigned long)objects.index);
        mac_sim_hash_vector(object->object.position.x, object->object.position.y, object->object.position.z);
        mac_sim_hash_vector(object->object.translational_velocity.i, object->object.translational_velocity.j, object->object.translational_velocity.k);
        mac_sim_hash_vector(object->object.forward.i, object->object.forward.j, object->object.forward.k);
        mac_sim_hash_vector(object->object.up.i, object->object.up.j, object->object.up.k);
        mac_sim_hash_vector(object->object.angular_velocity.i, object->object.angular_velocity.j, object->object.angular_velocity.k);
        mac_sim_hash_real(object->object.body_vitality);
        mac_sim_hash_real(object->object.shield_vitality);
        /* port: HALO_MAC_SIM_DIGEST=2 also hashes the node matrices and the bounding sphere apart (nodes=), so that a change to
           how they are computed shows */
        if (getenv("HALO_MAC_SIM_DIGEST")[0] == '2') {
            const real *matrices = (const real *)object_header_block_get(objects.index, &object->object.node_matrices);
            unsigned long words = matrices ? (unsigned long)object->object.node_matrices.size / sizeof(real) : 0, word;
            unsigned long saved = mac_sim_hash;
            mac_sim_hash = mac_sim_nodes_hash;
            for (word = 0; word < words; ++word)
                mac_sim_hash_real(matrices[word]);
            mac_sim_hash_vector(object->object.bounding_sphere_center.x, object->object.bounding_sphere_center.y, object->object.bounding_sphere_center.z);
            mac_sim_hash_real(object->object.bounding_sphere_radius);
            mac_sim_nodes_hash = mac_sim_hash;
            mac_sim_hash = saved;
        }
        if ((long)objects.index == player_unit)
            player_hash = mac_sim_hash ^ before;
        ++count;
    }
    mac_sim_hash_word(*get_global_local_random_seed_address());
    fprintf(stderr, "sim digest tick=%u objects=%lu player=%08lx all=%08lx seed=%08lx nodes=%08lx\n",
        context->ticks + 1, count, player_hash, mac_sim_hash, *get_global_local_random_seed_address(), mac_sim_nodes_hash);
}

/* HALO_MAC_SIM_INPUT=1 (diagnostic): input scripted from the tick number
   alone (exact float arithmetic), so that runs are comparable */
static void mac_interactive_sim_input(unsigned ticks, struct mac_input_frame *input)
{
    unsigned phase = ticks % 120;
    if (!getenv("HALO_MAC_SIM_INPUT"))
        return;
    input->forward = 1.0f;
    input->left = (ticks / 60) % 2 ? 0.5f : -0.5f;
    input->yaw = (real)(phase < 60 ? phase : 120 - phase) * 0.0078125f - 0.2f;
    input->pitch = 0.0f;
    if (ticks % 45 == 20)
        input->held |= MAC_INPUT_JUMP;
    if (ticks >= 30)
        input->held |= MAC_INPUT_FIRE, input->mouse_buttons |= 1U;
}

static unsigned long mac_interactive_now(void)
{
    return system_milliseconds();
}

/* port: the checkpoint and death lifecycle (mac_main_game_render_view.py):
   the time a step took, in milliseconds, when it did something (a save or a
   revert), and HALO_MAC_KILL_PLAYER */
static double mac_interactive_microseconds(void)
{
    static mach_timebase_info_data_t base;
    if (!base.denom)
        mach_timebase_info(&base);
    return (double)mach_absolute_time() * (double)base.numer / (double)base.denom / 1000.0;
}

/* port: HALO_MAC_PHASE_TIMING=<n> (diagnostic): every n frames, and at the end of a run, a line with the milliseconds a
   frame went to in the game's tick, in drawing the world, in presenting it and in the rest (the window's frame, the driver's
   waits). It splits the frame time of a slow Mac into its CPU phases, without the sampling tools it does not have. */
static unsigned mac_phase_window;
static int mac_phase_known;
static double mac_phase_tick, mac_phase_render, mac_phase_present, mac_phase_total;
static double mac_phase_run_tick, mac_phase_run_render, mac_phase_run_present, mac_phase_run_total;
static unsigned mac_phase_frames, mac_phase_run_frames;

static int mac_phase_enabled(void)
{
    if (!mac_phase_known) {
        const char *value = getenv("HALO_MAC_PHASE_TIMING");
        mac_phase_window = value ? (unsigned)atoi(value) : 0;
        mac_phase_known = 1;
    }
    return mac_phase_window != 0;
}

static void mac_phase_print(const char *label, unsigned frames, double tick, double render, double present, double total)
{
    if (frames)
        fprintf(stderr, "phase timing %s (%u frames): tick %.2f, render %.2f, present %.2f, other %.2f, total %.2f ms a frame\n",
            label, frames, tick / frames, render / frames, present / frames,
            (total - tick - render - present) / frames, total / frames);
}

/* (with HALO_MAC_RENDER_TIMING=1: the draws of the frames and what the renderer's buckets took, as they grew) */
static void mac_phase_buckets(unsigned frames, const char *label)
{
    static mac_render_timing_stamp last_us[mac_render_timing_bucket_count];
    static unsigned long last_calls[mac_render_timing_bucket_count];
    mac_render_timing_stamp us[mac_render_timing_bucket_count];
    unsigned long calls[mac_render_timing_bucket_count];
    int i;

    if (!mac_render_timing_enabled() || !frames)
        return;
    for (i = 0; i < mac_render_timing_bucket_count; ++i)
        mac_render_timing_get((enum mac_render_timing_bucket)i, &us[i], &calls[i]);
    fprintf(stderr, "phase timing %s: draws %.1f, draw call %.2f, texture bind %.2f, decode %.2f, shader %.2f, program %.2f, state %.2f ms a frame\n",
        label, (double)(calls[mac_render_timing_draw_call] - last_calls[mac_render_timing_draw_call]) / frames,
        (double)(us[mac_render_timing_draw_call] - last_us[mac_render_timing_draw_call]) / 1000.0 / frames,
        (double)(us[mac_render_timing_texture_bind] - last_us[mac_render_timing_texture_bind]) / 1000.0 / frames,
        (double)(us[mac_render_timing_stream_decode] - last_us[mac_render_timing_stream_decode]) / 1000.0 / frames,
        (double)(us[mac_render_timing_shader_sources] - last_us[mac_render_timing_shader_sources]) / 1000.0 / frames,
        (double)(us[mac_render_timing_program_lookup] - last_us[mac_render_timing_program_lookup]) / 1000.0 / frames,
        (double)(us[mac_render_timing_draw_state_setup] - last_us[mac_render_timing_draw_state_setup]) / 1000.0 / frames);
    for (i = 0; i < mac_render_timing_bucket_count; ++i) {
        last_us[i] = us[i];
        last_calls[i] = calls[i];
    }
}

static void mac_phase_frame(unsigned frame, double tick, double render, double present, double total)
{
    if (frame < 2)
        return;
    mac_phase_tick += tick;
    mac_phase_render += render;
    mac_phase_present += present;
    mac_phase_total += total;
    ++mac_phase_frames;
    mac_phase_run_tick += tick;
    mac_phase_run_render += render;
    mac_phase_run_present += present;
    mac_phase_run_total += total;
    ++mac_phase_run_frames;
    if (mac_phase_frames >= mac_phase_window) {
        char label[32];
        sprintf(label, "frames %u to %u", frame - mac_phase_frames + 1, frame);
        mac_phase_print(label, mac_phase_frames, mac_phase_tick, mac_phase_render, mac_phase_present, mac_phase_total);
        mac_phase_buckets(mac_phase_frames, label);
        mac_phase_tick = mac_phase_render = mac_phase_present = mac_phase_total = 0.0;
        mac_phase_frames = 0;
    }
}

static int mac_interactive_checkpoints_enabled(void)
{
    const char *value = getenv("HALO_MAC_CHECKPOINTS");
    return !(value && value[0] == '0');
}

static void mac_interactive_lifecycle(struct mac_interactive_context *context, int after_tick)
{
    double started = mac_interactive_microseconds(), elapsed;
    short before_saved = game_state_port_saved_game_valid();
    long time_before = game_time_get();
    boolean saving = main_saving_map();
    if (after_tick)
        mac_engine_main_save_step();
    else
        mac_engine_main_lifecycle_head();
    elapsed = (mac_interactive_microseconds() - started) / 1000.0;
    if (elapsed >= 2.0 || game_time_get() != time_before || (saving && !main_saving_map())) {
        long unit_index = player_control_get_unit_index(0);
        struct object_datum *object = unit_index != NONE ? object_try_and_get(unit_index) : NULL;
        fprintf(stderr, "checkpoint: tick %u game time %ld -> %ld %s %.2f ms (saved state %d -> %d) bsp %d player %s at (%.2f,%.2f,%.2f) health %.2f shield %.2f\n", context->ticks,
            time_before, (long)game_time_get(), after_tick ? "save step" : "lifecycle", elapsed,
            (int)before_saved, (int)game_state_port_saved_game_valid(), (int)global_structure_bsp_index,
            object ? "alive" : "none",
            object ? object->object.position.x : 0.0f, object ? object->object.position.y : 0.0f,
            object ? object->object.position.z : 0.0f, object ? object->object.body_vitality : 0.0f,
            object ? object->object.shield_vitality : 0.0f);
        if (object) {
            struct location found;
            scenario_location_from_point(&found, &object->object.position);
            fprintf(stderr, "checkpoint:   object location cluster %d leaf %ld, the point's lookup cluster %d leaf %ld, structure bsp %p\n",
                (int)object->object.location.cluster_index, (long)object->object.location.leaf_index,
                (int)found.cluster_index, (long)found.leaf_index, (void *)global_structure_bsp_get());
        }
    }
}

/* port: a diagnostic run has no menu to pick a player profile; the saved game
   (persistent storage) lives in the active profile's directory, so one is made
   active as the menus do (engine_menu.c) */
static void mac_interactive_profile_ensure(void)
{
    word count = 1;
    long index = NONE;
    struct player_profile profile;
    if (player_ui_get_active_player_profile_index(0) != NONE)
        return;
    player_profiles_enumerate_available_to_local_player_index(NONE, &count, &index, FALSE);
    if ((short)count <= 0)
        index = player_profile_new(0, L"Player");
    if (index != NONE && player_profile_get(index, &profile)) {
        player_ui_set_single_player_local_player_controller(0, 0);
        player_ui_set_active_player_profile(0, index, &profile);
    }
}

/* whether the map being played (main_set_map_name's) is one of the comma-separated names */
static int mac_interactive_map_listed(const char *list)
{
    const char *map = main_get_map_name();
    while (list && *list) {
        size_t length = strcspn(list, ",");
        char name[64];
        if (length && length < sizeof(name)) {
            memcpy(name, list, length);
            name[length] = 0;
            if (map && strstr(map, name))
                return 1;
        }
        list += length;
        if (*list == ',')
            ++list;
    }
    return 0;
}

static void mac_interactive_kill_player(struct mac_interactive_context *context)
{
    /* HALO_MAC_REVERT_AT=<tick> (diagnostic): the game is reverted then, as the pause menu's Revert would */
    if (getenv("HALO_MAC_REVERT_AT") && context->ticks == (unsigned)atoi(getenv("HALO_MAC_REVERT_AT")))
        main_revert_map();
    /* HALO_MAC_SAVE_QUIT_AT=<tick> (diagnostic): the pause menu's Quit, which saves the game
       (the last checkpoint) to the persistent storage in the save root */
    if (getenv("HALO_MAC_SAVE_QUIT_AT") && context->ticks == (unsigned)atoi(getenv("HALO_MAC_SAVE_QUIT_AT"))) {
        double started;
        mac_interactive_profile_ensure();
        started = mac_interactive_microseconds();
        game_state_save_to_persistent_storage();
        fprintf(stderr, "checkpoint: saved the game to the persistent storage at tick %u in %.2f ms\n", context->ticks,
            (mac_interactive_microseconds() - started) / 1000.0);
    }
    /* HALO_MAC_CONTINUE=1 (diagnostic): the saved game of the persistent storage is taken at tick 2, as
       a map's start does when the campaign is continued (main_new_map) */
    if (getenv("HALO_MAC_CONTINUE") && context->ticks == 2) {
        double started;
        long before = game_time_get();
        mac_interactive_profile_ensure();
        started = mac_interactive_microseconds();
        game_state_try_and_load_from_persistent_storage();
        fprintf(stderr, "checkpoint: continue took the saved game: game time %ld -> %ld in %.2f ms, saved state %d\n", before,
            (long)game_time_get(), (mac_interactive_microseconds() - started) / 1000.0, (int)game_state_port_saved_game_valid());
    }
    /* port: the end of main_new_map for a game the menus started: the requests cleared and, if the
       game was not chosen as a next level (ui_set_next_level disallows it), the saved game of the
       persistent storage taken (Continue is the menus' main_set_map_name on the saved game's level) */
    if (mac_engine_menu_enabled() && context->ticks == 2 && !getenv("HALO_MAC_CONTINUE") &&
        global_scenario_try_and_get() && global_scenario_try_and_get()->type != _scenario_type_main_menu) {
        long before = game_time_get();
        int looked = mac_engine_main_new_map_finish();
        fprintf(stderr, "campaign: new map %s: saved game %s (game time %ld -> %ld, saved state %d), difficulty %d\n",
            main_get_map_name(), looked ? "looked for" : "not allowed", before, (long)game_time_get(),
            (int)game_state_port_saved_game_valid(), (int)game_difficulty_level_get());
    }
    /* HALO_MAC_WIN_AT=<tick> (diagnostic): the level is won then, as a script's game_won would;
       HALO_MAC_WIN_MAPS=<name>[,<name>...] limits it to the maps whose path has one of the names */
    if (getenv("HALO_MAC_WIN_AT") && context->ticks == (unsigned)atoi(getenv("HALO_MAC_WIN_AT")) &&
        (!getenv("HALO_MAC_WIN_MAPS") || mac_interactive_map_listed(getenv("HALO_MAC_WIN_MAPS")))) {
        fprintf(stderr, "campaign: winning the level at tick %u (game time %ld)\n", context->ticks, (long)game_time_get());
        main_won_map();
    }
    /* HALO_MAC_SAVE_AT=<tick> (diagnostic): a checkpoint is asked for then, as game_save would */
    if (getenv("HALO_MAC_SAVE_AT") && context->ticks == (unsigned)atoi(getenv("HALO_MAC_SAVE_AT")))
        main_save_map_nonsafe();
    const char *at = getenv("HALO_MAC_KILL_PLAYER");
    while (at && *at) {
        if (context->ticks == (unsigned)atoi(at)) {
            long unit_index = player_control_get_unit_index(0);
            if (unit_index != NONE) {
                struct object_datum *object = object_try_and_get(unit_index);
                if (object) {
                    fprintf(stderr, "checkpoint: killing the player at tick %u (game time %ld) at (%.2f,%.2f,%.2f) bsp %d\n",
                        context->ticks, (long)game_time_get(), object->object.position.x, object->object.position.y,
                        object->object.position.z, (int)global_structure_bsp_index);
                    unit_kill(unit_index);
                }
            }
        }
        at = strchr(at, ',');
        if (at)
            ++at;
    }
}

static void mac_interactive_sleep(unsigned long milliseconds)
{
    unsigned long start = mac_interactive_now();
    while ((unsigned long)(mac_interactive_now() - start) < milliseconds)
        Sleep(1);
}

static void mac_interactive_update_native_camera(void)
{
    collision_log_continue_period(TRUE);
    director_update((real)MAC_INTERACTIVE_TICK);
    observer_update((real)MAC_INTERACTIVE_TICK);
    collision_log_end_period();
    game_engine_update_non_deterministic((real)MAC_INTERACTIVE_TICK);
}

static void mac_interactive_clear_edges(struct mac_input_frame *input)
{
    input->pressed = 0;
    input->released = 0;
    input->yaw = 0.0f;
    input->pitch = 0.0f;
    input->wheel = 0.0f;
    input->pointer_moved = 0;
    input->left_clicks = 0;
    input->right_clicks = 0;
    input->text_count = 0;
}

/* HALO_MAC_CHECK_MODIFIER_SHADERS (diagnostic): every object's modifier
   shader looked up as render_objects.c looks it up when the object is drawn
   (a unit's shield shimmer; a seated or third-person player draws its
   own), so that one the session did not load fails here, in a hidden run */
static void mac_interactive_check_modifier_shaders(void)
{
    struct object_iterator iterator;
    void *datum;
    long count = 0;
    object_iterator_new(&iterator, _object_mask_all, 0);
    while ((datum = object_iterator_next(&iterator)) != NULL) {
        struct object_datum *object = (struct object_datum *)datum;
        struct object_definition *definition = object_definition_get(object->definition_index);
        if (definition->object.modifier_shader.index != NONE) {
            (void)shader_definition_get(definition->object.modifier_shader.index);
            ++count;
        }
    }
    fprintf(stderr, "native modifier shaders: %ld objects' looked up\n", count);
}


/* HALO_MAC_MENU_KEYS (diagnostic, menu mode): a hidden run's key presses, a
   token every HALO_MAC_MENU_KEY_FRAMES frames (default 10), counted over all
   the runs of the process: up, down, left, right, return, esc, backspace
   (pressed for that frame), wait, shot (the frame is written to
   HALO_MAC_MENU_SHOTS-N.ppm, the prefix defaulting to HALO_MAC_RENDER_CAPTURE's)
   and end (the run stops); also move:X:Y and click:X:Y (the pointer, in the
   menus' 640x480), mouse:N, wheel:N and hold:N[:T] (a key down for T tokens' frames, default 1), key:N (a Mac virtual key) and type:TEXT (letters, digits and spaces typed). */
enum { MAC_SCRIPT_MAXIMUM = 1024 };
static struct {
    int parsed, count, interval;
    int tokens[MAC_SCRIPT_MAXIMUM];
    int arg_x[MAC_SCRIPT_MAXIMUM], arg_y[MAC_SCRIPT_MAXIMUM];
    char text[MAC_SCRIPT_MAXIMUM][24];
    float pointer_x, pointer_y;
    int hold_key;
    unsigned long frame, hold_until;
    unsigned shots;
    int done;
} mac_script;
enum { SCRIPT_WAIT = -1, SCRIPT_SHOT = -2, SCRIPT_END = -3, SCRIPT_MOVE = -4, SCRIPT_CLICK = -5, SCRIPT_TYPE = -6, SCRIPT_MOUSE = -7, SCRIPT_WHEEL = -8, SCRIPT_HOLD = -9 };

static int mac_script_key_code(const char *word, size_t length)
{
    static const struct { const char *name; int code; } keys[] = {
        { "up", 126 }, { "down", 125 }, { "left", 123 }, { "right", 124 },
        { "return", 36 }, { "enter", 36 }, { "esc", 53 }, { "backspace", 51 },
        { "wait", SCRIPT_WAIT }, { "shot", SCRIPT_SHOT }, { "end", SCRIPT_END }
    };
    size_t i;
    for (i = 0; i < sizeof(keys) / sizeof(keys[0]); ++i)
        if (strlen(keys[i].name) == length && !strncmp(keys[i].name, word, length))
            return keys[i].code;
    return -100;
}

static void mac_script_parse(void)
{
    const char *text = getenv("HALO_MAC_MENU_KEYS"), *every = getenv("HALO_MAC_MENU_KEY_FRAMES");
    mac_script.parsed = 1;
    mac_script.interval = every && atoi(every) > 0 ? atoi(every) : 10;
    while (text && *text) {
        size_t length = strcspn(text, ", ");
        if (length) {
            int code = mac_script_key_code(text, length), x = 0, y = 0;
            char word[40];
            word[0] = 0;
            if (length < sizeof(word)) {
                memcpy(word, text, length);
                word[length] = 0;
            }
            if (!strncmp(word, "move:", 5) || !strncmp(word, "click:", 6)) {
                const char *colon = strchr(word, ':');
                code = word[0] == 'm' ? SCRIPT_MOVE : SCRIPT_CLICK;
                x = atoi(colon + 1);
                colon = strchr(colon + 1, ':');
                y = colon ? atoi(colon + 1) : 0;
            } else if (!strncmp(word, "hold:", 5)) {
                /* a Mac virtual key held down for the token's frames */
                const char *colon = strchr(word + 5, ':');
                code = SCRIPT_HOLD;
                x = atoi(word + 5);
                y = colon ? atoi(colon + 1) : 1;
            } else if (!strncmp(word, "mouse:", 6) || !strncmp(word, "wheel:", 6)) {
                /* a mouse button (1 to 5) down for the frame; the wheel turned (+1 up, -1 down) */
                code = word[0] == 'm' ? SCRIPT_MOUSE : SCRIPT_WHEEL;
                x = atoi(word + 6);
            } else if (!strncmp(word, "key:", 4)) {
                /* a Mac virtual key code, pressed for the frame */
                code = atoi(word + 4);
            } else if (!strncmp(word, "type:", 5)) {
                /* a token for each character (a key is down for its frame) */
                const char *letter;
                for (letter = word + 5; *letter && mac_script.count < MAC_SCRIPT_MAXIMUM; ++letter) {
                    mac_script.text[mac_script.count][0] = *letter;
                    mac_script.text[mac_script.count][1] = 0;
                    mac_script.tokens[mac_script.count] = SCRIPT_TYPE;
                    mac_script.arg_x[mac_script.count] = mac_script.arg_y[mac_script.count] = 0;
                    ++mac_script.count;
                }
                code = -101;
            }
            if (code != -100 && code != -101 && mac_script.count < MAC_SCRIPT_MAXIMUM) {
                mac_script.arg_x[mac_script.count] = x;
                mac_script.arg_y[mac_script.count] = y;
            }
            if (code == -101) {
                /* (typed: its tokens are in) */
            } else if (code == -100)
                fprintf(stderr, "menu keys: unknown key %.*s\n", (int)length, text);
            else if (mac_script.count < MAC_SCRIPT_MAXIMUM)
                mac_script.tokens[mac_script.count++] = code;
            text += length;
        }
        while (*text == ',' || *text == ' ') ++text;
    }
    if (mac_script.count)
        fprintf(stderr, "menu keys: %d tokens, one every %d frames\n", mac_script.count, mac_script.interval);
}

/* the token for this frame: its key code (set in the frame's keys), or a
   control; SCRIPT_WAIT when there is none */
static int mac_script_step(struct mac_input_frame *input)
{
    int token = SCRIPT_WAIT;
    if (!mac_script.parsed)
        mac_script_parse();
    if (!mac_script.count)
        return SCRIPT_WAIT;
    if (mac_script.frame % (unsigned long)mac_script.interval == 0) {
        unsigned long index = mac_script.frame / (unsigned long)mac_script.interval;
        if (index < (unsigned long)mac_script.count)
            token = mac_script.tokens[index];
        else
            token = SCRIPT_END;
    }
    ++mac_script.frame;
    if (token >= 0 && token < 128)
        input->keys[token] = 1;
    else if (token == SCRIPT_HOLD) {
        mac_script.hold_key = mac_script.arg_x[(mac_script.frame - 1) / (unsigned long)mac_script.interval];
        mac_script.hold_until = mac_script.frame - 1 + (unsigned long)mac_script.interval *
            (unsigned long)(mac_script.arg_y[(mac_script.frame - 1) / (unsigned long)mac_script.interval] > 1 ?
            mac_script.arg_y[(mac_script.frame - 1) / (unsigned long)mac_script.interval] : 1);
    }
    if (mac_script.hold_until && mac_script.frame - 1 < mac_script.hold_until &&
        mac_script.hold_key >= 0 && mac_script.hold_key < 128)
        input->keys[mac_script.hold_key] = 1;
    else if (token == SCRIPT_MOVE || token == SCRIPT_CLICK) {
        unsigned long index = (mac_script.frame - 1) / (unsigned long)mac_script.interval;
        mac_script.pointer_x = (float)mac_script.arg_x[index];
        mac_script.pointer_y = (float)mac_script.arg_y[index];
        if (token == SCRIPT_CLICK) {
            input->click_x = mac_script.pointer_x;
            input->click_y = mac_script.pointer_y;
            input->left_clicks = 1;
        }
        input->pointer_moved = 1;
    } else if (token == SCRIPT_MOUSE || token == SCRIPT_WHEEL) {
        int argument = mac_script.arg_x[(mac_script.frame - 1) / (unsigned long)mac_script.interval];
        if (token == SCRIPT_MOUSE && argument >= 1 && argument <= 5)
            input->mouse_buttons |= 1U << (argument - 1);
        else if (token == SCRIPT_WHEEL)
            input->wheel = (float)argument;
    } else if (token == SCRIPT_TYPE) {
        /* the Mac virtual keys of the letters, by position in the alphabet, and of the digits */
        static const unsigned char letters[26] = { 0, 11, 8, 2, 14, 3, 5, 4, 34, 38, 40, 37, 46, 45, 31, 35, 12, 15, 1, 17, 32, 9, 13, 7, 16, 6 };
        static const unsigned char digits[10] = { 29, 18, 19, 20, 21, 23, 22, 26, 28, 25 };
        unsigned long index = (mac_script.frame - 1) / (unsigned long)mac_script.interval;
        const char *letter;
        input->text_count = 0;
        for (letter = mac_script.text[index]; *letter && input->text_count < 32; ++letter) {
            int key = -1;
            if (*letter >= 'a' && *letter <= 'z') key = letters[*letter - 'a'];
            else if (*letter >= 'A' && *letter <= 'Z') key = letters[*letter - 'A'];
            else if (*letter >= '0' && *letter <= '9') key = digits[*letter - '0'];
            else if (*letter == ' ') key = 49;
            if (key >= 0) {
                input->keys[key] = 1;
                input->text[input->text_count].key = (unsigned char)key;
                input->text[input->text_count].character = *letter;
                ++input->text_count;
            }
        }
    }
    if (token != SCRIPT_END && mac_script.count && (mac_script.pointer_x || mac_script.pointer_y)) {
        input->pointer_valid = 1;
        input->pointer_x = mac_script.pointer_x;
        input->pointer_y = mac_script.pointer_y;
    }
    return token;
}


/* HALO_MAC_CUTSCENE_LOG=<ticks> (diagnostic): every that many ticks, the
   running script threads (name, wake tick), the cinematic, the camera, the
   player and every vehicle (name, position, parent, recording) */
extern struct data_array *hs_thread_data;
struct mac_hs_thread_view
{
    short identifier;
    byte type;
    byte flags;
    long script_index;
    long sleep_until;
    long previous_sleep_until;
};

static void mac_interactive_cutscene_log(unsigned tick)
{
    struct scenario *scenario = global_scenario_get();
    struct data_iterator iterator;
    struct mac_hs_thread_view *thread;
    struct object_iterator objects;
    struct object_datum *object;
    struct observer_result const *camera = observer_get_camera(0);
    long unit_index = player_control_get_unit_index(0);
    struct object_datum *player = unit_index != NONE ? object_try_and_get(unit_index) : NULL;
    unsigned threads = 0;
    fprintf(stderr, "cutscene tick %u: cinematic=%d camera=(%.1f,%.1f,%.1f)", tick, (int)cinematic_in_progress(),
        camera ? camera->position.x : 0.0f, camera ? camera->position.y : 0.0f, camera ? camera->position.z : 0.0f);
    if (player)
        fprintf(stderr, " player=(%.1f,%.1f,%.1f) parent=%08lx", player->object.position.x, player->object.position.y,
            player->object.position.z, (unsigned long)player->object.parent_object_index);
    fprintf(stderr, "\n  threads:");
    data_iterator_new(&iterator, hs_thread_data);
    while ((thread = (struct mac_hs_thread_view *)data_iterator_next(&iterator)) != NULL) {
        const char *name = "?";
        if (thread->script_index >= 0 && thread->script_index < scenario->hs_scripts.count)
            name = ((struct hs_script *)scenario->hs_scripts.address)[thread->script_index].name;
        if (thread->sleep_until != -2 && strncmp(name, "save_", 5) && strncmp(name, "music", 5) &&
            strncmp(name, "obj_mark", 8) && strncmp(name, "cont_", 5) && strcmp(name, "shafta_killer"))
            fprintf(stderr, " %s%s@%ld", name, thread->sleep_until == -1 ? "(waiting)" : "", (long)thread->sleep_until);
        ++threads;
    }
    fprintf(stderr, " [%u]\n", threads);
    object_iterator_new(&objects, FLAG(_object_type_vehicle), 0);
    while ((object = (struct object_datum *)object_iterator_next(&objects)) != NULL) {
        const char *name = "-";
        if (object->object.name_index != NONE && object->object.name_index < scenario->object_names.count)
            name = ((struct scenario_object_name *)scenario->object_names.address)[object->object.name_index].name;
        fprintf(stderr, "  vehicle %s at (%.1f,%.1f,%.1f) v=(%.2f,%.2f,%.2f) rec=%d left=%ld children=%08lx\n", name,
            object->object.position.x, object->object.position.y, object->object.position.z,
            object->object.translational_velocity.i, object->object.translational_velocity.j,
            object->object.translational_velocity.k,
            (int)recorded_animation_controlling_unit(objects.index),
            recorded_animation_controlling_unit(objects.index) ? recorded_animation_get_time_left(objects.index) : 0L,
            (unsigned long)object->object.first_child_object_index);
    }
}

/* the presented frame as a PPM (the probe's capture, for the script's shots) */
static void mac_interactive_write_shot(void)
{
    const struct mac_device_state *device = mac_device_state_get();
    const char *prefix = getenv("HALO_MAC_MENU_SHOTS");
    char path[512], base[480];
    const unsigned int *pixels;
    unsigned long pitch, x, y;
    FILE *file;
    if (!device || !device->gpu)
        return;
    if (!prefix || !*prefix) {
        const char *capture = getenv("HALO_MAC_RENDER_CAPTURE");
        size_t length;
        snprintf(base, sizeof(base), "%s", capture && *capture ? capture : "menu-shot");
        length = strlen(base);
        if (length > 4 && !strcmp(base + length - 4, ".ppm"))
            base[length - 4] = 0;
        prefix = base;
    }
    snprintf(path, sizeof(path), "%s-%u.ppm", prefix, ++mac_script.shots);
    pixels = (const unsigned int *)PLATFORM_PHYSICAL_TO_VIRTUAL(device->back_buffer.Data);
    pitch = (device->gpu_width * 4 + D3DTEXTURE_PITCH_ALIGNMENT - 1) & ~(unsigned long)(D3DTEXTURE_PITCH_ALIGNMENT - 1);
    file = pixels ? fopen(path, "wb") : NULL;
    if (!file)
        return;
    fprintf(file, "P6\n%lu %lu\n255\n", device->gpu_width, device->gpu_height);
    for (y = 0; y < device->gpu_height; ++y)
        for (x = 0; x < device->gpu_width; ++x) {
            unsigned int pixel = pixels[(pitch / 4) * y + x];
            unsigned char rgb[3];
            rgb[0] = (unsigned char)(pixel >> 16); rgb[1] = (unsigned char)(pixel >> 8); rgb[2] = (unsigned char)pixel;
            fwrite(rgb, 1, 3, file);
        }
    fclose(file);
    fprintf(stderr, "menu keys: shot %s\n", path);
}

/* port: the screenshot key (controls.screenshot): the presented frame as a
   PNG in screenshots/ of the save folder (HALO_MAC_SAVE_ROOT, paths.saves,
   else ~/Library/Application Support/OpenCE), named YYYY-MM-DD_HH.MM.SS.png
   (_2, _3 for more in a second). The PNG's data is stored, not compressed:
   no library, and a frame of 640x480 is 920 KB. */
static unsigned long mac_png_crc_table[256];
static void mac_png_crc_init(void)
{
    unsigned long n, k, c;
    if (mac_png_crc_table[1])
        return;
    for (n = 0; n < 256; n++) {
        c = n;
        for (k = 0; k < 8; k++)
            c = c & 1 ? 0xEDB88320UL ^ (c >> 1) : c >> 1;
        mac_png_crc_table[n] = c;
    }
}
static unsigned long mac_png_crc(unsigned long crc, const unsigned char *data, unsigned long length)
{
    unsigned long i;
    for (i = 0; i < length; i++)
        crc = mac_png_crc_table[(crc ^ data[i]) & 0xff] ^ (crc >> 8);
    return crc;
}
static void mac_png_chunk(FILE *file, const char *type, const unsigned char *data, unsigned long length)
{
    unsigned char head[8], tail[4];
    unsigned long crc;
    head[0] = (unsigned char)(length >> 24); head[1] = (unsigned char)(length >> 16);
    head[2] = (unsigned char)(length >> 8); head[3] = (unsigned char)length;
    memcpy(head + 4, type, 4);
    crc = mac_png_crc(0xFFFFFFFFUL, head + 4, 4);
    if (length)
        crc = mac_png_crc(crc, data, length);
    crc ^= 0xFFFFFFFFUL;
    tail[0] = (unsigned char)(crc >> 24); tail[1] = (unsigned char)(crc >> 16);
    tail[2] = (unsigned char)(crc >> 8); tail[3] = (unsigned char)crc;
    fwrite(head, 1, 8, file);
    if (length)
        fwrite(data, 1, length, file);
    fwrite(tail, 1, 4, file);
}
static int mac_screenshot_directory(char *directory, size_t size)
{
    extern const char *config_string(const char *name);
    const char *root = getenv("HALO_MAC_SAVE_ROOT"), *home = getenv("HOME");
    char path[480];
    if (!root || !*root)
        root = config_string("paths.saves");
    if (root && *root)
        snprintf(path, sizeof(path), "%s", root);
    else if (home && *home)
        snprintf(path, sizeof(path), "%s/Library/Application Support/OpenCE", home);
    else
        return 0;
    mkdir(path, 0777);
    if (snprintf(directory, size, "%s/screenshots", path) >= (int)size)
        return 0;
    mkdir(directory, 0777);
    return 1;
}
static void mac_interactive_write_screenshot(void)
{
    const struct mac_device_state *device = mac_device_state_get();
    char directory[512], path[560], stamp[40];
    const unsigned int *pixels;
    unsigned long pitch, x, y, width, height, rowbytes, total, offset = 0, adler_a = 1, adler_b = 0, i;
    unsigned char *raw, ihdr[13], *z, *out;
    unsigned long zsize, block;
    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    unsigned collision;
    FILE *file;
    if (!device || !device->gpu || !local)
        return;
    if (!mac_screenshot_directory(directory, sizeof(directory)))
        return;
    width = device->gpu_width; height = device->gpu_height;
    pixels = (const unsigned int *)PLATFORM_PHYSICAL_TO_VIRTUAL(device->back_buffer.Data);
    if (!pixels || !width || !height)
        return;
    pitch = (width * 4 + D3DTEXTURE_PITCH_ALIGNMENT - 1) & ~(unsigned long)(D3DTEXTURE_PITCH_ALIGNMENT - 1);
    rowbytes = 1 + width * 3;
    total = rowbytes * height;
    raw = (unsigned char *)malloc(total);
    zsize = 2 + total + 5 * (total / 65535 + 1) + 4;
    z = (unsigned char *)malloc(zsize);
    if (!raw || !z) {
        free(raw); free(z);
        return;
    }
    for (y = 0; y < height; y++) {
        raw[y * rowbytes] = 0;
        for (x = 0; x < width; x++) {
            unsigned int pixel = pixels[(pitch / 4) * y + x];
            raw[y * rowbytes + 1 + x * 3] = (unsigned char)(pixel >> 16);
            raw[y * rowbytes + 2 + x * 3] = (unsigned char)(pixel >> 8);
            raw[y * rowbytes + 3 + x * 3] = (unsigned char)pixel;
        }
    }
    out = z;
    *out++ = 0x78; *out++ = 0x01;
    while (offset < total) {
        block = total - offset > 65535 ? 65535 : total - offset;
        *out++ = offset + block >= total ? 1 : 0;
        *out++ = (unsigned char)block; *out++ = (unsigned char)(block >> 8);
        *out++ = (unsigned char)~block; *out++ = (unsigned char)(~block >> 8);
        memcpy(out, raw + offset, block);
        out += block; offset += block;
    }
    for (i = 0; i < total; i++) {
        adler_a = (adler_a + raw[i]) % 65521UL;
        adler_b = (adler_b + adler_a) % 65521UL;
    }
    *out++ = (unsigned char)(adler_b >> 8); *out++ = (unsigned char)adler_b;
    *out++ = (unsigned char)(adler_a >> 8); *out++ = (unsigned char)adler_a;
    strftime(stamp, sizeof(stamp), "%Y-%m-%d_%H.%M.%S", local);
    for (collision = 0; collision < 1000; collision++) {
        if (collision)
            snprintf(path, sizeof(path), "%s/%s_%u.png", directory, stamp, collision + 1);
        else
            snprintf(path, sizeof(path), "%s/%s.png", directory, stamp);
        file = fopen(path, "rb");
        if (!file)
            break;
        fclose(file);
    }
    file = collision < 1000 ? fopen(path, "wb") : NULL;
    if (file) {
        static const unsigned char signature[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
        mac_png_crc_init();
        ihdr[0] = (unsigned char)(width >> 24); ihdr[1] = (unsigned char)(width >> 16);
        ihdr[2] = (unsigned char)(width >> 8); ihdr[3] = (unsigned char)width;
        ihdr[4] = (unsigned char)(height >> 24); ihdr[5] = (unsigned char)(height >> 16);
        ihdr[6] = (unsigned char)(height >> 8); ihdr[7] = (unsigned char)height;
        ihdr[8] = 8; ihdr[9] = 2; ihdr[10] = 0; ihdr[11] = 0; ihdr[12] = 0;
        fwrite(signature, 1, 8, file);
        mac_png_chunk(file, "IHDR", ihdr, 13);
        mac_png_chunk(file, "IDAT", z, (unsigned long)(out - z));
        mac_png_chunk(file, "IEND", NULL, 0);
        if (fclose(file) == 0)
            fprintf(stderr, "screenshot: %s\n", path);
        else
            fprintf(stderr, "screenshot: could not write %s\n", path);
    } else
        fprintf(stderr, "screenshot: could not open a file in %s\n", directory);
    free(raw); free(z);
}

/* port: what source/main/main.c's loop does for the netcode before its
   tick. A machine whose host has gone stops the game, as main.c does; there
   is no main menu here to show the error on, so it is logged. */
static int mac_interactive_network_start_frame(struct mac_interactive_context *context)
{
    short connection, connection_before;
    /* the menus' network games: the netcode started with the menus, and a
       begin held for this session's map made (engine_network_test.c) */
    mac_engine_network_menu_update();
    mac_engine_network_begin_resume();
    network_test_update(mac_engine_main_menu_loaded() ? TRUE : FALSE,
        (real)MAC_INTERACTIVE_TICK);
    /* and the hosting or joining network_test.c leaves to the menu */
    mac_engine_network_test_update((real)MAC_INTERACTIVE_TICK);
    network_voice_update();
    map_share_update();
    connection = connection_before = game_connection();
    if (connection == _game_connection_network_client)
    {
        if (!network_game_client_start_frame())
        {
            fprintf(stderr, "network: the game host went down\n");
            network_game_abort();
        }
    }
    else if (connection == _game_connection_network_server)
    {
        if (!network_game_client_start_frame())
        {
            struct network_game_client *client = global_network_game_client_get();
            short state_data = 0;
            fprintf(stderr,
                "network: the host's own client stopped (error %d, state %d, network available %d)\n",
                client ? (int)network_game_client_get_error(client) : -1,
                client ? (int)network_game_client_get_state(client, &state_data) : -1,
                (int)transport_network_available());
            network_game_abort();
        }
        else if (!network_game_server_start_frame())
        {
            fprintf(stderr, "network: the server stopped\n");
            network_game_abort();
        }
    }
    (void)context;
    /* port: a network game that ended in this frame (the host went, or this
       machine left): the connection is local again and the game has the main
       menu asked for, so the frame ticks nothing more of it
       (main.c's loop loads the main menu before its next tick) */
    return connection_before > _game_connection_local && game_connection() == _game_connection_local;
}

/* and after the player's input is gathered, before the tick runs */
static void mac_interactive_network_end_frame(struct mac_interactive_context *context)
{
    short connection = game_connection();
    if (connection > _game_connection_local && connection <= _game_connection_network_server &&
        !network_game_client_end_frame())
    {
        fprintf(stderr, "network: the client's end of frame failed\n");
        network_game_abort();
    }
    (void)context;
}

/* port: whether the developer console was open at the last tick (to say so when it changes) */
static int mac_console_was_open;

static int mac_interactive_frame(void *opaque,
    const struct mac_input_frame *native_input, unsigned width, unsigned height)
{
    struct mac_interactive_context *context =
        (struct mac_interactive_context *)opaque;
    struct mac_input_frame tick_input;
    unsigned long now;
    double elapsed;
    unsigned tick_count = 0;
    int shot = 0, screenshot = 0;
    unsigned long tick_started;
    int console_open = 0;
    double phase_tick_at = 0.0, phase_render_at = 0.0, phase_present_at = 0.0;

    if (!context || !native_input || !width || !height) {
        if (context)
            context->failed = 1;
        return 0;
    }
    if (native_input->keys[53] && !mac_engine_menu_enabled()) {
        context->quit_requested = 1;
        platform_request_quit();
        return 0;
    }

    now = mac_interactive_now();
    elapsed = (double)(unsigned long)(now - context->last_time) / 1000.0;
    context->last_time = now;
    {
        /* port: the limiter's wait is taken whether or not it is used, so that it does not add up */
        double limiter_wait = mac_window_take_limiter_wait();
        if (context->frames) {
            mac_interpolation_note_frame(elapsed); /* port: halo_interpolation_enabled (port/mac/engine_config.c) */
            mac_adaptive_detail_note_frame(elapsed, limiter_wait); /* port: mac.adaptive_detail (port/mac/mac_gpu_info.c) */
        }
    }
    if (elapsed > MAC_INTERACTIVE_MAX_ELAPSED)
        elapsed = MAC_INTERACTIVE_MAX_ELAPSED;
    if (context->fixed_step)
        context->accumulator = MAC_INTERACTIVE_TICK;
    else
        context->accumulator += elapsed;
    if (context->accumulator > MAC_INTERACTIVE_MAX_ELAPSED)
        context->accumulator = MAC_INTERACTIVE_MAX_ELAPSED;

    /* port: HALO_MAC_CHECKPOINTS=0 is the old behaviour, no game-state save:
       a save the scripts ask for (game_save, which a campaign level's
       cinematic_skip_start waits on) completes at once. Otherwise the
       checkpoints are the engine's (main_save_map_private, game_state_save)
       and a death reverts to the last (main_lost_map_private). */
    if (!mac_interactive_checkpoints_enabled() && main_saving_map())
        main_save_cancel();
    /* HALO_MAC_LOCAL_PLAYER_LOG=<frames> (diagnostic): every that many frames, each local
       player's controller, whether it has a game pad, its unit's position and the pad's
       left stick, so a hidden split screen run shows each controller reach its player */
    if (getenv("HALO_MAC_LOCAL_PLAYER_LOG") && atoi(getenv("HALO_MAC_LOCAL_PLAYER_LOG")) > 0 &&
        context->frames % (unsigned)atoi(getenv("HALO_MAC_LOCAL_PLAYER_LOG")) == 0 && game_in_progress()) {
        short controller;
        for (controller = 0; controller < MAXIMUM_GAMEPADS; ++controller) {
            long player_index = local_player_get_player_index(controller);
            long unit_index = player_control_get_unit_index(controller);
            struct object_datum *unit = unit_index != NONE ? object_try_and_get(unit_index) : NULL;
            struct gamepad_state const *pad = input_has_gamepad(controller) ? input_get_gamepad_state(controller) : NULL;
            if (player_index == NONE && !pad)
                continue;
            fprintf(stderr, "local players: frame %u controller %d player %08lx pad %s left stick (%d,%d) unit at (%.2f,%.2f,%.2f) facing %.2f\n",
                context->frames, (int)controller, (unsigned long)player_index, pad ? "yes" : "no",
                pad ? (int)pad->sticks[_gamepad_stick_left].x : 0, pad ? (int)pad->sticks[_gamepad_stick_left].y : 0,
                unit ? unit->object.position.x : 0.0f, unit ? unit->object.position.y : 0.0f,
                unit ? unit->object.position.z : 0.0f, (double)player_control_get(controller)->desired_angles.yaw);
        }
    }
    /* HALO_MAC_LEVEL_LOG (diagnostic): the camera every 30 frames */
    if (getenv("HALO_MAC_LEVEL_LOG") && context->frames % 30 == 0) {
        struct observer_result const *camera = observer_get_camera(0);
        long unit_index = player_control_get_unit_index(0);
        if (camera)
            fprintf(stderr, "level log: frame %u camera (%.2f,%.2f,%.2f) forward (%.2f,%.2f,%.2f) leaf %ld cluster %d fov %.2f cinematic %d player unit %08lx\n",
                context->frames, camera->position.x, camera->position.y, camera->position.z,
                camera->forward.i, camera->forward.j, camera->forward.k,
                (long)camera->location.leaf_index, (int)camera->location.cluster_index, camera->field_of_view,
                (int)cinematic_in_progress(), (unsigned long)unit_index);
        {
            real_rgb_color fade_color;short fade_ticks;boolean fading_out;long fade_start;
            player_effect_port_screen_fade_get(&fade_color, &fade_ticks, &fading_out, &fade_start);
            /* the scripts' threads: (layout of hs_runtime.c's hs_thread_datum) */
            if (getenv("HALO_MAC_LEVEL_LOG")[0] == '2' && hs_thread_data && global_scenario) {
                struct thread_view { short identifier; byte type; byte flags; long script_index; long sleep_until; };
                struct data_iterator threads;
                struct thread_view *thread;
                data_iterator_new(&threads, hs_thread_data);
                while ((thread = (struct thread_view *)data_iterator_next(&threads)) != NULL) {
                    struct hs_script *scripts = (struct hs_script *)global_scenario->hs_scripts.address;
                    fprintf(stderr, "level log: thread type %d flags %d script %s sleep_until %ld\n", (int)thread->type, (int)thread->flags,
                        thread->script_index >= 0 && thread->script_index < global_scenario->hs_scripts.count ? scripts[thread->script_index].name : "?",
                        thread->sleep_until);
                }
            }
            fprintf(stderr, "level log: fade color (%.2f,%.2f,%.2f) ticks %d out %d start %ld game time %ld\n",
                fade_color.red, fade_color.green, fade_color.blue, (int)fade_ticks, (int)fading_out,
                fade_start, (long)game_time_get());
        }
    }
    /* (HALO_MAC_AI_PLACE_TICK=<tick>: at that tick of the game, not the first frame:
       the encounters of a bsp the game has switched to) */
    if (getenv("HALO_MAC_AI_PLACE_TICK") ? context->ticks == (unsigned)atoi(getenv("HALO_MAC_AI_PLACE_TICK")) &&
        context->frames : context->frames == 0)
        mac_interactive_ai_place();
    if (context->frames >= 1)
        mac_interactive_ai_view(context->frames);
    /* HALO_MAC_AI_OFF (diagnostic): the AI frozen from the second frame, to
       measure its share of the tick time */
    if (context->frames == 1 && getenv("HALO_MAC_AI_OFF"))
        ai_globals_ai_active(FALSE);
    if (context->frames == 0 && getenv("HALO_MAC_CHECK_MODIFIER_SHADERS"))
        mac_interactive_check_modifier_shaders();
    /* HALO_MAC_RENDER_HURT (diagnostic): the player without shields and at
       30% health from the start, so a hidden run draws the HUD's health
       meter and its warnings */
    if (getenv("HALO_MAC_RENDER_HURT")) {
        long unit_index = player_control_get_unit_index(0);
        struct object_datum *object = unit_index != NONE ? object_try_and_get(unit_index) : NULL;
        if (object) {
            object->object.shield_vitality = 0.0f;
            object->object.body_vitality = 0.3f;
        }
    }
    tick_input = *native_input;
    mac_interactive_sim_input(context->ticks, &tick_input);
    /* HALO_MAC_INPUT_DEVICE=<p|k>@<tick>,... (diagnostic): at that tick a
       game pad's left stick is pushed (p) or a key goes down (k), as the
       player picking up the other device, to see the prompts follow it */
    if (getenv("HALO_MAC_INPUT_DEVICE")) {
        const char *at = getenv("HALO_MAC_INPUT_DEVICE");
        while (*at) {
            const char *mark = strchr(at, '@');
            if (!mark)
                break;
            if (context->ticks == (unsigned)atoi(mark + 1)) {
                if (*at == 'p')
                    mac_engine_input_script_pad(30000);
                else if (*at == 'k')
                    tick_input.keys[71] = 1;
            }
            at = strchr(mark, ',');
            if (!at)
                break;
            ++at;
        }
    }
    /* HALO_MAC_RENDER_FIRE=<tick> (diagnostic): the primary trigger held from
       that tick, so a hidden run draws a charging weapon's HUD */
    if (getenv("HALO_MAC_RENDER_FIRE") && context->ticks >= (unsigned)atoi(getenv("HALO_MAC_RENDER_FIRE")))
        tick_input.held |= MAC_INPUT_FIRE, tick_input.mouse_buttons |= 1U;
    /* HALO_MAC_RENDER_FIRE_LOG (diagnostic): the weapons' heat, charge and
       trigger state every ten ticks */
    if (getenv("HALO_MAC_RENDER_FIRE_LOG") && context->ticks % 10 == 0) {
        struct object_iterator weapons;
        struct object_datum *weapon_object;
        long player_unit_index = player_control_get_unit_index(0);
        struct unit_datum *player_unit = player_unit_index != NONE ? (struct unit_datum *)object_try_and_get(player_unit_index) : NULL;
        if (player_unit)
            fprintf(stderr, "fire log tick %u: unit %08lx control %08lx held %u active weapon %d\n", context->ticks,
                (unsigned long)player_unit_index, (unsigned long)player_unit->unit.control_flags, tick_input.held,
                (int)player_unit->unit.current_weapon_index);
        object_iterator_new(&weapons, FLAG(_object_type_weapon), 0);
        while ((weapon_object = (struct object_datum *)object_iterator_next(&weapons)) != NULL) {
            struct weapon_datum *weapon = (struct weapon_datum *)weapon_object;
            if (weapon->object.parent_object_index != NONE)
                fprintf(stderr, "fire log tick %u: weapon %08lx heat %.3f age %.3f overcharged %.3f flags %04x state %d trigger0 %d/%d\n",
                    context->ticks, (unsigned long)weapons.index, weapon->weapon.heat, weapon->weapon.age,
                    weapon->weapon.overcharged, (unsigned)weapon->weapon.flags, (int)weapon->weapon.state,
                    (int)weapon->weapon.triggers[0].state, (int)weapon->weapon.triggers[0].state_timer);
        }
    }
    /* port: HALO_MAC_MENU_KEYS also drives a game with no menus (to type at the developer console) */
    if (mac_engine_menu_enabled() || getenv("HALO_MAC_MENU_KEYS")) {
        int token = mac_script_step(&tick_input);
        if (token == SCRIPT_SHOT)
            shot = 1;
        else if (token == SCRIPT_END) {
            context->request = MAC_INTERACTIVE_SCRIPT_DONE;
            mac_script.done = 1;
        }
    }
    /* HALO_MAC_RENDER_SWEEP (diagnostic): a hidden run raises the view for
       its first ten frames, then turns it through a full circle, so that it
       passes the sky and the sun (their glow, the lens flares) */
    if (context->sweep) {
        if (context->frames < 10) tick_input.pitch += 0.06f;
        else tick_input.yaw += 0.08f;
    }
    /* HALO_MAC_CUTSCENE_WALK=<tick> (diagnostic): the player is put on the
       beach 20 units from the landing zone at that tick (the opening's
       pelicans leave when he leaves it) */
    if (getenv("HALO_MAC_CUTSCENE_WALK") && context->ticks == (unsigned)atoi(getenv("HALO_MAC_CUTSCENE_WALK"))) {
        long unit_index = player_control_get_unit_index(0);
        if (unit_index != NONE) {
            real_point3d at;
            real_vector3d forward, up;
            at.x = -62.4f; at.y = 20.7f; at.z = 1.6f;
            forward.i = 0.0f; forward.j = 1.0f; forward.k = 0.0f;
            up.i = 0.0f; up.j = 0.0f; up.k = 1.0f;
            object_set_position(unit_index, &at, &forward, &up);
        }
    }
    /* HALO_MAC_RADAR_FREE=<yaw degrees> (diagnostic, with HALO_MAC_CUTSCENE_WALK):
       from the walk tick the opening cutscene's camera and hidden HUD are
       ended, the player cannot die and faces that direction, so that the
       HUD (and its radar) is drawn and a capture shows it */
    /* (HALO_MAC_RADAR_ALL: the sensor shows still units too) */
    if (getenv("HALO_MAC_RADAR_ALL"))
        debug_motion_sensor_draw_all_units = TRUE;
    if (getenv("HALO_MAC_RADAR_FREE") && getenv("HALO_MAC_CUTSCENE_WALK")) {
        long unit_index = player_control_get_unit_index(0);
        if (unit_index != NONE) {
            struct object_datum *player_object = object_get(unit_index);
            player_object->object.body_vitality = 1.0f;
            player_object->object.shield_vitality = 1.0f;
            if (context->ticks < (unsigned)atoi(getenv("HALO_MAC_CUTSCENE_WALK")))
                goto radar_free_done;
            cinematic_stop();
            director_script_camera(FALSE);
            if (player_object->object.parent_object_index != NONE)
                unit_exit_seat_end(unit_index);
            player_control_get(0)->desired_angles.yaw = (real)atof(getenv("HALO_MAC_RADAR_FREE")) * 0.017453292f;
            player_control_get(0)->desired_angles.pitch = 0.0f;
        }
    }
radar_free_done:
    /* HALO_MAC_BSP_LOG (diagnostic): the scenario's structure bsp switch trigger
       volumes (volume, source and destination bsp, cutscene flag the player
       is moved to) with the volume's position and the flag's, once */
    if (getenv("HALO_MAC_BSP_LOG") && context->frames == 0) {
        struct scenario *scenario = global_scenario_get();
        struct mac_bsp_switch_volume { short volume, source, destination, flag; } *switches =
            (struct mac_bsp_switch_volume *)scenario->bsp_switch_trigger_volumes.address;
        long index;
        fprintf(stderr, "bsp: %ld structure bsps, %ld switch trigger volumes\n",
            (long)scenario->structure_bsp_references.count, (long)scenario->bsp_switch_trigger_volumes.count);
        for (index = 0; index < scenario->bsp_switch_trigger_volumes.count; ++index) {
            struct scenario_trigger_volume *volume = switches[index].volume >= 0 &&
                switches[index].volume < scenario->trigger_volumes.count ?
                &((struct scenario_trigger_volume *)scenario->trigger_volumes.address)[switches[index].volume] : NULL;
            struct scenario_cutscene_flag *flag = switches[index].flag >= 0 &&
                switches[index].flag < scenario->cutscene_flags.count ?
                &((struct scenario_cutscene_flag *)scenario->cutscene_flags.address)[switches[index].flag] : NULL;
            fprintf(stderr, "bsp: switch %ld: volume %d (%s) at (%.2f,%.2f,%.2f) extents (%.2f,%.2f,%.2f), bsp %d -> %d, flag %d (%s) at (%.2f,%.2f,%.2f)\n",
                index, (int)switches[index].volume, volume ? volume->name : "?",
                volume ? volume->position.x : 0.0f, volume ? volume->position.y : 0.0f, volume ? volume->position.z : 0.0f,
                volume ? volume->extents.i : 0.0f, volume ? volume->extents.j : 0.0f, volume ? volume->extents.k : 0.0f,
                (int)switches[index].source, (int)switches[index].destination, (int)switches[index].flag,
                flag ? flag->name : "?", flag ? flag->position.x : 0.0f, flag ? flag->position.y : 0.0f,
                flag ? flag->position.z : 0.0f);
        }
    }
    /* HALO_MAC_BSP_LOG=2: the encounters' names too */
    if (getenv("HALO_MAC_BSP_LOG") && atoi(getenv("HALO_MAC_BSP_LOG")) >= 2 && context->frames == 0) {
        struct scenario *scenario = global_scenario_get();
        long index;
        for (index = 0; index < scenario->ai_encounters.count; ++index)
            fprintf(stderr, "bsp: encounter %ld %s\n", index, ((struct encounter_definition *)scenario->ai_encounters.address)[index].name);
    }
    /* HALO_MAC_BSP_LOG=2: every trigger volume too (name, position, extents) */
    if (getenv("HALO_MAC_BSP_LOG") && atoi(getenv("HALO_MAC_BSP_LOG")) >= 2 && context->frames == 0) {
        struct scenario *scenario = global_scenario_get();
        long index;
        for (index = 0; index < scenario->trigger_volumes.count; ++index) {
            struct scenario_trigger_volume *volume = &((struct scenario_trigger_volume *)scenario->trigger_volumes.address)[index];
            fprintf(stderr, "bsp: volume %ld %s at (%.2f,%.2f,%.2f) extents (%.2f,%.2f,%.2f)\n", index, volume->name,
                volume->position.x, volume->position.y, volume->position.z, volume->extents.i, volume->extents.j, volume->extents.k);
        }
    }
    /* HALO_MAC_GOD (diagnostic): the player cannot die (health and shield
       are filled every frame), so that a long unattended run goes on */
    if (getenv("HALO_MAC_GOD")) {
        cheat.deathless_player = TRUE;
        long god_unit = player_control_get_unit_index(0);
        static long god_last_unit = NONE;
        static real_point3d god_last_position;
        if (god_unit == NONE && god_last_unit != NONE)
            fprintf(stderr, "campaign: the player's unit is gone at tick %u (last seen at (%.2f,%.2f,%.2f), bsp %d)\n",
                context->ticks, god_last_position.x, god_last_position.y, god_last_position.z, (int)global_structure_bsp_index);
        god_last_unit = god_unit;
        if (god_unit != NONE) {
            god_last_position = object_get(god_unit)->object.position;
            struct object_datum *god_object = object_get(god_unit);
            god_object->object.body_vitality = 1.0f;
            god_object->object.shield_vitality = 1.0f;
        }
    }
    /* HALO_MAC_VOLUME_TOUR=<first tick>:<interval>[:<first volume>[:<last volume>]]
       (diagnostic): from the first tick, every interval the player is put in
       the centre of the next trigger volume of the scenario, which is what a
       player walking the level does to its scripts (volume_test_objects, bsp
       switches). The cinematic and script camera are not touched. */
    if (getenv("HALO_MAC_VOLUME_TOUR")) {
        unsigned tour_first = 0, tour_interval = 0;
        int tour_from = 0, tour_to = 100000;
        struct scenario *tour_scenario = global_scenario_get();
        long tour_unit = player_control_get_unit_index(0);
        sscanf(getenv("HALO_MAC_VOLUME_TOUR"), "%u:%u:%d:%d", &tour_first, &tour_interval, &tour_from, &tour_to);
        if (tour_interval && tour_scenario && tour_unit != NONE && context->ticks >= tour_first &&
            (context->ticks - tour_first) % tour_interval == 0) {
            long tour_index = tour_from + (long)((context->ticks - tour_first) / tour_interval);
            if (tour_index <= tour_to && tour_index < tour_scenario->trigger_volumes.count) {
                struct scenario_trigger_volume *volume =
                    &((struct scenario_trigger_volume *)tour_scenario->trigger_volumes.address)[tour_index];
                real_vector3d side;
                real_point3d at;
                real_vector3d forward, up;
                side.i = volume->up.j * volume->forward.k - volume->up.k * volume->forward.j;
                side.j = volume->up.k * volume->forward.i - volume->up.i * volume->forward.k;
                side.k = volume->up.i * volume->forward.j - volume->up.j * volume->forward.i;
                at.x = volume->position.x + 0.5f * (volume->extents.i * volume->forward.i +
                    volume->extents.j * side.i + volume->extents.k * volume->up.i);
                at.y = volume->position.y + 0.5f * (volume->extents.i * volume->forward.j +
                    volume->extents.j * side.j + volume->extents.k * volume->up.j);
                at.z = volume->position.z + 0.5f * (volume->extents.i * volume->forward.k +
                    volume->extents.j * side.k + volume->extents.k * volume->up.k);
                forward.i = 0.0f; forward.j = 1.0f; forward.k = 0.0f;
                up.i = 0.0f; up.j = 0.0f; up.k = 1.0f;
                if (object_get(tour_unit)->object.parent_object_index != NONE)
                    unit_exit_seat_end(tour_unit);
                {
                    /* (a volume of another bsp, or one in the air outside this
                       one, would take the player out of the world, which ends
                       his unit: the player is put back) */
                    real_point3d before = object_get(tour_unit)->object.position;
                    object_set_position(tour_unit, &at, &forward, &up);
                    if (object_get(tour_unit)->object.location.cluster_index < 0) {
                        object_set_position(tour_unit, &before, &forward, &up);
                        fprintf(stderr, "campaign: tour volume %ld %s: (%.2f,%.2f,%.2f) is outside bsp %d, skipped\n",
                            tour_index, volume->name, at.x, at.y, at.z, (int)global_structure_bsp_index);
                    } else
                        fprintf(stderr, "campaign: tour volume %ld %s: player moved to (%.2f,%.2f,%.2f) tick %u bsp %d\n",
                            tour_index, volume->name, at.x, at.y, at.z, context->ticks, (int)global_structure_bsp_index);
                }
            }
        }
    }
    /* HALO_MAC_PLAYER_AT=<tick>:<x>:<y>:<z>[,<tick>:<x>:<y>:<z>...] (diagnostic):
       the player is put at that point at that tick (in the bsp then loaded) */
    if (getenv("HALO_MAC_PLAYER_AT")) {
        const char *entry = getenv("HALO_MAC_PLAYER_AT");
        long unit_index = player_control_get_unit_index(0);
        while (entry && *entry) {
            unsigned at_tick;
            float x, y, z;
            if (sscanf(entry, "%u:%f:%f:%f", &at_tick, &x, &y, &z) == 4 && context->ticks == at_tick &&
                unit_index != NONE) {
                real_point3d at;
                real_vector3d forward, up;
                at.x = x; at.y = y; at.z = z;
                forward.i = 0.0f; forward.j = 1.0f; forward.k = 0.0f;
                up.i = 0.0f; up.j = 0.0f; up.k = 1.0f;
                object_set_position(unit_index, &at, &forward, &up);
                fprintf(stderr, "campaign: player moved to (%.2f,%.2f,%.2f)\n", x, y, z);
            }
            entry = strchr(entry, ',');
            if (entry)
                ++entry;
        }
    }
    /* HALO_MAC_PLAYER_AT_WEAPON=<tick>[:<skip>] (diagnostic): the player is put beside
       the first weapon lying on the ground at that tick, so the action
       prompt ("Press ... to pick up") shows */
    if (getenv("HALO_MAC_PLAYER_AT_WEAPON") && context->ticks == (unsigned)atoi(getenv("HALO_MAC_PLAYER_AT_WEAPON"))) {
        long unit_index = player_control_get_unit_index(0);
        struct object_iterator weapons;
        struct object_datum *weapon_object;
        const char *skip_text = strchr(getenv("HALO_MAC_PLAYER_AT_WEAPON"), ':');
        int skip = skip_text ? atoi(skip_text + 1) : 0;
        object_iterator_new(&weapons, FLAG(_object_type_weapon), 0);
        while (unit_index != NONE && (weapon_object = (struct object_datum *)object_iterator_next(&weapons)) != NULL) {
            if (weapon_object->object.parent_object_index == NONE && weapon_object->object.location.cluster_index >= 0) {
                fprintf(stderr, "prompts: weapon %08lx at (%.2f,%.2f,%.2f)\n", (unsigned long)weapons.index,
                    weapon_object->object.position.x, weapon_object->object.position.y, weapon_object->object.position.z);
                if (skip-- > 0)
                    continue;
                real_point3d at = weapon_object->object.position;
                real_vector3d forward, up;
                at.x -= 1.0f;
                at.z += 0.5f;
                forward.i = 1.0f; forward.j = 0.0f; forward.k = 0.0f;
                up.i = 0.0f; up.j = 0.0f; up.k = 1.0f;
                object_set_position(unit_index, &at, &forward, &up);
                fprintf(stderr, "prompts: player moved to weapon %08lx at (%.2f,%.2f,%.2f)\n", (unsigned long)weapons.index,
                    at.x, at.y, at.z);
                break;
            }
        }
    }
    /* HALO_MAC_SPAWN=<tick>:<v|w>:<palette index>[:<distance>[:<up>]][,...]
       (diagnostic, gameplay QA): a vehicle or weapon of the scenario's palette
       is made in front of the player (the palettes are logged at the first
       tick of the game). HALO_MAC_NEAR=<tick>:<v|w>[:<skip>] puts the player
       beside the Nth such object lying free in the world. HALO_MAC_BOARD=
       <tick>:<seat label> puts the player into the nearest vehicle's seat
       (the script function unit_enter_vehicle). HALO_MAC_ACTIONS=<tick>:
       <ticks>:<name>[+<name>...][,...] holds controls: fire grenade melee
       reload zoom action flash switch nade fwd back left right jump crouch
       and yaw=<radians>/pitch=<radians>. HALO_MAC_UNIT_LOG=<interval> logs
       the player's unit, vehicle and weapons. */
    if ((getenv("HALO_MAC_GRENADES") || getenv("HALO_MAC_SPAWN") || getenv("HALO_MAC_SPAWN_TAG") || getenv("HALO_MAC_NEAR") || getenv("HALO_MAC_BOARD") || getenv("HALO_MAC_UNIT_LOG")) &&
        global_scenario_get() && player_control_get_unit_index(0) != NONE) {
        long unit_index = player_control_get_unit_index(0);
        struct scenario *qa_scenario = global_scenario_get();
        const char *entry;
        if (context->ticks == 1 && getenv("HALO_MAC_SPAWN")) {
            long palette_index;
            for (palette_index = 0; palette_index < qa_scenario->vehicle_palette.count; ++palette_index)
                fprintf(stderr, "qa: vehicle palette %ld %s\n", palette_index,
                    ((struct scenario_object_palette_entry *)qa_scenario->vehicle_palette.address)[palette_index].reference.name);
            for (palette_index = 0; palette_index < qa_scenario->weapon_palette.count; ++palette_index)
                fprintf(stderr, "qa: weapon palette %ld %s\n", palette_index,
                    ((struct scenario_object_palette_entry *)qa_scenario->weapon_palette.address)[palette_index].reference.name);
        }
        entry = getenv("HALO_MAC_SPAWN");
        while (entry && *entry) {
            unsigned at_tick;
            char kind;
            long palette_index;
            float distance = 4.0f, up = 1.0f;
            if (sscanf(entry, "%u:%c:%ld:%f:%f", &at_tick, &kind, &palette_index, &distance, &up) >= 3 && at_tick == context->ticks) {
                struct tag_block *palette = kind == 'v' ? &qa_scenario->vehicle_palette : &qa_scenario->weapon_palette;
                if (palette_index >= 0 && palette_index < palette->count) {
                    struct object_datum *player_object = object_get(unit_index);
                    struct object_placement_data placement;
                    long made;
                    object_placement_data_new(&placement,
                        ((struct scenario_object_palette_entry *)palette->address)[palette_index].reference.index, NONE);
                    placement.position = player_object->object.position;
                    placement.position.y += distance;
                    placement.position.z += up;
                    made = object_new(&placement);
                    fprintf(stderr, "qa: spawned %s %ld at (%.2f,%.2f,%.2f): object %08lx\n",
                        ((struct scenario_object_palette_entry *)palette->address)[palette_index].reference.name, palette_index,
                        placement.position.x, placement.position.y, placement.position.z, (unsigned long)made);
                }
            }
            entry = strchr(entry, ',');
            if (entry)
                ++entry;
        }
        /* HALO_MAC_SPAWN_TAG=<tick>:<tag name>[:<distance>[:<up>]] (a weapon, vehicle
           or other object tag loaded in the map, by name) */
        entry = getenv("HALO_MAC_SPAWN_TAG");
        while (entry && *entry) {
            unsigned at_tick = (unsigned)atoi(entry);
            const char *colon = strchr(entry, ':');
            const char *end = strchr(entry, ',');
            if (colon && at_tick == context->ticks) {
                char name[128];
                char *more;
                float distance = 1.0f, up = 0.5f;
                size_t length = end ? (size_t)(end - colon - 1) : strlen(colon + 1);
                if (length >= sizeof(name))
                    length = sizeof(name) - 1;
                memcpy(name, colon + 1, length);
                name[length] = 0;
                more = strchr(name, ':');
                if (more) {
                    *more++ = 0;
                    sscanf(more, "%f:%f", &distance, &up);
                }
                {
                    static const unsigned long groups[] = { 'weap', 'vehi', 'eqip', 'scen' };
                    long tag = NONE;
                    int group_index;
                    for (group_index = 0; group_index < 4 && tag == NONE; ++group_index)
                        tag = tag_loaded((long)groups[group_index], name);
                    if (tag == NONE)
                        fprintf(stderr, "qa: tag %s is not loaded in this map\n", name);
                    else {
                        struct object_placement_data placement;
                        struct object_datum *player_object = object_get(unit_index);
                        long made;
                        object_placement_data_new(&placement, tag, NONE);
                        placement.position = player_object->object.position;
                        placement.position.y += distance;
                        placement.position.z += up;
                        made = object_new(&placement);
                        fprintf(stderr, "qa: spawned %s: object %08lx (made as %s)\n", name, (unsigned long)made,
                            made != NONE ? tag_get_name(object_get(made)->definition_index) : "nothing");
                    }
                }
            }
            entry = end ? end + 1 : NULL;
        }
        entry = getenv("HALO_MAC_NEAR");
        while (entry && *entry) {
            unsigned at_tick;
            char kind;
            int skip = 0;
            if (sscanf(entry, "%u:%c:%d", &at_tick, &kind, &skip) >= 2 && at_tick == context->ticks) {
                struct object_iterator found;
                struct object_datum *found_object;
                object_iterator_new(&found, kind == 'v' ? FLAG(_object_type_vehicle) : FLAG(_object_type_weapon), 0);
                while ((found_object = (struct object_datum *)object_iterator_next(&found)) != NULL) {
                    if (found_object->object.parent_object_index == NONE && found_object->object.location.cluster_index >= 0 && skip-- <= 0) {
                        real_point3d at = found_object->object.position;
                        real_vector3d forward, up;
                        at.x -= kind == 'v' ? 3.0f : 1.0f;
                        at.z += 1.0f;
                        forward.i = 1.0f; forward.j = 0.0f; forward.k = 0.0f;
                        up.i = 0.0f; up.j = 0.0f; up.k = 1.0f;
                        object_set_position(unit_index, &at, &forward, &up);
                        fprintf(stderr, "qa: player moved beside %s %08lx (%s) at (%.2f,%.2f,%.2f)\n", kind == 'v' ? "vehicle" : "weapon",
                            (unsigned long)found.index, tag_get_name(found_object->definition_index),
                            found_object->object.position.x, found_object->object.position.y, found_object->object.position.z);
                        break;
                    }
                }
            }
            entry = strchr(entry, ',');
            if (entry)
                ++entry;
        }
        entry = getenv("HALO_MAC_BOARD");
        while (entry && *entry) {
            unsigned at_tick = (unsigned)atoi(entry);
            const char *colon = strchr(entry, ':');
            const char *end = strchr(entry, ',');
            if (colon && at_tick == context->ticks) {
                char seat[64];
                size_t length = end ? (size_t)(end - colon - 1) : strlen(colon + 1);
                struct object_iterator found;
                struct object_datum *found_object;
                struct object_datum *player_object = object_get(unit_index);
                long best = NONE;
                real best_distance = 1.0e30f;
                if (length >= sizeof(seat))
                    length = sizeof(seat) - 1;
                memcpy(seat, colon + 1, length);
                seat[length] = 0;
                object_iterator_new(&found, FLAG(_object_type_vehicle), 0);
                while ((found_object = (struct object_datum *)object_iterator_next(&found)) != NULL) {
                    real dx = found_object->object.position.x - player_object->object.position.x;
                    real dy = found_object->object.position.y - player_object->object.position.y;
                    real dz = found_object->object.position.z - player_object->object.position.z;
                    real d = dx * dx + dy * dy + dz * dz;
                    if (d < best_distance && found_object->object.location.cluster_index >= 0)
                        best = found.index, best_distance = d;
                }
                if (best != NONE) {
                    {
                        struct unit_definition *vehicle_definition = unit_definition_get(object_get(best)->definition_index);
                        short seat_index;
                        for (seat_index = 0; seat_index < vehicle_definition->unit.seats.count; ++seat_index) {
                            struct unit_seat *vehicle_seat = TAG_BLOCK_GET_ELEMENT(&vehicle_definition->unit.seats, seat_index, struct unit_seat);
                            long occupant;
                            fprintf(stderr, "qa: seat %d label %s can_enter %d\n", (int)seat_index, vehicle_seat->label,
                                (int)unit_can_enter_seat(unit_index, best, seat_index, &occupant));
                        }
                    }
                    unit_scripting_enter_vehicle(unit_index, best, seat);
                    fprintf(stderr, "qa: board vehicle %08lx (%s) seat %s: parent %08lx seat index %d\n", (unsigned long)best,
                        tag_get_name(object_get(best)->definition_index), seat,
                        (unsigned long)object_get(unit_index)->object.parent_object_index,
                        (int)((struct unit_datum *)object_get(unit_index))->unit.parent_seat_index);
                } else
                    fprintf(stderr, "qa: board: no vehicle\n");
            }
            entry = end ? end + 1 : NULL;
        }
        /* HALO_MAC_GRENADES=<count> (diagnostic): the player has that many grenades of each type from the first tick */
        if (getenv("HALO_MAC_GRENADES") && context->ticks <= 5) {
            struct unit_datum *grenade_unit = (struct unit_datum *)object_get(unit_index);
            grenade_unit->unit.grenade_counts[0] = grenade_unit->unit.grenade_counts[1] = (char)atoi(getenv("HALO_MAC_GRENADES"));
        }
        if (getenv("HALO_MAC_UNIT_LOG") && atoi(getenv("HALO_MAC_UNIT_LOG")) > 0 &&
            context->ticks % (unsigned)atoi(getenv("HALO_MAC_UNIT_LOG")) == 0) {
            struct unit_datum *unit = (struct unit_datum *)object_get(unit_index);
            short slot;
            fprintf(stderr, "qa: tick %u unit %08lx cluster %d dmgflags %04lx at (%.2f,%.2f,%.2f) hp %.2f/%.2f parent %08lx seat %d grenades %d/%d weapon %d",
                context->ticks, (unsigned long)unit_index, (int)unit->object.location.cluster_index, (unsigned long)unit->object.damage_flags,
                unit->object.position.x, unit->object.position.y, unit->object.position.z,
                unit->object.body_vitality, unit->object.shield_vitality, (unsigned long)unit->object.parent_object_index,
                (int)unit->unit.parent_seat_index, (int)unit->unit.grenade_counts[0], (int)unit->unit.grenade_counts[1],
                (int)unit->unit.current_weapon_index);
            for (slot = 0; slot < MAXIMUM_WEAPONS_PER_UNIT; ++slot)
                if (unit->unit.weapon_object_indices[slot] != NONE) {
                    struct weapon_datum *weapon = (struct weapon_datum *)object_get(unit->unit.weapon_object_indices[slot]);
                    fprintf(stderr, " [%s %d+%d state %d]", tag_get_name(weapon->definition_index),
                        (int)weapon->weapon.magazines[0].rounds_loaded, (int)weapon->weapon.magazines[0].rounds_total,
                        (int)weapon->weapon.state);
                }
            {
                struct object_iterator free_weapons;
                struct object_datum *free_weapon;
                object_iterator_new(&free_weapons, FLAG(_object_type_weapon), 0);
                while ((free_weapon = (struct object_datum *)object_iterator_next(&free_weapons)) != NULL)
                    if (free_weapon->object.parent_object_index == NONE && getenv("HALO_MAC_UNIT_LOG_FREE") &&
                        (free_weapon->object.position.x - unit->object.position.x) * (free_weapon->object.position.x - unit->object.position.x) +
                        (free_weapon->object.position.y - unit->object.position.y) * (free_weapon->object.position.y - unit->object.position.y) < 100.0f)
                        fprintf(stderr, " {free %s at (%.2f,%.2f,%.2f)}", tag_get_name(free_weapon->definition_index),
                            free_weapon->object.position.x, free_weapon->object.position.y, free_weapon->object.position.z);
            }
            if (unit->object.parent_object_index != NONE) {
                struct object_datum *vehicle = object_get(unit->object.parent_object_index);
                fprintf(stderr, " vehicle %s at (%.2f,%.2f,%.2f)", tag_get_name(vehicle->definition_index),
                    vehicle->object.position.x, vehicle->object.position.y, vehicle->object.position.z);
            }
            fprintf(stderr, "\n");
        }
    }
    if (getenv("HALO_MAC_ACTIONS")) {
        const char *entry = getenv("HALO_MAC_ACTIONS");
        while (entry && *entry) {
            unsigned at_tick, length;
            int consumed = 0;
            if (sscanf(entry, "%u:%u:%n", &at_tick, &length, &consumed) >= 2 && consumed > 0 &&
                context->ticks >= at_tick && context->ticks < at_tick + length) {
                const char *name = entry + consumed;
                while (*name && *name != ',') {
                    if (!strncmp(name, "fire", 4))
                        tick_input.held |= MAC_INPUT_FIRE, tick_input.mouse_buttons |= 1U;
                    else if (!strncmp(name, "grenade", 7))
                        tick_input.mouse_buttons |= 2U;
                    else if (!strncmp(name, "melee", 5))
                        tick_input.keys[3] = 1;
                    else if (!strncmp(name, "reload", 6))
                        tick_input.keys[15] = 1;
                    else if (!strncmp(name, "zoom", 4))
                        tick_input.keys[6] = 1;
                    else if (!strncmp(name, "action", 6))
                        tick_input.keys[14] = 1, tick_input.held |= MAC_INPUT_ACTION;
                    else if (!strncmp(name, "flash", 5))
                        tick_input.keys[12] = 1;
                    else if (!strncmp(name, "switch", 6))
                        tick_input.keys[18] = 1;
                    else if (!strncmp(name, "nade", 4))
                        tick_input.keys[7] = 1;
                    else if (!strncmp(name, "fwd", 3))
                        tick_input.keys[13] = 1;
                    else if (!strncmp(name, "back", 4))
                        tick_input.keys[1] = 1;
                    else if (!strncmp(name, "left", 4))
                        tick_input.keys[0] = 1;
                    else if (!strncmp(name, "right", 5))
                        tick_input.keys[2] = 1;
                    else if (!strncmp(name, "jump", 4))
                        tick_input.keys[49] = 1;
                    else if (!strncmp(name, "crouch", 6))
                        tick_input.keys[8] = 1;
                    else if (!strncmp(name, "yaw=", 4))
                        tick_input.yaw = (float)atof(name + 4);
                    else if (!strncmp(name, "pitch=", 6))
                        tick_input.pitch = (float)atof(name + 6);
                    while (*name && *name != '+' && *name != ',')
                        ++name;
                    if (*name == '+')
                        ++name;
                }
            }
            entry = strchr(entry, ',');
            if (entry)
                ++entry;
        }
    }
    /* HALO_MAC_RUN_SCRIPT=<tick>:<name>[,<tick>:<name>...] (diagnostic): the
       scenario's script of that name is started at that tick, as a console
       command would (the script's own switch_bsp, cutscenes and AI run) */
    if (getenv("HALO_MAC_RUN_SCRIPT")) {
        const char *entry = getenv("HALO_MAC_RUN_SCRIPT");
        while (entry && *entry) {
            unsigned at_tick = (unsigned)atoi(entry);
            const char *colon = strchr(entry, ':');
            const char *end = strchr(entry, ',');
            if (colon && (!end || colon < end) && context->ticks == at_tick) {
                char name[64];
                size_t length = end ? (size_t)(end - colon - 1) : strlen(colon + 1);
                if (length >= sizeof(name))
                    length = sizeof(name) - 1;
                memcpy(name, colon + 1, length);
                name[length] = 0;
                fprintf(stderr, "campaign: script %s started (%d)\n", name, (int)hs_evaluate_by_name(name));
            }
            entry = end ? end + 1 : NULL;
        }
    }
    /* HALO_MAC_RUN_DORMANT=<first tick>:<interval>[:<first>] (diagnostic): from
       the first tick, every interval the next dormant script of the scenario
       is started (the mission's scripts are woken by one another as the
       player plays; this wakes each in turn, whatever the player has done) */
    if (getenv("HALO_MAC_RUN_DORMANT")) {
        unsigned dormant_first = 0, dormant_interval = 0;
        int dormant_from = 0;
        struct scenario *dormant_scenario = global_scenario_get();
        sscanf(getenv("HALO_MAC_RUN_DORMANT"), "%u:%u:%d", &dormant_first, &dormant_interval, &dormant_from);
        if (dormant_interval && dormant_scenario && context->ticks >= dormant_first &&
            (context->ticks - dormant_first) % dormant_interval == 0) {
            long wanted = dormant_from + (long)((context->ticks - dormant_first) / dormant_interval);
            long script_index, seen = 0;
            struct hs_script *dormant_scripts = (struct hs_script *)dormant_scenario->hs_scripts.address;
            for (script_index = 0; script_index < dormant_scenario->hs_scripts.count; ++script_index) {
                if (dormant_scripts[script_index].script_type != 1)
                    continue;
                if (seen++ == wanted) {
                    fprintf(stderr, "campaign: dormant script %ld %s started (%d) at tick %u bsp %d\n", wanted,
                        dormant_scripts[script_index].name,
                        (int)hs_evaluate_by_name(dormant_scripts[script_index].name), context->ticks,
                        (int)global_structure_bsp_index);
                    break;
                }
            }
        }
    }
    /* HALO_MAC_SWITCH_BSP=<tick>:<index> (diagnostic): the scenario switches
       structure bsp at that tick, as a script's switch_bsp would */
    if (getenv("HALO_MAC_SWITCH_BSP")) {
        unsigned at_tick = (unsigned)atoi(getenv("HALO_MAC_SWITCH_BSP"));
        const char *colon = strchr(getenv("HALO_MAC_SWITCH_BSP"), ':');
        if (colon && context->ticks == at_tick) {
            boolean switched = scenario_switch_structure_bsp((short)atoi(colon + 1));
            fprintf(stderr, "campaign: switch_bsp %s -> %d (bsp now %d)\n", colon + 1, (int)switched, (int)global_structure_bsp_index);
        }
    }
    /* port: the netcode, in main.c's order (source/main/main.c's loop). The
       test hosts or joins, voice and map sharing run in the lobby and in the
       game, and the host's and the client's frames bracket the tick below:
       input, then what this machine sends, then the tick that consumes it. */
    if (mac_interactive_network_start_frame(context) && mac_engine_menu_enabled() && !context->request) {
        int request = mac_engine_menu_poll();
        if (request)
            context->request = (unsigned)request;
        context->accumulator = 0.0;
    }
    /* port: the network test's game is over (a game that ended, or a host
       that went down): the session stops as Escape stops it */
    if (mac_engine_network_test_game_over()) {
        context->quit_requested = 1;
        platform_request_quit();
        return 0;
    }
    /* port: the script console (debug.telnet_console), once a frame as main.c's loop does */
    telnet_console_process();
    phase_tick_at = mac_phase_enabled() ? mac_interactive_microseconds() : 0.0;
    tick_started = system_milliseconds();
    while (context->accumulator + 0.0000001 >= MAC_INTERACTIVE_TICK &&
        tick_count < 3) {
        if (tick_count)
            mac_interactive_clear_edges(&tick_input);
        /* port: a switch_bsp or BSP trigger volume's request is applied
           at the head of a tick, as main's loop does */
        mac_engine_main_apply_bsp_switch();
        if (!mac_engine_menu_active() && mac_interactive_checkpoints_enabled())
            mac_interactive_lifecycle(context, 0);
        mac_engine_menu_input(&tick_input);
        mac_engine_input_publish(&tick_input);
        input_frame_begin();
        input_update();
        input_abstraction_update();
        console_open = 0;
        /* the menus (HALO_MAC_MENU=1) take the input; no player is controlled */
        if (mac_engine_menu_active())
            mac_engine_menu_tick();
        else {
            /* port: the developer console (the backquote key) and the cheats, in main.c's order. While the console is
               open on a local game the player's controls and the game's time stand still, as main.c's loop leaves them. */
            console_open = 0;
            if (game_in_progress()) {
                terminal_update();
                console_open = console_update() && game_connection() == _game_connection_local;
                if (console_open != mac_console_was_open)
                    fprintf(stderr, "console: %s at tick %u\n", console_open ? "open" : "closed", context->ticks);
                mac_console_was_open = console_open;
                if (!console_open)
                    cheats_update();
            }
            if (!console_open)
                (void)player_control_update((real)MAC_INTERACTIVE_TICK);
            /* (in a game, Escape opens the pause menu) */
            if (mac_engine_menu_enabled())
                mac_engine_menu_tick();
        }
        if (mac_engine_menu_enabled() && !context->request) {
            int request = mac_engine_menu_poll();
            if (request)
                context->request = (unsigned)request;
        }
        mac_interactive_network_end_frame(context);
        if (!console_open)
            game_time_update((real)MAC_INTERACTIVE_TICK);
        /* (HALO_MAC_KILL_PLAYER=<tick>[,<tick>...] (diagnostic): the player is killed then) */
        mac_interactive_kill_player(context);
        if (!mac_engine_menu_active() && mac_interactive_checkpoints_enabled())
            mac_interactive_lifecycle(context, 1);
        mac_interactive_trace_tick(context);
        mac_interactive_sim_digest(context);
        mac_interactive_update_native_camera();
        input_frame_end();
        context->accumulator -= MAC_INTERACTIVE_TICK;
        ++tick_count;
        ++context->ticks;
        mac_interactive_trace_rng(context, "tick");
    }
    context->tick_milliseconds += system_milliseconds() - tick_started;
    if (phase_tick_at != 0.0)
        phase_render_at = mac_interactive_microseconds();
    /* HALO_MAC_HUD_MESSAGE=<index>|sweep (diagnostic): the HUD's help
       message of that index (sweepN: the next one every 30 ticks, the first at tick 30*N) is set every
       tick, to draw its button prompts */
    if (getenv("HALO_MAC_HUD_MESSAGE") && player_control_get_unit_index(0) != NONE && context->ticks >= (getenv("HALO_MAC_HUD_MESSAGE")[0] == 's' && getenv("HALO_MAC_HUD_MESSAGE")[1] ? 30u * (unsigned)atoi(getenv("HALO_MAC_HUD_MESSAGE") + 1) : 0u)) {
        const char *value = getenv("HALO_MAC_HUD_MESSAGE");
        short index = value[0] == 's' ? (short)(context->ticks / 30 - (value[1] ? atoi(value + 1) : 0)) : (short)atoi(value);
        if (context->ticks % 30 == 0)
            fprintf(stderr, "prompts: hud message %d from tick %u\n", (int)index, context->ticks);
        hud_set_state_message(0, index);
    }
    if (getenv("HALO_MAC_CUTSCENE_LOG") && tick_count) {
        unsigned every = (unsigned)atoi(getenv("HALO_MAC_CUTSCENE_LOG"));
        if (every && context->ticks / every != (context->ticks - tick_count) / every)
            mac_interactive_cutscene_log(context->ticks);
    }
    if (getenv("HALO_MAC_RADAR_LOG") && tick_count && context->ticks / 30 != (context->ticks - tick_count) / 30)
        mac_interactive_radar_report(context->ticks);
    if (getenv("HALO_MAC_AI_LOG") && tick_count && context->ticks / 30 != (context->ticks - tick_count) / 30) {
        mac_interactive_ai_report(context->ticks, context->tick_milliseconds);
        context->tick_milliseconds = 0;
    }

    render_interpolation_frame_begin();
    mac_engine_main_game_render(MAC_INTERACTIVE_TICK);
    render_interpolation_frame_end();
    if (phase_tick_at != 0.0)
        phase_present_at = mac_interactive_microseconds();
    mac_interactive_trace_rng(context, "render");
    /* The caller reads the last bounded frame's pixels. */
    /* HALO_MAC_SHOT_EVERY (diagnostic): a capture every that many frames
       (<capture>-<n>.ppm) */
    if (getenv("HALO_MAC_SHOT_EVERY") && atoi(getenv("HALO_MAC_SHOT_EVERY")) > 0 &&
        context->frames % (unsigned)atoi(getenv("HALO_MAC_SHOT_EVERY")) == 0)
        shot = 1;
    /* port: the screenshot key, once for each press */
    screenshot = mac_engine_input_screenshot_pressed();
    if ((context->max_frames && context->frames + 1 == context->max_frames) || shot || screenshot || context->request)
        mac_device_gpu_request_readback();
    main_present_frame();
    if (phase_tick_at != 0.0) {
        double phase_end = mac_interactive_microseconds();
        mac_phase_frame(context->frames + 1, (phase_render_at - phase_tick_at) / 1000.0, (phase_present_at - phase_render_at) / 1000.0,
            (phase_end - phase_present_at) / 1000.0, elapsed * 1000.0);
    }
    if (shot)
        mac_interactive_write_shot();
    if (screenshot)
        mac_interactive_write_screenshot();
    mac_interactive_trace_rng(context, "present");
    ++context->frames;
    if(context->frames==1&&mac_render_timing_enabled()) {
        fprintf(stderr,"native render timing: first frame (startup included)\n");
        mac_render_timing_report();
        mac_render_timing_init();
        fprintf(stderr,"native render timing: subsequent frame counters reset\n");
        context->warm_audio_valid = mac_dsound_audio_statistics(&context->warm_audio);
    }
    return 1;
}

int mac_session_interactive_run(unsigned max_frames, int visible,
    struct mac_session_interactive_stats *stats)
{
    struct mac_interactive_context context;
    int frame_result;
    unsigned long start_time, first_frame_ms = 0;
    const char *fixed_step;
    memset(&context, 0, sizeof(context));
    if ((visible != 0 && visible != 1) || (!max_frames && !visible)) {
        if (stats) {
            stats->frames = 0;
            stats->ticks = 0;
            stats->quit_requested = 0;
            stats->request = 0;
        }
        return 0;
    }
    context.max_frames = max_frames;
    /* port: the node matrices of objects that nothing moves are kept (source/objects/objects.c); HALO_MAC_PERF_NODE_STAMPS=0 or
       HALO_MAC_PERF=0 computes them every tick as the game does */
    object_node_stamps_set(getenv("HALO_MAC_PERF_NODE_STAMPS") && getenv("HALO_MAC_PERF_NODE_STAMPS")[0] == '2' ?
        2 : mac_perf_flag("HALO_MAC_PERF_NODE_STAMPS") ? 1 : 0);
    /* port: the developer console's state, as main.c sets it before the game starts (the game is already initialized) */
    {
        static int console_ready;
        if (!console_ready) {
            console_initialize();
            console_ready = 1;
        }
    }
    /* port: the Mac's shader translator makes no per-pixel lighting (mac_nv2a_arbfp.c refuses it), so the setting is
       read by no one; the log says so when a config.toml has it on (the start window shows the row disabled) */
    if (config_boolean("display.per_pixel_lighting"))
        fprintf(stderr, "display.per_pixel_lighting is on, but this renderer does not draw per-pixel lighting yet: ignored\n");
    /* A bounded hidden image-comparison probe advances the real engine once
       per captured frame, independently of driver speed. Visible play always
       retains wall-clock scheduling. */
    context.sweep = getenv("HALO_MAC_RENDER_SWEEP") != NULL;
    fixed_step = getenv("HALO_MAC_RENDER_FIXED_STEP");
    context.fixed_step = !visible && max_frames && fixed_step &&
        fixed_step[0] == '1' && fixed_step[1] == '\0';
    if (context.fixed_step)
        fprintf(stderr,"native rendered probe: fixed step, one engine tick per frame\n");
    context.trace_enabled = getenv("HALO_MAC_SESSION_TRACE") != NULL;
    {
        const char *rng_trace = getenv("HALO_MAC_RNG_TRACE");
        context.rng_trace = !visible && rng_trace &&
            rng_trace[0] == '1' && rng_trace[1] == '\0';
    }
    context.accumulator = MAC_INTERACTIVE_TICK;
    start_time = mac_interactive_now();
    context.last_time = start_time;
    frame_result = 1;
    while (frame_result && !context.request &&
        (!max_frames || context.frames < max_frames)) {
        if (!context.fixed_step && context.frames && context.accumulator < MAC_INTERACTIVE_TICK) {
            double needed_ms = (MAC_INTERACTIVE_TICK - context.accumulator) * 1000.0;
            unsigned long wait_ms = (unsigned long)needed_ms;
            unsigned long elapsed_ms =
                (unsigned long)(mac_interactive_now() - context.last_time);
            if ((double)wait_ms < needed_ms) ++wait_ms;
            if (elapsed_ms < wait_ms)
                mac_interactive_sleep(wait_ms - elapsed_ms);
        }
        frame_result = mac_device_gpu_frame(mac_interactive_frame, &context);
        if (context.frames == 1)
            first_frame_ms = (unsigned long)(mac_interactive_now() - start_time);
        if (context.failed)
            break;
    }
    if(!frame_result&&!context.failed&&mac_window_quit_requested_active())
        context.quit_requested=1;
    fprintf(stderr,"native rendered timing: frames=%u elapsed_ms=%lu first_frame_ms=%lu\n",
        context.frames,(unsigned long)(mac_interactive_now()-start_time),first_frame_ms);
    if (mac_phase_enabled())
        mac_phase_print("whole run after the first frame", mac_phase_run_frames, mac_phase_run_tick, mac_phase_run_render,
            mac_phase_run_present, mac_phase_run_total);
    object_node_stamps_report();
    mac_adaptive_detail_summary(); /* port: mac.adaptive_detail (port/mac/mac_gpu_info.c) */
    if (context.warm_audio_valid && context.frames > 1) {
        struct mac_dsound_audio_stats audio;
        if (mac_dsound_audio_statistics(&audio)) {
            double mix_ms = (double)(audio.mix_nanoseconds -
                context.warm_audio.mix_nanoseconds) / 1000000.0;
            fprintf(stderr,
                "native warm audio mix: frames=%u mix_ms=%.1f per_frame_ms=%.2f calls=%llu "
                "mixed_frames=%llu\n",
                context.frames - 1, mix_ms, mix_ms / (double)(context.frames - 1),
                (unsigned long long)(audio.mix_calls - context.warm_audio.mix_calls),
                (unsigned long long)(audio.mix_frames - context.warm_audio.mix_frames));
        }
    }
    mac_engine_input_publish(NULL);
    input_update();
    if (stats) {
        stats->frames = context.frames;
        stats->ticks = context.ticks;
        stats->quit_requested = context.quit_requested;
        stats->request = context.request;
    }
    if (context.failed)
        return 0;
    if (context.quit_requested || context.request)
        return 1;
    if (max_frames)
        return context.frames == max_frames && frame_result;
    return 0;
}
