/* MCC script functions the Xbox function table lacks, by MCC's editing kit's
 * hs_doc.txt (its names, return types and parameters), so that a map calling
 * them loads. Each is outside the unchanged native table and only an MCC
 * map's scripts (or the console while one is loaded) reach it.
 *
 * Those the engine can do are done: integer and real arithmetic, the
 * gravity and music gain the map set, whether a script started, printing to
 * the log, active camouflage and changing maps. The rest are MCC's debug,
 * console, film, event and PC settings commands: they evaluate their
 * arguments (so sleeps and side effects in them still happen), do nothing
 * else, and return zero or false (the master gain, one). The first call of
 * each in a run is noted in debug.txt. */
#include "cseries.h"
#include "errors.h"
#include "hs/hs.h"
#include "memory/data.h"
#include "game/game.h"
#include "game/players.h"
#include "objects/objects.h"
#include "units/units.h"
#include "game/cheats.h"
#include "mcc_cache.h"
#include "mcc_campaign.h"
#include "mcc_main.h"
#include "mcc_script_library.h"
#include <stdint.h>
#include <string.h>

boolean hs_macro_function_parse(short function, long expression);

struct mcc_library_definition {
    struct hs_function_definition function;
    short rest[2];
};

union mcc_library_word {
    long bits;
    real number;
};

static real mcc_library_real(long bits)
{
    union mcc_library_word word;
    word.bits = bits;
    return word.number;
}

static long mcc_library_bits(real number)
{
    union mcc_library_word word;
    word.number = number;
    return word.bits;
}

static char const *mcc_library_string(long value)
{
    return value ? (char const *)(uintptr_t)value : "";
}

static struct hs_function_definition *mcc_library_get(short function);

/* Evaluate the arguments; NULL until they are all evaluated (the caller
 * returns, and is called again). A function with none has nothing to wait for. */
static long *mcc_library_arguments(short function, long thread, boolean initialize)
{
    static long none[1];
    return mcc_library_get(function)->parameter_count ?
        hs_macro_function_evaluate(function, thread, initialize) : none;
}

static void mcc_library_ignore_evaluate(short function, long thread, boolean initialize)
{
    static byte reported[128];
    struct hs_function_definition *definition = mcc_library_get(function);
    short index = (short)(function - MCC_HS_LIBRARY_FIRST);
    if (!mcc_library_arguments(function, thread, initialize)) return;
    if (index >= 0 && index < (short)sizeof(reported) && !reported[index]) {
        reported[index] = TRUE;
        error(_error_silent, "mcc: %s has no effect in this port", definition->name);
    }
    hs_return(thread, 0);
}

/* MCC's pin: the first value, no less than the second and no more than the third. */
static void mcc_pin_evaluate(short function, long thread, boolean initialize)
{
    long *args = mcc_library_arguments(function, thread, initialize);
    real value, lower, upper;
    if (!args) return;
    value = mcc_library_real(args[0]);
    lower = mcc_library_real(args[1]);
    upper = mcc_library_real(args[2]);
    if (value < lower) value = lower;
    else if (value > upper) value = upper;
    hs_return(thread, mcc_library_bits(value));
}

/* 32-bit operations on unsigned values, as the other MCC bitwise functions. */
static void mcc_bitwise_xor_evaluate(short function, long thread, boolean initialize)
{
    long *args = mcc_library_arguments(function, thread, initialize);
    if (args) hs_return(thread, (long)((uint32_t)args[0] ^ (uint32_t)args[1]));
}

/* The value with the flags set (TRUE) or cleared (FALSE). */
static void mcc_bitwise_flags_evaluate(short function, long thread, boolean initialize)
{
    long *args = mcc_library_arguments(function, thread, initialize);
    uint32_t value, flags;
    if (!args) return;
    value = (uint32_t)args[0];
    flags = (uint32_t)args[1];
    hs_return(thread, (long)((boolean)args[2] ? value | flags : value & ~flags));
}

static void mcc_log_print_evaluate(short function, long thread, boolean initialize)
{
    long *args = mcc_library_arguments(function, thread, initialize);
    if (!args) return;
    error(_error_silent, "hs: %.255s", mcc_library_string(args[0]));
    hs_return(thread, 0);
}

