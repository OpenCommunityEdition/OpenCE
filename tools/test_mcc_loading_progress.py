"""Real bounded MCC loading leases, transport adapter and server deadline branch."""
from pathlib import Path
import re
import shutil
import subprocess
import pytest
from harness import function, structure
from test_mcc_identity import compile_tool

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / "port/linux/game"


@pytest.fixture(scope="module")
def load_tool(tmp_path_factory):
    work = tmp_path_factory.mktemp("mcc-load-network")
    headers = {
        "cseries.h": "#pragma once\n#include <stddef.h>\ntypedef int boolean;typedef unsigned short word;typedef unsigned char byte;\n#define NONE -1\n",
        "networking/network_game_manager.h": "#pragma once\nstruct network_game {struct {long version;char name[128];} map;unsigned char mcc_content_sha256[32],mcc_metadata_sha256[32];long number_of_games_played;struct {int name;} machines[4];};\n",
        "networking/network_game_globals.h": "#pragma once\n#include \"networking/network_game_manager.h\"\nstruct network_game_server_client_machine {int flags,machine_index;};struct network_game_server {int state;unsigned long time_of_first_client_loading_completion,time_of_last_keep_alive;struct network_game game;struct network_game_server_client_machine client_machines[4];};struct network_game_client {int dummy;};\nstruct network_game_server *global_network_game_server_get(void);struct network_game_client *global_network_game_client_get(void);struct network_game *network_game_get_game(void);\n",
        "networking/network_client_manager.h": "struct network_game *network_game_client_get_game(struct network_game_client *);\nboolean network_game_client_server_has_started_game(struct network_game_client *);\n",
    }
    for name, text in headers.items():
        path = work / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text)
    path = work / "bungie_net/common/message_header.h"
    path.parent.mkdir(parents=True)
    path.write_text((ROOT / "source/bungie_net/common/message_header.h").read_text())
    distributed = (GAME / "network_distributed.h").read_text()
    wire = structure(distributed, "distributed_message_header")
    # Every game build uses ILP32; the asset-free Linux runner may be LP64.
    wire = re.sub(r"\bunsigned long\b", "uint32_t", wire)
    wire = re.sub(r"\blong\b", "int32_t", wire)
    (work / "network_distributed.h").write_text('#pragma once\n#include <stdint.h>\n#include "bungie_net/common/message_header.h"\n' +
        wire + '\ntypedef char wire_header_size[sizeof(struct distributed_message_header)==12 ? 1 : -1];\n' +
        'enum {_distributed_message_mcc_load_progress=106};\n')
    adapter = work / "adapter.c"
    adapter.write_text((GAME / "mcc_load_network_game.c").read_text())
    server_source = (ROOT / "source/networking/network_server_manager.c").read_text()
    pregame = function(server_source, "network_game_server_idle_pregame_tasks")
    deadline = pregame[pregame.index("else if (server->time_of_first_client_loading_completion)"):pregame.rindex("return success;")]
    source = work / "test.c"
    source.write_text(DRIVER + '\nstatic void deadline(struct network_game_server *server) {long now=(long)clock_ms;if(0){}' + deadline + '}\n' + MAIN)
    return compile_tool(work, [source, adapter, GAME / "mcc_load_network.c"])


