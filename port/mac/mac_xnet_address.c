#include "mac_xnet_address.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <ifaddrs.h>
#include <string.h>

int mac_xnet_local_address(unsigned long *ipv4,unsigned char hardware[6])
{
    struct ifaddrs *interfaces,*item,*link;
    const char *interface_name=NULL;
    unsigned long address=0;
    unsigned i;
    if(!ipv4 || !hardware)return 0;
    *ipv4=0;
    for(i=0;i<6;++i)hardware[i]=0;
    if(getifaddrs(&interfaces)!=0)return 0;
    for(item=interfaces;item;item=item->ifa_next)
        if(item->ifa_addr && item->ifa_addr->sa_family==AF_INET &&
            (item->ifa_flags&IFF_UP) && !(item->ifa_flags&IFF_LOOPBACK))
        {
            address=((struct sockaddr_in *)item->ifa_addr)->sin_addr.s_addr;
            if(address){interface_name=item->ifa_name;break;}
        }
    if(interface_name)
        for(link=interfaces;link;link=link->ifa_next)
            if(link->ifa_addr && link->ifa_addr->sa_family==AF_LINK &&
                !strcmp(link->ifa_name,interface_name))
            {
                struct sockaddr_dl *ethernet=(struct sockaddr_dl *)link->ifa_addr;
                if(ethernet->sdl_alen==6)
                    memcpy(hardware,LLADDR(ethernet),6);
                break;
            }
    freeifaddrs(interfaces);
    *ipv4=address;
    return address!=0;
}
