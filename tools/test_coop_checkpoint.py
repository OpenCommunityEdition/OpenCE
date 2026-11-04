"""Checkpoint gameplay must not replace the host's live session membership."""
from pathlib import Path

from harness import function
from test_hud_button_style import run_c

ROOT = Path(__file__).resolve().parents[1]


def test_host_checkpoint_sends_loaded_peers_fresh_objects(tmp_path):
    actual = function((ROOT / "port/linux/game/network_objects.c").read_text(),
                      "network_objects_host_reverted")
    run_c(tmp_path, r'''
#include <assert.h>
static int camera_mode=1;
static int third_person_session_active(void){return camera_mode;}
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
 camera_mode=0;network_objects_host_reverted();assert(!calls);camera_mode=1;
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
static int camera_mode=1;
static int third_person_session_active(void){return camera_mode;}
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
static int camera_mode=1;
static int third_person_session_active(void){return camera_mode;}
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
 assert(distributed_client_reuses_object(55,&object,&host));
 coop=1;known=0;network_objects_expect_live_unit(55);assert(!objects_client_ask_again);
 known=1;network_objects_expect_live_unit(NONE);assert(!objects_client_ask_again);
 object.object.damage_flags=0;network_objects_expect_live_unit(55);assert(!objects_client_ask_again);
 assert(distributed_client_reuses_object(55,&object,&host));
 object.object.damage_flags=1;network_objects_expect_live_unit(55);
 assert(objects_client_ask_again && !objects_client_synchronized);
 assert(objects_client_failed_time==10 && objects_client_ready_interval==3 && objects_client_retry_ticks==6);
 assert(!distributed_client_reuses_object(55,&object,&host));
 now=11;network_objects_expect_live_unit(55);assert(objects_client_retry_ticks==6); /* One pending request. */
 host.flags=2;assert(distributed_client_reuses_object(55,&object,&host)); /* Host still says dead. */
 assert(!distributed_client_reuses_object(54,&object,&host));
 host.definition_index=8;assert(!distributed_client_reuses_object(55,&object,&host));
 host.definition_index=7;host.flags=0;object.object.type=2;
 assert(distributed_client_reuses_object(55,&object,&host)); /* Items are unaffected. */
 object.object.type=0;
 for(i=0;i<12;i++){now+=200;objects_client_ready_time=now;now++;network_objects_expect_live_unit(55);}
 assert(objects_client_retry_ticks==CLIENT_RETRY_MAXIMUM_TICKS);
 assert(objects_client_ready_interval<=CLIENT_RETRY_MAXIMUM_TICKS);
 return 0;
}
''')
