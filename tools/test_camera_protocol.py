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
#include "mcc_protocol.h"
typedef unsigned char byte;
typedef unsigned short word;
typedef int boolean;
#define TRUE 1
#define FALSE 0
#define CHECK(x) do {if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
#define match_assert(...) ((void)0)
#define HALO_PORT_ADVERTISED_DISTRIBUTED_FLAG 1
#define THIRD_PERSON_POLICY_MASK 24
#define THIRD_PERSON_POLICY_SHIFT 3
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
unsigned int mcc_protocol_version(char const *map){return !strncmp(map,"mcc_maps",8)?MCC_PROTOCOL_VERSION:32718;}

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
    CHECK(HALO_PORT_NETWORK_VERSION==32718 && MCC_PROTOCOL_VERSION==67);
    host=&server;third_person_session_host_begin();CHECK(!third_person_session_active());
    network_game_server_open_game(&server);CHECK(!third_person_session_locked());
    CHECK(third_person_session_set(1));CHECK(third_person_session_set(0));
    CHECK(third_person_session_version("bloodgulch")==32718);
    CHECK(third_person_session_version("mcc_maps\\test")==67);
    CHECK(third_person_session_compatible("mcc_maps\\test",67,9));
    CHECK(!third_person_session_compatible("mcc_maps\\test",66,9));
    CHECK(third_person_session_compatible("bloodgulch",32718,1));
    CHECK(!third_person_session_compatible("bloodgulch",41,1));
    CHECK(!third_person_session_compatible("bloodgulch",32714,1));
    CHECK(!third_person_session_compatible("bloodgulch",32716,1));
    CHECK(!third_person_session_compatible("mcc_maps\\test",32717,9));
    CHECK(!third_person_session_compatible("bloodgulch",32712,1));
    CHECK(!third_person_session_compatible("mcc_maps\\test",32713,9));
    CHECK(third_person_session_compatible("bloodgulch",32720,1));
    CHECK(!third_person_session_compatible("bloodgulch",32721,1));
    CHECK(!third_person_session_compatible("bloodgulch",32720,0));
    CHECK(!third_person_session_compatible("mcc_maps\\test",32721,1));
    CHECK(third_person_session_compatible("mcc_maps\\test",32721,9));
    CHECK(!third_person_session_compatible("bloodgulch",32708,1));
    CHECK(third_person_session_set(1));CHECK(server.game.cooperative_flags==8);
    CHECK(third_person_session_version("bloodgulch")==32720);
    CHECK(third_person_session_version("mcc_maps\\test")==32721);
    CHECK(lobby_join_reset());CHECK(!third_person_session_set(0));
    CHECK(!lobby_join.adding && lobby_join.controller==NONE);
    CHECK(third_person_session_active() && third_person_session_locked());
    CHECK(THIRD_PERSON_PROTOCOL_JOIN_KIND!=MCC_PROTOCOL_JOIN_KIND);
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
    for(i=0;i<10;i++) {
        saved=((byte *)outer)[i];((byte *)outer)[i]^=1;
        CHECK(!third_person_protocol_unwrap(outer,520,inner,&size));
        CHECK(!network_game_server_handle_client_message(&server,&machine,outer,520));
        ((byte *)outer)[i]=saved;
    }
    ((byte *)outer)[519]=_message_client_game_start_request;
    CHECK(!third_person_protocol_unwrap(outer,520,inner,&size));
    /* A matching MCC envelope passes to the existing MCC validator unchanged. */
    build_message_header(join,40,2,0);((byte *)join)[2]=MCC_PROTOCOL_JOIN_KIND;
    CHECK(third_person_protocol_join(join,40,outer)==outer);
    CHECK(third_person_protocol_unwrap(outer,48,inner,&size));
    CHECK(size==40 && !memcmp(join,inner,40));
    CHECK(network_game_server_handle_client_message(&server,&machine,outer,48));
    CHECK(!network_game_server_handle_client_message(&server,&machine,join,40));
    CHECK(!third_person_protocol_join(outer,48,inner)); /* no recursive envelopes */
    ((byte *)join)[2]=93;CHECK(!third_person_protocol_join(join,40,outer));
    /* Established connections keep the ordinary dispatcher and its direction checks. */
    machine.joined=1;CHECK(network_game_server_handle_client_message(&server,&machine,join,40));
    machine.joined=0;CHECK(!network_game_server_handle_client_message(&server,&machine,join,40));
    third_person_session_host_end();host=NULL;CHECK(!third_person_session_active());
    third_person_session_client_begin(32720);CHECK(third_person_session_active());
    network_game_client_late_join_time=9;network_game_client_late_join_clock_pending=1;
    network_game_client_start_pending=1;network_game_client_incompatibility_told=1;
    network_game_client_reset(&client,0);CHECK(!third_person_session_active());
    CHECK(!network_game_client_late_join_time&&!network_game_client_late_join_clock_pending);
    CHECK(!network_game_client_start_pending&&!network_game_client_incompatibility_told);
    CHECK(resets>0&&sends>0);
    third_person_session_client_begin(32721);client.connection=&client;
    network_game_client_reset(&client,1);CHECK(!third_person_session_active());
    mode="gulchce_third_person";host=&server;third_person_session_host_begin();
    network_game_client_reset(&client,0);CHECK(third_person_session_active());
    third_person_session_host_end();host=NULL;
    third_person_session_client_begin(41);CHECK(!third_person_session_active());
    third_person_session_client_begin(32716);CHECK(!third_person_session_active());
    third_person_session_client_begin(32717);CHECK(!third_person_session_active());
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
    kind = items["define THIRD_PERSON_PROTOCOL_JOIN_KIND"]
    assert kind not in [value for key, value in items.items()
                        if key.startswith("enum _distributed_message_")]
    assert kind != items["define MCC_PROTOCOL_JOIN_KIND"]


