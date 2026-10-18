/* A conservative native XNet link query. Darwin exposes interface-up state,
   but not an Ethernet speed or duplex value through getifaddrs. */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <ifaddrs.h>

#define XNET_ETHERNET_LINK_ACTIVE 0x01

unsigned long XNetGetEthernetLinkStatus(void)
{
	struct ifaddrs *interfaces, *item;
	unsigned long status=0;
	if(getifaddrs(&interfaces)!=0)return 0;
	for(item=interfaces;item;item=item->ifa_next)
	{
		if(item->ifa_addr && item->ifa_addr->sa_family==AF_INET &&
			(item->ifa_flags&IFF_UP) && !(item->ifa_flags&IFF_LOOPBACK))
		{
			status=XNET_ETHERNET_LINK_ACTIVE;
			break;
		}
	}
	freeifaddrs(interfaces);
	return status;
}
