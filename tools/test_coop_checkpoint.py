"""Checkpoint gameplay must not replace the host's live session membership."""
from pathlib import Path
import subprocess

import pytest

from harness import function, mutated
from test_coop_checkpoint_recovery import define_value
from test_hud_button_style import run_c

ROOT = Path(__file__).resolve().parents[1]


def test_host_checkpoint_sends_loaded_peers_fresh_objects(tmp_path):
    actual = function((ROOT / "port/linux/game/network_objects.c").read_text(),
                      "network_objects_host_reverted")
    run_c(tmp_path, r'''
#include <assert.h>
typedef int boolean;
#define TRUE 1
#define NONE -1
#define HALO_PORT_MAXIMUM_NETWORK_MACHINES 4
#define NUMBEROF(a) (sizeof(a)/sizeof((a)[0]))
#define _game_connection_network_server 2
static int connection=2,coop=1,calls;
static struct {long time;} objects_host_machines[4]={{55},{55},{NONE},{55}};
static int game_connection(void){return connection;}
static int network_coop_active(void){return coop;}
static short distributed_client_machines(long *ids,short size){
 assert(size==4);ids[0]=1;ids[1]=2;ids[2]=-1;ids[3]=4;return 4;
}
static void network_objects_client_asked(long machine,boolean again){
 assert(machine==1 && again && objects_host_machines[machine].time==NONE);
 objects_host_machines[machine].time=60;calls++;
}
''' + actual + r'''
int main(void){
 connection=1;network_objects_host_reverted();assert(!calls);
 connection=2;coop=0;network_objects_host_reverted();assert(!calls);
 coop=1;network_objects_host_reverted();assert(calls==1);
 assert(objects_host_machines[0].time==55 && objects_host_machines[3].time==55);
 assert(objects_host_machines[2].time==NONE); /* Still loading: normal ready handshake. */
 network_objects_host_reverted();assert(calls==2); /* Another host restore, even recently. */
 return 0;
}
''')


