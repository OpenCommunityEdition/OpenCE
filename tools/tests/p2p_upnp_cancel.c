#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
typedef unsigned long posix_ulong;
enum { DISCOVERY_DELAY=2000,PORT_ATTEMPTS=8,MAXIMUM_CHECKED_FORWARDINGS=64,
    UPNPCOMMAND_SUCCESS=0,UPNP_CONNECTED_IGD=1,UPNP_PRIVATEIP_IGD=2,UPNP_DISCONNECTED_IGD=3 };
#define FORWARDING_DESCRIPTION "Halo internet play"
#define LEASE_SECONDS "3600"
struct UPNPDev {int unused;};
static struct {int found;struct {char *controlURL;} urls;struct {struct {char *servicetype;} first;} data;
    char lan_address[64];int permanent_only;} upnp;
static int upnp_cancel_requested;
void posix_upnp_cancel(void);
static char const *test_case;
static int discoveries,igds,scans,addresses,adds,deletes,freed;
static struct UPNPDev dev;
static struct UPNPDev *upnpDiscover(int d,void *a,void *b,int c,int e,int f,int *err) {
    (void)d;(void)a;(void)b;(void)c;(void)e;(void)f;(void)err;discoveries++;
    if(!strcmp(test_case,"discovery"))posix_upnp_cancel();return &dev;
}
static void freeUPNPDevlist(struct UPNPDev *d) {assert(d==&dev);freed++;}
static int UPNP_GetValidIGD(struct UPNPDev *d,void *u,void *v,char *lan,size_t n,char *wan,size_t m) {
    (void)d;(void)u;(void)v;(void)n;(void)wan;(void)m;strcpy(lan,"local");igds++;return UPNP_CONNECTED_IGD;
}
static void FreeUPNPUrls(void *u) {(void)u;}
static int UPNP_GetGenericPortMappingEntry(char *u,char *t,char *i,char *ext,char *client,char *in,
    char *proto,char *desc,char *en,char *remote,char *duration) {
    (void)u;(void)t;(void)i;(void)ext;(void)en;(void)remote;(void)duration;scans++;
    if(!strcmp(test_case,"scan")) {
        strcpy(client,"local");strcpy(in,"42");strcpy(proto,"UDP");strcpy(desc,FORWARDING_DESCRIPTION);
        posix_upnp_cancel();return 0;
    }
    return -1;
}
static int port_unused(unsigned short p) {(void)p;return 1;}
static int UPNP_DeletePortMapping(char *u,char *t,char *p,char *proto,void *remote) {
    (void)u;(void)t;(void)p;(void)proto;(void)remote;deletes++;return 0;
}
static int UPNP_GetExternalIPAddress(char *u,char *t,char *address) {
    (void)u;(void)t;strcpy(address,"1.2.3.4");addresses++;
    if(!strcmp(test_case,"address"))posix_upnp_cancel();return 0;
}
static int UPNP_AddPortMapping(char *u,char *t,const char *ext,const char *in,char *lan,const char *desc,
    const char *proto,void *remote,const char *lease) {
    (void)u;(void)t;(void)ext;(void)in;(void)lan;(void)desc;(void)proto;(void)remote;(void)lease;adds++;
    if(!strcmp(test_case,"retry")) {posix_upnp_cancel();return 718;}
    if(!strcmp(test_case,"permanent")) {posix_upnp_cancel();return 725;}
    if(!strcmp(test_case,"success"))posix_upnp_cancel();
    if(!strcmp(test_case,"normal")&&adds==1)return 718;
    if(!strcmp(test_case,"normal")&&adds==2)return 725;
    return 0;
}
static char const *strupnperror(int r) {(void)r;return "router error";}
static void posix_random_bytes(void *v,size_t n) {memset(v,42,n);}
/* ACTUAL FUNCTIONS */
int main(int argc,char **argv) {
    posix_ulong address=0;unsigned short port=0;char error[160]="";int result;
    assert(argc==2);test_case=argv[1];
    if(!strcmp(test_case,"before"))posix_upnp_cancel();
    result=posix_upnp_forward_udp(10,10,&address,&port,error,sizeof(error));
    if(!strcmp(test_case,"before"))assert(!result&&!discoveries);
    else if(!strcmp(test_case,"discovery"))assert(!result&&freed==1&&!igds);
    else if(!strcmp(test_case,"scan"))assert(!result&&scans==1&&!deletes&&!addresses);
    else if(!strcmp(test_case,"address"))assert(!result&&addresses==1&&!adds);
    else if(!strcmp(test_case,"retry")||!strcmp(test_case,"permanent"))assert(!result&&adds==1);
    else {
        assert(result&&address&&port);
        assert(adds==(!strcmp(test_case,"normal")?3:1));
        /* A mapping made just as cancellation arrives remains owned and
         * must be removable, even with the cancellation latch set. */
        posix_upnp_stop_forwarding_udp(port);assert(deletes==1);
    }
    return 0;
}