def test_camera_mcc_inner_version_reaches_admission_only_when_current(tmp_path):
    """Run both envelopes through the complete dispatcher, not only its camera gate."""
    camera = (ROOT / "port/linux/game/third_person_protocol.c").read_text()
    mcc = (ROOT / "port/linux/game/mcc_protocol.c").read_text()
    server = (ROOT / "source/networking/network_server_message_handler.c").read_text()
    implementation = "\n".join(function(camera, name) for name in (
        "third_person_protocol_inner_valid", "third_person_protocol_join",
        "third_person_protocol_unwrap"))
    implementation += "\n" + "\n".join(function(mcc, name) for name in (
        "mcc_protocol_client_begin", "mcc_protocol_client_clear", "mcc_protocol_join",
        "mcc_protocol_unwrap", "mcc_protocol_forget", "mcc_protocol_peer",
        "mcc_protocol_reset", "mcc_protocol_allows"))
    # The sink below observes admission routing and runs the real MCC capability
    # check. The separate test_mcc_protocol join-handler harness covers native
    # packet decoding, tokens, bans, slots and session-state authorization.
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
#include "mcc_protocol.h"
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
static struct network_game_server_client_machine *mcc_protocol_peers[HALO_PORT_MAXIMUM_NETWORK_MACHINES];
static int mcc_protocol_client_capable;
static int camera_active,join_calls,join_capable,unexpected_calls;
static int unexpected(void) {unexpected_calls++;return FALSE;}
int third_person_session_active(void) {return camera_active;}
static int mcc_level_name(char const *map) {return !strncmp(map,"mcc_maps\\",9);}
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
    word *message,short bytes,boolean capable) {
    join_calls++;join_capable=capable;
    if(bytes!=32 || GET_MESSAGE_TYPE(*message)!=_message_type_packet ||
        ((byte *)message)[bytes-1]!=_message_client_join_game_request)return FALSE;
    return mcc_protocol_peer(machine,capable) && mcc_protocol_allows(&server->game,machine);
}
''' + function(server, "network_game_server_handle_client_message") + r'''
int main(void) {
    struct network_game_server server;
    struct network_game_server_client_machine machine={0};
    word plain[16]={0},mcc_join[MCC_PROTOCOL_JOIN_MAX/2],camera_join[THIRD_PERSON_PROTOCOL_JOIN_MAX/2];
    word *message;
    short bytes;
    CHECK(MCC_PROTOCOL_VERSION==67);
    build_message_header(plain,sizeof(plain),_message_type_packet,0);
    ((byte *)plain)[sizeof(plain)-1]=_message_client_join_game_request;
    strcpy(server.game.map.name,"mcc_maps\\test");
    mcc_protocol_client_begin(MCC_PROTOCOL_ADVERTISED_FLAG);
    /* The matching camera discovery version still requires its inner MCC revision.
     * Check the composed route before the direct one. */
    for(camera_active=1;camera_active>=0;camera_active--) {
        CHECK(mcc_protocol_join(plain,sizeof(plain),mcc_join)==mcc_join);
        ((byte *)mcc_join)[4]=65;
        ((byte *)mcc_join)[5]=0;
        message=mcc_join;bytes=40;
        if(camera_active) {
            CHECK(third_person_protocol_join(mcc_join,40,camera_join)==camera_join);
            message=camera_join;bytes=48;
        }
        mcc_protocol_reset();join_calls=join_capable=0;
        if(network_game_server_handle_client_message(&server,&machine,message,bytes)) {
            fprintf(stderr,"old MCC version reached admission\n");return 64;
        }
        CHECK(join_calls==0 && !mcc_protocol_allows(&server.game,&machine));
        CHECK(mcc_protocol_join(plain,sizeof(plain),mcc_join)==mcc_join);
        if(camera_active)CHECK(third_person_protocol_join(mcc_join,40,camera_join)==camera_join);
        CHECK(network_game_server_handle_client_message(&server,&machine,message,bytes));
        CHECK(join_calls==1 && join_capable && mcc_protocol_allows(&server.game,&machine));
        CHECK(!unexpected_calls);
    }
    /* Camera proof without an MCC envelope never grants MCC capability. */
    camera_active=1;mcc_protocol_reset();join_calls=join_capable=0;
    CHECK(third_person_protocol_join(plain,sizeof(plain),camera_join)==camera_join);
    CHECK(!network_game_server_handle_client_message(&server,&machine,camera_join,40));
    CHECK(join_calls==1 && !join_capable && !mcc_protocol_allows(&server.game,&machine));
    /* Legacy camera and ordinary sessions do not require MCC capability. */
    strcpy(server.game.map.name,"levels\\test\\bloodgulch\\bloodgulch");
    join_calls=join_capable=0;
    CHECK(network_game_server_handle_client_message(&server,&machine,camera_join,40));
    CHECK(join_calls==1 && !join_capable);
    camera_active=0;join_calls=join_capable=0;mcc_protocol_client_clear();
    CHECK(mcc_protocol_join(plain,sizeof(plain),mcc_join)==plain);
    CHECK(network_game_server_handle_client_message(&server,&machine,plain,sizeof(plain)));
    CHECK(join_calls==1 && !join_capable && !unexpected_calls);
    return 0;
}
'''
    source = tmp_path / "camera_mcc_admission.c"
    source.write_text(code)
    exe = compile_tool(tmp_path, [source])
    run = subprocess.run([str(exe)], capture_output=True, text=True)
    assert run.returncode == 0, run.stdout + run.stderr

    # If the production MCC revision check is removed, the unchanged camera
    # envelope cannot hide the missing gate: the old revision must reach our sink
    # and fail the very first (nested) rejection check.
    check = "in[4] != (MCC_PROTOCOL_VERSION & 255)"
    assert code.count(check) == 1
    source.write_text(code.replace(check, "0"))
    exe = compile_tool(tmp_path, [source])
    run = subprocess.run([str(exe)], capture_output=True, text=True)
    assert run.returncode == 64 and "old MCC version reached admission" in run.stderr, run.stdout + run.stderr
