#define BUILDING_CSERIES 1
#include "engine_xdk_prefix.h"

static unsigned checks,failures;
#define CHECK(x) do { ++checks;if(!(x)){++failures;fprintf(stderr,"XNet compat FAIL line %d: %s\n",__LINE__,#x);} } while(0)
static int bytes_differ(const unsigned char *left,const unsigned char *right,unsigned size)
{
    unsigned i;
    for(i=0;i<size;++i)if(left[i]!=right[i])return 1;
    return 0;
}

int main(void)
{
    struct XNKID identifier,zero_identifier;
    struct XNKEY key,zero_key;
    struct XNADDR address;
    struct XNADDR title;
    struct WSAData winsock;
    struct in_addr translated;
    unsigned char random_bytes[32];
    unsigned i,nonzero=0;
    for(i=0;i<sizeof(zero_identifier.ab);++i)zero_identifier.ab[i]=0;
    for(i=0;i<sizeof(zero_key.ab);++i)zero_key.ab[i]=0;
    CHECK(XNetStartup(NULL)==0);
    CHECK(XNetCleanup()==0);
    CHECK(XNetGetTitleXnAddr(NULL)==WSAEFAULT);
    {
        unsigned long title_status=XNetGetTitleXnAddr(&title);
        CHECK(title.bSizeOfStruct==sizeof(title));
        CHECK((title_status&~(XNET_GET_XNADDR_ETHERNET|XNET_GET_XNADDR_DHCP|XNET_GET_XNADDR_NONE))==0);
        if(title_status==XNET_GET_XNADDR_NONE)CHECK(title.ina.s_addr==0);
        else CHECK((title_status&(XNET_GET_XNADDR_ETHERNET|XNET_GET_XNADDR_DHCP))==
            (XNET_GET_XNADDR_ETHERNET|XNET_GET_XNADDR_DHCP) && title.ina.s_addr!=0);
    }
    CHECK(WSAStartup(0x0002,&winsock)==0);
    CHECK(winsock.wVersion==0x0002 && winsock.wHighVersion==0x0202);
    CHECK(winsock.iMaxSockets==64 && winsock.iMaxUdpDg==1264 && winsock.lpVendorInfo==NULL);
    CHECK(winsock.szDescription[0]!=0 && winsock.szSystemStatus[0]!=0);
    CHECK(WSAStartup(0x0003,&winsock)==WSAVERNOTSUPPORTED);
    CHECK(WSAStartup(0x0002,NULL)==WSAEFAULT);
    CHECK(WSACleanup()==0);
    CHECK(XNetRandom(NULL,0)==0);
    CHECK(XNetRandom(NULL,1)==WSAEFAULT);
    CHECK(XNetRandom(random_bytes,sizeof(random_bytes))==0);
    for(i=0;i<sizeof(random_bytes);++i)nonzero|=random_bytes[i];
    CHECK(nonzero!=0);
    CHECK(XNetCreateKey(&identifier,&key)==0);
    CHECK(bytes_differ((const unsigned char *)&identifier,(const unsigned char *)&zero_identifier,sizeof(identifier)));
    CHECK(bytes_differ((const unsigned char *)&key,(const unsigned char *)&zero_key,sizeof(key)));
    CHECK(XNetCreateKey(NULL,&key)==WSAEFAULT);
    CHECK(XNetRegisterKey(&identifier,&key)==0);
    CHECK(XNetRegisterKey(NULL,&key)==WSAEFAULT);
    CHECK(XNetUnregisterKey(&identifier)==0);
    CHECK(XNetUnregisterKey(NULL)==WSAEFAULT);
    address.ina.s_addr=0;
    address.ina.s_addr=1;
    CHECK(XNetXnAddrToInAddr(&address,NULL,&translated)==0);
    CHECK(translated.s_addr==address.ina.s_addr);
    address.ina.s_addr=0;
    CHECK(XNetXnAddrToInAddr(&address,NULL,&translated)==WSAEHOSTUNREACH);
    CHECK(XNetXnAddrToInAddr(NULL,NULL,&translated)==WSAEFAULT);
    printf("native XNet system-link compatibility: %u checks %u failures\n",checks,failures);
    return failures?1:0;
}
