"""Real MCC lease core: authenticated identity, bounded time and cleanup."""
from pathlib import Path
import subprocess
import pytest
from test_mcc_identity import compile_tool

ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="module")
def admission_tool(tmp_path_factory):
    work = tmp_path_factory.mktemp("mcc-admission")
    source = work / "test.c"
    source.write_text(r'''
#include "mcc_admission.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static struct mcc_admission host,client;
static struct mcc_admission_message sent;
static int sends,blocked,valid=1,players;
static long destination;
static int send(void *user,long machine,struct mcc_admission_message const *m){
 (void)user;sent=*m;destination=machine;sends++;return !blocked;
}
static int peer(void *user,long machine){(void)user;return valid && machine==2;}
static int has_players(void *user,long machine){(void)user;(void)machine;return players;}
static struct mcc_admission_identity identity;
static void setup(uint32_t now){
 struct mcc_admission_env env={NULL,send,peer,has_players};
 memset(&identity,0,sizeof(identity));strcpy(identity.name,"mcc_maps\\package\\level");
 identity.name[64]=7;identity.content[31]=42;identity.metadata[0]=19;identity.checksum=123;
 mcc_admission_init(&host,&env);mcc_admission_init(&client,&env);
 mcc_admission_tick(&host,now,&identity,1);
}
static void deliver(uint32_t now){
 struct mcc_admission_message request=sent;
 mcc_admission_receive(&host,2,&request,sizeof(request),now);
 if(destination==2)mcc_admission_receive(&client,-1,&sent,sizeof(sent),now);
}
int main(int argc,char **argv){
 struct mcc_admission_message message;uint32_t now=1000,start;int test,old,i;
 if(argc!=2)return 2;test=atoi(argv[1]);setup(now);
 CHECK(mcc_admission_begin(&client,&identity,now));message=sent;
 if(test==0){
  CHECK(!mcc_admission_status(&client));deliver(now);CHECK(mcc_admission_status(&client)==1);
  CHECK(mcc_admission_active(&host,2,now,1));start=host.peers[2].started;
  for(i=1;i<24;i++){
   now=1000+i*5000;mcc_admission_tick(&client,now,NULL,0);deliver(now);
   mcc_admission_tick(&host,now,&identity,1);
   CHECK(mcc_admission_active(&host,2,now,1));CHECK(host.peers[2].started==start);
  }
  now=121000;mcc_admission_tick(&host,now,&identity,1);
  CHECK(!mcc_admission_active(&host,2,now,0));message.generation++;
  mcc_admission_receive(&host,2,&message,sizeof(message),now);
  CHECK(sent.op==MCC_ADMISSION_REFUSED && !mcc_admission_active(&host,2,now,0));
  mcc_admission_tick(&client,now,NULL,0);CHECK(mcc_admission_status(&client)==-1);
 }else if(test==1){
  old=sends;
  mcc_admission_receive(&host,2,&message,sizeof(message)-1,now);
  mcc_admission_receive(&host,3,&message,sizeof(message),now);
  message.generation=0;mcc_admission_receive(&host,2,&message,sizeof(message),now);message=sent;
  message.identity.name[64]^=1;mcc_admission_receive(&host,2,&message,sizeof(message),now);message=sent;
  message.identity.metadata[31]^=1;mcc_admission_receive(&host,2,&message,sizeof(message),now);message=sent;
  message.identity.content[31]^=1;mcc_admission_receive(&host,2,&message,sizeof(message),now);message=sent;
  message.identity.checksum++;mcc_admission_receive(&host,2,&message,sizeof(message),now);message=sent;
  message.op=99;mcc_admission_receive(&host,2,&message,sizeof(message),now);
  CHECK(sends==old && !mcc_admission_active(&host,2,now,1));
  message=sent;message.op=MCC_ADMISSION_ACK;message.generation++;
  mcc_admission_receive(&client,-1,&message,sizeof(message),now);CHECK(!mcc_admission_status(&client));
  message.generation--;message.identity.name[64]^=1;
  mcc_admission_receive(&client,-1,&message,sizeof(message),now);CHECK(!mcc_admission_status(&client));
 }else if(test==2){
  deliver(now);players=1;mcc_admission_tick(&host,now,&identity,1);
  CHECK(mcc_admission_active(&host,2,now,1)); /* next mission still needs validation */
  players=0;mcc_admission_finish(&client,1,now+1);deliver(now+1);
  CHECK(!mcc_admission_active(&host,2,now+1,1));CHECK(mcc_admission_active(&host,2,now+1,0));
  CHECK(!mcc_admission_active(&host,2,now+15001,0));
  players=1;mcc_admission_tick(&host,now+2,&identity,1);CHECK(!mcc_admission_active(&host,2,now+2,0));
 }else if(test==3){
  deliver(now);now+=MCC_ADMISSION_HEARD;
  mcc_admission_receive(&host,2,&message,sizeof(message),now);CHECK(sent.op==MCC_ADMISSION_REFUSED);
  CHECK(!mcc_admission_active(&host,2,now,0));
  mcc_admission_forget(&host,2);mcc_admission_receive(&host,2,&message,sizeof(message),now);
  CHECK(mcc_admission_active(&host,2,now,1));
  valid=0;mcc_admission_tick(&host,now+1,&identity,1);CHECK(!mcc_admission_active(&host,2,now+1,1));
 }else if(test==4){
  deliver(now);message=client.client;message.op=MCC_ADMISSION_CANCEL;
  mcc_admission_receive(&host,2,&message,sizeof(message),now);CHECK(!mcc_admission_active(&host,2,now,0));
  message.op=MCC_ADMISSION_BEGIN;message.generation++;
  mcc_admission_receive(&host,2,&message,sizeof(message),now+1000);
  CHECK(sent.op==MCC_ADMISSION_REFUSED && !mcc_admission_active(&host,2,now+1000,0));
  mcc_admission_tick(&host,now,&host.host,2); /* alias-safe identity reset */
  mcc_admission_receive(&host,2,&message,sizeof(message),now);CHECK(mcc_admission_active(&host,2,now,1));
  identity.name[64]++;mcc_admission_tick(&host,now,&identity,3);
  mcc_admission_receive(&host,2,&message,sizeof(message),now);CHECK(!mcc_admission_active(&host,2,now,1));
 }else if(test==5){
  setup(0xfffff000u);now=0xfffff000u;CHECK(mcc_admission_begin(&client,&identity,now));deliver(now);
  now+=5000;mcc_admission_tick(&client,now,NULL,0);deliver(now);CHECK(mcc_admission_status(&client)==1);
  blocked=1;mcc_admission_finish(&client,1,now);CHECK(client.state>0);
  blocked=0;mcc_admission_tick(&client,now+1,NULL,0);CHECK(client.state==0 && sent.op==MCC_ADMISSION_READY);
 }else if(test==6){
  deliver(now);old=sends;message=client.client;
  for(i=0;i<1000;i++)mcc_admission_receive(&host,2,&message,sizeof(message),now);
  CHECK(sends==old);message.op=MCC_ADMISSION_REFUSED;
  mcc_admission_receive(&client,-1,&message,sizeof(message),now);CHECK(mcc_admission_status(&client)==-1);
 }else return 2;
 return 0;
}
''')
    return compile_tool(work, [source, ROOT / "port/linux/game/mcc_admission.c"])


