/* Real engine action delivery on an already-created local player. This probe
   also applies one action through the actual player update and checks PVS.
   It does not advance object physics or the full simulation tick. */
#include "cseries.h"
#include "game/game.h"
#include "game/players.h"
#include "game/local_players.h"
#include "game/player_queues_new.h"
#include "units/units.h"
#include "main/main.h"
#include "objects/objects.h"
#include "scenario/scenario.h"
#include "structures/structure_bsp_definitions.h"

#define CHECK(x) do { ++checks; if (!(x)) { ++failures; fprintf(stderr,"queue FAIL %s line %d\n",#x,__LINE__); } } while (0)
static int player_queue_probe(long player_index,int apply)
{
    struct player_action input, output[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
    unsigned checks=0, failures=0;
    unsigned long reload=FLAG(_unit_control_weapon_reload_bit);
    short old_connection=game_connection();
    long old_time=game_time_get();
    short slot=DATUM_INDEX_TO_ABSOLUTE_INDEX(player_index);
    CHECK(slot>=0 && slot<HALO_PORT_MAXIMUM_NETWORK_PLAYERS);
    CHECK(local_player_get_player_index(0)==player_index);
    CHECK(machine_get_player_list(0)[0]==player_index);
    if(failures)return 0;
    game_connection_set(_game_connection_local);
    CHECK(update_server_new());
    if(failures){game_connection_set(old_connection);return 0;}
    update_server_start();
    update_queues_distributed_reset();
    CHECK(!update_client_dequeue(output));
    csmemset(&input,0,sizeof(input));
    input.desired_weapon_index=NONE;input.desired_grenade_index=NONE;input.desired_zoom_level=NONE;
    input.desired_facing.yaw=0.75f;input.desired_facing.pitch=-0.125f;
    input.throttle.i=0.5f;input.throttle.j=-0.25f;
    input.control_flags=reload;input.primary_trigger=0.75f;
    game_time_set_distributed(old_time+1);
    update_client_queue_push();update_client_queue(&input);
    /* A release before the same tick preserves its brief button press and
       peak trigger, but facing and throttle use the latest frame. */
    input.control_flags=0;input.primary_trigger=0.25f;input.throttle.i=0.625f;
    update_client_queue_push();update_client_queue(&input);
    update_client_local_ticks(1);
    CHECK(update_client_dequeue(output));
    CHECK(output[slot].control_flags==reload);
    CHECK(output[slot].primary_trigger==0.75f);
    CHECK(output[slot].throttle.i==0.625f && output[slot].throttle.j==-0.25f);
    CHECK(output[slot].desired_facing.yaw==0.75f && output[slot].desired_facing.pitch==-0.125f);
    CHECK(output[slot].desired_weapon_index==NONE && output[slot].desired_grenade_index==NONE && output[slot].desired_zoom_level==NONE);
    CHECK(!update_client_dequeue(output));
    /* A continued one-shot press is suppressed, then a release rearms it. */
    game_time_set_distributed(old_time+2);input.control_flags=reload;
    update_client_queue_push();update_client_queue(&input);update_client_local_ticks(1);
    CHECK(update_client_dequeue(output));CHECK(output[slot].control_flags==0);
    CHECK(output[slot].primary_trigger==0.25f);
    game_time_set_distributed(old_time+3);input.control_flags=0;
    update_client_queue_push();update_client_queue(&input);update_client_local_ticks(1);
    CHECK(update_client_dequeue(output));CHECK(output[slot].control_flags==0);
    game_time_set_distributed(old_time+4);input.control_flags=reload;
    update_client_queue_push();update_client_queue(&input);update_client_local_ticks(1);
    CHECK(update_client_dequeue(output));CHECK(output[slot].control_flags==reload);
    CHECK(!update_client_dequeue(output));
    /* The real player update consumes this action, stores unit control and
       computes visibility from the native BSP. No gameplay flags are forced. */
#ifdef MAC_PLAYER_QUEUE_APPLY
    if(apply){
        long unit_index=player_get(player_index)->unit_index;
        struct unit_datum *unit=unit_get(unit_index);
        struct structure_bsp *bsp=global_structure_bsp_get();
        short cluster=unit->object.location.cluster_index;
        short activating=objects_get_activating_cluster_index();
        unsigned long *pvs, *extra=NULL;
        unsigned bit, nonvisible=0;
        CHECK(unit_controllable(unit_index));CHECK(player_input_enabled());
        CHECK(cluster>=0 && cluster<bsp->clusters.count);
        if(!failures){
            input.control_flags=0;input.primary_trigger=0.f;
            input.throttle.i=0.375f;input.throttle.j=-0.125f;
            game_time_set_distributed(old_time+5);
            update_client_queue_push();update_client_queue(&input);update_client_local_ticks(1);
            players_update_before_game();
            CHECK(unit->unit.throttle.i==0.375f && unit->unit.throttle.j==-0.125f && unit->unit.throttle.k==0.f);
            CHECK(unit->unit.primary_trigger==0.f && unit->unit.control_flags==0);
            CHECK(player_get(player_index)->cluster_index==cluster);
            pvs=structure_bsp_get_cluster_pvs(bsp,cluster);
            if(activating!=NONE)extra=structure_bsp_get_cluster_pvs(bsp,activating);
            for(bit=0;bit<(unsigned)bsp->clusters.count;++bit){
                unsigned long mask=1UL<<(bit&31);
                int expected=!!((pvs[bit>>5] | (extra?extra[bit>>5]:0))&mask);
                CHECK(!!(players_get_combined_pvs()[bit>>5]&mask)==expected);
                CHECK(!!(players_get_combined_pvs_local()[bit>>5]&mask)==expected);
                if(!expected)++nonvisible;
            }
            printf("actual player PVS: cluster=%d clusters=%ld hidden=%u\n",cluster,bsp->clusters.count,nonvisible);
            CHECK(!update_client_dequeue(output));
        }
    }
#else
    (void)apply;
#endif
    update_server_delete();game_time_set_distributed(old_time);game_connection_set(old_connection);
    printf("actual player queue: %u checks %u failures\n",checks,failures);
    return failures==0;
}

int mac_player_queue_probe(long player_index) { return player_queue_probe(player_index,0); }
#ifdef MAC_PLAYER_QUEUE_APPLY
int mac_player_queue_apply_probe(long player_index) { return player_queue_probe(player_index,1); }
#endif