DRIVER = r'''
#include "cseries.h"
#include "networking/network_game_globals.h"
#include "network_distributed.h"
#include "mcc_load_network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
static struct network_game_server server;
static struct network_game_client client;
static struct network_game game;
static int hosting=1,joined=1,connected=1,started,sends,blocked,removed,complete;
static unsigned long clock_ms=1000,revision=1;
static struct {struct distributed_message_header header;struct mcc_load_message body;} packet,last;
typedef char wire_packet_size[sizeof(packet)==216 ? 1 : -1];
struct network_game_server *global_network_game_server_get(void){return hosting ? &server : NULL;}
struct network_game_client *global_network_game_client_get(void){return connected ? &client : NULL;}
struct network_game *network_game_get_game(void){return &game;}
struct network_game *network_game_client_get_game(struct network_game_client *c){(void)c;return &game;}
boolean network_game_client_server_has_started_game(struct network_game_client *c){(void)c;return started;}
struct network_game_server_client_machine *network_game_server_get_client_machine_at_index(struct network_game_server *s,long n){(void)s;return n==2 ? &server.client_machines[2] : NULL;}
boolean network_game_server_client_machine_is_joined_to_game(struct network_game_server *s,struct network_game_server_client_machine *p){(void)s;return joined && p==&server.client_machines[2];}
boolean mcc_level_name(char const *name){return name && !strncmp(name,"mcc_maps\\",9);}
unsigned long mcc_identity_host_revision(void){return revision;}
void const *mcc_identity_host_settings(void const *g,long size,struct network_game *copy){if(size!=sizeof(*copy))return NULL;memcpy(copy,g,size);return copy;}
unsigned long system_milliseconds(void){return clock_ms;}
void build_message_header(word *out,word size,byte type,byte flags){*out=(word)((size<<4)|(type<<2)|flags);}
boolean network_distributed_client_send_reliably(void *p,word size){if(size!=sizeof(last))return 0;memcpy(&last,p,size);sends++;return !blocked;}
boolean network_distributed_server_send_to_machine_reliably(long n,void *p,word size){return n==2 ? network_distributed_client_send_reliably(p,size) : 0;}
#define TRUE 1
#define FALSE 0
#define MAXIMUM_NETWORK_MACHINE_COUNT 4
#define MAXIMUM_MACHINE_NAME_LENGTH 32
#define MILLISECONDS_PER_SECOND 1000
#define NETWORK_GAME_SERVER_MAXIMUM_WAIT_TIME_FOR_LEVEL_LOADING 60000
#define NETWORK_SERVER_MANAGER_FILE "network_server_manager.c"
#define TEST_FLAG(flags,bit) (((flags)&(1u<<(bit)))!=0)
#define _network_client_machine_connected_bit 0
#define _network_client_machine_level_loaded_bit 1
#define _network_game_server_state_pregame 0
#define _message_server_pregame_keep_alive 0
struct message_server_pregame_keep_alive {int unused;};
struct network_message {int unused;};
static struct network_message *create_network_game_message(int type,void *data,unsigned size){(void)type;(void)data;(void)size;return NULL;}
static void network_game_server_send_message_to_all_machines(struct network_game_server *s,struct network_message *m){(void)s;(void)m;}
static int map_share_machine_needs_map(long index){(void)index;return 0;}
static void network_event(char const *format,...){(void)format;}
static char const *wide_to_ascii(int n,char *text,unsigned size){(void)n;(void)text;(void)size;return "peer";}
static int network_game_server_remove_client_machine_from_game(struct network_game_server *s,struct network_game_server_client_machine *m){(void)s;m->flags=0;removed++;return 1;}
static void network_game_server_all_machines_have_loaded(struct network_game_server *s){(void)s;complete++;}
#define match_assert(file,line,x) do {if(!(x))abort();}while(0)
'''

