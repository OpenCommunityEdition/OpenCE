/* MCC's game_speed_value uses the native host clock, which already belongs
 * to checkpoints. Clients keep a separate, host-only clock multiplier:
 * cheats_network_client_enforce still resets their native speed to one. */
#include "mcc_game_speed.h"
#include "cseries_windows.h"
#include "mcc_cache.h"
#include "game/game.h"
#include "hs/hs.h"
#include "hs/hs_scenario_definitions.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "network_distributed.h"
#include "errors.h"
#include <stdint.h>
#include <string.h>

static real mcc_remote_speed=1.0f;
static boolean mcc_clock_started,mcc_speed_sent;
static unsigned long mcc_clock_wall,mcc_clock_time,mcc_speed_last_sent;
static double mcc_clock_fraction;
static real mcc_clock_speed=1.0f,mcc_sent_speed;

void mcc_game_speed_reset(void)
{
    mcc_remote_speed=1.0f;
    mcc_clock_started=mcc_speed_sent=FALSE;
    mcc_clock_fraction=0.0;
    mcc_clock_speed=1.0f;
}

boolean mcc_game_speed_global(word designator)
{
    struct scenario *scenario;
    struct hs_global const *global;
    if (!mcc_cache_tags_loaded() || (designator&0x8000)) return FALSE;
    scenario=global_scenario_get();
    if (!scenario || designator>=scenario->hs_globals.count) return FALSE;
    global=(struct hs_global const *)scenario->hs_globals.address+designator;
    return global->type==_hs_type_real && memchr(global->name,0,sizeof(global->name)) &&
        !csstrcasecmp(global->name,"game_speed_value");
}

/* Keep the host's existing speed-hack detector, measured in authorized
 * simulation milliseconds. Integrate speed changes rather than applying
 * the latest multiplier to a whole detection window. A paused host still
 * has a small budget so a running client is detected instead of exempted. */
unsigned long mcc_game_speed_clock(unsigned long now)
{
    uint64_t whole;
    double elapsed;
    if (!mcc_cache_tags_loaded() || game_connection()!=_game_connection_network_server) return now;
    if (!mcc_clock_started) {
        mcc_clock_started=TRUE;mcc_clock_wall=now;mcc_clock_time=now;
    }
    elapsed=(double)(unsigned long)(now-mcc_clock_wall)*
        (mcc_clock_speed>0.01f ? mcc_clock_speed : 0.01f)+mcc_clock_fraction;
    whole=(uint64_t)elapsed;
    mcc_clock_time+=(unsigned long)whole;mcc_clock_fraction=elapsed-(double)whole;
    mcc_clock_wall=now;mcc_clock_speed=game_time_get_speed();
    return mcc_clock_time;
}

void mcc_game_speed_set(real value)
{
    if (!mcc_cache_tags_loaded() || game_connection()==_game_connection_network_client) return;
    if (!(value>=0.0f && value<=100.0f)) {
        error(_error_silent,"mcc: game_speed_value must be finite and between 0 and 100");
        return;
    }
    mcc_game_speed_clock(system_milliseconds());
    game_time_set_speed(value);
    mcc_clock_speed=value;
}

real mcc_game_speed_client(real original)
{
    return mcc_cache_tags_loaded() && game_connection()==_game_connection_network_client ?
        mcc_remote_speed : original;
}

void mcc_game_speed_frame(void)
{
    struct {
        struct distributed_message_header header;
        real speed;
    } message;
    unsigned long now;
    if (!mcc_cache_tags_loaded() || game_connection()!=_game_connection_network_server ||
        !game_in_progress()) return;
    now=system_milliseconds();
    mcc_game_speed_clock(now);
    message.speed=game_time_get_speed();
    if (mcc_speed_sent && mcc_sent_speed==message.speed && now-mcc_speed_last_sent<250u) return;
    mcc_speed_sent=TRUE;mcc_sent_speed=message.speed;mcc_speed_last_sent=now;
    /* Reliable ordered messages are sent in real time, including at zero
     * speed and for late joiners. Never queue these in the tick-only batch. */
    distributed_send(&message,_distributed_message_mcc_game_speed,1,sizeof(message),
        _distributed_to_clients_reliably);
}

void mcc_game_speed_receive(void const *entries, short count)
{
    real value;
    if (!mcc_cache_tags_loaded() || game_connection()!=_game_connection_network_client || count!=1) return;
    memcpy(&value,entries,sizeof(value));
    if (value>=0.0f && value<=100.0f) mcc_remote_speed=value;
}