static void mcc_test_print_bool_evaluate(short function, long thread, boolean initialize)
{
    long *args = mcc_library_arguments(function, thread, initialize);
    if (!args) return;
    error(_error_silent, "hs: %.200s = %s", mcc_library_string(args[0]), (boolean)args[1] ? "true" : "false");
    hs_return(thread, 0);
}

static void mcc_test_print_real_evaluate(short function, long thread, boolean initialize)
{
    long *args = mcc_library_arguments(function, thread, initialize);
    if (!args) return;
    error(_error_silent, "hs: %.200s = %f", mcc_library_string(args[0]), (double)mcc_library_real(args[1]));
    hs_return(thread, 0);
}

static void mcc_gravity_get_evaluate(short function, long thread, boolean initialize)
{
    (void)function;
    (void)initialize;
    hs_return(thread, mcc_library_bits(mcc_campaign_gravity_scale()));
}

static void mcc_music_gain_get_evaluate(short function, long thread, boolean initialize)
{
    (void)function;
    (void)initialize;
    hs_return(thread, mcc_library_bits(mcc_campaign_music_gain()));
}

/* The map's multiplier over every sound (mcc_campaign.c), after the
 * player's own volume, which it does not change. */
static void mcc_master_gain_get_evaluate(short function, long thread, boolean initialize)
{
    (void)function;
    (void)initialize;
    hs_return(thread, mcc_library_bits(mcc_campaign_master_gain()));
}

static void mcc_master_gain_set_evaluate(short function, long thread, boolean initialize)
{
    long *args = mcc_library_arguments(function, thread, initialize);
    if (!args) return;
    mcc_campaign_set_master_gain(mcc_library_real(args[0]));
    hs_return(thread, 0);
}

/* The look rates (degrees a second) a map's scripts set for a local player,
 * as MCC's set_yaw_rate and set_pitch_rate do, until the next map: they
 * stand in for the player's sensitivity (mcc_script_look_rates). A rate is
 * the machine's own; only a host or a local game runs the map's scripts. */
static struct {
    boolean set;
    real rate;
} mcc_look_rates[MAXIMUM_NUMBER_OF_LOCAL_PLAYERS][2];

void mcc_script_library_reset(void)
{
    memset(mcc_look_rates, 0, sizeof(mcc_look_rates));
}

void mcc_script_look_rates(short controller_index)
{
    if (!mcc_cache_tags_loaded() || controller_index < 0 || controller_index >= MAXIMUM_NUMBER_OF_LOCAL_PLAYERS)
        return;
    if (mcc_look_rates[controller_index][0].set)
        player_look_yaw_rate[controller_index] = mcc_look_rates[controller_index][0].rate;
    if (mcc_look_rates[controller_index][1].set)
        player_look_pitch_rate[controller_index] = mcc_look_rates[controller_index][1].rate;
}

/* get_ and set_yaw_rate, get_ and set_pitch_rate: the player's rate, a
 * local player's index first. */
static void mcc_look_rate_evaluate(short function, long thread, boolean initialize)
{
    long *args = mcc_library_arguments(function, thread, initialize);
    char const *name = mcc_library_get(function)->name;
    boolean pitch = name[4] == 'p';
    short player;
    if (!args) return;
    player = (short)args[0];
    if (player < 0 || player >= MAXIMUM_NUMBER_OF_LOCAL_PLAYERS) {
        hs_return(thread, 0);
        return;
    }
    if (name[0] == 'g') {
        hs_return(thread, mcc_library_bits(pitch ? player_look_pitch_rate[player] : player_look_yaw_rate[player]));
        return;
    }
    if (mcc_cache_tags_loaded()) {
        real rate = mcc_library_real(args[1]);
        /* any finite rate, as MCC takes any */
        if (rate >= -1000000.0f && rate <= 1000000.0f) {
            mcc_look_rates[player][pitch].set = TRUE;
            mcc_look_rates[player][pitch].rate = rate;
            if (pitch) player_look_pitch_rate[player] = rate;
            else player_look_yaw_rate[player] = rate;
        }
    }
    hs_return(thread, 0);
}