MAIN = r'''
static void setup(void){
 strcpy(game.map.name,"mcc_maps\\package\\level");game.map.name[64]=7;
 game.map.version=123;game.mcc_content_sha256[31]=42;game.mcc_metadata_sha256[0]=19;game.number_of_games_played=4;
 server.client_machines[2].machine_index=2;server.client_machines[2].flags=1;
 server.time_of_first_client_loading_completion=1000;
 mcc_load_network_start(&game);mcc_load_network_begin(game.map.name);packet=last;
}
static void receive(void){mcc_load_network_handle_message(2,&packet,sizeof(packet),1);}
int main(int argc,char **argv){
 int test,i,old;struct mcc_load_network state;struct mcc_load_message message;
 if(argc!=2)return 2;test=atoi(argv[1]);setup();CHECK(sends==1 && packet.body.sequence==1);
 if(test==0){
  for(i=1;i<=24;i++){clock_ms=1000+i*5000;packet.body.sequence++;receive();deadline(&server);CHECK(!removed && !complete);}
  CHECK(mcc_load_network_host_waiting(2));
  server.client_machines[2].flags|=2;deadline(&server);CHECK(complete==1 && !removed);
 }else if(test==1){
  receive();clock_ms+=29000;receive();clock_ms+=1000;CHECK(!mcc_load_network_host_waiting(2));
  packet.body.sequence++;receive();CHECK(!mcc_load_network_host_waiting(2));clock_ms=61000;deadline(&server);CHECK(removed==1 && complete==1);
 }else if(test==2){
  for(i=0;i<48;i++){clock_ms=1000+i*5000;packet.body.sequence++;receive();CHECK(mcc_load_network_host_waiting(2));}
  clock_ms=241000;packet.body.sequence++;receive();CHECK(!mcc_load_network_host_waiting(2));
  mcc_load_network_start(&game);receive();CHECK(!mcc_load_network_host_waiting(2));deadline(&server);CHECK(removed==1 && complete==1);
 }else if(test==3){
  mcc_load_network_handle_message(2,&packet,sizeof(packet),0);CHECK(!mcc_load_network_host_waiting(2));
  mcc_load_network_handle_message(2,&packet,sizeof(packet)-1,1);CHECK(!mcc_load_network_host_waiting(2));
  packet.header.count=2;receive();CHECK(!mcc_load_network_host_waiting(2));packet.header.count=1;
  packet.header.header|=1;receive();CHECK(!mcc_load_network_host_waiting(2));packet.header.header&=~1;
  packet.body.round++;receive();CHECK(!mcc_load_network_host_waiting(2));packet.body.round--;
  packet.body.identity.name[64]++;receive();CHECK(!mcc_load_network_host_waiting(2));packet.body.identity.name[64]--;
  packet.body.identity.content[0]++;receive();CHECK(!mcc_load_network_host_waiting(2));packet.body.identity.content[0]--;
  packet.body.identity.metadata[0]++;receive();CHECK(!mcc_load_network_host_waiting(2));packet.body.identity.metadata[0]--;
  joined=0;receive();CHECK(!mcc_load_network_host_waiting(2));joined=1;
  receive();CHECK(mcc_load_network_host_waiting(2));revision++;CHECK(!mcc_load_network_host_waiting(2));
 }else if(test==4){
  receive();CHECK(mcc_load_network_host_waiting(2));mcc_load_network_peer_left(2);packet.body.sequence++;receive();CHECK(!mcc_load_network_host_waiting(2));
  game.number_of_games_played++;mcc_load_network_start(&game);receive();CHECK(!mcc_load_network_host_waiting(2));packet.body.round++;receive();CHECK(mcc_load_network_host_waiting(2));
  mcc_load_network_disconnect();CHECK(!mcc_load_network_host_waiting(2));old=sends;clock_ms+=5000;mcc_load_network_pulse();CHECK(old==sends);
 }else if(test==5){
  old=sends;for(i=0;i<100;i++){clock_ms++;mcc_load_network_pulse();}CHECK(sends==old);
  clock_ms+=5000;blocked=1;mcc_load_network_pulse();CHECK(sends==old+1);blocked=0;
  clock_ms+=5000;mcc_load_network_pulse();CHECK(sends==old+2 && last.body.sequence>packet.body.sequence);
  mcc_load_network_end();clock_ms+=5000;mcc_load_network_pulse();CHECK(sends==old+2);
 }else if(test==6){
  mcc_load_network_disconnect();hosting=0;mcc_load_network_begin(game.map.name);CHECK(sends==2);
  packet=last;mcc_load_network_handle_message(NONE,&packet,sizeof(packet),1);
  clock_ms=121000;CHECK(!mcc_load_network_client_expired());clock_ms=241000;CHECK(mcc_load_network_client_expired());
  started=1;CHECK(!mcc_load_network_client_expired());started=0;game.mcc_metadata_sha256[0]++;CHECK(!mcc_load_network_client_expired());
  old=sends;mcc_load_network_pulse();CHECK(sends==old);
 }else if(test==7){
  mcc_load_network_disconnect();strcpy(game.map.name,"maps\\bloodgulch");mcc_load_network_start(&game);mcc_load_network_begin(game.map.name);
  receive();clock_ms=61000;deadline(&server);CHECK(removed==1 && complete==1 && sends==1);
 }else if(test==8){
  memset(&state,0,sizeof(state));message=packet.body;mcc_load_network_bind(&state,&message.identity,message.round,0xfffffff0u);
  CHECK(mcc_load_network_receive(&state,2,&message,0xfffffff0u));CHECK(mcc_load_network_active(&state,2,100u));
  CHECK(!mcc_load_network_receive(&state,-1,&message,100u));message.sequence=0;CHECK(!mcc_load_network_receive(&state,2,&message,100u));
  CHECK(!mcc_load_network_active(&state,2,30000u));
 }else if(test==9){
  memset(&state,0,sizeof(state));message=packet.body;
  mcc_load_network_bind(&state,&message.identity,message.round,1000);
  CHECK(mcc_load_network_receive(&state,2,&message,1000));message.sequence++;
  CHECK(!mcc_load_network_receive(&state,2,&message,5999));CHECK(state.peers[2].sequence==1);
  CHECK(mcc_load_network_receive(&state,2,&message,6000));
 }else if(test==10){
  old=sends;mcc_load_network_work(3,1,100);clock_ms+=5000;
  mcc_load_network_work(3,1,100);mcc_load_network_work(2,90,100);mcc_load_network_work(3,2,99);
  CHECK(sends==old);mcc_load_network_work(3,2,100);CHECK(sends==old+1);
  clock_ms+=5000;mcc_load_network_work(4,0,0);CHECK(sends==old+2);
  mcc_load_network_end();clock_ms+=5000;mcc_load_network_work(5,0,0);CHECK(sends==old+2);
 }else return 2;
 return 0;
}
'''


