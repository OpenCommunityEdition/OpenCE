"""Admission, framing and the audited differences from OpenCE 41.

The AGPL-3.0-or-later baseline fixture comes unchanged from OpenCommunityEdition/OpenCE
c507cb26b6ea7e89c7669fb7c911e27b80d5d779, port/linux/network_format.txt.
Keep this historical fixture unchanged; GulchCE's shared checkpoint agreement
has explicit differences below and does not claim OpenCE crossplay.
"""
from pathlib import Path
import re
import subprocess

from harness import enum_with, function
from test_mcc_identity import compile_tool
import network_format

ROOT = Path(__file__).resolve().parents[1]


def test_audited_changes_from_opence_41_contract():
    lines = (ROOT / "tools/fixtures/opence_network_41.txt").read_text().splitlines()
    assert lines[0] == "version 41"
    expected = {f"{kind} {name}": value for kind, name, value in
                (line.split(" ", 2) for line in lines[1:])}
    actual = network_format.items()
    # MCC appends two digests after the complete upstream settings prefix.
    # Compare the upstream size separately rather than accepting a fork size.
    assert actual["layout HALO_PORT_NETWORK_GAME_SIZE"] == "(HALO_PORT_NETWORK_GAME_PLAYERS_END + 0x3A + 64) /* MCC content and metadata SHA-256 */"
    actual["layout HALO_PORT_NETWORK_GAME_SIZE"] = "(HALO_PORT_NETWORK_GAME_PLAYERS_END + 0x3A)"
    # Join agreement, world epoch and the optional host scripted pose extend
    # existing upstream wire items. New kinds are checked separately.
    changed = {
        "fields client_join_game_request": "d7dd965724e6",
        "size message_client_join_game_request": "0x70 + 170 + 64 + 2",
        "struct distributed_message_header": "5bc35a4ad92a",
        "bound DISTRIBUTED_UNIT_STATE_MAXIMUM_SIZE": "3 + 4 + (4 + 2) + (12 + 3 * 6) + 2 + 2 * 2 + 4 * 2 + 2 * NUMBER_OF_PLAYER_POWERUPS + 2 + 1 + NUMBER_OF_DISTRIBUTED_POSE_VECTORS * 6",
    }
    assert {key: actual.get(key) for key in expected if actual.get(key) != expected[key]} == changed
    limits = (ROOT / "port/linux/include/halo_port_limits.h").read_text()
    assert re.search(r"#define HALO_PORT_NETWORK_VERSION 32738\b", limits)
    # All fields of the transmitted prefix match upstream, including names,
    # types and order. The Windows build also asserts the actual ABI offsets.
    header = (ROOT / "source/networking/network_game_manager.h").read_text()
    header = re.sub(r"/\*.*?\*/|//[^\n]*", " ", header, flags=re.S)
    block = next(network_format._blocks(header, r"^struct network_game\s*\{"))
    block = re.sub(r"\s*byte mcc_(?:content|metadata)_sha256\[32\];", "", block)
    assert network_format._hash(block) == 'd0025984b339'