/* Every player's unit, where the native cheat camouflages only the first
 * player's: an MCC map's scripts run on the host for all its players, and
 * each unit's camouflage reaches the other machines with its state. */
static void mcc_active_camouflage_evaluate(short function, long thread, boolean initialize)
{
    struct data_iterator iterator;
    struct player_datum *player;
    (void)function;
    (void)initialize;
    if (mcc_cache_tags_loaded() && game_connection() != _game_connection_network_client) {
        data_iterator_new(&iterator, player_data);
        while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL) {
            struct unit_datum *unit = player->unit_index != NONE ? unit_try_and_get(player->unit_index) : NULL;
            if (!unit) continue;
            unit->unit.active_camouflage = 1.0f;
            if (TEST_FLAG(unit->unit.flags, _unit_active_camouflaged_bit))
                unit->unit.flags |= FLAG(_unit_super_camouflaged_bit);
            unit->unit.flags |= FLAG(_unit_active_camouflaged_bit);
        }
    }
    hs_return(thread, 0);
}

/* MCC's cheats that drop a shotgun, the map's characters or a warthog
 * (cheats.c). Objects are the host's to create; clients see them through
 * the host's object replication, as cheat_all_vehicles' (mcc_campaign.c). */
static void mcc_cheat_evaluate(short function, long thread, boolean initialize)
{
    char const *name = mcc_library_get(function)->name;
    (void)initialize;
    if (mcc_cache_tags_loaded() && game_connection() != _game_connection_network_client) {
        if (!csstrcasecmp(name, "cheat_give_some_gun")) cheat_give_some_gun();
        else if (!csstrcasecmp(name, "cheat_all_chars")) cheat_all_chars();
        else cheat_spawn_warthog();
    }
    hs_return(thread, 0);
}

/* MCC's map_name changes to another map of the map's package; the native one
 * sets the solo map's name, which a map's scripts may not call. */
static void mcc_map_name_evaluate(short function, long thread, boolean initialize)
{
    long *args = mcc_library_arguments(function, thread, initialize);
    if (!args) return;
    mcc_main_request_map(args[0] ? mcc_library_string(args[0]) : NULL);
    hs_return(thread, 0);
}

#define MCC_LIBRARY_IGNORE(type, name, count, first, second, third) \
    {{type, 0, name, hs_macro_function_parse, mcc_library_ignore_evaluate, NULL, NULL, count, {first}}, {second, third}}
#define MCC_LIBRARY(type, name, evaluate, count, first, second, third) \
    {{type, 0, name, hs_macro_function_parse, evaluate, NULL, NULL, count, {first}}, {second, third}}

