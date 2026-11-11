/* The workers, initializer and shutdown below come from p2p.c. Only the
 * external services are fake; worker completion uses real OS threads. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
typedef CRITICAL_SECTION test_mutex;
static void mutex_init(test_mutex *m) { InitializeCriticalSection(m); }
static void mutex_lock(test_mutex *m) { EnterCriticalSection(m); }
static void mutex_unlock(test_mutex *m) { LeaveCriticalSection(m); }
#define TLS __declspec(thread)
#else
#include <pthread.h>
#include <unistd.h>
typedef pthread_mutex_t test_mutex;
typedef unsigned long DWORD;
typedef struct native_thread *HANDLE;
#define WINAPI
#define INFINITE 0xffffffffu
static void Sleep(unsigned n) { usleep(n*1000); }
static void mutex_init(test_mutex *m) { pthread_mutex_init(m,NULL); }
static void mutex_lock(test_mutex *m) { pthread_mutex_lock(m); }
static void mutex_unlock(test_mutex *m) { pthread_mutex_unlock(m); }
#define TLS _Thread_local
#endif
static test_mutex p2p_lock;
static TLS int lock_depth;
static void lock(test_mutex *m) { mutex_lock(m); lock_depth++; }
static void unlock(test_mutex *m) { lock_depth--; mutex_unlock(m); }
#define pthread_mutex_lock lock
#define pthread_mutex_unlock unlock
static int cancelled, allow_poll, polling, allow_router, routing;
static int calls, closes, waits, fail_create, failed_creates, fail_hook, online=1, upnp_allowed;
static int lobby_quits, router_releases, work_after_stop, renewed, flowing, passes;
static struct { HANDLE handle;int joined; } handles[16];
static void (*exit_hook)(void);
static int load(int *p) { return __atomic_load_n(p,__ATOMIC_SEQ_CST); }
static void set(int *p,int n) { __atomic_store_n(p,n,__ATOMIC_SEQ_CST); }
static void until(int *p) { int i; for(i=0;i<5000&&!load(p);i++)Sleep(1); assert(load(p)); }
#ifndef _WIN32
struct native_thread { pthread_t id; DWORD (*start)(void*); void *arg; };
static void *native_start(void *v) { struct native_thread *t=v; t->start(t->arg);return NULL; }
#endif
static HANDLE create_thread(void *a,DWORD size,DWORD (WINAPI *start)(void*),void *arg,DWORD flags,void *id) {
    HANDLE h;
    (void)a;(void)size;(void)flags;(void)id;calls++;
    if(fail_create){failed_creates++;return NULL;}
#ifdef _WIN32
    h=CreateThread(NULL,0,start,arg,0,NULL);
#else
    h=calloc(1,sizeof(*h));h->start=start;h->arg=arg;
    assert(!pthread_create(&h->id,NULL,native_start,h));
#endif
    assert(h);handles[calls-1].handle=h;return h;
}
static void wait_thread(HANDLE h,DWORD timeout) {
    int i;
    assert(!lock_depth && timeout==INFINITE);__atomic_fetch_add(&waits,1,__ATOMIC_SEQ_CST);
#ifdef _WIN32
    assert(WaitForSingleObject(h,timeout)==WAIT_OBJECT_0);
#else
    assert(!pthread_join(h->id,NULL));
#endif
    for(i=0;i<calls;i++)if(handles[i].handle==h){handles[i].joined=1;break;}
    assert(i<calls);
}
static void close_thread(HANDLE h) {
    int i;
    /* A detached worker reaching CRT teardown is the original defect. */
    for(i=0;i<calls;i++)if(handles[i].handle==h)break;
    assert(i<calls&&handles[i].joined);handles[i].handle=NULL;
    __atomic_fetch_add(&closes,1,__ATOMIC_SEQ_CST);
#ifdef _WIN32
    assert(CloseHandle(h));
#else
    free(h);