def test_restore_current_roster_and_machine_ownership(tmp_path):
    actual = function((ROOT / "port/linux/game/network_coop.c").read_text(),
                      "network_coop_restore_players")
    valid = function((ROOT / "source/networking/network_game_manager.c").read_text(),
                     "network_player_is_valid")
    run_c(tmp_path, r'''
#include <assert.h>
#include <string.h>
typedef int boolean;
typedef unsigned char byte;
#define TRUE 1
#define FALSE 0
#define NONE -1
#define MAXIMUM_LOCAL_PLAYERS 4
#define HALO_PORT_MAXIMUM_NETWORK_MACHINES 4
#define MAXIMUM_NETWORK_MACHINE_COUNT 4
#define HALO_PORT_MAXIMUM_NETWORK_PLAYERS 8
#define DATUM_INDEX_TO_ABSOLUTE_INDEX(i) ((i)&65535)
#define csmemset memset
#define csmemcpy memcpy
#define _error_silent 0
struct network_player {short name[12];int machine_index,controller_index,player_list_index;};
struct player_datum {int identifier;short name[12];int local_player_index,unit_index,quit_out_of_game,quit_out_of_game_time;struct network_player network_player_data;};
struct network_game {struct network_player players[8];};
struct data_iterator {int datum_index,next;};
static struct player_datum players[8];
static struct {int valid;} data={1},*player_data=&data;
static struct network_game game;
static long machines[4][4],locals[4];
static int hosting=1,spawned,deleted,errors;
static int coop_host(void){return hosting;}
static struct network_game *network_game_get_game(void){return &game;}
static long *machine_get_player_list(int m){assert(m>=0 && m<4);return machines[m];}
static int network_game_player_is_local(struct network_player *p){return p->machine_index==0;}
static void local_player_set_player_index(int local,long index){
 if(locals[local]!=NONE)players[locals[local]&65535].local_player_index=NONE;
 locals[local]=index;if(index!=NONE)players[index&65535].local_player_index=local;
}
static void data_iterator_new(struct data_iterator *it,void *d){it->next=0;}
static struct player_datum *data_iterator_next(struct data_iterator *it){
 while(it->next<8){int i=it->next++;if(players[i].identifier){it->datum_index=(players[i].identifier<<16)|i;return &players[i];}}return 0;
}
static struct player_datum *distributed_player(short i){return players[i].identifier?&players[i]:0;}
static long distributed_player_from_byte(byte i){return (players[i].identifier<<16)|i;}
static void network_player_detach_unit(long i){players[i&65535].unit_index=NONE;}
static void object_delete(long i){assert(i==11 || i==14 || i==15 || i==17);deleted++;}
static int network_game_spawn_player(struct network_player *n){
 struct player_datum *p=&players[n->player_list_index];int i;
 assert(!p->identifier || (p->quit_out_of_game && p->unit_index==NONE));
 memset(p,0,sizeof(*p));p->identifier=2;p->unit_index=NONE;p->local_player_index=NONE;
 p->network_player_data=*n;p->quit_out_of_game_time=NONE;spawned++;
 for(i=0;i<4;i++)if(machines[n->machine_index][i]==NONE){machines[n->machine_index][i]=distributed_player_from_byte((byte)n->player_list_index);return 1;}
 assert(0);return 0;
}
static void error(int level,const char *text,...){errors++;}
''' + valid + actual + r'''
int main(void){
 struct player_datum before[8];long old_machines[4][4];int i,m,c;
 memset(&game,255,sizeof(game));memset(machines,255,sizeof(machines));memset(locals,255,sizeof(locals));
 for(i=0;i<8;i++)if(i!=2 && i!=6){
  players[i].identifier=1;players[i].unit_index=10+i;players[i].local_player_index=NONE;
  players[i].network_player_data.machine_index=i==3?0:i==0?0:1;
  players[i].network_player_data.controller_index=i==3?1:0;
  players[i].network_player_data.player_list_index=i;
 }
 locals[0]=65536;locals[1]=65539;players[0].local_player_index=0;players[3].local_player_index=1;
 game.players[0]=players[0].network_player_data;game.players[3]=players[3].network_player_data;
 game.players[0].name[0]='H';game.players[3].name[0]='L';
 game.players[2].machine_index=2;game.players[2].controller_index=0;game.players[2].player_list_index=2;
 game.players[4].machine_index=3;game.players[4].controller_index=0;game.players[4].player_list_index=4;
 /* Invalid membership must not index machine/controller tables or claim a slot. */
 game.players[5].machine_index=4;game.players[5].controller_index=0;game.players[5].player_list_index=5;
 game.players[7].machine_index=1;game.players[7].controller_index=4;game.players[7].player_list_index=7;
 machines[1][0]=65537;machines[2][0]=123456;machines[3][3]=65540;
 memcpy(before,players,sizeof(players));memcpy(old_machines,machines,sizeof(machines));
 hosting=0;network_coop_restore_players();
 assert(!memcmp(before,players,sizeof(players)) && !memcmp(old_machines,machines,sizeof(machines)));
 hosting=1;player_data->valid=0;network_coop_restore_players();assert(!spawned && !deleted);
 player_data->valid=1;network_coop_restore_players();
 assert(spawned==2 && deleted==4 && !errors);
 assert(players[0].unit_index==10 && players[3].unit_index==13);
 assert(players[0].name[0]=='H' && players[3].name[0]=='L');
 assert(locals[0]==65536 && locals[1]==65539 && locals[2]==NONE && locals[3]==NONE);
 assert(machines[0][0]==65536 && machines[0][1]==65539);
 assert(machines[2][0]==131074 && machines[3][0]==131076);
 assert(players[1].quit_out_of_game && players[1].unit_index==NONE);
 assert(players[2].network_player_data.machine_index==2 && !players[2].quit_out_of_game);
 assert(players[4].network_player_data.machine_index==3 && !players[4].quit_out_of_game);
 for(m=0;m<4;m++)for(c=0;c<4;c++)if(machines[m][c]!=NONE){
  struct player_datum *p=&players[machines[m][c]&65535];assert(!p->quit_out_of_game && p->network_player_data.machine_index==m);
 }
 network_coop_restore_players();assert(spawned==2 && deleted==4 && !errors);
 return 0;
}
''')


