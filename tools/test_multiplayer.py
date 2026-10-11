"""Exercise actual encrypted tunnel code with UDP blocked and MQTT relay delivery."""
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
import wasm_test
import re, shutil, subprocess
root=Path(__file__).resolve().parent.parent
out=root/'build/multiplayer-tests';out.mkdir(parents=True,exist_ok=True)
p=(root/'port/linux/src/p2p.c').read_text()
s=(root/'port/linux/src/p2p_signal.c').read_text()
def block(text,start):
 begin=text.index(start);brace=text.index('{',begin);depth=1;end=brace+1
 while depth:
  depth+=(text[end]=='{')-(text[end]=='}');end+=1
 return text[begin:end]
def function(text,name):
 m=re.search(r'^(?:static )?[\w *]+\b'+name+r'\([^;]*?\)\s*\{',text,re.M)
 assert m,name
 return block(text,text[m.start():text.index('{',m.start())])
lib=r'''
typedef unsigned long size_t;
#define NULL ((void*)0)
void *memcpy(void*d,const void*s,size_t n){unsigned char*a=d;const unsigned char*b=s;while(n--)*a++=*b++;return d;}
void *memset(void*d,int v,size_t n){unsigned char*a=d;while(n--)*a++=v;return d;}
int memcmp(const void*a,const void*b,size_t n){const unsigned char*x=a,*y=b;while(n--){if(*x!=*y)return *x-*y;x++;y++;}return 0;}
void *memmove(void*d,const void*s,size_t n){unsigned char*a=d;const unsigned char*b=s;if(a<b)return memcpy(d,s,n);while(n){n--;a[n]=b[n];}return d;}
size_t strlen(const char*s){size_t n=0;while(s[n])n++;return n;}
int strcmp(const char*a,const char*b){while(*a&&*a==*b){a++;b++;}return (unsigned char)*a-(unsigned char)*b;}
int strncmp(const char*a,const char*b,size_t n){while(n&&*a&&*a==*b){a++;b++;n--;}return n?(unsigned char)*a-(unsigned char)*b:0;}
char *strcpy(char*d,const char*s){char*r=d;while((*d++=*s++)){}return r;}
void posix_random_bytes(void*d,int n){memset(d,7,n);}
void platform_log(const char*f,...){ }
static unsigned long now;
unsigned long p2p_now(void){return now;}
static int elapsed(unsigned long t,unsigned long duration){return now-t>=duration;}
enum{P2P_IDENTIFIER_SIZE=6,P2P_SHA256_SIZE=32,P2P_NONCE_SIZE=12,P2P_TAG_SIZE=16,P2P_KEY_SIZE=32,P2P_SEAL_OVERHEAD=28,P2P_MAXIMUM_CANDIDATES=4,P2P_MAXIMUM_PEERS=127,P2P_RELAY_PACKET_SIZE=2048,P2P_RELAY_PROBE_SIZE=36};
'''
lib+="\nint p2p_equal(const void*,const void*,int);\n"
crypto=re.sub(r'^#include.*\n','',(root/'port/linux/src/p2p_crypto.c').read_text(),flags=re.M)
# These relay tests exercise SHA/HMAC and the sealed tunnel. Public listing
# signatures are covered separately by upstream's p2p_lobby_check.
crypto=crypto[:crypto.index('void p2p_sha512(')]
transport=r'''
enum{TUNNEL_MAGIC=0x69,TUNNEL_HEADER_SIZE=15,MAXIMUM_INNER_SIZE=1400,MAXIMUM_PACKET_SIZE=1431,REPLAY_WINDOW=64,MAXIMUM_PEER_PROXIES=4,ENDPOINT_SWITCH_TIME=3000,PING_INTERVAL=1000,PUNCH_INTERVAL=200,PEER_TIMEOUT=20000,PUNCH_TIMEOUT=30000,_packet_ping=1,_packet_pong,_packet_datagram,_packet_stream,_packet_bye};
struct p2p_candidate{unsigned long address;unsigned short port;};
struct sockaddr_in{struct{unsigned long s_addr;}sin_addr;unsigned short sin_port;};
'''+block(p,'struct peer\n')+r''';
static struct{int tunnel_socket,joining;unsigned char join_host[6];struct peer peers[127];}p2p;
static unsigned char identifier[6],captured[2048];static int captured_size,udp,relayed,datagrams,streams;
static struct peer*find_peer(const unsigned char*id){for(int i=0;i<127;i++)if(p2p.peers[i].used&&!memcmp(id,p2p.peers[i].identifier,6))return &p2p.peers[i];return NULL;}
void make_address(struct sockaddr_in*a,unsigned long ip,unsigned short port){a->sin_addr.s_addr=ip;a->sin_port=port;}
int posix_socket_sendto(int fd,const void*d,int n,int f,const void*a,int z){udp++;memcpy(captured,d,n);captured_size=n;return n;}
int p2p_signal_relay_send(const unsigned char*id,const unsigned char*d,int n){relayed++;memcpy(captured,d,n);captured_size=n;return 1;}
static int webrtc_sent;
void p2p_webrtc_send(int connection,const void*d,int n){webrtc_sent++;}
void p2p_signal_stop_joining(void){}
char *address_text(unsigned long a,unsigned short p,char*t){return "direct";}
void stun_received(const unsigned char*p,int n,const struct sockaddr_in*a){}
void datagram_received(struct peer*p,const unsigned char*d,int n){datagrams++;}
void stream_received(struct peer*p,const unsigned char*d,int n){streams++;}
static void drop_peer(struct peer*p,const char*why){p->used=0;}
'''
for name in ['packet_counter','packet_nonce','peer_send_to','peer_send','peer_ping','packet_fresh','packet_received','peer_heard','update_peers','tunnel_received','p2p_relay_received']:
 transport+='\n'+function(p,name)
