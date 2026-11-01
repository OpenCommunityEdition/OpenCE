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
struct network_game_server {int unused;};
struct network_game_server_client_machine {int joined;};
static int network_game_server_client_machine_is_joined_to_game(struct network_game_server *s,
    struct network_game_server_client_machine *m) {(void)s;return m->joined;}
static void network_event(char const *fmt,...) {(void)fmt;}
''' + version + "\n" + (ROOT / "source/bungie_net/common/message_header.h").read_text().replace("#pragma once", "") + "\n" + enum_with(
        (ROOT / "source/networking/network_messages.h").read_text(),
        "_message_client_broadcast_game_search") + r'''
void build_message_header(word *out,word bytes,byte type,byte flags) {*out=(word)((bytes<<4)|(type<<2)|flags);}
''' + camera + "\n" + gate + r'''
int main(void) {
    struct network_game_server server={0};
    struct network_game_server_client_machine machine={0};
    word join[128]={0},outer[132],inner[132],size;
    byte saved;
    int i,bytes;
    CHECK(HALO_PORT_NETWORK_VERSION==32710 && MCC_PROTOCOL_VERSION==32711);
    CHECK(THIRD_PERSON_PROTOCOL_JOIN_KIND!=MCC_PROTOCOL_JOIN_KIND);
    for(bytes=3;bytes<=256;bytes++) {
        build_message_header(join,(word)bytes,_message_type_packet,0);
        ((byte *)join)[bytes-1]=_message_client_join_game_request;
        CHECK(third_person_protocol_join(join,(word)bytes,outer)==outer);
        CHECK(third_person_protocol_unwrap(outer,(word)(bytes+8),inner,&size));
        CHECK(size==bytes && !memcmp(inner,join,bytes));
        CHECK(network_game_server_handle_client_message(&server,&machine,outer,(short)(bytes+8)));
        /* Stock/main/old camera direct joins have no camera envelope. */
        CHECK(!network_game_server_handle_client_message(&server,&machine,join,(short)bytes));
    }
    for(i=0;i<=265;i++) if(i!=264) CHECK(!third_person_protocol_unwrap(outer,(word)i,inner,&size));
    for(i=0;i<10;i++) {
        saved=((byte *)outer)[i];((byte *)outer)[i]^=1;
        CHECK(!third_person_protocol_unwrap(outer,264,inner,&size));
        CHECK(!network_game_server_handle_client_message(&server,&machine,outer,264));
        ((byte *)outer)[i]=saved;
    }
    ((byte *)outer)[263]=_message_client_game_start_request;
    CHECK(!third_person_protocol_unwrap(outer,264,inner,&size));
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