@pytest.mark.parametrize("case", range(7), ids=["fixed-deadline", "untrusted-input", "ready-grace",
                                               "silence-and-peer-lifetime", "tombstone-revision",
                                               "clock-wrap-backpressure", "rate-and-refusal"])
def test_lease(admission_tool, case):
    result = subprocess.run([str(admission_tool), str(case)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr


@pytest.mark.parametrize("missing_lobby_route", [False, True], ids=["current-route", "before-fix-route"])
def test_game_adapter_authenticates_transport_and_current_settings(tmp_path, missing_lobby_route):
    from harness import structure, function, enum_with

    headers = {
        "cseries.h": "#pragma once\n#include <stddef.h>\ntypedef int boolean;typedef unsigned short word;typedef unsigned char byte;\n#define NONE -1\n#define NUMBEROF(a) (sizeof(a)/sizeof((a)[0]))\n",
        "networking/network_game_manager.h": "#pragma once\nstruct network_player {int valid,machine_index;};\nstruct network_game {struct {long version;char name[128];} map;unsigned char mcc_content_sha256[32],mcc_metadata_sha256[32];struct network_player players[4];};\nboolean network_player_is_valid(struct network_player *);\n",
        "networking/network_game_globals.h": "#pragma once\nstruct network_game_server {int dummy;};struct network_game_client {int dummy;};struct network_game_server_client_machine {int dummy;};\nstruct network_game_server *global_network_game_server_get(void);struct network_game_client *global_network_game_client_get(void);struct network_game *network_game_get_game(void);\n",
        "networking/network_client_manager.h": "struct network_game *network_game_client_get_game(struct network_game_client *);\n",
    }
    for name, text in headers.items():
        path = tmp_path / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text)
    path = tmp_path / "bungie_net/common/message_header.h"
    path.parent.mkdir(parents=True)
    path.write_text((ROOT / "source/bungie_net/common/message_header.h").read_text())
    distributed = (ROOT / "port/linux/game/network_distributed.h").read_text()
    # Production is ILP32 and asserts this header is12 bytes. Native Linux
    # test compilers are LP64; preserve wire fields instead of adding tail
    # padding to the adapter envelope and falsely rejecting valid packets.
    (tmp_path / "network_distributed.h").write_text(
        '#include <stdint.h>\n#include "bungie_net/common/message_header.h"\n' + structure(distributed, "distributed_message_header").replace('unsigned long', 'uint32_t').replace('long', 'int32_t') +
        '\ntypedef char wire_header_is_12[sizeof(struct distributed_message_header)==12 ? 1 : -1];' +
        '\nenum {_distributed_message_mcc_admission=105};\n')
    # Compile the complete production adapter with the same public APIs and a
    # controlled engine, not a second implementation of its packet checks.
    adapter = tmp_path / "adapter.c"
    adapter.write_text((ROOT / "port/linux/game/mcc_admission_game.c").read_text())
    server_source = (ROOT / "source/networking/network_server_message_handler.c").read_text()
    ingress = function(server_source, "network_game_server_handle_client_message")
    if missing_lobby_route:
        before = "if (mcc_admission_handles_message(message, message_buffer_size))"
        assert ingress.count(before) == 1
        ingress = ingress.replace(before, "if (0)")
    datagram = function(server_source, "network_game_server_handle_datagram")
    unrelated = "".join(f"#define {name}(...) 1\n" for name in sorted(set(
        __import__("re").findall(r"\b(network_game_server_handle_message_\w+)\(", ingress + datagram))))
    declarations = r'''
#include "third_person_protocol.h"
#include "mcc_protocol.h"
#define MINIMUM_TRANSPORT_ERROR_MESSAGE_SIZE 4
#define TRANSPORT_ERROR_MESSAGE_TEXT_LENGTH 1
#define _network_game_server_state_ingame 1
#define match_assert(...) ((void)0)
static void network_event(char const *format,...){(void)format;}
#define third_person_session_active() 0
#define third_person_protocol_unwrap(a,b,c,d) ((void)(a),(void)(b),(void)(c),(void)(d),0)
#define mcc_protocol_unwrap(a,b,c,d) ((void)(a),(void)(b),(void)(c),(void)(d),0)
#define network_game_server_get_state(...) ingame
#define network_game_server_client_machine_is_loaded(...) loaded
static int network_game_server_get_client_machine(struct network_game_server *s,struct network_game_server_client_machine *m,long *index){(void)s;(void)m;*index=2;return mapped;}
#define network_game_server_client_machine_heard(...) ((void)0)
#define network_distributed_handle_stream_message(...) (++ordinary)
#define network_voice_handles_message(...) 0
#define network_voice_handle_message(...) ((void)0)
#define mcc_share_game_handles_message(...) 0
#define mcc_share_game_handle_message(...) ((void)0)
#define map_share_handles_message(...) 0
#define map_share_handle_message(...) ((void)0)
#define network_map_vote_handles_message(...) 0
#define network_map_vote_handle_message(...) ((void)0)
#define transport_error_message_text(...) "unused"
#define TRUE 1
#define FALSE 0
#define NETWORK_GAME_MESSAGE_VERSION 2
#define _network_game_packet_class_client_search 0
#define _network_game_packet_class_client_ingame 1
#define MAXIMUM_NETWORK_MACHINE_COUNT 4
#define VALID_INDEX(i,n) ((unsigned)(i)<(unsigned)(n))
struct transport_address {struct {unsigned long long_words[1];} address;};
struct message_client_broadcast_game_search {int dummy;};
struct message_client_ping {int dummy;};
struct message_client_game_update {int dummy;};
static struct {void *connection;struct transport_address address;} network_distributed_return_addresses[4];
#define network_game_server_datagram_event_logged() 0
#define transport_address_to_string(...) "unused"
#define network_game_should_accept_remote_connections() 0
#define decode_network_game_message(a,b,c,d,e,f) ((void)(a),(void)(b),(void)(c),(void)(d),(void)(e),(void)(f),0)
#define network_game_server_get_client_machine_at_address(...) (joined ? &peer : NULL)
#define network_game_server_get_remote_client_machine_at_address(...) (joined ? &peer : NULL)
#define network_game_server_handle_client_update_packet(...) ((void)0)
#define network_game_server_get_client_connection(...) NULL
#define network_distributed_handle_message(i,m,n) (mcc_load_network_handles_message(m,n) ? mcc_load_network_handle_message(i,m,n,0) : mcc_admission_handle_message(i,m,n,0))
''' + unrelated + enum_with((ROOT / "source/networking/network_messages.h").read_text(),
                           "_message_client_broadcast_game_search") + enum_with(server_source, "_message_type_error")
    source = tmp_path / "test.c"
    source.write_text(r'''
#include "cseries.h"
#include "networking/network_game_globals.h"
#include "networking/network_game_manager.h"
#include "network_distributed.h"
#include "mcc_admission.h"
#include <string.h>
#include <stdio.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static struct network_game game;
static struct network_game_server server;
static struct network_game_client client;
static struct network_game_server_client_machine peer;
static int hosting=1,joined=1,capable=1,connected=1,sends,ingame,loaded,ordinary,mapped=1,progress;
static unsigned long now=1000,revision=1;
static struct {struct distributed_message_header header;struct mcc_admission_message body;} packet,last;
typedef char wire_message_is_216[sizeof(packet)==216 ? 1 : -1];
static int mcc_load_network_handles_message(void const *data,unsigned size){return size>=sizeof(struct distributed_message_header) && ((struct distributed_message_header const *)data)->type==106;}
static void mcc_load_network_handle_message(long index,void const *data,unsigned size,int reliable){(void)index;(void)data;(void)size;if(reliable)progress++;}
struct network_game_server *global_network_game_server_get(void){return hosting ? &server : NULL;}
struct network_game_client *global_network_game_client_get(void){return connected ? &client : NULL;}
struct network_game *network_game_get_game(void){return &game;}
struct network_game *network_game_client_get_game(struct network_game_client *c){(void)c;return &game;}
struct network_game_server_client_machine *network_game_server_get_client_machine_at_index(struct network_game_server *s,long n){(void)s;return n==2 ? &peer : NULL;}
boolean network_game_server_client_machine_is_joined_to_game(struct network_game_server *s,struct network_game_server_client_machine *p){(void)s;return joined && p==&peer;}
int mcc_protocol_allows(struct network_game const *g,struct network_game_server_client_machine *p){(void)g;(void)p;return capable;}
boolean network_player_is_valid(struct network_player *p){return p->valid;}
boolean mcc_level_name(char const *name){return !strncmp(name,"mcc_maps\\",9);}
unsigned long mcc_identity_host_revision(void){return revision;}
void const *mcc_identity_host_settings(void const *g,long bytes,struct network_game *copy){if(bytes!=sizeof(*copy))return NULL;memcpy(copy,g,bytes);return copy;}
unsigned long system_milliseconds(void){return now;}
void build_message_header(word *out,word size,byte type,byte flags){*out=(word)((size<<4)|(type<<2)|flags);}
boolean network_distributed_client_send_reliably(void *p,word size){if(size!=sizeof(last))return 0;memcpy(&last,p,size);sends++;return 1;}
boolean network_distributed_server_send_to_machine_reliably(long n,void *p,word size){if(n!=2)return 0;return network_distributed_client_send_reliably(p,size);}
''' + declarations + ingress + datagram + r'''
static void receive(int reliable){
 struct transport_address address={{0}};
 if(reliable)network_game_server_handle_client_message(&server,&peer,(word *)&packet,sizeof(packet));
 else network_game_server_handle_datagram(&server,(word *)&packet,sizeof(packet),&address);
}
int main(void){
 int old;uint32_t token;
 strcpy(game.map.name,"mcc_maps\\package\\level");game.map.name[64]=9;
 game.map.version=123;game.mcc_content_sha256[31]=42;game.mcc_metadata_sha256[0]=7;
 packet.header.type=105;packet.header.count=1;build_message_header(&packet.header.header,sizeof(packet),2,0);
 packet.body.op=MCC_ADMISSION_BEGIN;packet.body.generation=1;
 memcpy(packet.body.identity.name,game.map.name,128);packet.body.identity.checksum=123;
 memcpy(packet.body.identity.content,game.mcc_content_sha256,32);memcpy(packet.body.identity.metadata,game.mcc_metadata_sha256,32);
 CHECK(mcc_admission_handles_message(&packet,sizeof(packet)));
 packet.header.type=106;receive(0);CHECK(!progress);
 ingame=loaded=1;receive(0);CHECK(!progress);ingame=loaded=0;
 joined=0;receive(1);CHECK(!progress);joined=1;
 receive(1);CHECK(progress==1 && !ingame && !loaded);packet.header.type=105;
 receive(0);CHECK(!sends && !mcc_admission_host_active(2));
 ingame=loaded=1;receive(0);CHECK(!sends && !mcc_admission_host_active(2));ingame=loaded=0;
 mcc_admission_handle_message(3,&packet,sizeof(packet),1);CHECK(!sends);
 mcc_admission_handle_message(2,&packet,sizeof(packet)-1,1);CHECK(!sends);
 packet.header.count=2;receive(1);CHECK(!sends);packet.header.count=1;
 packet.header.header|=1;receive(1);CHECK(!sends);packet.header.header&=~1;
 capable=0;receive(1);CHECK(!sends);capable=1;joined=0;receive(1);CHECK(!sends);joined=1;
 mapped=0;receive(1);CHECK(!sends);mapped=1;
 receive(1);CHECK(sends==1 && last.body.op==MCC_ADMISSION_ACK && mcc_admission_host_needs_map(2));
 CHECK(!ingame && !loaded && !ordinary);
 packet.header.type=1;receive(1);CHECK(!ordinary);
 ingame=loaded=1;receive(1);CHECK(ordinary==1);ingame=loaded=0;packet.header.type=105;
 old=sends;packet.body.identity.name[64]++;receive(1);CHECK(sends==old);packet.body.identity.name[64]--;
 revision++;game.map.name[64]++;mcc_admission_update();CHECK(!mcc_admission_host_active(2));
 receive(1);CHECK(sends==old); /* old origin cannot inherit revised hold */
 mcc_admission_disconnect();hosting=0;game.map.name[64]--;
 CHECK(mcc_admission_client_begin(&game));CHECK(mcc_admission_client_pending());
 token=mcc_admission_client_token();mcc_admission_client_clear();
 CHECK(mcc_admission_client_begin(&game));CHECK(token!=mcc_admission_client_token());
 mcc_admission_client_release(token);CHECK(mcc_admission_client_pending());
 packet=last;packet.body.op=MCC_ADMISSION_ACK;
 mcc_admission_handle_message(NONE,&packet,sizeof(packet),0);CHECK(!mcc_admission_client_status());
 mcc_admission_handle_message(NONE,&packet,sizeof(packet),1);CHECK(mcc_admission_client_status()==1);
 game.map.name[64]++;mcc_admission_update();CHECK(mcc_admission_client_status()==-1);
 mcc_admission_disconnect();CHECK(!mcc_admission_client_pending());
 CHECK(mcc_admission_client_begin(&game));token=mcc_admission_client_token();
 mcc_admission_client_release(token);CHECK(!mcc_admission_client_pending());
 return 0;
}
''')
    exe = compile_tool(tmp_path, [source, adapter, ROOT / "port/linux/game/mcc_admission.c"])
    result = subprocess.run([str(exe)], capture_output=True, text=True)
    if missing_lobby_route:
        assert result.returncode == 1 and "sends==1" in result.stderr, result.stderr
    else:
        assert result.returncode == 0, result.stderr
