"""Shared OpenCE protocol 43 and host admission regression checks.

The unchanged AGPL-3.0-or-later fixture comes from OpenCommunityEdition/OpenCE
14ff64cd8, port/linux/network_format.txt. Camera extensions are separately
covered by test_camera_wire.py and test_camera_protocol.py.
"""
from pathlib import Path
import re
import subprocess
from harness import enum_with, function
from test_mcc_identity import compile_tool
import network_format
ROOT = Path(__file__).resolve().parents[1]


def test_shared_opence_43_contract():
    lines = (ROOT / "tools/fixtures/opence_network_43.txt").read_text().splitlines()
    assert lines[0] == "version 43"
    expected = {f"{kind} {name}": value for kind, name, value in
                (line.split(" ", 2) for line in lines[1:])}
    assert network_format.items() == expected
    assert network_format.version() == 43


def test_shared_protocol_preserves_host_admission(tmp_path):
    source = (ROOT / "source/networking/network_server_message_handler.c").read_text()
    join = function(source, "network_game_server_handle_message_client_join_game_request")
    # Execute the real admission handler. Only its external services are fakes;
    # Both camera modes must retain tokens, slots, bans and session state.
    code = r'''
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunused-but-set-variable"
#pragma clang diagnostic ignored "-Wunused-but-set-parameter"
typedef int boolean;
typedef unsigned char byte;
typedef unsigned short word;
#define TRUE 1
#define FALSE 0
#define NONE -1
#define MAXIMUM_MACHINE_NAME_LENGTH 16
#define JOIN_GAME_TOKEN_LENGTH 16
#define NETWORK_GAME_MESSAGE_VERSION 1
#define HALO_PORT_NETWORK_VERSION 43
#define csmemcmp memcmp
#define csmemcpy memcpy
#define CHECK(x) do {if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}} while(0)
enum {_network_game_server_state_pregame, _network_game_server_state_ingame};
enum {_message_client_join_game_request, _network_game_packet_class_client_pregame,
 _message_server_machine_accepted, _message_server_machine_rejected,
 _network_game_server_rejection_reason_bad_join_token, _rejection_code_game_is_full,
 _network_game_server_rejection_reason_game_not_open};
struct network_game {int machine_count;};
struct network_game_server {int unused;};
struct network_game_server_client_machine {int unused;};
struct network_connection {int unused;};
struct transport_address {struct {unsigned long long_words[1];} address;};
#define IPV4_LOOPBACK_ADDRESS 0x7f000001UL
struct network_machine {wchar_t name[16];};
struct message_client_join_game_request {wchar_t machine_name[16];char hardware_id[32];byte join_game_token[16];byte link_pass[192];byte link_proof[64];word network_version;};
struct message_server_machine_accepted {short machine_index;long random_seed;};
struct message_server_machine_rejected {int reason;};
struct network_message {word header;};
static struct network_game game;
static struct network_machine machine;
static struct network_message fake_reply;
static int state,late,joined,decode_ok,token_ok,slots,open_game,banned,accepted,name_blocked;
static int camera_locked;
static word join_version;
static int replaceable_bots;
static int bots_population_can_join(struct network_game_server *s) {(void)s;return replaceable_bots;}
static unsigned long peer_address;
static void third_person_session_lock(void) {camera_locked=1;}
static int decode(void *out) {
 struct message_client_join_game_request *q=out;
 memset(q,0,sizeof(*q));memset(q->join_game_token,token_ok?7:8,16);
 q->network_version=join_version;return decode_ok;
}
static int accept(void) {if(banned)return 0;accepted++;return 1;}
#define network_game_server_accepts_late_joins(s) late
#define network_game_server_get_state(s,p) state
#define network_game_server_client_machine_is_joined_to_game(s,m) joined
#define decode_network_game_message(q,...) decode(q)
#define network_game_has_free_player_slot(g) slots
#define network_game_server_get_game(s) (&game)
#define network_game_server_set_machine_hardware_id(m,id) ((void)0)
#define network_game_server_set_machine_link(...) ((void)0)
#define network_game_server_clean_name(...) ((void)0)
#define wide_to_ascii(...) ((void)0)
#define network_connection_get_address(c,a,l) ((a)->address.long_words[0]=peer_address)
#define network_game_server_game_is_open(s) open_game
#define network_game_generate_join_game_token(t) memset(t,7,16)
#define network_game_server_accept_client_machine_into_game(s,m) accept()
#define network_game_server_get_client_machine(s,m,i) (&machine)
#define match_assert(...) ((void)0)
#define network_game_get_random_seed() 0
#define create_network_game_message(...) (&fake_reply)
#define GET_MESSAGE_SIZE(h) 2
#define network_game_server_get_client_connection(m) ((struct network_connection *)0)
#define network_game_server_write(...) 1
#define network_game_server_send_game_settings_to_client_machine(...) 1
#define network_game_server_send_game_data_pregame(s) 1
#define network_game_server_last_refusal_code() 0
#define network_chat_name_blocked(...) name_blocked
#define network_game_server_refuse_with(...) ((void)0)
#define network_event(...) ((void)0)
''' + join + r'''
static void reset(void) {
 state=0;late=joined=banned=accepted=name_blocked=0;
 camera_locked=replaceable_bots=0;peer_address=2;game.machine_count=1;
 decode_ok=token_ok=slots=open_game=1;
 join_version=HALO_PORT_NETWORK_VERSION;
}
static int try_join(void) {
 word packet[16]={0};struct network_game_server s;struct network_game_server_client_machine m;
 return network_game_server_handle_message_client_join_game_request(&s,&m,packet,sizeof(packet));
}
int main(void) {
 int i;
 word invalid_versions[]={0,41,42,65,66,67,68,69,70,32712,32713,32714,32716,32717,32718,32720,32721,32722,32724,32725,32726,32728,32729,32730,32732,32733,65535};
 {
  for(i=0;i<(int)(sizeof(invalid_versions)/sizeof(invalid_versions[0]));i++) {
   reset();join_version=invalid_versions[i];
   CHECK(!try_join() && !accepted && !camera_locked);
  }
  reset();CHECK(try_join()==1);CHECK(accepted==1);CHECK(camera_locked==accepted);
  reset();peer_address=IPV4_LOOPBACK_ADDRESS;
  CHECK(try_join()==1);CHECK(!camera_locked);
  reset();peer_address=IPV4_LOOPBACK_ADDRESS;game.machine_count=2;
  CHECK(try_join()==1);CHECK(camera_locked==accepted);
  reset();decode_ok=0;CHECK(!try_join() && !accepted);
  reset();token_ok=0;CHECK(!try_join() && !accepted);
  reset();slots=0;CHECK(!try_join() && !accepted);
  reset();slots=0;replaceable_bots=1;
  CHECK(try_join()==1);CHECK(accepted==1);
  reset();slots=0;replaceable_bots=1;token_ok=0;CHECK(!try_join() && !accepted);
  reset();slots=0;replaceable_bots=1;open_game=0;CHECK(!try_join() && !accepted);
  reset();slots=0;replaceable_bots=1;banned=1;CHECK(!try_join() && !accepted);
  reset();open_game=0;CHECK(!try_join() && !accepted);
  reset();banned=1;CHECK(!try_join() && !accepted);
  reset();name_blocked=1;CHECK(!try_join() && !accepted);
  reset();state=1;CHECK(!try_join() && !accepted);
  reset();state=1;late=1;CHECK(try_join()==1);CHECK(accepted==1);
  reset();joined=1;try_join();CHECK(!accepted && !camera_locked);
 }
 return 0;
}
'''
    path = tmp_path / "admission.c"
    path.write_text(code)
    exe = compile_tool(tmp_path, [path])
    run = subprocess.run([str(exe)], capture_output=True, text=True)
    assert run.returncode == 0, run.stdout + run.stderr