@pytest.mark.parametrize("case", range(11), ids=["slow-client-real-deadline", "no-progress-replay", "absolute-cap",
    "untrusted-binding", "round-and-peer-lifetime", "send-only-rate-backpressure", "slow-host-client-limit",
    "legacy-deadline", "clock-wrap", "receive-rate", "monotonic-headless-stage-work"])
def test_loading_progress(load_tool, case):
    result = subprocess.run([str(load_tool), str(case)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr


def test_before_fix_deadline_drops_progressing_client(load_tool, tmp_path):
    for source in load_tool.parent.rglob("*"):
        if source.is_file() and source.suffix in (".h", ".c"):
            target = tmp_path / source.relative_to(load_tool.parent)
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, target)
    source = tmp_path / "test.c"
    text = source.read_text()
    guard = "if (mcc_load_network_host_waiting(server->client_machines[i].machine_index))"
    assert text.count(guard) == 1
    source.write_text(text.replace(guard, "if (0)"))
    exe = compile_tool(tmp_path, [source, tmp_path / "adapter.c", GAME / "mcc_load_network.c"])
    result = subprocess.run([str(exe), "0"], capture_output=True, text=True)
    assert result.returncode == 1 and "!removed && !complete" in result.stderr, result.stderr


def test_real_transport_wrappers_only_write_while_loading(load_tool, tmp_path):
    for source in load_tool.parent.rglob("*"):
        if source.is_file() and source.suffix in (".h", ".c"):
            target = tmp_path / source.relative_to(load_tool.parent)
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, target)
    source = tmp_path / "test.c"
    text = source.read_text()
    client_stub = "boolean network_distributed_client_send_reliably(void *p,word size){if(size!=sizeof(last))return 0;memcpy(&last,p,size);sends++;return !blocked;}"
    host_stub = "boolean network_distributed_server_send_to_machine_reliably(long n,void *p,word size){return n==2 ? network_distributed_client_send_reliably(p,size) : 0;}"
    assert client_stub in text and host_stub in text
    server = (ROOT / "source/networking/network_server_message_handler.c").read_text()
    client = (ROOT / "source/networking/network_client_manager.c").read_text()
    globals_source = (ROOT / "source/networking/network_game_globals.c").read_text()
    wrappers = r'''
#define FALSE 0
#define NETWORK_MESSAGE_BUFFER_SIZE 4096
#define MAXIMUM_NETWORK_MACHINE_COUNT 4
#define VALID_INDEX(i,n) ((unsigned long)(i)<(unsigned long)(n))
#define csmemcpy memcpy
#define global_network_game_client (connected ? &client : NULL)
struct network_connection {int unused;};
struct transport_address {int unused;};
static struct network_connection connection;
static struct network_connection *network_game_client_get_connection(struct network_game_client *c){(void)c;return &connection;}
static struct network_connection *network_game_server_get_client_connection(struct network_game_server_client_machine *m){(void)m;return &connection;}
static int network_connection_active(struct network_connection *c){(void)c;return connected;}
static int network_game_server_get_client_machine(struct network_game_server *s,struct network_game_server_client_machine *m,long *index){(void)s;*index=m->machine_index;return 1;}
static boolean network_connection_write(struct network_connection *c,void *data,word size,struct transport_address *a,boolean reliable){
 if(c!=&connection || a || !reliable || size!=sizeof(last))abort();
 memcpy(&last,data,size);sends++;((byte *)data)[0]^=255;return !blocked;
}
''' + function(server, "network_game_server_write") + function(client, "network_game_client_write") + function(
        globals_source, "network_distributed_client_send_reliably") + function(server,
        "network_game_server_client_machine_in_game") + function(server, "network_distributed_server_send_to_machine_reliably")
    source.write_text(text.replace(client_stub, wrappers).replace(host_stub, ""))
    exe = compile_tool(tmp_path, [source, tmp_path / "adapter.c", GAME / "mcc_load_network.c"])
    for case in (5, 6):
        result = subprocess.run([str(exe), str(case)], capture_output=True, text=True)
        assert result.returncode == 0, result.stderr