#endif
}
#define CreateThread create_thread
#define WaitForSingleObject wait_thread
#define CloseHandle close_thread
static int hook(void (*fn)(void)) { if(fail_hook)return -1;assert(!exit_hook);exit_hook=fn;return 0; }
#define atexit hook
typedef unsigned long posix_ulong;
struct p2p_candidate { unsigned long address; unsigned short port; };
struct stream { int used,socket,state,pending_size; void *kcp; unsigned long created_time; };
enum { MAXIMUM_PROXIES=1,MAXIMUM_LISTENERS=1,MAXIMUM_STREAMS=1,LOOP_INTERVAL=10,PEER_TIMEOUT=20000,
    STREAM_WINDOW=100,_stream_open_state=1,_stream_connecting=2,LOG_ADDRESS_SIZE=80,
    UPNP_RENEW_INTERVAL=100,UPNP_RETRY_INTERVAL=100,HANDOFF_PORT=2000 };
#ifndef SOCK_DGRAM
#define SOCK_DGRAM 2
#define SOL_SOCKET 1
#define SO_SNDBUF 1
#define SO_RCVBUF 2
#endif
static struct {
    int running,stopping,exit_registered;
    HANDLE thread,upnp_thread;
    unsigned long local_address;
    int tunnel_socket,handoff_socket;
    unsigned short tunnel_port;
    struct {int socket;} proxies[1],listeners[1];
    struct stream streams[1];
    int has_handoff_key;unsigned char handoff_key[32];
    int upnp_working,upnp_asked,upnp_forwarded;
    unsigned long upnp_time;
    struct p2p_candidate upnp_candidate;
    int hosting,hosting_socket,has_token,token_listed;unsigned char token[32];
} p2p = /* ACTUAL INITIALIZER */;
static unsigned long p2p_now(void) { return 1000; }
static int elapsed(unsigned long t,unsigned long n) { (void)t;(void)n;return renewed; }
static int config_boolean(const char *s) { return !strcmp(s,"network.online")?online:upnp_allowed; }
static long config_integer(const char *s) { (void)s;return 0; }
static void platform_log(const char *s,...) { (void)s;assert(!lobby_quits); }
static void posix_upnp_cancel(void) { set(&cancelled,1); }
static int posix_upnp_forward_udp(unsigned short p,unsigned short old,posix_ulong *addr,unsigned short *out,char *e,int z) {
    (void)p;(void)old;(void)e;(void)z;set(&routing,1);until(&allow_router);*addr=3;*out=4;return 1;
}
static void posix_upnp_stop_forwarding_udp(unsigned short port) { assert(!lock_depth && port==4);router_releases++; }
static char *log_address_ipv4(unsigned long a,unsigned short p,char *s,int z) { (void)a;(void)p;(void)z;return s; }
static int upnp_needed(void) { return 1; }
static void p2p_lobby_quit(void) { assert(!p2p.thread && !p2p.upnp_thread);lobby_quits++; }
static int posix_socket_select(int *r,int *nr,int *w,int *nw,void *e,int *ne,int a,int b,int c) {
    (void)r;(void)w;(void)e;(void)ne;(void)a;(void)b;(void)c;set(&polling,1);until(&allow_poll);*nr=*nw=0;return 0;
}
static void p2p_register_url_scheme(const char *a,const char *b) { (void)a;(void)b; }
static void p2p_signal_select_sets(int *r,int *nr,int *w,int *nw,int n) { (void)r;(void)nr;(void)w;(void)nw;(void)n; }
static void p2p_signal_update(int *r,int nr,int *w,int nw) {
    (void)r;(void)nr;(void)w;(void)nw;if(p2p.stopping)work_after_stop++;
    if(flowing){set(&allow_poll,0);set(&polling,0);set(&passes,1);}
}
#define ikcp_waitsnd(k) 0
#define network_resumed(n) ((void)0)
#define tunnel_readable() ((void)0)
#define handoff_readable() ((void)0)
#define proxy_readable(p) ((void)0)
#define listener_readable(p) ((void)0)
#define stream_readable(p) ((void)0)
#define stream_writeable(p) ((void)0)
#define stream_local_closed(p) ((void)0)
#define stream_update(p) ((void)0)
#define update_peers() ((void)0)
#define update_probes() ((void)0)
#define p2p_webrtc_update() 0
#define expire_proxies() ((void)0)
#define public_address_update() ((void)0)
#define stun_update() ((void)0)
#define update_hosting() ((void)0)
#define p2p_lobby_browsing() 0
#define p2p_signal_start() ((void)0)
#define hosted_player_counts(c,m) (*(c)=1,*(m)=2)
#define p2p_lobby_update(t,c,m) ((void)0)
#define p2p_lobby_listed() 0
#define update_joining() ((void)0)
#define update_answering() ((void)0)
#define update_reaching() ((void)0)
#define p2p_discord_update() ((void)0)
#define p2p_identifier() ((void)0)
#define network_short(n) (n)
#define network_long(n) (n)
#define posix_socket_setsockopt(s,l,o,v,z) ((void)(v))
#define handoff_key(k) 0
#define stun_setup() ((void)0)
#define command_line_invite(i,z) ((void)(i),0)
#define p2p_join_invite(i) ((void)0)
static int open_socket(int a,unsigned long b,unsigned short c,unsigned short *p) { (void)a;(void)b;(void)c;if(p)*p=6;return 7; }
static void close_socket(int *s) {*s=-1;}
/* ACTUAL FUNCTIONS */
static DWORD WINAPI shutdown_start(void *unused) { (void)unused;p2p_shutdown();return 0; }
int main(int argc,char **argv) {
    HANDLE shutdown;
    assert(argc==2);mutex_init(&p2p_lock);
    if(!strcmp(argv[1],"offline")) { online=0;p2p_initialize(1);assert(!calls&&!exit_hook);return 0; }
    if(!strcmp(argv[1],"hook_fail")) { fail_hook=1;p2p_initialize(1);assert(!calls&&!p2p.running&&p2p.tunnel_socket==-1);return 0; }
    if(!strcmp(argv[1],"start_fail")) { fail_create=1;p2p_initialize(1);assert(!p2p.running&&exit_hook);exit_hook();assert(!waits);return 0; }
    if(!strcmp(argv[1],"retry_start")) {
        fail_create=1;p2p_initialize(1);assert(!p2p.running&&exit_hook);
        fail_create=0;
    }
    flowing=!strcmp(argv[1],"flow");
    upnp_allowed=strcmp(argv[1],"poll")!=0&&strcmp(argv[1],"retry_start")!=0&&!flowing;
    if(upnp_allowed) {
        fail_create=!strcmp(argv[1],"upnp_fail");
        lock(&p2p_lock);update_upnp();unlock(&p2p_lock);
        if(fail_create) { assert(!p2p.upnp_working&&!p2p.upnp_thread);p2p_shutdown();return 0; }
        until(&routing);
        if(!strcmp(argv[1],"renew")) {
            set(&allow_router,1);
            for(;;) { int done;lock(&p2p_lock);done=!p2p.upnp_working;unlock(&p2p_lock);if(done)break;Sleep(1); }
            renewed=1;lock(&p2p_lock);update_upnp();unlock(&p2p_lock);
            assert(waits==1&&closes==1&&calls==2);
        }
    } else {
        if(flowing)set(&allow_poll,1);
        p2p_initialize(1);assert(exit_hook&&p2p.running);
        if(flowing)until(&passes);
        until(&polling);
    }
    shutdown=create_thread(NULL,0,shutdown_start,NULL,0,NULL);
    until(&cancelled);
    set(&allow_poll,1);set(&allow_router,1);
    wait_thread(shutdown,INFINITE);close_thread(shutdown);
    assert(!work_after_stop&&lobby_quits==1&&!p2p.thread&&!p2p.upnp_thread);
    assert(router_releases==upnp_allowed);
    assert(waits==closes&&closes==calls-failed_creates);
    assert(!lock_depth);
    return 0;
}