def test_admission_and_settings(tmp_path):
    implementation = "\n".join(line for line in
        (ROOT / "port/linux/game/mcc_protocol.c").read_text().splitlines()
        if not line.startswith("#include"))
    handler = (ROOT / "source/networking/network_client_message_handler.c").read_text()
    reassembly = function(handler, "network_game_client_receive_game_settings_piece")
    code = r'''
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "mcc_protocol.h"
typedef unsigned char byte;
typedef unsigned short word;
typedef int boolean;
#define TRUE 1
#define FALSE 0
#define HALO_PORT_NETWORK_GAME_PLAYERS_END 6
#define HALO_PORT_NETWORK_VERSION 32738
#define HALO_PORT_MAXIMUM_NETWORK_MACHINES 4
#define NUMBEROF(a) (sizeof(a)/sizeof((a)[0]))
#define csmemcpy memcpy
#define csmemset memset
#define CHECK(x) do { if (!(x)) {fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;} } while (0)
struct network_game {struct {char name[64];} map;byte mcc_content_sha256[32];byte mcc_metadata_sha256[32];};
struct network_game_client {int unused;};
struct network_game_server {int unused;};
struct network_game_server_client_machine {int joined;};
static struct network_game_server_client_machine peers[4];
static int removed,applied;
static int mcc_level_name(char const *s) {return !strncmp(s,"mcc_maps\\",9);}
static void network_event(char const *fmt,...) {(void)fmt;}
''' + (ROOT / "source/bungie_net/common/message_header.h").read_text().replace("#pragma once", "") + "\n" + enum_with(
        (ROOT / "source/networking/network_messages.h").read_text(),
        "_message_client_broadcast_game_search") + r'''
void build_message_header(word *out,word bytes,byte type,byte flags) {*out=(word)((bytes<<4)|(type<<2)|flags);}
struct network_game_server_client_machine *network_game_server_get_client_machine_at_index(
    struct network_game_server *s,long i) {(void)s;return peers+i;}
boolean network_game_server_client_machine_is_joined_to_game(struct network_game_server *s,
    struct network_game_server_client_machine *p) {(void)s;return p->joined;}
boolean network_game_server_remove_client_machine_from_game(struct network_game_server *s,
    struct network_game_server_client_machine *p) {(void)s;p->joined=0;removed++;mcc_protocol_forget(p);return 1;}
static boolean network_game_client_game_settings_updated(struct network_game_client *c,struct network_game *g)
    {(void)c;(void)g;applied++;return 1;}
struct message_server_game_settings_update {word total_size,offset,length,pad;byte data[48];};
static struct network_game network_game_client_settings_staging;
static long network_game_client_settings_staging_size,network_game_client_settings_staging_total;
''' + implementation + "\n" + reassembly + r'''
int main(void) {
    struct network_game game;
    struct network_game_server server;
    struct message_server_game_settings_update piece;
    word original[16]={0},outer[MCC_PROTOCOL_JOIN_MAX / 2],inner[MCC_PROTOCOL_JOIN_MAX / 2],size;
    word expanded[200]={0};
    byte saved;
    int i;
    memset(&game,0,sizeof(game));strcpy(game.map.name,"levels\\test\\bloodgulch\\bloodgulch");
    CHECK(mcc_protocol_version(game.map.name)==32738);
    CHECK(mcc_protocol_settings_size(&game)==64);
    CHECK(mcc_protocol_settings_valid(&game,64));CHECK(!mcc_protocol_settings_valid(&game,96));
    CHECK(!mcc_protocol_settings_valid(&game,128));
    build_message_header(original,sizeof(original),3,0);
    ((byte *)original)[sizeof(original)-1]=_message_client_join_game_request;
    CHECK(mcc_protocol_join(original,sizeof(original),outer)==original);
    mcc_protocol_client_begin(MCC_PROTOCOL_ADVERTISED_FLAG);
    CHECK(mcc_protocol_join(original,sizeof(original),outer)==outer);
    CHECK(mcc_protocol_unwrap(outer,40,inner,&size));CHECK(size==32 && !memcmp(inner,original,32));
    /* Package transfer and required metadata identity need the updated join agreement.
     * Reject the previous MCC revision before interpreting settings. */
    CHECK(MCC_PROTOCOL_VERSION==73);
    for(i=67;i<=71;i++) {
        ((byte *)outer)[4]=(byte)i;CHECK(!mcc_protocol_unwrap(outer,40,inner,&size));
    }
    ((byte *)outer)[4]=MCC_PROTOCOL_VERSION;
    for(i=0;i<257;i++) if(i!=40) CHECK(!mcc_protocol_unwrap(outer,(word)i,inner,&size));
    for(i=0;i<10;i++) {
        saved=((byte *)outer)[i];((byte *)outer)[i]^=1;
        CHECK(!mcc_protocol_unwrap(outer,40,inner,&size));((byte *)outer)[i]=saved;
    }
    /* Protocol 41 link credentials exceed the old 256-byte MCC envelope.
     * Both the host's loopback join and a remote MCC peer use this path. */
    build_message_header(expanded,sizeof(expanded),_message_type_packet,0);
    ((byte *)expanded)[sizeof(expanded)-1]=_message_client_join_game_request;
    CHECK(mcc_protocol_join(expanded,sizeof(expanded),outer)==outer);
    CHECK(mcc_protocol_unwrap(outer,sizeof(expanded)+8,inner,&size));
    CHECK(size==sizeof(expanded) && !memcmp(inner,expanded,size));
    CHECK(!mcc_protocol_join(expanded,MCC_PROTOCOL_JOIN_MAX-7,outer));
    CHECK(mcc_protocol_join(original,sizeof(original),outer)==outer);
    ((byte *)outer)[39]=_message_client_game_start_request;
    CHECK(!mcc_protocol_unwrap(outer,40,inner,&size));
    strcpy(game.map.name,"mcc_maps\\Package\\level");
    CHECK(mcc_protocol_version(game.map.name)==MCC_PROTOCOL_VERSION);
    CHECK(mcc_protocol_settings_size(&game)==128);CHECK(!mcc_protocol_settings_valid(&game,96));
    CHECK(!mcc_protocol_settings_valid(&game,64));CHECK(mcc_protocol_settings_valid(&game,128));
    mcc_protocol_client_clear();CHECK(!mcc_protocol_settings_valid(&game,128));
    CHECK(!mcc_protocol_allows(&game,peers));
    CHECK(mcc_protocol_peer(peers,1));CHECK(mcc_protocol_allows(&game,peers));
    mcc_protocol_forget(peers);CHECK(!mcc_protocol_allows(&game,peers));
    for(i=0;i<4;i++) peers[i].joined=1;
    CHECK(mcc_protocol_peer(peers+1,1));CHECK(mcc_protocol_peer(peers+3,1));
    mcc_protocol_prune(&server,&game);CHECK(removed==2 && peers[1].joined && peers[3].joined);
    mcc_protocol_reset();CHECK(!mcc_protocol_allows(&game,peers+1));
    memset(&piece,0,sizeof(piece));piece.total_size=64;piece.length=48;
    strcpy((char *)piece.data,"levels\\test\\bloodgulch\\bloodgulch");
    memset(&network_game_client_settings_staging,255,sizeof(game));
    CHECK(network_game_client_receive_game_settings_piece(NULL,&piece));
    piece.offset=48;piece.length=16;memset(piece.data,0,sizeof(piece.data));
    CHECK(network_game_client_receive_game_settings_piece(NULL,&piece));CHECK(applied==1);
    CHECK(network_game_client_settings_staging.mcc_content_sha256[0]==0);
    CHECK(network_game_client_settings_staging.mcc_metadata_sha256[0]==0);
    piece.offset=0;piece.length=48;piece.total_size=128;
    strcpy((char *)piece.data,"mcc_maps\\level");
    CHECK(network_game_client_receive_game_settings_piece(NULL,&piece));
    piece.offset=48;piece.total_size=64;piece.length=16;
    network_game_client_receive_game_settings_piece(NULL,&piece);CHECK(applied==1);
    piece.offset=0;piece.length=48;piece.total_size=64;
    CHECK(network_game_client_receive_game_settings_piece(NULL,&piece));
    piece.offset=48;piece.length=16;memset(piece.data,0,sizeof(piece.data));
    CHECK(!network_game_client_receive_game_settings_piece(NULL,&piece));CHECK(applied==1);
    piece.offset=0;piece.length=0;CHECK(!network_game_client_receive_game_settings_piece(NULL,&piece));
    piece.length=48;piece.offset=48;piece.total_size=64;
    CHECK(!network_game_client_receive_game_settings_piece(NULL,&piece));
    /* The real framing and protocol gates wait for both identities; the former
     * one-digest size must never admit a truncated metadata agreement. */
    mcc_protocol_client_begin(MCC_PROTOCOL_ADVERTISED_FLAG);
    memset(game.mcc_content_sha256,42,32);memset(game.mcc_metadata_sha256,17,32);
    piece.total_size=sizeof(game);
    for(i=0;i<(int)sizeof(game);i+=piece.length) {
        piece.offset=(word)i;piece.length=(word)(sizeof(game)-i);
        if(piece.length>sizeof(piece.data))piece.length=sizeof(piece.data);
        memcpy(piece.data,(byte *)&game+i,piece.length);
        CHECK(network_game_client_receive_game_settings_piece(NULL,&piece));
        CHECK(applied==1+(i+piece.length==sizeof(game)));
    }
    CHECK(!memcmp(&network_game_client_settings_staging,&game,sizeof(game)));
    /* A subsequent CE prefix clears every byte of both MCC-only identities. */
    memset(&game,0,sizeof(game));strcpy(game.map.name,"custom_maps\\bloodgulch");
    piece.total_size=64;piece.offset=0;piece.length=48;memcpy(piece.data,&game,48);
    CHECK(network_game_client_receive_game_settings_piece(NULL,&piece));
    piece.offset=48;piece.length=16;memcpy(piece.data,(byte *)&game+48,16);
    CHECK(network_game_client_receive_game_settings_piece(NULL,&piece));CHECK(applied==3);
    CHECK(!memcmp(&network_game_client_settings_staging,&game,sizeof(game)));
    return 0;
}
'''
    source = tmp_path / "protocol.c"
    source.write_text(code)
    exe = compile_tool(tmp_path, [source])
    run = subprocess.run([str(exe)], capture_output=True, text=True)
    assert run.returncode == 0, run.stdout + run.stderr


