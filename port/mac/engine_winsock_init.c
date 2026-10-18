/* POSIX sockets need no subsystem start or cleanup. Preserve Winsock's
   version negotiation and output record so native game callers get the
   requested compatible version rather than a fabricated Xbox runtime. */
#define BUILDING_CSERIES 1
#include "engine_xdk_prefix.h"

/* port/linux/src/net_settings.c (declared here, as the socket adapter
   declares the host's calls, so this unit needs no new include path) */
void net_settings_read(void);

static void copy_text(char *destination,unsigned size,const char *source)
{
    unsigned i=0;
    if(!size)return;
    while(i+1<size && source[i]){destination[i]=source[i];++i;}
    destination[i]=0;
    while(++i<size)destination[i]=0;
}

int WSAStartup(unsigned short requested,struct WSAData *data)
{
    unsigned major=requested&0xffU;
    unsigned minor=(requested>>8)&0xffU;
    if(!data)return WSAEFAULT;
    if(major!=2 || minor>2)return WSAVERNOTSUPPORTED;
    /* here, before the game's network threads start: nothing locks them */
    net_settings_read();
    data->wVersion=requested;
    data->wHighVersion=0x0202;
    copy_text(data->szDescription,sizeof(data->szDescription),"Native POSIX sockets");
    copy_text(data->szSystemStatus,sizeof(data->szSystemStatus),"Sockets ready");
    data->iMaxSockets=64;
    data->iMaxUdpDg=1264;
    data->lpVendorInfo=NULL;
    return 0;
}

int WSACleanup(void)
{
    return 0;
}