def test_gameplay_direction_and_mcc_boundary(tmp_path):
    source = (ROOT / "port/linux/game/network_distributed.c").read_text()
    dispatch = function(source, "network_distributed_handle_message")
    family = dispatch[dispatch.index("\tif (header.type >= _distributed_message_mcc_grenades"):
                      dispatch.index("\t/* a tick's messages in one:")]
    direction = dispatch[dispatch.index("\t/* (each kind from the host, or from a client) */"):
                         dispatch.index("\tif (!distributed_message_epoch_valid")]
    kinds = enum_with((ROOT / "port/linux/game/network_distributed.h").read_text(),
                      "_distributed_message_player_prediction")
    code = r'''
#include <stdio.h>
#define NONE -1
enum {_game_connection_local,_game_connection_network_client,_game_connection_network_server};
static int connection,mcc,applied;
#define HALO_PORT_MAXIMUM_NETWORK_MACHINES 4
static struct {int heard;} distributed_round_trips[4];
#define system_milliseconds() 1
#define game_connection() connection
#define mcc_cache_tags_loaded() mcc
''' + kinds + r'''
static void check(int type,int machine_index) {
 struct {int type,game_time;} header;
 header.type=type;header.game_time=1;
''' + family + direction + r'''
 applied++;
}
int main(void) {
 /* Damage, spawns, scores, inventories, world state and MCC state are all
  * host-to-client messages. Neither client build may submit them to a host. */
 int types[]={_distributed_message_damage_events,_distributed_message_object_changes,
  _distributed_message_player_statistics,_distributed_message_inventories,
  _distributed_message_game_state,_distributed_message_mcc_grenades,
  _distributed_message_mcc_campaign,_distributed_message_mcc_object_names,
  _distributed_message_mcc_game_speed};
 unsigned i;
 for(i=0;i<sizeof(types)/sizeof(types[0]);i++) {
  applied=0;mcc=1;connection=_game_connection_network_server;check(types[i],1);
  if(applied) return 1;
  connection=_game_connection_network_client;check(types[i],NONE);if(applied!=1) return 2;
  if(types[i]>=_distributed_message_mcc_grenades) {
   mcc=0;check(types[i],NONE);if(applied!=1) return 3;
  }
 }
 applied=0;connection=_game_connection_network_client;check(_distributed_message_player_inputs,NONE);
 if(applied) return 4;
 connection=_game_connection_network_server;check(_distributed_message_player_inputs,NONE);
 if(applied) return 5;
 check(_distributed_message_player_inputs,1);if(applied!=1) return 6;
 applied=0;mcc=0;connection=_game_connection_network_server;
 check(_distributed_message_client_build,1);if(applied!=1)return 7;
 connection=_game_connection_network_client;
 check(_distributed_message_client_build,NONE);if(applied!=1)return 8;
 /* OpenCE fire origins remain client-to-host in both map families. */
 for(mcc=0;mcc<2;mcc++) {
  applied=0;connection=_game_connection_network_client;
  check(_distributed_message_fire_origins,NONE);if(applied)return 9;
  connection=_game_connection_network_server;
  check(_distributed_message_fire_origins,NONE);if(applied)return 10;
  check(_distributed_message_fire_origins,1);if(applied!=1)return 11;
 }
 return 0;
}
'''
    path = tmp_path / "direction.c"
    path.write_text(code)
    exe = compile_tool(tmp_path, [path])
    run = subprocess.run([str(exe)], capture_output=True, text=True)
    assert run.returncode == 0, run.stdout + run.stderr