def test_loopback_host_and_remote_capability_selection(tmp_path):
    source = (ROOT / "source/networking/network_client_manager.c").read_text()
    join = function(source, "network_game_client_initiate_join_game")
    selection = join[join.index("\t{\n\t\tunsigned int mcc_flags"):join.index("\tclient->join_in_progress = TRUE;")]
    code = r'''
#include <stdio.h>
#include "mcc_protocol.h"
#define MAXIMUM_NETWORK_ADVERTISED_GAMES 4
#define IPV4_LOOPBACK_ADDRESS 1
struct network_advertised_game {int dummy;};
struct network_game_client {struct network_advertised_game available_games[4];};
struct transport_address {struct {unsigned long long_words[1];} address;};
static struct {unsigned int flags,version;} network_game_client_advertised_versions[4];
static void third_person_session_client_begin(unsigned int version){(void)version;}
static int local_server;
static unsigned int selected;
static int global_network_game_server_get(void) {return local_server;}
void mcc_protocol_client_begin(unsigned int flags) {selected=flags;}
static void choose(struct network_game_client *client,struct network_advertised_game *game,
    struct transport_address *address) {
''' + selection + r'''
}
int main(void) {
    struct network_game_client client;
    struct network_advertised_game temporary;
    struct transport_address address={{{IPV4_LOOPBACK_ADDRESS}}};
    local_server=1;choose(&client,&temporary,&address);
    if(selected!=MCC_PROTOCOL_ADVERTISED_FLAG) return 1;
    local_server=0;choose(&client,&temporary,&address);if(selected) return 2;
    network_game_client_advertised_versions[2].flags=MCC_PROTOCOL_ADVERTISED_FLAG;
    choose(&client,&client.available_games[2],&address);if(selected!=MCC_PROTOCOL_ADVERTISED_FLAG) return 3;
    choose(&client,&client.available_games[1],&address);if(selected) return 4;
    local_server=1;address.address.long_words[0]=2;
    choose(&client,&client.available_games[1],&address);if(selected) return 5;
    return 0;
}
'''
    path = tmp_path / "selection.c"
    path.write_text(code)
    exe = compile_tool(tmp_path, [path])
    run = subprocess.run([str(exe)], capture_output=True, text=True)
    assert run.returncode == 0, run.stdout + run.stderr


