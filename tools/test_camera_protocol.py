"""Exercise camera admission, including direct joins that bypass discovery."""
from pathlib import Path
import re
import subprocess

from harness import enum_with, function
from test_mcc_identity import compile_tool

ROOT = Path(__file__).resolve().parents[1]


def test_camera_admission(tmp_path):
    camera = (ROOT / "port/linux/game/third_person_protocol.c").read_text()
    camera = "\n".join(line for line in camera.splitlines() if not line.startswith("#include"))
    server = (ROOT / "source/networking/network_server_message_handler.c").read_text()
    # Execute the real dispatcher's size and admission gates before message dispatch.
    gate = function(server, "network_game_server_handle_client_message")
    gate = gate[:gate.index("\tmessage_type =")]
    gate += "(void)result;(void)message_type;(void)packet_type;return TRUE;\n}\n"
    reset = function((ROOT / "source/networking/network_client_manager.c").read_text(), "network_game_client_reset")
    open_game = function((ROOT / "source/networking/network_server_manager.c").read_text(), "network_game_server_open_game")
    lobby = function((ROOT / "port/linux/game/menu_functions.c").read_text(), "lobby_join_reset")
    limits = (ROOT / "port/linux/include/halo_port_limits.h").read_text()
    version = re.search(r"^#define HALO_PORT_NETWORK_VERSION \d+$", limits, re.M).group()
    code = r'''
#include <stdio.h>
#include <string.h>
#include "third_person_protocol.h"
typedef unsigned char byte;
typedef unsigned short word;
typedef int boolean;
#define TRUE 1
#define FALSE 0
#define CHECK(x) do {if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
#define match_assert(...) ((void)0)
#define HALO_PORT_ADVERTISED_DISTRIBUTED_FLAG 1
#define THIRD_PERSON_POLICY_MASK 48
#define THIRD_PERSON_POLICY_SHIFT 4
struct network_game {int machine_count;unsigned char cooperative_flags;};
struct network_game_server {struct network_game game;int flags;void *connection;};
static struct network_game_server *host;
static char const *mode="opence_crossplay";
static int resets,sends;
char const *config_string(char const *name){(void)name;return mode;}
int config_write(char const *name,char const *value){(void)name;mode=value;return 1;}
static struct network_game_server *global_network_game_server_get(void){return host;}
struct network_game *network_game_server_get_game(struct network_game_server *s){return &s->game;}
boolean network_game_server_send_game_data_pregame(struct network_game_server *s){(void)s;sends++;return 1;}
static short third_person_host_policy(void){return 1;}
static void third_person_reset(void){resets++;}

struct network_game_server_client_machine {int joined;};
static int network_game_server_client_machine_is_joined_to_game(struct network_game_server *s,
    struct network_game_server_client_machine *m) {(void)s;return m->joined;}
static void network_event(char const *fmt,...) {(void)fmt;}
#define NONE -1
#define SET_FLAG(flags,bit,value) ((flags) = ((flags) & ~(1 << (bit))) | ((value) << (bit)))
#define csmemset memset
#define _network_game_server_game_open_bit 0
static void network_server_allow_client_connections(void *c,int allow) {(void)c;(void)allow;}
static void halo_virtual_gamepads_start(void) {}
static struct {int adding,controller;} lobby_join;
enum {_network_game_client_state_searching, _network_game_client_connection_established_bit,
 _network_game_client_join_request_sent_bit, _network_game_client_error_unknown, _network_game_client_error_none};
struct network_game_client {struct network_game game;void *connection;int machine_index,state,flags,error,
 join_in_progress,last_broadcast_search_time,next_update_number,connection_silent,seconds_to_game_start;};
static int network_game_client_late_join_time,network_game_client_late_join_clock_pending,
 network_game_client_start_pending,network_game_client_incompatibility_told,network_game_client_add_player_requested[4];
static void network_game_invalidate(struct network_game *g){memset(g,0,sizeof(*g));}
static int network_connection_connected(void *c){(void)c;return 1;}
static int network_connection_active(void *c){(void)c;return 1;}
static int network_connection_disconnect(void *c){(void)c;return 1;}
static void network_game_client_set_error(struct network_game_client *c,int e){c->error=e;}

''' + version + "\n" + (ROOT / "source/bungie_net/common/message_header.h").read_text().replace("#pragma once", "") + "\n" + enum_with(
        (ROOT / "source/networking/network_messages.h").read_text(),
        "_message_client_broadcast_game_search") + r'''
void build_message_header(word *out,word bytes,byte type,byte flags) {*out=(word)((bytes<<4)|(type<<2)|flags);}
''' + camera + "\n" + reset + "\n" + open_game + "\n" + lobby + "\n" + gate + r'''
int main(void) {
    struct network_game_server server={0};
    struct network_game_client client={0};
    struct network_game_server_client_machine machine={0};
    word join[256]={0},outer[260],inner[260],size;
    byte saved;
    int i,bytes;

    CHECK(HALO_PORT_NETWORK_VERSION==43);
    host=&server;third_person_session_host_begin();CHECK(!third_person_session_active());
    network_game_server_open_game(&server);CHECK(!third_person_session_locked());
    server.game.cooperative_flags=8; /* upstream vehicle riding survives camera policy changes */
    CHECK(third_person_session_set(1));CHECK(server.game.cooperative_flags==24);
    CHECK(third_person_session_set(0));CHECK(server.game.cooperative_flags==8);
    {
        char const *maps[]={"maps\\bloodgulch","custom_maps\\test","mcc_maps\\test"};
        int family;
        for(family=0;family<3;family++) {
            CHECK(third_person_session_version(maps[family])==43);
            CHECK(third_person_session_compatible(maps[family],43,1));
            CHECK(!third_person_session_compatible(maps[family],43,0));
            CHECK(third_person_session_compatible(maps[family],THIRD_PERSON_PROTOCOL_VERSION,1));
            CHECK(!third_person_session_compatible(maps[family],73,9));
            CHECK(!third_person_session_compatible(maps[family],32738,1));
            CHECK(!third_person_session_compatible(maps[family],32740,1));
            CHECK(!third_person_session_compatible(maps[family],32742,9));
        }
    }
    CHECK(third_person_session_set(1));CHECK(server.game.cooperative_flags==24);
    CHECK(third_person_session_version("bloodgulch")==THIRD_PERSON_PROTOCOL_VERSION);
    CHECK(third_person_session_version("mcc_maps\\test")==THIRD_PERSON_PROTOCOL_VERSION);
    CHECK(lobby_join_reset());CHECK(!third_person_session_set(0));
    CHECK(!lobby_join.adding && lobby_join.controller==NONE);
    CHECK(third_person_session_active() && third_person_session_locked());
    for(bytes=3;bytes<=512;bytes++) {
        build_message_header(join,(word)bytes,_message_type_packet,0);
        ((byte *)join)[bytes-1]=_message_client_join_game_request;
        CHECK(third_person_protocol_join(join,(word)bytes,outer)==outer);
        CHECK(third_person_protocol_unwrap(outer,(word)(bytes+8),inner,&size));
        CHECK(size==bytes && !memcmp(inner,join,bytes));
        CHECK(network_game_server_handle_client_message(&server,&machine,outer,(short)(bytes+8)));
        /* Stock/main/old camera direct joins have no camera envelope. */
        CHECK(!network_game_server_handle_client_message(&server,&machine,join,(short)bytes));
    }
    for(i=0;i<=521;i++) if(i!=520) CHECK(!third_person_protocol_unwrap(outer,(word)i,inner,&size));
    /* Old camera peers cannot bypass discovery by sending their envelope. */
    for(i=32720;i<=32725;i++) {
        ((byte *)outer)[4]=(byte)i;((byte *)outer)[5]=(byte)(i>>8);
        CHECK(!third_person_protocol_unwrap(outer,520,inner,&size));
        CHECK(!network_game_server_handle_client_message(&server,&machine,outer,520));
    }
    ((byte *)outer)[4]=THIRD_PERSON_PROTOCOL_VERSION&255;
    ((byte *)outer)[5]=THIRD_PERSON_PROTOCOL_VERSION>>8;
    for(i=0;i<10;i++) {
        saved=((byte *)outer)[i];((byte *)outer)[i]^=1;
        CHECK(!third_person_protocol_unwrap(outer,520,inner,&size));
        CHECK(!network_game_server_handle_client_message(&server,&machine,outer,520));
        ((byte *)outer)[i]=saved;
    }
    ((byte *)outer)[519]=_message_client_game_start_request;
    CHECK(!third_person_protocol_unwrap(outer,520,inner,&size));

    /* Retired MCC and nested camera envelopes cannot pass as ordinary joins. */
    build_message_header(join,40,2,0);((byte *)join)[2]=101;
    CHECK(!third_person_protocol_join(join,40,outer));
    ((byte *)join)[2]=THIRD_PERSON_PROTOCOL_JOIN_KIND;
    CHECK(!third_person_protocol_join(join,40,outer));
    /* Established connections keep the ordinary dispatcher and its direction checks. */
    machine.joined=1;CHECK(network_game_server_handle_client_message(&server,&machine,join,40));
    machine.joined=0;CHECK(!network_game_server_handle_client_message(&server,&machine,join,40));
    third_person_session_host_end();host=NULL;CHECK(!third_person_session_active());
    third_person_session_client_begin(THIRD_PERSON_PROTOCOL_VERSION);CHECK(third_person_session_active());
    network_game_client_late_join_time=9;network_game_client_late_join_clock_pending=1;
    network_game_client_start_pending=1;network_game_client_incompatibility_told=1;
    network_game_client_reset(&client,0);CHECK(!third_person_session_active());
    CHECK(!network_game_client_late_join_time&&!network_game_client_late_join_clock_pending);
    CHECK(!network_game_client_start_pending&&!network_game_client_incompatibility_told);
    CHECK(resets>0&&sends>0);
    third_person_session_client_begin(THIRD_PERSON_PROTOCOL_VERSION);client.connection=&client;
    network_game_client_reset(&client,1);CHECK(!third_person_session_active());
    mode="gulchce_third_person";host=&server;third_person_session_host_begin();
    network_game_client_reset(&client,0);CHECK(third_person_session_active());
    third_person_session_host_end();host=NULL;
    third_person_session_client_begin(41);CHECK(!third_person_session_active());
    third_person_session_client_begin(32716);CHECK(!third_person_session_active());
    third_person_session_client_begin(32717);CHECK(!third_person_session_active());
    third_person_session_client_begin(32720);CHECK(!third_person_session_active());
    third_person_session_client_begin(32721);CHECK(!third_person_session_active());
    third_person_session_client_begin(32724);CHECK(!third_person_session_active());
    third_person_session_client_begin(32725);CHECK(!third_person_session_active());
    third_person_session_client_begin(32734);CHECK(!third_person_session_active());
    third_person_session_client_begin(32736);CHECK(!third_person_session_active());
    third_person_session_client_begin(32737);CHECK(!third_person_session_active());
    build_message_header(join,3,_message_type_packet,0);((byte *)join)[2]=_message_client_join_game_request;
    CHECK(network_game_server_handle_client_message(&server,&machine,join,3));
    third_person_session_client_begin(32708);CHECK(!third_person_session_active());
    CHECK(!third_person_session_set(1));
    return 0;
}
'''
    path = tmp_path / "camera_admission.c"
    path.write_text(code)
    exe = compile_tool(tmp_path, [path])
    run = subprocess.run([str(exe)], capture_output=True, text=True)
    assert run.returncode == 0, run.stdout + run.stderr


