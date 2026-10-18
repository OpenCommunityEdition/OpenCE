/* Native XNet compatibility for system-link IPv4. Xbox key material is not
   used by the shared transport; internet peers require the separate P2P
   provider and are never guessed from an XNADDR here. */
#define BUILDING_CSERIES 1
#include "engine_xdk_prefix.h"
#include "mac_xnet_address.h"
#include "net_settings.h"

void posix_random_bytes(void *buffer,unsigned int size);

int XNetStartup(const struct XNetStartupParams *parameters)
{
    (void)parameters;
    return 0;
}

int XNetCleanup(void)
{
    return 0;
}

unsigned long XNetGetTitleXnAddr(struct XNADDR *address)
{
    unsigned long ipv4;
    unsigned char hardware[6];
    unsigned i;
    if(!address)return WSAEFAULT;
    address->bSizeOfStruct=(unsigned char)sizeof(*address);
    address->bFlags=0;
    address->ina.s_addr=0;
    for(i=0;i<sizeof(address->abEnet);++i)address->abEnet[i]=0;
    {
        /* port: network.address is this machine's system link address when
           it is set, as title_address has it in port/linux/src/xnet.c. This
           is the address the game advertises itself at, so several copies on
           one machine, each on its own loopback address, must answer with
           theirs and not with the one interface they share. */
        unsigned long configured;
        if(net_settings_local_address(&configured))
        {
            mac_xnet_local_address(&ipv4,hardware);
            address->ina.s_addr=configured;
            for(i=0;i<sizeof(address->abEnet);++i)address->abEnet[i]=hardware[i];
            return XNET_GET_XNADDR_ETHERNET|XNET_GET_XNADDR_DHCP;
        }
    }
    if(!mac_xnet_local_address(&ipv4,hardware))return XNET_GET_XNADDR_NONE;
    address->ina.s_addr=ipv4;
    for(i=0;i<sizeof(address->abEnet);++i)address->abEnet[i]=hardware[i];
    return XNET_GET_XNADDR_ETHERNET|XNET_GET_XNADDR_DHCP;
}

int XNetRandom(unsigned char *buffer,unsigned int size)
{
    if(size && !buffer)return WSAEFAULT;
    posix_random_bytes(buffer,size);
    return 0;
}

int XNetCreateKey(struct XNKID *identifier,struct XNKEY *key)
{
    if(!identifier || !key)return WSAEFAULT;
    posix_random_bytes(identifier,sizeof(*identifier));
    posix_random_bytes(key,sizeof(*key));
    return 0;
}

int XNetRegisterKey(const struct XNKID *identifier,const struct XNKEY *key)
{
    /* The shared Linux system-link transport also treats this key as unused;
       no Xbox encryption layer exists on these native ports. */
    if(!identifier || !key)return WSAEFAULT;
    return 0;
}

int XNetUnregisterKey(const struct XNKID *identifier)
{
    if(!identifier)return WSAEFAULT;
    return 0;
}

int XNetXnAddrToInAddr(const struct XNADDR *address,const struct XNKID *key,
    struct in_addr *result)
{
    (void)key;
    if(!address || !result)return WSAEFAULT;
    /* Native compatibility supports ordinary IPv4 system-link addresses.
       Opaque tunnel identities need a real P2P mapping and must not silently
       turn into an unrelated address. */
    if(!address->ina.s_addr)return WSAEHOSTUNREACH;
    *result=address->ina;
    return 0;
}