transport+=r'''
int run_tests(void){
 struct peer *peer=&p2p.peers[0];struct p2p_candidate relay={0,0};unsigned char inner[1400],saved[2048],sender[6]={1,2,3,4,5,6};int n;
 p2p.tunnel_socket=1;peer->used=1;peer->connection=-1;memcpy(identifier,sender,6);memcpy(peer->identifier,sender,6);
 memset(peer->send_key,42,32);memcpy(peer->receive_key,peer->send_key,32);peer->receive_window=1;
 inner[0]=_packet_datagram;memset(inner+1,99,1399);
 now=4000;peer_send_to(peer,&relay,inner,1400);if(udp||relayed!=1||captured_size!=1431)return 1;
 n=captured_size;memcpy(saved,captured,n);if(!p2p_relay_received(sender,saved,n)||!peer->connected||peer->endpoint.address||datagrams!=1)return 2;
 if(p2p_relay_received(sender,saved,n)||datagrams!=1)return 3; /* replay */
 peer_send_to(peer,&relay,inner,1400);memcpy(saved,captured,n);saved[n-1]^=1;
 if(p2p_relay_received(sender,saved,n)||datagrams!=1)return 4; /* tampered tag */
 saved[n-1]^=1;saved[1]^=1;if(p2p_relay_received(sender,saved,n))return 5; /* wrong sender */
 saved[1]^=1;peer->receive_key[0]^=1;if(p2p_relay_received(sender,saved,n))return 6;
 peer->receive_key[0]^=1;if(!p2p_relay_received(sender,saved,n)||datagrams!=2)return 7;
 peer_heard(peer,123,456,1);if(peer->endpoint.address!=123)return 8; /* immediate UDP upgrade */
 now=4100;peer_heard(peer,0,0,1);if(peer->endpoint.address!=123)return 9; /* relay cannot downgrade fresh UDP */
 now=7500;peer_heard(peer,0,0,1);if(peer->endpoint.address)return 10; /* lost UDP fallback */
 peer->connected=0;peer->offered_time=8000;peer->probe_time=8000;peer->sent_time=8000;peer->candidate_count=1;peer->candidates[0].address=123;peer->candidates[0].port=456;
 now=10000;relayed=udp=0;update_peers();if(relayed||!udp)return 11; /* first try UDP */
 now=12000;update_peers();if(!relayed)return 12; /* strict NAT relay probe */
 peer->connected=1;peer->endpoint=relay;peer->heard_time=12000;now=14000;udp=0;update_peers();if(!udp)return 13; /* keep trying direct */
 peer_send_to(peer,&relay,inner,1401);if(captured_size>1431)return 14;
 return 0;
}
'''
def run(name,code):
 (out/(name+'.c')).write_text(lib+crypto+code)
 ndk=Path('C:/Android/ndk/29.0.14206865/toolchains/llvm/prebuilt/windows-x86_64/bin')
 subprocess.run([str(ndk/'clang.exe'),'--target=wasm32','-DHALO_ANDROID=1','-std=c99','-ffreestanding','-fno-builtin','-c',str(out/(name+'.c')),'-o',str(out/(name+'.o'))],check=True)
 subprocess.run([str(ndk/'ld.lld.exe'),'-flavor','wasm','--no-entry','--export=run_tests',str(out/(name+'.o')),'-o',str(out/(name+'.wasm'))],check=True)
 wasm_test.check(out/(name+'.wasm'), name, name+' passed')