def test_chat_and_mcc_message_ids_are_disjoint():
    actual = network_format.items()
    messages = {key: int(value) for key, value in actual.items()
                if key.startswith("enum _distributed_message_")}
    assert len(messages.values()) == len(set(messages.values()))
    assert messages["enum _distributed_message_aim_assist_policy"] == 90
    assert messages["enum _distributed_message_shield_effects"] == 91
    assert messages["enum _distributed_message_client_build"] == 92
    assert messages["enum _distributed_message_fire_origins"] == 93
    assert messages["enum _distributed_message_map_vote"] == 94
    assert messages["enum _distributed_message_map_vote_choice"] == 95
    header = (ROOT / "port/linux/game/network_distributed.h").read_text()
    assert messages["enum _distributed_message_checkpoint"] == 96
    camera = (ROOT / "port/linux/game/third_person_protocol.h").read_text()
    camera_join = int(re.search(r"#define THIRD_PERSON_PROTOCOL_JOIN_KIND (\d+)", camera).group(1))
    assert camera_join == 102 and camera_join not in messages.values()
    assert header.index("_distributed_message_aim_assist_policy = 90") < header.index("_distributed_message_mcc_grenades = 97")
    assert [messages["enum _distributed_message_chat_" + name]
            for name in ("up", "down", "config")] == [87, 88, 89]
    assert [messages["enum _distributed_message_mcc_" + name]
            for name in ("grenades", "campaign", "object_names", "game_speed")] == [97, 98, 99, 100]
    # The receive boundary must admit chat in ordinary sessions while rejecting
    # MCC data before dispatch; it also applies recursively to batched messages.
    receiver = function((ROOT / "port/linux/game/network_distributed.c").read_text(),
                        "network_distributed_handle_message")
    assert "header.type >= _distributed_message_mcc_grenades && !mcc_cache_tags_loaded()" in receiver
