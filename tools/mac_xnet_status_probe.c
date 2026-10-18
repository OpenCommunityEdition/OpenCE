#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <stdio.h>

#define XNET_ETHERNET_LINK_ACTIVE 0x01
unsigned long XNetGetEthernetLinkStatus(void);

static int checks, failures;
#define CHECK(x) do { ++checks; if(!(x)) { ++failures; fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x); } } while(0)

static int has_active_ipv4_interface(void)
{
	struct ifaddrs *interfaces,*item;
	int found=0;
	if(getifaddrs(&interfaces)!=0)return -1;
	for(item=interfaces;item;item=item->ifa_next)
		if(item->ifa_addr && item->ifa_addr->sa_family==AF_INET &&
			(item->ifa_flags&IFF_UP) && !(item->ifa_flags&IFF_LOOPBACK)) { found=1; break; }
	freeifaddrs(interfaces);
	return found;
}

int main(void)
{
	int expected=has_active_ipv4_interface();
	unsigned long first=XNetGetEthernetLinkStatus();
	unsigned long second=XNetGetEthernetLinkStatus();
	CHECK(expected>=0);
	if(expected>=0)CHECK(first==(expected?XNET_ETHERNET_LINK_ACTIVE:0));
	CHECK(second==first);
	CHECK((first&~XNET_ETHERNET_LINK_ACTIVE)==0); /* speed/duplex unknown */
	printf("Mac XNet link status: %d checks, %d failures (status 0x%lx)\n",checks,failures,first);
	return failures?1:0;
}