run('encrypted-tunnel-fallback',transport)
relay=r'''
enum { TOPIC_SIZE=64, BUFFER_SIZE=4096,OUTPUT_BUFFER_SIZE=65536,MAXIMUM_MESSAGE_SIZE=512,NUMBER_OF_TOPICS=5,MAXIMUM_LISTING_SIZE=256,MAXIMUM_IN_FLIGHT=4,PUBLISH_BURST=8,PUBLISH_INTERVAL=125,_topic_host=0,_topic_join,_topic_own_slot,_topic_query,_topic_slots,_broker_idle=0,_broker_ready=3,WSAEWOULDBLOCK=10035,WSAEINPROGRESS=10036,_message_join=1,_message_accept=2,MESSAGE_VERSION=3};
#define P2P_LOBBY_SLOT_PREFIX "hceu/3/lobby/s/"
#define P2P_LOBBY_QUERY_TOPIC "hceu/3/lobby/q"
'''+block(s,'struct broker\n')+';\n'+block(s,'struct relay_session\n')+r''';
static struct{struct relay_session relays[127];struct broker brokers[4];int broker_count,hosting,joining,lobby_listed,lobby_browsing;char own_slot[64],host_topic[64],join_topic[64];unsigned char host_key[32],join_key[32];}signalling;
static int enabled=1,accept_packet=1,received;
int config_boolean(const char*key){return enabled;}
void p2p_hex(const unsigned char*b,int n,char*t){const char*h="0123456789abcdef";for(int i=0;i<n;i++){t[2*i]=h[b[i]>>4];t[2*i+1]=h[b[i]&15];}t[2*n]=0;}
int posix_socket_send(int fd,const void*d,int n,int f){return -1;}
int posix_socket_last_error(void){return WSAEWOULDBLOCK;}
void posix_socket_close(int fd){}
int p2p_relay_received(const unsigned char*id,const unsigned char*d,int n){received++;return accept_packet;}
void join_received(struct broker*b,const unsigned char*d,int n){}
void accept_received(const unsigned char*d,int n){}
void p2p_lobby_slot_heard(const char*t,const unsigned char*d,int n,int retained){}
void p2p_lobby_query_heard(void){}
'''
for name in ['broker_close','broker_flush','put_variable','broker_send','put_string','next_packet_identifier','broker_topic','broker_publish','broker_carries_lobby','broker_sync_topics','publish_received','relay_topic','p2p_signal_relay_add','p2p_signal_relay_remove','p2p_signal_relay_send']:
 relay+='\n'+function(s,name)
