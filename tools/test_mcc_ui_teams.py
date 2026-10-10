"""Exercise the real network settings handler and MCC's team-only ingress."""
import os
from pathlib import Path
import shutil
import subprocess

import pytest
from harness import function, structure

ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="module")
def teams_tool(tmp_path_factory):
    compiler = os.environ.get("CC") or shutil.which("clang") or shutil.which("cc")
    if not compiler:
        pytest.skip("a C compiler is needed")
    own = (ROOT / "port/linux/game/mcc_ui_teams.c").read_text()
    native = (ROOT / "source/networking/network_server_message_handler.c").read_text()
    engine = (ROOT / "source/game/game_engine.c").read_text()
    slayer = (ROOT / "source/game/game_engine_slayer.c").read_text()
    damage = function((ROOT / "source/objects/damage.c").read_text(), "object_damage_aftermath")
    start = damage.index("{", damage.index("else if (game_engine_can_score())"))
    end, depth = start + 1, 1
    while depth:
        depth += (damage[end] == "{") - (damage[end] == "}")
        end += 1
    no_statistics_branch = damage[start:end]
    cache = (ROOT / "port/linux/game/mcc_cache.c").read_text()
    source = r'''
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
typedef int boolean;
typedef uint16_t word;
#define TRUE 1
#define FALSE 0
#define NONE (-1)
#define NUMBEROF(a) (sizeof(a)/sizeof(*(a)))
#define VALID_INDEX(i,n) ((unsigned long)(i)<(unsigned long)(n))
#define MAXIMUM_NUMBER_OF_PLAYERS 4
#define NUMBER_OF_MULTIPLAYER_TEAMS 2
#define HALO_PORT_NETWORK_GAME_MESSAGE_VERSION 12
#define NETWORK_GAME_MESSAGE_VERSION HALO_PORT_NETWORK_GAME_MESSAGE_VERSION
#define _message_client_player_settings_request 16
#define _network_game_packet_class_client_pregame 3
#define HALO_PORT_MAXIMUM_NETWORK_PLAYERS 4
#define DATUM_INDEX_TO_ABSOLUTE_INDEX(i) ((unsigned long)(i)&0xFFFF)
#define TEST_FLAG(v,b) ((v)&(1u<<(b)))
#define _object_die_act_of_god_no_statistics_bit 2
#define _damage_no_statistics_bit 3
#define _game_connection_network_server 2
#define _game_connection_network_client 1
#define _object_type_vehicle 1
#define TICKS_PER_SECOND 30
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define match_assert(file,line,condition) do{if(!(condition))exit(91);}while(0)
struct network_player {uint16_t name[12];short color,icon;char machine_index,controller_index,team_index,player_list_index;};
struct network_game {struct {char name[128];} map;struct {struct {int teams;}universal_variant;}variant;struct network_player players[4];int machines[4];};
struct network_game_server {int state;struct network_game game;}server;
struct network_game_server_client_machine {int index;}machine;
struct player_datum {int quit_out_of_game,team_index,unit_index;struct network_player network_player_data;
    long quit_out_of_game_time,death_time,respawn_timer,respawn_penalty,multiplayer_special;
    struct {int multiple_kills,kills_in_a_row;}statistics;}players[4];
struct unit_datum {struct {unsigned damage_flags;}object;}units[4];
struct object_datum {struct {int type;}object;}object;
static long player_handles[4]={0x10000,0x10001,0x10002,0x10003};
static long unit_handles[4]={0x20000,0x20001,0x20002,0x20003};
struct data_iterator {int i;long datum_index;};
struct {int auto_team_balance,friendly_fire_penalty;}options;
static int player_data,mcc=1,bad_decode,legacy,send_count,kill_count,killed,decoded;
static int connection=2,score,death_broadcast,death_friendly,engine_active=1;
static long death_killer,death_object;
static struct network_player request;
static struct {int type;}scenario={1};
#define global_scenario (&scenario)
static boolean mcc_cache_tags_loaded(void){return mcc;}
static short game_connection(void){return (short)connection;}
static struct player_datum *player_try_and_get(long index){unsigned long i=DATUM_INDEX_TO_ABSOLUTE_INDEX(index);return i<4&&player_handles[i]==index?&players[i]:NULL;}
static struct player_datum *player_get(long index){struct player_datum *p=player_try_and_get(index);if(!p)exit(92);return p;}
static struct unit_datum *unit_try_and_get(long index){unsigned long i=DATUM_INDEX_TO_ABSOLUTE_INDEX(index);return i<4&&unit_handles[i]==index?&units[i]:NULL;}
static int network_game_server_get_state(struct network_game_server *s,void *unused){(void)unused;return s->state;}
static struct network_game *network_game_server_get_game(struct network_game_server *s){return &s->game;}
static boolean mcc_level_name(char const *s){return !strncmp(s,"mcc_maps\\",9);}
static boolean decode_network_game_message(void *out,void const *wire,short *size,short *type,short *version,int cls){
 (void)wire;(void)size;decoded++;if(*type!=16||*version!=12||cls!=3)exit(80);memcpy(out,&request,sizeof(request));return !bad_decode;
}
static void *network_game_server_get_client_machine(struct network_game_server *s,struct network_game_server_client_machine *m,long *index){(void)s;*index=m->index;return m->index==-1?NULL:m;}
static boolean network_player_is_valid(struct network_player *p){return p->machine_index>=0;}
static void data_iterator_new(struct data_iterator *it,int data){(void)data;it->i=0;}
static void *data_iterator_next(struct data_iterator *it){if(it->i>=4)return NULL;it->datum_index=player_handles[it->i];return &players[it->i++];}
static void *game_variant_options_get(void);
#define game_variant_options_get() (&options)
static void unit_kill_no_statistics(long unit){struct unit_datum *p=unit_try_and_get(unit);if(!p)exit(93);p->object.damage_flags|=1u<<_object_die_act_of_god_no_statistics_bit;kill_count++;killed=(int)unit;}
static void network_event(char const *fmt,...){(void)fmt;}
static boolean network_game_server_lobby_is_open(struct network_game_server *s){legacy++;return s->state==0;}
static void network_game_server_clean_player_name(struct network_game_server *s,struct network_player *p,long slot){(void)s;(void)p;(void)slot;}
static boolean network_game_update_player(struct network_game *g,struct network_player *p){g->players[p->player_list_index]=*p;return TRUE;}
static boolean network_game_server_send_game_data_pregame(struct network_game_server *s){(void)s;send_count++;return TRUE;}
/* Native game-engine and Slayer callbacks below run unchanged. */
static struct {struct {int respawn_time,respawn_time_growth,suicide_penalty,teams;}universal_variant;}global_variant;
static short game_engine_betrayal_penalty[4];
static struct {struct {struct {int kill_in_order;}slayer;}game_engine_variant;}variant;
static struct {void (*player_killed_player)(long,long,long,boolean);}engine_callbacks;
#define game_engine (engine_active?&engine_callbacks:NULL)
static int network_game_distributed_client(void){return connection==1;}
static void network_distributed_player_killed(long *k,long *o,long d,boolean *f){(void)d;death_broadcast++;death_killer=*k;death_object=*o;death_friendly=*f;}
static long game_time_get(void){return 1000;}
static struct network_game_server *global_network_game_server_get(void){return connection==2?&server:NULL;}
static struct object_datum *object_get(long index){(void)index;return &object;}
static long player_index_from_unit_index(long unit){int i;for(i=0;i<4;i++)if(players[i].unit_index==unit)return player_handles[i];return NONE;}
struct damage_data {unsigned flags;};
static void game_show_score_extended(long p,long m,long d){(void)p;(void)m;(void)d;}
static void multiplayer_message(long p,long m,long d){(void)p;(void)m;(void)d;}
static void update_speed_for_score(long d,long k){(void)d;(void)k;}
#define game_engine_get_variant() (&variant)
static void find_next_target(long p){(void)p;}
static void slayer_engine_adjust_score(long p,int amount){(void)p;score+=amount;}
enum {_game_engine_message_killed_by_unknown,_game_engine_message_killed_by_biped,_game_engine_message_killed_by_vehicle,
_game_engine_message_killed_by_self,_game_engine_message_killed_by_player,_game_engine_message_killed_by_friendly_fire,
_game_engine_message_quit,_game_engine_message_killed_friendly,_game_engine_message_multi_kill,_game_engine_message_triple_kill,
_game_engine_message_double_kill,_game_engine_message_five_kills_in_row,_game_engine_message_killed_enemy,_game_engine_message_ten_kills_in_a_row};
#define _object_type_biped 0
''' + structure(own, "mcc_team_respawn") + r'''
static struct mcc_team_respawn mcc_team_respawns[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
''' + '\n'.join(function(own, n) for n in ["mcc_ui_teams_reset", "mcc_ui_team_respawn_track", "mcc_ui_team_respawn_damage", "mcc_ui_team_respawn_death",
        "mcc_ui_team_balance_allows", "mcc_ui_team_request"]) + '\n' + function(
        native, "network_game_server_handle_message_client_player_settings_request") + '\n' + function(
        slayer, "slayer_engine_player_killed_player") + '\n' + function(engine, "game_engine_player_killed") + '\n' + \
        'static void no_statistics_death(long object_index, struct damage_data *damage)\n' + no_statistics_branch + r'''
/* Exercise the actual MCC teardown, with resource releases reduced to fakes. */
static struct {void *tags;unsigned capacity;}mcc_loaded_cache;
static int mcc_loaded;
static FILE *mcc_stream;
static void cleanup(void *unused){(void)unused;}
#define mcc_checkpoint_dispose() cleanup(NULL)
#define mcc_grenades_reset() cleanup(NULL)
#define mcc_parameters_dispose() cleanup(NULL)
#define mcc_texture_cache_dispose() cleanup(NULL)
#define mcc_geometry_dispose(p) cleanup(p)
#define mcc_audio_dispose(p) cleanup(p)
#define mcc_bitmaps_dispose(p) cleanup(p)
#define mcc_validation_dispose(p) cleanup(p)
#define mcc_memory_release(p,n) cleanup(p)
''' + function(cache, "mcc_cache_tags_unload") + r'''
#define CHECK(c,n) do{if(!(c)){fprintf(stderr,"check%d failed\n",n);return n;}}while(0)
int main(int argc,char **argv){
    word wire[18]={16};int i;short size=sizeof(wire);struct network_game before;struct player_datum prior[4];
    char const *test=argc>1?argv[1]:"";
    ((unsigned char *)wire)[size-1]=16;
    strcpy(server.game.map.name,"mcc_maps\\dangercanyon");server.game.variant.universal_variant.teams=TRUE;server.state=1;machine.index=2;
    for(i=0;i<4;i++){server.game.players[i].machine_index=-1;players[i].quit_out_of_game=TRUE;players[i].unit_index=NONE;}
    request=(struct network_player){{'A',0},2,3,2,1,0,1};server.game.players[1]=request;
    players[1].quit_out_of_game=FALSE;players[1].team_index=0;players[1].unit_index=0x20001;players[1].network_player_data=request;
    players[1].quit_out_of_game_time=NONE;engine_callbacks.player_killed_player=slayer_engine_player_killed_player;
    global_variant.universal_variant.respawn_time=120;
    request.team_index=1;
    if(!strcmp(test,"non-mcc"))mcc=0;
    if(!strcmp(test,"non-namespace"))strcpy(server.game.map.name,"custom_maps\\x");
    if(!strcmp(test,"pregame"))server.state=0;
    if(!strcmp(test,"postgame"))server.state=2;
    if(!strcmp(test,"no-teams"))server.game.variant.universal_variant.teams=FALSE;
    if(!strcmp(test,"campaign"))scenario.type=0;
    if(!strcmp(test,"unjoined"))machine.index=-1;
    if(!strcmp(test,"malformed"))bad_decode=1;
    if(!strcmp(test,"short"))size=1;
    if(!strcmp(test,"negative"))size=-1;
    if(!strcmp(test,"minimum"))size=-32768;
    if(!strcmp(test,"oversized"))size=128;
    if(!strcmp(test,"wrong-packet"))((unsigned char *)wire)[size-1]=15;
    if(!strcmp(test,"machine"))machine.index=3;
    if(!strcmp(test,"controller"))request.controller_index=2;
    if(!strcmp(test,"bad-team"))request.team_index=2;
    if(!strcmp(test,"bad-slot"))request.player_list_index=-1;
    if(!strcmp(test,"unknown-slot"))request.player_list_index=2;
    if(!strcmp(test,"name"))request.name[0]='B';
    if(!strcmp(test,"color"))request.color=4;
    if(!strcmp(test,"icon"))request.icon=4;
    if(!strcmp(test,"unknown-live"))players[1].network_player_data.controller_index=2;
    if(!strcmp(test,"already-team"))request.team_index=0;
    if(!strcmp(test,"dead"))players[1].unit_index=NONE;
    if(!strcmp(test,"balance")){options.auto_team_balance=TRUE;players[0].quit_out_of_game=FALSE;players[0].team_index=1;}
    before=server.game;memcpy(prior,players,sizeof(prior));
    CHECK(network_game_server_handle_message_client_player_settings_request(&server,&machine,wire,size),1);
    if(!strcmp(test,"valid")||!strcmp(test,"dead")){
        CHECK(!legacy&&server.game.players[1].team_index==1&&players[1].team_index==1&&players[1].network_player_data.team_index==1,2);
        CHECK(kill_count==(!strcmp(test,"valid")?1:0),3);if(kill_count)CHECK(killed==0x20001,4);return 0;
    }
    if(!strncmp(test,"death-",6)){
        long victim=player_handles[1],killer=victim,unit=unit_handles[1];
        struct damage_data cause={1u<<_damage_no_statistics_bit};
        CHECK(kill_count==1,9);
        if(strcmp(test,"death-real-suicide"))mcc_ui_team_respawn_damage(victim,unit,TRUE);
        if(!strcmp(test,"death-unload"))mcc_cache_tags_unload();
        if(!strcmp(test,"death-non-mcc"))mcc=FALSE;
        if(!strcmp(test,"death-client"))connection=1;
        if(!strcmp(test,"death-new-player")){player_handles[1]=0x30001;victim=killer=player_handles[1];}
        if(!strcmp(test,"death-new-unit")){unit_handles[1]=0x40001;unit=players[1].unit_index=unit_handles[1];}
        if(!strcmp(test,"death-deleted-unit"))unit_handles[1]=NONE;
        if(!strcmp(test,"death-no-flag"))units[1].object.damage_flags=0;
        if(!strcmp(test,"death-other-killer"))killer=player_handles[0];
        if(!strcmp(test,"death-no-engine")){
            engine_active=FALSE;no_statistics_death(unit,&cause);CHECK(!death_broadcast,16);engine_active=TRUE;
        }
        if(!strcmp(test,"death-neutral"))no_statistics_death(unit,&cause);
        else game_engine_player_killed(killer,unit,victim,TRUE);
        CHECK(death_broadcast==1&&players[1].death_time==1000&&players[1].respawn_timer==120,10);
        if(!strcmp(test,"death-neutral")){
            CHECK(!score&&death_killer==NONE&&death_object==NONE&&!death_friendly,11);
            game_engine_player_killed(killer,unit,victim,TRUE);
            CHECK(score==-1&&death_killer==killer&&death_object==unit&&death_friendly,12);
        }else if(!strcmp(test,"death-client"))CHECK(!score&&death_killer==killer,13);
        else {
            CHECK(score==-1&&death_killer==killer&&death_object==unit&&death_friendly,14);
            if(!strcmp(test,"death-other-killer")){
                game_engine_player_killed(victim,unit,victim,TRUE);
                CHECK(score==-2&&death_killer==victim,15);
            }
        }
        return 0;
    }
    if(!strcmp(test,"pregame")){CHECK(legacy==1&&send_count==1&&!kill_count,5);return 0;}
    CHECK(!memcmp(&before,&server.game,sizeof(before))&&!memcmp(prior,players,sizeof(prior))&&!kill_count,6);
    if(!strcmp(test,"non-mcc")||!strcmp(test,"non-namespace")||!strcmp(test,"postgame"))CHECK(legacy==1,7);
    else CHECK(!legacy,8);
    return 0;
}
'''
    work = tmp_path_factory.mktemp("mcc-ui-teams")
    path = work / "teams.c"
    path.write_text(source)
    executable = work / ("teams.exe" if os.name == "nt" else "teams")
    command = [compiler, "-std=c99", str(path), "-o", str(executable)]
    if os.name == "nt":
        command[1:1] = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"]
    result = subprocess.run(command, capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return executable


@pytest.mark.parametrize("case", ["valid", "dead", "non-mcc", "non-namespace", "pregame", "postgame", "no-teams",
    "campaign", "unjoined", "malformed", "short", "negative", "minimum", "oversized", "wrong-packet", "machine", "controller", "bad-team", "bad-slot", "unknown-slot", "name", "color", "icon",
    "unknown-live", "already-team", "balance", "death-neutral", "death-unload", "death-non-mcc", "death-client",
    "death-new-player", "death-new-unit", "death-deleted-unit", "death-no-flag", "death-other-killer", "death-real-suicide", "death-no-engine"])
def test_mcc_ingame_team_request(teams_tool, case):
    result = subprocess.run([str(teams_tool), case], capture_output=True, text=True)
    assert result.returncode == 0, (case, result.returncode, result.stdout, result.stderr)