def test_dead_client_units_request_bounded_authoritative_recreation(tmp_path):
    source = (ROOT / "port/linux/game/network_objects.c").read_text()
    actual = "\n".join(function(source, name) for name in (
        "distributed_client_request_objects", "network_objects_expect_live_unit",
        "distributed_client_reuses_object"))
    run_c(tmp_path, r'''
#include <assert.h>
typedef int boolean;
#define TRUE 1
#define FALSE 0
#define NONE -1
#define TEST_FLAG(v,b) ((v)&(1u<<(b)))
#define MIN(a,b) ((a)<(b)?(a):(b))
#define CLIENT_RETRY_MAXIMUM_TICKS 120
enum {_object_dead_bit=0,_distributed_object_dead_bit=1,_object_mask_unit=3};
struct object_datum {long definition_index;struct {int type;unsigned damage_flags;} object;};
struct distributed_object_change {long object_index,definition_index;unsigned flags;};
static struct object_datum object={7,{0,1}};
static int client=1,coop=1,known=1,now=10;
static int objects_client_failed_time=NONE,objects_client_ready_time=NONE;
static int objects_client_ready_interval,objects_client_retry_ticks=3;
static int objects_client_ask_again,objects_client_synchronized=1;
static int game_time_get(void){return now;}
static int network_game_distributed_client(void){return client;}
static int network_coop_active(void){return coop;}
static int network_objects_client_has(long id){return known && id==55;}
static struct object_datum *object_try_and_get_and_verify_type(long id,int mask){return id==55 && TEST_FLAG(mask,object.object.type)?&object:0;}
''' + actual + r'''
int main(void){
 struct distributed_object_change host={55,7,0};int i;
 client=0;network_objects_expect_live_unit(55);assert(!objects_client_ask_again);
 client=1;coop=0;network_objects_expect_live_unit(55);assert(!objects_client_ask_again);
 assert(distributed_client_reuses_object(55,&object,&host,TRUE));
 coop=1;known=0;network_objects_expect_live_unit(55);assert(!objects_client_ask_again);
 known=1;network_objects_expect_live_unit(NONE);assert(!objects_client_ask_again);
 object.object.damage_flags=0;network_objects_expect_live_unit(55);assert(!objects_client_ask_again);
 assert(distributed_client_reuses_object(55,&object,&host,TRUE));
 object.object.damage_flags=1;network_objects_expect_live_unit(55);
 assert(objects_client_ask_again && !objects_client_synchronized);
 assert(objects_client_failed_time==10 && objects_client_ready_interval==3 && objects_client_retry_ticks==6);
 assert(!distributed_client_reuses_object(55,&object,&host,TRUE));
 assert(distributed_client_reuses_object(55,&object,&host,FALSE)); /* This epoch's corpse stays dead. */
 now=11;network_objects_expect_live_unit(55);assert(objects_client_retry_ticks==6); /* One pending request. */
 host.flags=2;assert(distributed_client_reuses_object(55,&object,&host,TRUE)); /* Host still says dead. */
 assert(!distributed_client_reuses_object(54,&object,&host,TRUE));
 host.definition_index=8;assert(!distributed_client_reuses_object(55,&object,&host,TRUE));
 host.definition_index=7;host.flags=0;object.object.type=2;
 assert(distributed_client_reuses_object(55,&object,&host,TRUE)); /* Items are unaffected. */
 object.object.type=0;
 for(i=0;i<12;i++){now+=200;objects_client_ready_time=now;now++;network_objects_expect_live_unit(55);}
 assert(objects_client_retry_ticks==CLIENT_RETRY_MAXIMUM_TICKS);
 assert(objects_client_ready_interval<=CLIENT_RETRY_MAXIMUM_TICKS);
 return 0;
}
''')