def test_camera_join_kind_is_reserved():
    import network_format
    items = network_format.items()
    kind = re.search(r"#define THIRD_PERSON_PROTOCOL_JOIN_KIND (\d+)",
                     (ROOT / "port/linux/game/third_person_protocol.h").read_text()).group(1)
    assert kind not in [value for key, value in items.items()
                        if key.startswith("enum _distributed_message_")]
    assert kind == "102"


def test_camera_and_crossplay_dispatch_admit_all_map_families(tmp_path):
    """Run both envelopes through the complete dispatcher, not only its camera gate."""
    camera = (ROOT / "port/linux/game/third_person_protocol.c").read_text()
    server = (ROOT / "source/networking/network_server_message_handler.c").read_text()
    implementation = "\n".join(function(camera, name) for name in (
        "third_person_protocol_inner_valid", "third_person_protocol_join",
        "third_person_protocol_unwrap"))
    unrelated_handlers = (
        "client_add_player_request_ingame", "client_add_player_request_pregame",
        "client_game_start_request", "client_graceful_game_exit_postgame",
        "client_graceful_game_exit_pregame", "client_loaded",
        "client_map_is_precached_pregame", "client_player_settings_request",
        "client_remove_player_request_ingame", "client_remove_player_request_postgame",
        "client_remove_player_request_pregame", "client_settings_request",
        "client_switch_to_pregame",
    )
    stubs = "\n".join(
        f"#define network_game_server_handle_message_{name}(...) unexpected()"
        for name in unrelated_handlers)
    code = r'''
#include <stdio.h>
#include <string.h>
#include "third_person_protocol.h"
typedef unsigned char byte;
typedef unsigned short word;
typedef int boolean;
#define TRUE 1
#define FALSE 0
#define NUMBEROF(x) (sizeof(x)/sizeof((x)[0]))
#define HALO_PORT_MAXIMUM_NETWORK_MACHINES 4
#define CHECK(x) do {if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
#define match_assert(...) ((void)0)
#define MINIMUM_TRANSPORT_ERROR_MESSAGE_SIZE 4
#define TRANSPORT_ERROR_MESSAGE_TEXT_LENGTH 1
#define _network_game_server_state_ingame 1
struct network_game {struct {char name[64];} map;};
struct network_game_server {struct network_game game;};
struct network_game_server_client_machine {int joined;};
static int camera_active,join_calls,unexpected_calls;
static int unexpected(void) {unexpected_calls++;return FALSE;}
int third_person_session_active(void) {return camera_active;}
static int network_game_server_client_machine_is_joined_to_game(struct network_game_server *server,
    struct network_game_server_client_machine *machine) {(void)server;return machine->joined;}
static void network_event(char const *format,...) {(void)format;}
#define network_game_server_client_machine_heard(...) ((void)unexpected())
#define network_game_server_client_machine_is_loaded(...) unexpected()
#define network_game_server_get_client_machine(server,machine,index) (*(index)=0,unexpected())
#define network_game_server_get_state(...) unexpected()
#define network_distributed_handle_stream_message(...) ((void)unexpected())
#define network_voice_handles_message(...) unexpected()
#define network_voice_handle_message(...) ((void)unexpected())
#define mcc_load_network_handles_message(...) unexpected()
#define mcc_load_network_handle_message(...) ((void)unexpected())
#define mcc_admission_handles_message(...) unexpected()
#define mcc_admission_handle_message(...) ((void)unexpected())
#define mcc_share_game_handles_message(...) unexpected()
#define mcc_share_game_handle_message(...) ((void)unexpected())
#define map_share_handles_message(...) unexpected()
#define map_share_handle_message(...) ((void)unexpected())
#define network_map_vote_handles_message(...) unexpected()
#define network_map_vote_handle_message(...) ((void)unexpected())
#define transport_error_message_text(...) "unexpected transport error"
''' + stubs + "\n" + (ROOT / "source/bungie_net/common/message_header.h").read_text().replace(
        "#pragma once", "") + "\n" + enum_with(
            (ROOT / "source/networking/network_messages.h").read_text(),
            "_message_client_broadcast_game_search") + "\n" + enum_with(server, "_message_type_error") + r'''
void build_message_header(word *out,word bytes,byte type,byte flags) {*out=(word)((bytes<<4)|(type<<2)|flags);}
''' + implementation + r'''
static boolean network_game_server_handle_message_client_join_game_request(
    struct network_game_server *server,struct network_game_server_client_machine *machine,
    word *message,short bytes) {
    (void)server;(void)machine;
    join_calls++;
    if(bytes!=32 || GET_MESSAGE_TYPE(*message)!=_message_type_packet ||
        ((byte *)message)[bytes-1]!=_message_client_join_game_request)return FALSE;
    return TRUE;
}
''' + function(server, "network_game_server_handle_client_message") + r'''
int main(void) {
    struct network_game_server server;
    struct network_game_server_client_machine machine={0};

    word plain[16]={0},camera_join[THIRD_PERSON_PROTOCOL_JOIN_MAX/2];
    char const *maps[]={"maps\\bloodgulch","custom_maps\\test","mcc_maps\\test"};
    int family;
    build_message_header(plain,sizeof(plain),_message_type_packet,0);
    ((byte *)plain)[sizeof(plain)-1]=_message_client_join_game_request;
    for(family=0;family<3;family++) {
        strcpy(server.game.map.name,maps[family]);
        camera_active=0;join_calls=unexpected_calls=0;
        CHECK(network_game_server_handle_client_message(&server,&machine,plain,sizeof(plain)));
        CHECK(join_calls==1 && !unexpected_calls);
        camera_active=1;join_calls=0;
        CHECK(!network_game_server_handle_client_message(&server,&machine,plain,sizeof(plain)));
        CHECK(!join_calls);
        CHECK(third_person_protocol_join(plain,sizeof(plain),camera_join)==camera_join);
        CHECK(network_game_server_handle_client_message(&server,&machine,camera_join,sizeof(plain)+8));
        CHECK(join_calls==1 && !unexpected_calls);
        ((byte *)camera_join)[4]^=1;join_calls=0;
        CHECK(!network_game_server_handle_client_message(&server,&machine,camera_join,sizeof(plain)+8));
        CHECK(!join_calls);
    }
    return 0;
}
'''
    source = tmp_path / "camera_mcc_admission.c"
    source.write_text(code)
    exe = compile_tool(tmp_path, [source])
    run = subprocess.run([str(exe)], capture_output=True, text=True)
    assert run.returncode == 0, run.stdout + run.stderr