def test_capability_does_not_bypass_host_admission(tmp_path):
    source = (ROOT / "source/networking/network_server_message_handler.c").read_text()
    join = function(source, "network_game_server_handle_message_client_join_game_request")
    # Execute the real admission handler. Only its external services are fakes;
    # capability must not skip decoding, the token, slots, bans or session state.
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
#define HALO_PORT_NETWORK_VERSION 32738
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
static int state,late,joined,decode_ok,token_ok,slots,open_game,banned,accepted,capable,requires_mcc,name_blocked;
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
#define mcc_protocol_peer(m,c) (capable=(c),1)
#define mcc_protocol_allows(g,m) (!requires_mcc || capable)
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
 state=0;late=joined=banned=accepted=capable=requires_mcc=name_blocked=0;
 camera_locked=replaceable_bots=0;peer_address=2;game.machine_count=1;
 decode_ok=token_ok=slots=open_game=1;
 join_version=HALO_PORT_NETWORK_VERSION;
}
static int try_join(int capability) {
 word packet[16]={0};struct network_game_server s;struct network_game_server_client_machine m;
 return network_game_server_handle_message_client_join_game_request(&s,&m,packet,sizeof(packet),capability);
}
int main(void) {
 int cap,mcc,i;
 word invalid_versions[]={0,41,42,65,66,67,68,69,70,32712,32713,32714,32716,32717,32718,32720,32721,32722,32724,32725,32726,32728,32729,32730,32732,32733,65535};
 for(mcc=0;mcc<2;mcc++) for(cap=0;cap<2;cap++) {
  for(i=0;i<(int)(sizeof(invalid_versions)/sizeof(invalid_versions[0]));i++) {
   reset();requires_mcc=mcc;join_version=invalid_versions[i];
   CHECK(!try_join(cap) && !accepted && !capable && !camera_locked);
  }
  reset();requires_mcc=mcc;CHECK(try_join(cap)==(!mcc||cap));CHECK(accepted==(!mcc||cap));CHECK(camera_locked==accepted);
  reset();requires_mcc=mcc;peer_address=IPV4_LOOPBACK_ADDRESS;
  CHECK(try_join(cap)==(!mcc||cap));CHECK(!camera_locked);
  reset();requires_mcc=mcc;peer_address=IPV4_LOOPBACK_ADDRESS;game.machine_count=2;
  CHECK(try_join(cap)==(!mcc||cap));CHECK(camera_locked==accepted);
  reset();requires_mcc=mcc;decode_ok=0;CHECK(!try_join(cap) && !accepted);
  reset();requires_mcc=mcc;token_ok=0;CHECK(!try_join(cap) && !accepted);
  reset();requires_mcc=mcc;slots=0;CHECK(!try_join(cap) && !accepted);
  reset();requires_mcc=mcc;slots=0;replaceable_bots=1;
  CHECK(try_join(cap)==(!mcc||cap));CHECK(accepted==(!mcc||cap));
  reset();requires_mcc=mcc;slots=0;replaceable_bots=1;token_ok=0;CHECK(!try_join(cap) && !accepted);
  reset();requires_mcc=mcc;slots=0;replaceable_bots=1;open_game=0;CHECK(!try_join(cap) && !accepted);
  reset();requires_mcc=mcc;slots=0;replaceable_bots=1;banned=1;CHECK(!try_join(cap) && !accepted);
  reset();requires_mcc=mcc;open_game=0;CHECK(!try_join(cap) && !accepted);
  reset();requires_mcc=mcc;banned=1;CHECK(!try_join(cap) && !accepted);
  reset();requires_mcc=mcc;name_blocked=1;CHECK(!try_join(cap) && !accepted);
  reset();requires_mcc=mcc;state=1;CHECK(!try_join(cap) && !accepted);
  reset();requires_mcc=mcc;state=1;late=1;CHECK(try_join(cap)==(!mcc||cap));CHECK(accepted==(!mcc||cap));
  reset();requires_mcc=mcc;joined=1;try_join(cap);CHECK(!accepted && !capable && !camera_locked);
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
    protocol = (ROOT / "port/linux/game/mcc_protocol.h").read_text()
    join_kind = int(re.search(r"#define MCC_PROTOCOL_JOIN_KIND (\d+)", protocol).group(1))
    assert join_kind not in messages.values()
    header = (ROOT / "port/linux/game/network_distributed.h").read_text()
    assert messages["enum _distributed_message_checkpoint"] == 96
    assert join_kind == 101
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


def test_advertised_kind_and_mcc_capability_are_independent(tmp_path):
    """Execute the merged writer/reader for all kind bytes and stale capability."""
    server = (ROOT / "source/networking/network_server_message_handler.c").read_text()
    writer = server[server.index("\t\t\tadvertisement.reserved[HALO_PORT_ADVERTISED_VERSION_OFFSET]"):]
    writer = writer[:writer.index("\t\t\t/* (open to joins:")]
    client = (ROOT / "source/networking/network_client_manager.c").read_text()
    reader = client[client.index("\t\t\tnetwork_game_client_advertised_versions[game_index].version ="):]
    reader = reader[:reader.index("\n\t\t}")]
    limits = (ROOT / "port/linux/include/halo_port_limits.h").read_text()
    defines = "\n".join(line for line in limits.splitlines()
                        if line.startswith("#define HALO_PORT_ADVERTISED_"))
    base = re.search(r"#define HALO_PORT_NETWORK_VERSION (\d+)", limits).group(1)
    code = r'''
#include <string.h>
#include "mcc_protocol.h"
typedef unsigned char byte;
typedef unsigned short word;
#define _network_game_server_state_pregame 0
struct game {struct {char name[32];} map; int variant, variant_options;};
static struct {word version;byte flags,kind;} network_game_client_advertised_versions[1];
static int dedicated;
static int halo_server_mode(void) {return dedicated;}
static int game_modes_kind(int const *variant,int const *options) {(void)options;return *variant;}
static int network_game_server_get_state(void *server,void *state) {(void)server;(void)state;return 0;}
static int network_game_server_game_is_loading(void *server) {(void)server;return 0;}
''' + defines + '\n#define BASE_VERSION ' + base + r'''
unsigned int third_person_session_version(char const *map) {return map[0] ? MCC_PROTOCOL_VERSION : BASE_VERSION;}
static void write_advertisement(byte *bytes,int kind,int mcc) {
    struct {byte reserved[26];} advertisement={{0}};
    struct game data={{""},0,0};
    struct game *game=&data;
    void *server=0;
    data.variant=kind;data.map.name[0]=(char)mcc;
''' + writer + r'''
    memcpy(bytes,advertisement.reserved,26);
}
static void read_advertisement(byte const *bytes) {
    struct {byte __unknown5A[26];} data;
    void *unused=0;
    int game_index=0;
    /* The anonymous struct pointer stays local to the actual reader block. */
    #define advertisement (&data)
    (void)unused;memcpy(data.__unknown5A,bytes,26);
''' + reader + r'''
    #undef advertisement
}
int main(void) {
    byte bytes[26];int kind,mcc;
    for(dedicated=0;dedicated<2;dedicated++)for(mcc=0;mcc<2;mcc++)for(kind=0;kind<256;kind++) {
        write_advertisement(bytes,kind,mcc);read_advertisement(bytes);
        if(network_game_client_advertised_versions[0].kind!=kind)return 1;
        if(network_game_client_advertised_versions[0].version!=(mcc?MCC_PROTOCOL_VERSION:BASE_VERSION))return 2;
        if(!(network_game_client_advertised_versions[0].flags&MCC_PROTOCOL_ADVERTISED_FLAG))return 3;
        if(!!(network_game_client_advertised_versions[0].flags&HALO_PORT_ADVERTISED_DEDICATED_FLAG)!=dedicated)return 6;
        bytes[MCC_PROTOCOL_ADVERTISED_VERSION_OFFSET]^=1;read_advertisement(bytes);
        if(network_game_client_advertised_versions[0].flags&MCC_PROTOCOL_ADVERTISED_FLAG)return 4;
        if(network_game_client_advertised_versions[0].kind!=kind)return 5;
        if(!!(network_game_client_advertised_versions[0].flags&HALO_PORT_ADVERTISED_DEDICATED_FLAG)!=dedicated)return 7;
    }
    return 0;
}
'''
    path = tmp_path / "advertisement.c"
    path.write_text(code)
    exe = compile_tool(tmp_path, [path])
    run = subprocess.run([str(exe)], capture_output=True, text=True)
    assert run.returncode == 0, run.stdout + run.stderr