def test_recovery_retires_history_without_losing_object_ownership(tmp_path):
    source = (ROOT / "port/linux/game/network_objects.c").read_text()
    definitions = "\n".join(define_value(source, name) for name in (
        "HOST_OBJECTS_RESEND_TICKS", "CLIENT_READY_INTERVAL_TICKS", "CLIENT_RETRY_TICKS"))
    run_c(tmp_path, r'''
#include <assert.h>
#include <string.h>
#define NONE -1
#define TRUE 1
#define FALSE 0
#define TICKS_PER_SECOND 30
#define csmemset memset
/* Reduced world capacities; the actual recovery function is extracted. */
#define MAXIMUM_TRACKED_OBJECTS 3
#define MAXIMUM_TRACKED_PLAYERS 3
#define MAXIMUM_LOCAL_PLAYERS 2
#define OWN_VEHICLE_POSITION_TICKS 4
#define HALO_PORT_MAXIMUM_NETWORK_MACHINES 2
static long objects_host_told[3]={10,11,12},objects_client_has[3]={10,11,12};
static int objects_client_checkpoint_pending[3];
static long objects_host_told_count=3,objects_host_resting_cursor=2;
static int objects_host_state_moving[3]={1,1,1};
static long objects_host_rest_times[3]={99,99,99};
static struct {long checksum,weapons_checksum,time,carried_time,ammunition_time;} objects_host_inventories[3];
static struct {int count;} objects_host_viewers={2};
static struct {long time;} objects_host_machines[2]={{99},{88}};
static struct {int valid;long accepted_vehicle_index,ground_vehicle_index,rest_vehicle_index;} objects_host_vehicle_predictions[3];
static struct {long object_index,time;} objects_client_dead[3];
static struct {long object_index,time;float interval;} objects_client_unsteered_path[3];
static int host_damage_animation_count=3,client_damage_animations[3]={1,2,3};
static int objects_client_synchronized=1,objects_client_ready_time=77,objects_client_ready_interval=99;
static int objects_client_failed_time=66,objects_client_retry_ticks=99,objects_client_ask_again=1;
static int objects_client_local_allocation=1,objects_client_creating_index=-1,objects_client_creating,objects_client_deleting;
static int objects_client_new_object_count=3,objects_client_retried_inventory_count=3,objects_client_check_all;
static int objects_client_local_free_index=8,objects_client_object_count=3;
#define LOCAL_OBJECTS_FIRST_INDEX 8
static struct {long unit_index;} objects_client_own_inventories[2];
static struct {long definition_index;} objects_client_picked_up_weapons[2];
static struct {long time,vehicle_index;} objects_client_own_vehicles[2][4];
static int mcc_resets;
static void mcc_network_new_game(void){mcc_resets++;}
''' + definitions + "\n" + function(source, "network_objects_reverted") + function(source, "network_objects_new_game") + r'''
int main(void){
 int i,j;
 memset(objects_host_vehicle_predictions,1,sizeof(objects_host_vehicle_predictions));
 memset(objects_host_inventories,1,sizeof(objects_host_inventories));
 memset(objects_client_dead,1,sizeof(objects_client_dead));
 memset(objects_client_unsteered_path,1,sizeof(objects_client_unsteered_path));
 network_objects_reverted();
 assert(mcc_resets==1 && objects_host_told_count==3 && objects_client_local_allocation);
 assert(objects_host_machines[0].time==99 && objects_host_machines[1].time==88);
 for(i=0;i<3;i++){
  assert(objects_host_told[i]==10+i && objects_client_has[i]==10+i);
  assert(objects_client_checkpoint_pending[i]);
  assert(!objects_host_inventories[i].checksum && !objects_host_inventories[i].weapons_checksum);
  assert(objects_host_inventories[i].carried_time==NONE && objects_host_inventories[i].ammunition_time==NONE);
  assert(!objects_host_state_moving[i] && objects_host_rest_times[i]==NONE);
  assert(!objects_host_vehicle_predictions[i].valid && objects_host_vehicle_predictions[i].accepted_vehicle_index==NONE);
  assert(objects_host_vehicle_predictions[i].ground_vehicle_index==NONE && objects_host_vehicle_predictions[i].rest_vehicle_index==NONE);
  assert(objects_client_dead[i].object_index==NONE && objects_client_unsteered_path[i].object_index==NONE);
  assert(objects_client_unsteered_path[i].time==NONE && !objects_client_unsteered_path[i].interval);
  assert(!client_damage_animations[i]);
 }
 assert(!host_damage_animation_count && !objects_host_viewers.count && !objects_host_resting_cursor);
 assert(!objects_client_synchronized && objects_client_ready_time==NONE && objects_client_failed_time==NONE);
 assert(objects_client_ready_interval==CLIENT_READY_INTERVAL_TICKS && objects_client_retry_ticks==CLIENT_RETRY_TICKS);
 assert(!objects_client_ask_again && !objects_client_retried_inventory_count && !objects_client_new_object_count);
 assert(objects_client_check_all && objects_client_object_count==NONE);
 for(i=0;i<2;i++){
  assert(objects_client_own_inventories[i].unit_index==NONE && objects_client_picked_up_weapons[i].definition_index==NONE);
  for(j=0;j<4;j++)assert(objects_client_own_vehicles[i][j].time==NONE && objects_client_own_vehicles[i][j].vehicle_index==NONE);
 }
 network_objects_new_game();assert(mcc_resets==2 && !objects_host_told_count && !objects_client_local_allocation);
 for(i=0;i<3;i++)assert(objects_host_told[i]==NONE && objects_client_has[i]==NONE);
 for(i=0;i<2;i++)assert(objects_host_machines[i].time==NONE);
 return 0;
}
''')