static struct mcc_library_definition mcc_library_functions[] = {
    MCC_LIBRARY(_hs_type_real, "pin", mcc_pin_evaluate, 3, _hs_type_real, _hs_type_real, _hs_type_real),
    MCC_LIBRARY(_hs_type_boolean, "script_started", mcc_script_started_evaluate, 1, _hs_type_string, 0, 0),
    MCC_LIBRARY(_hs_type_long_integer, "bitwise_xor", mcc_bitwise_xor_evaluate, 2, _hs_type_long_integer, _hs_type_long_integer, 0),
    MCC_LIBRARY(_hs_type_long_integer, "bitwise_flags_toggle", mcc_bitwise_flags_evaluate, 3, _hs_type_long_integer, _hs_type_long_integer, _hs_type_boolean),
    MCC_LIBRARY(_hs_type_void, "log_print", mcc_log_print_evaluate, 1, _hs_type_string, 0, 0),
    MCC_LIBRARY(_hs_type_void, "TestPrintBool", mcc_test_print_bool_evaluate, 2, _hs_type_string, _hs_type_boolean, 0),
    MCC_LIBRARY(_hs_type_void, "TestPrintReal", mcc_test_print_real_evaluate, 2, _hs_type_string, _hs_type_real, 0),
    MCC_LIBRARY(_hs_type_real, "physics_get_gravity", mcc_gravity_get_evaluate, 0, 0, 0, 0),
    MCC_LIBRARY(_hs_type_real, "sound_get_music_gain", mcc_music_gain_get_evaluate, 0, 0, 0, 0),
    MCC_LIBRARY(_hs_type_void, "cheat_active_camouflage", mcc_active_camouflage_evaluate, 0, 0, 0, 0),
    MCC_LIBRARY(_hs_type_void, "map_name", mcc_map_name_evaluate, 1, _hs_type_string, 0, 0),
    /* the script debugger */
    MCC_LIBRARY_IGNORE(_hs_type_void, "debug_script_thread", 2, _hs_type_string, _hs_type_boolean, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "debug_scripting", 1, _hs_type_boolean, 0, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "breakpoint", 1, _hs_type_string, 0, 0),
    MCC_LIBRARY(_hs_type_void, "kill_active_scripts", mcc_kill_active_scripts_evaluate, 0, 0, 0, 0),
    /* cheats that place particular tags */
    MCC_LIBRARY(_hs_type_void, "cheat_give_some_gun", mcc_cheat_evaluate, 0, 0, 0, 0),
    MCC_LIBRARY(_hs_type_void, "cheat_all_chars", mcc_cheat_evaluate, 0, 0, 0, 0),
    MCC_LIBRARY(_hs_type_void, "cheat_spawn_warthog", mcc_cheat_evaluate, 0, 0, 0, 0),
    /* the debug camera's files, screenshots, reloading and dumps */
    MCC_LIBRARY_IGNORE(_hs_type_void, "debug_camera_save_name", 1, _hs_type_string, 0, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "debug_camera_load_name", 1, _hs_type_string, 0, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "debug_camera_save_simple_name", 1, _hs_type_string, 0, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "debug_camera_load_simple_name", 1, _hs_type_string, 0, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "debug_camera_load_text", 1, _hs_type_string, 0, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "quit", 0, 0, 0, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "screenshot_cubemap", 1, _hs_type_string, 0, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "sound_cache_dump_to_file", 0, 0, 0, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "reload_shader_transparent_chicago", 0, 0, 0, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "rasterizer_reload_effects", 0, 0, 0, 0),
    /* the map's multiplier over every sound */
    MCC_LIBRARY(_hs_type_void, "sound_set_master_gain", mcc_master_gain_set_evaluate, 1, _hs_type_real, 0, 0),
    MCC_LIBRARY(_hs_type_real, "sound_get_master_gain", mcc_master_gain_get_evaluate, 0, 0, 0, 0),
    /* the player's sound settings */
    MCC_LIBRARY_IGNORE(_hs_type_void, "sound_enable_eax", 1, _hs_type_boolean, 0, 0),
    MCC_LIBRARY_IGNORE(_hs_type_boolean, "sound_eax_enabled", 0, 0, 0, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "sound_set_env", 1, _hs_type_short_integer, 0, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "sound_enable_hardware", 2, _hs_type_boolean, _hs_type_boolean, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "sound_set_supplementary_buffers", 2, _hs_type_short_integer, _hs_type_boolean, 0),
    MCC_LIBRARY_IGNORE(_hs_type_short_integer, "sound_get_supplementary_buffers", 0, 0, 0, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "sound_set_rolloff", 1, _hs_type_real, 0, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "sound_set_factor", 1, _hs_type_real, 0, 0),
    /* the player's look rates */
    MCC_LIBRARY(_hs_type_real, "get_yaw_rate", mcc_look_rate_evaluate, 1, _hs_type_short_integer, 0, 0),
    MCC_LIBRARY(_hs_type_real, "get_pitch_rate", mcc_look_rate_evaluate, 1, _hs_type_short_integer, 0, 0),
    MCC_LIBRARY(_hs_type_void, "set_yaw_rate", mcc_look_rate_evaluate, 2, _hs_type_short_integer, _hs_type_real, 0),
    MCC_LIBRARY(_hs_type_void, "set_pitch_rate", mcc_look_rate_evaluate, 2, _hs_type_short_integer, _hs_type_real, 0),
    /* the player's controls */
    MCC_LIBRARY_IGNORE(_hs_type_void, "bind", 3, _hs_type_string, _hs_type_string, _hs_type_string),
    MCC_LIBRARY_IGNORE(_hs_type_void, "unbind", 2, _hs_type_string, _hs_type_string, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "print_binds", 0, 0, 0, 0),
    /* the server, profiles and checkpoint files */
    MCC_LIBRARY_IGNORE(_hs_type_void, "sv_map", 2, _hs_type_string, _hs_type_string, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "profile_load", 1, _hs_type_string, 0, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "checkpoint_save", 0, 0, 0, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "checkpoint_load", 1, _hs_type_string, 0, 0),
    /* network events */
    MCC_LIBRARY_IGNORE(_hs_type_void, "events_enabled", 1, _hs_type_boolean, 0, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "events_disable_suppression", 1, _hs_type_boolean, 0, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "event_global_display_category", 1, _hs_type_string, 0, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "event_global_log_category", 1, _hs_type_string, 0, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "event_global_remote_log_category", 1, _hs_type_string, 0, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "event_display_category", 2, _hs_type_string, _hs_type_string, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "event_force_display_category", 2, _hs_type_string, _hs_type_string, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "event_log_category", 2, _hs_type_string, _hs_type_string, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "event_remote_log_category", 2, _hs_type_string, _hs_type_string, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "event_debugger_break_category", 2, _hs_type_string, _hs_type_string, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "event_halt_category", 2, _hs_type_string, _hs_type_string, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "event_list_categories", 1, _hs_type_string, 0, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "events_suppress_console_display", 1, _hs_type_boolean, 0, 0),
    /* saved films */
    MCC_LIBRARY_IGNORE(_hs_type_void, "saved_film_play", 2, _hs_type_short_integer, _hs_type_string, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "saved_film_play_last", 0, 0, 0, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "saved_film_disable_version_checking", 1, _hs_type_boolean, 0, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "saved_film_toggle_debug_saving", 1, _hs_type_boolean, 0, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "saved_films_delete_on_level_load", 1, _hs_type_boolean, 0, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "saved_films_show_timestamp", 1, _hs_type_boolean, 0, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "saved_film_set_playback_game_speed", 1, _hs_type_real, 0, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "saved_film_seek_to_film_tick", 1, _hs_type_long_integer, 0, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "saved_film_set_repro_mode", 1, _hs_type_boolean, 0, 0),
    /* determinism debugging */
    MCC_LIBRARY_IGNORE(_hs_type_void, "determinism_debug_manager_enable_logging", 1, _hs_type_boolean, 0, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "determinism_debug_manager_set_trace_flags", 1, _hs_type_long_integer, 0, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "determinism_debug_manager_enable_game_state_checksum", 1, _hs_type_boolean, 0, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "determinism_debug_manager_enable_trace", 1, _hs_type_boolean, 0, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "determinism_debug_manager_set_consumer_sample_level", 2, _hs_type_string, _hs_type_long_integer, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "determinism_debug_manager_enable_log_file_comparision_on_oos", 1, _hs_type_boolean, 0, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "determinism_log_compare_log_files", 2, _hs_type_string, _hs_type_string, 0),
    /* the debug menu and the network's timeout */
    MCC_LIBRARY_IGNORE(_hs_type_void, "debug_menu_rebuild", 0, 0, 0, 0),
    MCC_LIBRARY_IGNORE(_hs_type_void, "net_never_timeout", 1, _hs_type_boolean, 0, 0)
};

static struct hs_function_definition *mcc_library_get(short function)
{
    return &mcc_library_functions[function - MCC_HS_LIBRARY_FIRST].function;
}

struct hs_function_definition *mcc_script_library_function(short index)
{
    index = (short)(index - MCC_HS_LIBRARY_FIRST);
    return index >= 0 && index < (short)NUMBEROF(mcc_library_functions) ?
        &mcc_library_functions[index].function : NULL;
}

short mcc_script_library_find(char const *name)
{
    short i;
    for (i = 0; i < (short)NUMBEROF(mcc_library_functions); i++)
        if (!csstrcasecmp(name, mcc_library_functions[i].function.name))
            return (short)(MCC_HS_LIBRARY_FIRST + i);
    return NONE;
}