relay+=r'''
int run_tests(void){
 unsigned char id[6]={1,2,3,4,5,6},tx[32],rx[32],packet[2048];char remote_topic[64];int before;
 memset(tx,1,32);memset(rx,2,32);signalling.broker_count=2;
 for(int b=0;b<2;b++){signalling.brokers[b].socket=b+1;signalling.brokers[b].state=_broker_ready;}
 p2p_signal_relay_add(id,tx,rx);if(!signalling.relays[0].used||!signalling.brokers[0].relay_subscribed[0]||!signalling.brokers[1].relay_subscribed[0])return 1;
 relay_topic(rx,remote_topic);if(strcmp(remote_topic,signalling.relays[0].receive_topic)||!strcmp(signalling.relays[0].send_topic,remote_topic))return 2;
 for(int b=0;b<2;b++)signalling.brokers[b].output_size=0;
 if(!p2p_signal_relay_send(id,packet,1431)||!signalling.brokers[0].output_size||!signalling.brokers[1].output_size)return 3;
 if(signalling.brokers[0].output[0]!=0x30)return 4; /* QoS0, nonretained */
 accept_packet=0;publish_received(&signalling.brokers[1],remote_topic,packet,1431,0);if(signalling.relays[0].preferred_broker!=-1)return 5;
 accept_packet=1;publish_received(&signalling.brokers[1],remote_topic,packet,1431,0);if(received!=2||signalling.relays[0].preferred_broker!=1)return 6;
 before=signalling.brokers[0].output_size;p2p_signal_relay_send(id,packet,1431);if(signalling.brokers[0].output_size!=before)return 7;
 p2p_signal_relay_send(id,packet,36);if(signalling.brokers[0].output_size==before)return 8; /* probes all brokers */
 before=received;publish_received(&signalling.brokers[0],"hceu/r/wrong",packet,1431,0);publish_received(&signalling.brokers[0],remote_topic,packet,2049,0);if(received!=before)return 9;
 signalling.brokers[1].output_size=OUTPUT_BUFFER_SIZE-1024;
 if(p2p_signal_relay_send(id,packet,1431)||signalling.brokers[1].state!=_broker_ready)return 10; /* congestion drops without disconnect */
 if(p2p_signal_relay_send(id,packet,2049)||p2p_signal_relay_send(id,packet,-1))return 11;
 broker_close(&signalling.brokers[1],1);if(signalling.brokers[1].relay_subscribed[0])return 12;
 if(!p2p_signal_relay_send(id,packet,1431)||signalling.relays[0].preferred_broker!=-1)return 13;
 signalling.brokers[1].socket=2;signalling.brokers[1].state=_broker_ready;broker_sync_topics(&signalling.brokers[1]);if(!signalling.brokers[1].relay_subscribed[0])return 14;
 p2p_signal_relay_remove(id);if(signalling.relays[0].used||signalling.brokers[0].relay_subscribed[0]||signalling.brokers[1].relay_subscribed[0]||p2p_signal_relay_send(id,packet,10))return 15;
 enabled=0;p2p_signal_relay_add(id,tx,rx);if(signalling.relays[0].used)return 16;
 return 0;
}
'''
run('mqtt-relay-session',relay)
# Verify the actual browser filters with mixed LAN and invited games.
m=(root/'port/linux/game/menu_functions.c').read_text()
browser=r'''
typedef int boolean;typedef short wchar_t;
#define MAX(a,b) ((a)>(b)?(a):(b))
#define MAXIMUM_ADVERTISED_GAMES 4
enum{_multiplayer_mode_server_browser,_multiplayer_mode_lan,_multiplayer_mode_direct_link};
struct advertised_game{int valid,progress,peer;};static struct advertised_game games[4];
static struct{int mode;short game_count,game_chosen;struct advertised_game *games[4];}multiplayer;
void *global_network_game_client_get(void){return (void*)1;}
struct advertised_game *network_game_client_get_available_games(void*c){return games;}
int network_game_client_advertised_game_is_valid(struct advertised_game*g){return g->valid;}
int advertised_in_progress(struct advertised_game*g){return g->progress;}
int game_from_peer(struct advertised_game*g){return g->peer;}
'''+function(m,'browser_games_read')+r'''
int run_tests(void){games[0].valid=games[1].valid=games[2].valid=1;games[1].peer=games[2].peer=1;games[2].progress=1;
 multiplayer.mode=_multiplayer_mode_server_browser;browser_games_read();if(multiplayer.game_count!=3||multiplayer.games[2]!=&games[2])return 1;
 multiplayer.mode=_multiplayer_mode_direct_link;browser_games_read();if(multiplayer.game_count!=2||multiplayer.games[0]!=&games[1])return 2;
 multiplayer.mode=_multiplayer_mode_lan;browser_games_read();if(multiplayer.game_count!=1||multiplayer.games[0]!=&games[0])return 3;
 multiplayer.game_chosen=3;browser_games_read();if(multiplayer.game_chosen)return 4;return 0;
}
'''
run('server-browser-discovery',browser)
assert 'return TRUE' not in function(m,'browser_initialize')
assert 'ui_widget_port_browse(screen, event, widget_deleted)' in function(m,'browser_initialize')
print('Browser initializes discovery in every mode')
invite=r'''
enum{P2P_KEY_HASH_SIZE=16,P2P_TOKEN_SIZE=16};
struct peer{int used,connected;unsigned char identifier[6];struct{unsigned long address;}endpoint;};
static struct{int running,joining,join_requested,join_error;unsigned char join_host[6],join_host_hash[16],join_token[16];}p2p;
static struct peer remote;static unsigned char identifier[6]={1,1,1,1,1,1};static int p2p_lock;
void pthread_mutex_lock(int*l){}void pthread_mutex_unlock(int*l){}
void p2p_identifier(void){}
void p2p_identifier_from_hash(const unsigned char*h,unsigned char*id){memcpy(id,h,6);}
struct peer *find_peer(const unsigned char*id){return remote.used&&!memcmp(id,remote.identifier,6)?&remote:NULL;}
int p2p_signal_connected(void){return 1;}
'''
for name in ['hex_value','parse_invite','join_invite','p2p_join_invite','p2p_join_status']:
 invite+='\n'+function(p,name)
invite+=r'''
int run_tests(void){
 const char *own="halo://join/0101010101010000000000000000000000000000000000000000000000000000";
 const char *link="hello HALO://JOIN/0202020202020000000000000000000000000000000000000000000000000000 thanks";
 if(p2p_join_invite(link)||p2p.join_requested||p2p_join_status()!=-1)return 1;
 p2p.running=1;if(p2p_join_invite(own)||p2p_join_status()!=-4)return 2;
 if(p2p_join_invite("not a link")||p2p_join_status()!=-3)return 3;
 if(!p2p_join_invite(link)||!p2p.join_requested||p2p_join_status()!=2)return 4;
 if(!p2p_join_invite(link))return 5;
 remote.used=1;memcpy(remote.identifier,p2p.join_host,6);if(p2p_join_status()!=3)return 6;
 remote.connected=1;if(p2p_join_status()!=4)return 7;remote.endpoint.address=123;if(p2p_join_status()!=5)return 8;
 p2p.join_error=-2;if(p2p_join_status()!=-2)return 9;return 0;
}
'''
run('invite-validation-status',invite)