def test_coop_recovery_retires_queued_events_and_preserves_checkpoint_metadata(tmp_path):
    source = (ROOT / "port/linux/game/network_coop.c").read_text()
    actual = function(source, "skip_vote_clear") + function(source, "network_coop_reset_recovery")
    run_c(tmp_path, r'''
#include <assert.h>
#include <string.h>
#define FALSE 0
#define NONE -1
#define NUMBEROF(x) (sizeof(x)/sizeof((x)[0]))
#define csmemset memset
static struct {int cinematic_started,host_camera,buffered[4];} coop_presentation;
static struct {int count,applied_any,next_number,applied_number;} coop_events;
static int client_devices[4],client_names_differing[4],host_player_structure_bsps[4];
static struct {int machine_count,joined,refresh;} host_resend;
static struct {int voted,votes,client_heard_times[4],client_vote_times[4],cooldown_until;} skip_vote;
/* Authoritative metadata and comparison baselines are not disposable queues. */
static int checkpoint_players=17,players_vitality=18,host_devices=19;
static int host_sent_transforms=20,host_sent_looks=21,host_sent_devices=22,host_hud_state=23;
static int client,stops,cinematics,cameras;
static int coop_client(void){return client;}
static void client_stop_script_sounds(void){stops++;}
static void client_cinematic_end(void){assert(coop_presentation.cinematic_started);cinematics++;}
static void client_host_camera_set(int enabled){assert(!enabled && coop_presentation.host_camera);cameras++;}
''' + actual + r'''
int main(void){
 int phase,i;
 for(phase=0;phase<2;phase++){
  client=phase;memset(&coop_presentation,1,sizeof(coop_presentation));
  memset(&coop_events,1,sizeof(coop_events));memset(client_devices,1,sizeof(client_devices));
  memset(client_names_differing,1,sizeof(client_names_differing));memset(&host_resend,1,sizeof(host_resend));
  memset(&skip_vote,1,sizeof(skip_vote));skip_vote.cooldown_until=77;
  network_coop_reset_recovery();
  assert(!coop_events.count && !coop_events.applied_any && !coop_events.next_number && !coop_events.applied_number);
  assert(!coop_presentation.cinematic_started && !coop_presentation.host_camera && !coop_presentation.buffered[0]);
  assert(!host_resend.machine_count && !host_resend.joined && !host_resend.refresh);
  assert(!skip_vote.voted && !skip_vote.votes && skip_vote.cooldown_until==77);
  for(i=0;i<4;i++)assert(!client_devices[i] && !client_names_differing[i] && host_player_structure_bsps[i]==NONE &&
   skip_vote.client_heard_times[i]==NONE && skip_vote.client_vote_times[i]==NONE);
  assert(checkpoint_players==17 && players_vitality==18 && host_devices==19 && host_sent_transforms==20 &&
   host_sent_looks==21 && host_sent_devices==22 && host_hud_state==23);
  assert(stops==phase && cinematics==phase && cameras==phase);
 }
 return 0;
}
''')


@pytest.mark.parametrize("fault", [None, "damage", "object", "delete", "harmless"])
def test_delayed_effect_producers_follow_normal_deletion(tmp_path, fault):
    source = (ROOT / "source/effects/effects.c").read_text()
    actual = function(source, "effect_delete") + function(source, "effects_discard_gameplay")
    if fault in ("damage", "object"):
        token = "DAMAGE_EFFECT_DEFINITION_TAG" if fault == "damage" else "OBJECT_DEFINITION_TAG"
        actual = mutated(actual, "part->runtime_base_class_tag == " + token, "FALSE")
    elif fault == "delete":
        actual = mutated(actual, "\t\t\teffect_delete(effect_index);", "\t\t\t(void)effect_index;")
    elif fault == "harmless":
        actual = mutated(actual, "if (gameplay)", "if (TRUE)")
    # Real effect deletion traverses linked locations and removes the datum.
    # Tag/data-array views are reduced fake-world boundaries, not ABI claims.
    code = r'''
#include <assert.h>
#include <string.h>
#define NONE -1
#define TRUE 1
#define FALSE 0
#define MIN(a,b) ((a)<(b)?(a):(b))
#define NUMBEROF(a) (sizeof(a)/sizeof((a)[0]))
#define TAG_BLOCK_GET_ELEMENT(block,index,type) (&((type *)(block)->elements)[index])
typedef int boolean;
struct data_array {int valid;};
struct effect_datum {long definition_index;long location_datum_indices[2];};
struct effect_location_datum {long next_instance_location_index;};
struct effect_part_definition {unsigned long runtime_base_class_tag;struct {long index;} reference;};
struct block {long count;void *elements;};
struct effect_event_definition {struct block parts;};
struct effect_definition {struct block events;struct {long count;} locations;};
static struct data_array effects={1},locations={1},*effect_data=&effects,*effect_location_data=&locations;
static struct effect_datum datums[6];
static struct effect_location_datum location_datums[12];
static struct effect_definition definitions[6];
static struct effect_event_definition events[6][2];
static struct effect_part_definition parts[6][2];
static int alive[6],location_alive[12],deleted,deleted_locations;
static struct effect_datum *effect_try_and_get(long i){assert(i>=0 && i<6);return alive[i]?&datums[i]:0;}
#define effect_get effect_try_and_get
static struct effect_definition *effect_definition_get(long i){assert(i>=0 && i<6);return &definitions[i];}
static struct effect_location_datum *effect_location_get(long i){assert(i>=0 && i<12 && location_alive[i]);return &location_datums[i];}
static long data_next_index(struct data_array *data,long previous){long i;assert(data==effect_data);for(i=previous+1;i<6;i++)if(alive[i])return i;return NONE;}
static void datum_delete(struct data_array *data,long i){
 if(data==effect_data){assert(alive[i]);alive[i]=0;deleted++;}
 else{assert(data==effect_location_data && location_alive[i]);location_alive[i]=0;deleted_locations++;}
}
'''
    code += define_value((ROOT / "source/objects/object_definitions.h").read_text(), "OBJECT_DEFINITION_TAG") + "\n"
    code += define_value((ROOT / "source/objects/damage_effect_definitions.h").read_text(), "DAMAGE_EFFECT_DEFINITION_TAG") + "\n"
    code += actual + r'''
int main(void){
 int i,j;
 for(i=0;i<6;i++){
  alive[i]=1;datums[i].definition_index=i;datums[i].location_datum_indices[0]=i*2;datums[i].location_datum_indices[1]=NONE;
  definitions[i].locations.count=1;definitions[i].events.count=2;definitions[i].events.elements=events[i];
  for(j=0;j<2;j++){
   events[i][j].parts.count=1;events[i][j].parts.elements=&parts[i][j];parts[i][j].runtime_base_class_tag='snd!';parts[i][j].reference.index=1;
   location_alive[i*2+j]=1;location_datums[i*2+j].next_instance_location_index=j?NONE:i*2+1;
  }
 }
 /* A later event's damage, an object producer, a missing reference, harmless
    light/sound, and empty events/parts. Identity is irrelevant. */
 parts[0][1].runtime_base_class_tag=DAMAGE_EFFECT_DEFINITION_TAG;
 parts[1][0].runtime_base_class_tag=OBJECT_DEFINITION_TAG;
 parts[2][1].runtime_base_class_tag=DAMAGE_EFFECT_DEFINITION_TAG;parts[2][1].reference.index=NONE;
 parts[3][1].runtime_base_class_tag='ligh';definitions[4].events.count=0;
 events[5][0].parts.count=events[5][1].parts.count=0;
 effects.valid=0;effects_discard_gameplay();assert(!deleted && !deleted_locations);effects.valid=1;
 effects_discard_gameplay();
 if(!(deleted==2 && deleted_locations==4 && !alive[0] && !alive[1]))return 1;
 for(i=2;i<6;i++)assert(alive[i] && location_alive[i*2] && location_alive[i*2+1]);
 effects_discard_gameplay();assert(deleted==2 && deleted_locations==4);
 effect_data=0;effects_discard_gameplay();assert(deleted==2);
 return 0;
}
'''
    if fault:
        with pytest.raises(subprocess.CalledProcessError) as failure:
            run_c(tmp_path, code)
        assert failure.value.returncode == 1
    else:
        run_c(tmp_path, code)
