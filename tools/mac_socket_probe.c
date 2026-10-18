/* Darwin test helpers are declared under a private BSD namespace so this
   probe can include and exercise the clean XDK Winsock declarations too. */
#define accept mac_bsd_accept
#define bind mac_bsd_bind
#define connect mac_bsd_connect
#define getpeername mac_bsd_getpeername
#define getsockname mac_bsd_getsockname
#define getsockopt mac_bsd_getsockopt
#define htonl mac_bsd_htonl
#define htons mac_bsd_htons
#define inet_addr mac_bsd_inet_addr
#define ioctl mac_bsd_ioctl
#define listen mac_bsd_listen
#define ntohl mac_bsd_ntohl
#define ntohs mac_bsd_ntohs
#define recv mac_bsd_recv
#define recvfrom mac_bsd_recvfrom
#define select mac_bsd_select
#define send mac_bsd_send
#define sendto mac_bsd_sendto
#define setsockopt mac_bsd_setsockopt
#define shutdown mac_bsd_shutdown
#define socket mac_bsd_socket
#define sockaddr mac_bsd_sockaddr
#define sockaddr_in mac_bsd_sockaddr_in
#define sockaddr_storage mac_bsd_sockaddr_storage
#define in_addr mac_bsd_in_addr
#define fd_set mac_bsd_fd_set
#define timeval mac_bsd_timeval
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#undef accept
#undef bind
#undef connect
#undef getpeername
#undef getsockname
#undef getsockopt
#undef htonl
#undef htons
#undef inet_addr
#undef ioctl
#undef listen
#undef ntohl
#undef ntohs
#undef recv
#undef recvfrom
#undef select
#undef send
#undef sendto
#undef setsockopt
#undef shutdown
#undef socket
#undef sockaddr
#undef sockaddr_in
#undef sockaddr_storage
#undef in_addr
#undef fd_set
#undef timeval
#undef AF_INET
#undef SOCK_STREAM
#undef SOCK_DGRAM
#undef IPPROTO_IP
#undef SOL_SOCKET
#undef SO_REUSEADDR
#undef SO_BROADCAST
#undef SO_SNDBUF
#undef SO_RCVBUF
#undef INADDR_ANY
#undef INADDR_BROADCAST
#undef INADDR_NONE
#undef FD_SETSIZE
#include "engine_sockets.h"
#include "net_settings.h"
#include "net_fault.h"
#include "mac_xnet_address.h"
#include <stdarg.h>

/* What net_settings.c reads: port_config.c's in the game, these here. They
   are set before the first socket call, because the settings are read once. */
static const char *probe_network_address = "";
static const char *probe_network_broadcast = "";
static double probe_fault_latency, probe_fault_loss, probe_fault_corrupt;

double config_real(const char *name)
{
    if(!strcmp(name,"debug.network_latency"))return probe_fault_latency;
    if(!strcmp(name,"debug.network_loss"))return probe_fault_loss;
    if(!strcmp(name,"debug.network_corrupt"))return probe_fault_corrupt;
    return 0.0;
}
static unsigned long probe_alias;

const char *config_string(const char *name)
{
    if(!strcmp(name,"network.address"))return probe_network_address;
    if(!strcmp(name,"network.broadcast"))return probe_network_broadcast;
    return "";
}

void platform_log(const char *format,...)
{
    va_list arguments;
    fputs("net settings: ",stderr);
    va_start(arguments,format);
    vfprintf(stderr,format,arguments);
    va_end(arguments);
    fputc('\n',stderr);
}

static int checks,failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; fprintf(stderr,"FAIL line %d: %s (WSA=%d errno=%d)\n",__LINE__,#x,WSAGetLastError(),errno); } } while (0)

static void *thread_error(void *unused)
{
    int i;(void)unused;
    if(WSAGetLastError()!=0)return (void*)1;
    for(i=0;i<1000;++i){WSASetLastError(WSAEACCES);if(WSAGetLastError()!=WSAEACCES)return (void*)1;}
    return NULL;
}

static void init_address(struct sockaddr_in *address,unsigned short port)
{
    memset(address,0,sizeof(*address));address->sin_family=AF_INET;address->sin_port=port;
    address->sin_addr.S_un.S_un_b.s_b1=127;address->sin_addr.S_un.S_un_b.s_b2=0;
    address->sin_addr.S_un.S_un_b.s_b3=0;address->sin_addr.S_un.S_un_b.s_b4=1;
}

static int test_tcp(void)
{
    SOCKET server,client,accepted;struct sockaddr_in local,peer;int length,one=1,buffer_size,readable;
    char incoming[16],outgoing[]="loopback";fd_set reads;struct timeval wait;
    server=socket(AF_INET,SOCK_STREAM,IPPROTO_IP);CHECK(server!=INVALID_SOCKET);if(server==INVALID_SOCKET)return 0;
    CHECK(setsockopt(server,SOL_SOCKET,SO_REUSEADDR,(const char*)&one,sizeof(one))==0);
    init_address(&local,0);CHECK(bind(server,(struct sockaddr*)&local,sizeof(local))==0);
    length=sizeof(local);CHECK(getsockname(server,(struct sockaddr*)&local,&length)==0);
    CHECK(length==sizeof(local)&&local.sin_family==AF_INET&&local.sin_port!=0);
    CHECK(local.sin_addr.S_un.S_un_b.s_b1==127&&local.sin_addr.S_un.S_un_b.s_b2==0&&
        local.sin_addr.S_un.S_un_b.s_b3==0&&local.sin_addr.S_un.S_un_b.s_b4==1);
    CHECK(listen(server,4)==0);
    CHECK(ioctlsocket(server,FIONBIO,(unsigned long*)&one)==0);
    CHECK(accept(server,NULL,NULL)==INVALID_SOCKET&&WSAGetLastError()==WSAEWOULDBLOCK);
    one=0;CHECK(ioctlsocket(server,FIONBIO,(unsigned long*)&one)==0);
    client=socket(AF_INET,SOCK_STREAM,IPPROTO_IP);CHECK(client!=INVALID_SOCKET);if(client==INVALID_SOCKET){closesocket(server);return 0;}
    CHECK(connect(client,(struct sockaddr*)&local,sizeof(local))==0);
    length=sizeof(peer);CHECK(getpeername(client,(struct sockaddr*)&peer,&length)==0);
    CHECK(length==sizeof(peer)&&peer.sin_family==AF_INET&&peer.sin_port==local.sin_port);
    accepted=accept(server,(struct sockaddr*)&peer,&length);CHECK(accepted!=INVALID_SOCKET);
    if(accepted==INVALID_SOCKET){closesocket(client);closesocket(server);return 0;}
    CHECK(length==sizeof(peer)&&peer.sin_family==AF_INET);
    CHECK(peer.sin_addr.S_un.S_un_b.s_b1==127&&peer.sin_addr.S_un.S_un_b.s_b4==1);
    buffer_size=32768;CHECK(setsockopt(client,SOL_SOCKET,SO_SNDBUF,(const char*)&buffer_size,sizeof(buffer_size))==0);
    length=sizeof(buffer_size);buffer_size=0;
    CHECK(getsockopt(client,SOL_SOCKET,SO_SNDBUF,(char*)&buffer_size,&length)==0);
    CHECK(length==sizeof(buffer_size)&&buffer_size>=4096);
    CHECK(send(client,outgoing,sizeof(outgoing)-1,0)==(int)sizeof(outgoing)-1);
    FD_ZERO(&reads);FD_SET(accepted,&reads);wait.tv_sec=0;wait.tv_usec=200000;
    CHECK(select(1,&reads,NULL,NULL,&wait)==1&&FD_ISSET(accepted,&reads));
    readable=0;CHECK(ioctlsocket(accepted,FIONREAD,(unsigned long*)&readable)==0&&readable==(int)sizeof(outgoing)-1);
    memset(incoming,0,sizeof(incoming));CHECK(recv(accepted,incoming,sizeof(incoming),MSG_PEEK)==(int)sizeof(outgoing)-1);
    CHECK(memcmp(incoming,outgoing,sizeof(outgoing)-1)==0);
    memset(incoming,0,sizeof(incoming));CHECK(recv(accepted,incoming,sizeof(incoming),0)==(int)sizeof(outgoing)-1);
    CHECK(memcmp(incoming,outgoing,sizeof(outgoing)-1)==0);
    one=1;CHECK(ioctlsocket(accepted,FIONBIO,(unsigned long*)&one)==0);
    CHECK(recv(accepted,incoming,sizeof(incoming),0)==SOCKET_ERROR&&WSAGetLastError()==WSAEWOULDBLOCK);
    CHECK(closesocket(accepted)==0);CHECK(closesocket(client)==0);CHECK(closesocket(server)==0);
    return 1;
}

static int test_udp(void)
{
    SOCKET receiver,sender;struct sockaddr_in local,destination,source;int length,one=1;char payload[]="datagram",buffer[32],oversized[48];
    fd_set reads;struct timeval wait;
    receiver=socket(AF_INET,SOCK_DGRAM,IPPROTO_IP);CHECK(receiver!=INVALID_SOCKET);if(receiver==INVALID_SOCKET)return 0;
    sender=socket(AF_INET,SOCK_DGRAM,IPPROTO_IP);CHECK(sender!=INVALID_SOCKET);if(sender==INVALID_SOCKET){closesocket(receiver);return 0;}
    CHECK(setsockopt(receiver,SOL_SOCKET,SO_BROADCAST,(const char*)&one,sizeof(one))==0);
    init_address(&local,0);CHECK(bind(receiver,(struct sockaddr*)&local,sizeof(local))==0);
    length=sizeof(destination);CHECK(getsockname(receiver,(struct sockaddr*)&destination,&length)==0);
    CHECK(length==sizeof(destination)&&destination.sin_family==AF_INET&&destination.sin_port!=0);
    CHECK(destination.sin_addr.S_un.S_un_b.s_b1==127&&destination.sin_addr.S_un.S_un_b.s_b4==1);
    CHECK(sendto(sender,payload,sizeof(payload)-1,0,(struct sockaddr*)&destination,sizeof(destination))==(int)sizeof(payload)-1);
    FD_ZERO(&reads);FD_SET(receiver,&reads);wait.tv_sec=1;wait.tv_usec=0;
    CHECK(select(0,&reads,NULL,NULL,&wait)==1&&FD_ISSET(receiver,&reads));
    memset(&source,0,sizeof(source));length=sizeof(source);memset(buffer,0,sizeof(buffer));
    CHECK(recvfrom(receiver,buffer,sizeof(buffer),0,(struct sockaddr*)&source,&length)==(int)sizeof(payload)-1);
    CHECK(memcmp(buffer,payload,sizeof(payload)-1)==0);
    CHECK(length==sizeof(source)&&source.sin_family==AF_INET&&source.sin_port!=0);
    CHECK(source.sin_addr.S_un.S_un_b.s_b1==127&&source.sin_addr.S_un.S_un_b.s_b4==1);
    memset(oversized,'x',sizeof(oversized));
    CHECK(sendto(sender,oversized,sizeof(oversized),0,(struct sockaddr*)&destination,sizeof(destination))==(int)sizeof(oversized));
    length=sizeof(source);
    CHECK(recvfrom(receiver,buffer,8,0,(struct sockaddr*)&source,&length)==SOCKET_ERROR&&WSAGetLastError()==WSAEMSGSIZE);
    one=1;CHECK(ioctlsocket(receiver,FIONBIO,(unsigned long*)&one)==0);
    CHECK(recv(receiver,buffer,sizeof(buffer),0)==SOCKET_ERROR&&WSAGetLastError()==WSAEWOULDBLOCK);
    CHECK(closesocket(sender)==0);CHECK(closesocket(receiver)==0);return 1;
}

static int test_select_array_abi(void)
{
    SOCKET low;int high;fd_set reads;struct timeval wait;
    low=socket(AF_INET,SOCK_DGRAM,IPPROTO_IP);CHECK(low!=INVALID_SOCKET);if(low==INVALID_SOCKET)return 0;
    high=fcntl((int)low,F_DUPFD,100);CHECK(high>=100);
    if(high<0){closesocket(low);return 0;}
    close((int)low);FD_ZERO(&reads);FD_SET((SOCKET)high,&reads);
    wait.tv_sec=0;wait.tv_usec=0;
    CHECK(select(0,&reads,NULL,NULL,&wait)==0&&reads.fd_count==0);
    CHECK(closesocket((SOCKET)high)==0);
    FD_ZERO(&reads);reads.fd_count=FD_SETSIZE+1;
    CHECK(select(0,&reads,NULL,NULL,&wait)==SOCKET_ERROR&&WSAGetLastError()==WSAEINVAL);
    FD_ZERO(&reads);FD_SET(INVALID_SOCKET,&reads);
    CHECK(select(0,&reads,NULL,NULL,&wait)==SOCKET_ERROR&&WSAGetLastError()==WSAENOTSOCK);
    return 1;
}

static int test_select_over_sixty_four(void)
{
    SOCKET sockets[76];fd_set reads;struct timeval wait;unsigned int i;int result;
    if(FD_SETSIZE<76){CHECK(0);return 0;}
    for(i=0;i<76;++i){
        sockets[i]=socket(AF_INET,SOCK_DGRAM,IPPROTO_IP);
        CHECK(sockets[i]!=INVALID_SOCKET);
        if(sockets[i]==INVALID_SOCKET){while(i)closesocket(sockets[--i]);return 0;}
    }
    FD_ZERO(&reads);for(i=0;i<76;++i)FD_SET(sockets[i],&reads);
    CHECK(reads.fd_count==76);
    wait.tv_sec=0;wait.tv_usec=0;result=select(0,&reads,NULL,NULL,&wait);
    CHECK(result==0&&reads.fd_count==0);
    for(i=0;i<76;++i)CHECK(closesocket(sockets[i])==0);
    return 1;
}

/* Winsock says a non-blocking connect that has not finished with
   WSAEWOULDBLOCK, and connect_endpoint
   (source/bungie_net/network/transport_endpoint_winsock.c) waits on exactly
   that and treats anything else as refused. Darwin says EINPROGRESS. */
static int test_pending_connect(void)
{
    SOCKET server,client;struct sockaddr_in local;int length,one=1,result;
    unsigned long nonblocking=1;
    server=socket(AF_INET,SOCK_STREAM,IPPROTO_IP);CHECK(server!=INVALID_SOCKET);
    if(server==INVALID_SOCKET)return 0;
    CHECK(setsockopt(server,SOL_SOCKET,SO_REUSEADDR,(const char*)&one,sizeof(one))==0);
    init_address(&local,0);CHECK(bind(server,(struct sockaddr*)&local,sizeof(local))==0);
    length=sizeof(local);CHECK(getsockname(server,(struct sockaddr*)&local,&length)==0);
    CHECK(listen(server,1)==0);
    client=socket(AF_INET,SOCK_STREAM,IPPROTO_IP);CHECK(client!=INVALID_SOCKET);
    CHECK(ioctlsocket(client,FIONBIO,&nonblocking)==0);
    result=connect(client,(struct sockaddr*)&local,sizeof(local));
    /* either it finished at once or it is pending, and pending is
       WSAEWOULDBLOCK and never WSAEINPROGRESS */
    CHECK(result==0||(result==SOCKET_ERROR&&WSAGetLastError()==WSAEWOULDBLOCK));
    CHECK(closesocket(client)==0);CHECK(closesocket(server)==0);
    return 1;
}

/* The game's connections send every tick's messages at once, so they must
   not wait for Nagle to fill a segment: port/linux/src/xnet.c sets
   TCP_NODELAY on a stream socket it makes and on one it accepts. This is
   also the only check that an option level other than SOL_SOCKET reaches
   the host, which the adapter passes through as posix_net.c does. */
static int test_no_delay(void)
{
    SOCKET server,client,accepted;struct sockaddr_in local,peer;
    int length,one=1,value=0,size=sizeof(value);
    server=socket(AF_INET,SOCK_STREAM,IPPROTO_IP);CHECK(server!=INVALID_SOCKET);
    if(server==INVALID_SOCKET)return 0;
    CHECK(getsockopt(server,IPPROTO_TCP,TCP_NODELAY,(char*)&value,&size)==0&&value!=0);
    CHECK(setsockopt(server,SOL_SOCKET,SO_REUSEADDR,(const char*)&one,sizeof(one))==0);
    init_address(&local,0);CHECK(bind(server,(struct sockaddr*)&local,sizeof(local))==0);
    length=sizeof(local);CHECK(getsockname(server,(struct sockaddr*)&local,&length)==0);
    CHECK(listen(server,1)==0);
    client=socket(AF_INET,SOCK_STREAM,IPPROTO_IP);CHECK(client!=INVALID_SOCKET);
    CHECK(connect(client,(struct sockaddr*)&local,sizeof(local))==0);
    length=sizeof(peer);accepted=accept(server,(struct sockaddr*)&peer,&length);
    CHECK(accepted!=INVALID_SOCKET);
    value=0;size=sizeof(value);
    CHECK(getsockopt(accepted,IPPROTO_TCP,TCP_NODELAY,(char*)&value,&size)==0&&value!=0);
    CHECK(closesocket(accepted)==0);CHECK(closesocket(client)==0);CHECK(closesocket(server)==0);
    return 1;
}

/* The game does not clear sin_zero: bind_endpoint in
   source/bungie_net/network/transport_endpoint_winsock.c fills in the
   family, the address and the port and leaves the eight padding bytes as
   the stack had them. Winsock ignores them; Darwin refuses a sockaddr_in
   whose sin_zero is not zero, with EADDRNOTAVAIL. */
static int test_dirty_sockaddr_padding(void)
{
    SOCKET bound;struct sockaddr_in address;int length;
    bound=socket(AF_INET,SOCK_DGRAM,IPPROTO_IP);CHECK(bound!=INVALID_SOCKET);
    if(bound==INVALID_SOCKET)return 0;
    init_address(&address,0);
    memset(address.sin_zero,0xAB,sizeof(address.sin_zero));
    CHECK(bind(bound,(struct sockaddr*)&address,sizeof(address))==0);
    length=sizeof(address);
    CHECK(getsockname(bound,(struct sockaddr*)&address,&length)==0);
    /* and the caller reads back the zeros Winsock would give it */
    CHECK(address.sin_zero[0]==0&&address.sin_zero[7]==0);
    CHECK(closesocket(bound)==0);
    return 1;
}

/* Network order is big-endian whatever the host is. Darwin's own htonl and
   friends are macros, so there is no function to compare against: the test is
   the bytes themselves, which is what the order means. */
static unsigned long bytes_as_long(const unsigned char *bytes)
{
    return ((unsigned long)bytes[0]<<24)|((unsigned long)bytes[1]<<16)|
        ((unsigned long)bytes[2]<<8)|(unsigned long)bytes[3];
}

static int test_byte_order(void)
{
    static const unsigned long longs[]={0,1,0x01020304UL,0x7F000001UL,0xFFFFFFFFUL,0x0000FF00UL};
    static const unsigned short shorts[]={0,1,0x1234,0x2734,0xFF00,0xFFFF};
    unsigned i;unsigned char bytes[4];unsigned long value;unsigned short half;
    for(i=0;i<sizeof(longs)/sizeof(longs[0]);++i)
    {
        value=htonl(longs[i]);memcpy(bytes,&value,4);
        CHECK(bytes[0]==(unsigned char)(longs[i]>>24)&&bytes[1]==(unsigned char)(longs[i]>>16)&&
            bytes[2]==(unsigned char)(longs[i]>>8)&&bytes[3]==(unsigned char)longs[i]);
        CHECK(ntohl(value)==longs[i]);
        CHECK(ntohl(htonl(longs[i]))==longs[i]);
    }
    for(i=0;i<sizeof(shorts)/sizeof(shorts[0]);++i)
    {
        half=htons(shorts[i]);memcpy(bytes,&half,2);
        CHECK(bytes[0]==(unsigned char)(shorts[i]>>8)&&bytes[1]==(unsigned char)shorts[i]);
        CHECK(ntohs(half)==shorts[i]);
        CHECK(ntohs(htons(shorts[i]))==shorts[i]);
    }
    /* what a sockaddr carries: the loopback address and a port */
    value=htonl(0x7F000001UL);memcpy(bytes,&value,4);
    CHECK(bytes[0]==127&&bytes[1]==0&&bytes[2]==0&&bytes[3]==1);
    half=htons(2302);memcpy(bytes,&half,2);
    CHECK(bytes[0]==(2302>>8)&&bytes[1]==(2302&0xFF));
    CHECK(bytes_as_long((const unsigned char *)"\x7F\x00\x00\x01")==0x7F000001UL);
    return 1;
}

/* The dotted quad and nothing else, in network order. */
static int test_inet_addr(void)
{
    static const char *const good[]={"0.0.0.0","127.0.0.1","1.2.3.4","10.0.0.255"};
    static const unsigned long expected[]={0,0x7F000001UL,0x01020304UL,0x0A0000FFUL};
    static const char *const bad[]={"","127.1","1.2.3","1.2.3.4.5","1.2.3.256"," 1.2.3.4","1.2.3.4x","1.2.3.",".1.2.3","1.2..4"};
    unsigned i;unsigned char bytes[4];unsigned long value;
    for(i=0;i<sizeof(good)/sizeof(good[0]);++i)
    {
        value=inet_addr(good[i]);memcpy(bytes,&value,4);
        CHECK(bytes_as_long(bytes)==expected[i]);
        CHECK(ntohl(value)==expected[i]);
    }
    for(i=0;i<sizeof(bad)/sizeof(bad[0]);++i)
        CHECK(inet_addr(bad[i])==INADDR_NONE);
    CHECK(inet_addr(NULL)==INADDR_NONE);
    /* the broadcast address reads as the failure value, as it does on Windows */
    CHECK(inet_addr("255.255.255.255")==INADDR_NONE);
    return 1;
}

/* Darwin refuses a shutdown once the connection has ended, so each of the
   three ways is asked of a connection that is still up. */
static int test_shutdown(void)
{
    SOCKET server,client,accepted,second,accepted_second;
    struct sockaddr_in local,peer;int length,one=1;char incoming[4];
    server=socket(AF_INET,SOCK_STREAM,IPPROTO_IP);CHECK(server!=INVALID_SOCKET);if(server==INVALID_SOCKET)return 0;
    CHECK(setsockopt(server,SOL_SOCKET,SO_REUSEADDR,(const char*)&one,sizeof(one))==0);
    init_address(&local,0);CHECK(bind(server,(struct sockaddr*)&local,sizeof(local))==0);
    length=sizeof(local);CHECK(getsockname(server,(struct sockaddr*)&local,&length)==0);
    CHECK(listen(server,2)==0);
    client=socket(AF_INET,SOCK_STREAM,IPPROTO_IP);CHECK(client!=INVALID_SOCKET);
    CHECK(connect(client,(struct sockaddr*)&local,sizeof(local))==0);
    length=sizeof(peer);accepted=accept(server,(struct sockaddr*)&peer,&length);
    CHECK(accepted!=INVALID_SOCKET);
    CHECK(shutdown(accepted,0)==0);
    /* the peer reads the end of the stream after the sending side is shut */
    CHECK(shutdown(client,1)==0);
    CHECK(recv(accepted,incoming,sizeof(incoming),0)==0);
    second=socket(AF_INET,SOCK_STREAM,IPPROTO_IP);CHECK(second!=INVALID_SOCKET);
    CHECK(connect(second,(struct sockaddr*)&local,sizeof(local))==0);
    length=sizeof(peer);accepted_second=accept(server,(struct sockaddr*)&peer,&length);
    CHECK(accepted_second!=INVALID_SOCKET);
    CHECK(shutdown(accepted_second,2)==0);
    CHECK(shutdown(client,3)==SOCKET_ERROR&&WSAGetLastError()==WSAEINVAL);
    CHECK(shutdown(INVALID_SOCKET,1)==SOCKET_ERROR&&WSAGetLastError()==WSAENOTSOCK);
    CHECK(closesocket(accepted_second)==0);CHECK(closesocket(second)==0);
    CHECK(closesocket(accepted)==0);CHECK(closesocket(client)==0);CHECK(closesocket(server)==0);
    return 1;
}

/* network.address and network.broadcast (port/linux/src/net_settings.c):
   what lets several copies of the game share one machine. The settings are
   read once, so this runs as its own program (--net-settings), and it takes
   the machine's own address, because Darwin refuses to bind 127.0.0.2 and
   the rest of 127/8 unless lo0 has been given that alias. */
static int parse_text(const char *text,unsigned long *address)
{
    return net_settings_parse_ipv4(text,text+strlen(text),address);
}

static void set_address(struct sockaddr_in *address,unsigned long ip,unsigned short port)
{
    memset(address,0,sizeof(*address));
    address->sin_family=AF_INET;
    address->sin_port=port;
    address->sin_addr.s_addr=ip;
}

/* -1 if nothing arrives: a check that fails must not leave the probe in a
   recvfrom that never returns */
static int receive_with_timeout(SOCKET ready_socket,char *buffer,int length,
    struct sockaddr_in *from)
{
    fd_set reads;struct timeval wait;int address_length=(int)sizeof(*from);
    FD_ZERO(&reads);FD_SET(ready_socket,&reads);
    wait.tv_sec=2;wait.tv_usec=0;
    if(select(0,&reads,NULL,NULL,&wait)!=1)return -1;
    return recvfrom(ready_socket,buffer,length,0,(struct sockaddr*)from,&address_length);
}

/* A stream connection from an unbound socket leaves from network.address,
   not from whichever address the route picks (to 127.0.0.2 that is
   127.0.0.1, which the host would read as itself). Needs 127.0.0.2 on lo0,
   and is not run without it. */
static int test_connect_source(unsigned long alias)
{
    SOCKET listener,client,accepted;struct sockaddr_in where,peer;int length;
    unsigned long other=inet_addr("127.0.0.2");
    listener=socket(AF_INET,SOCK_STREAM,IPPROTO_IP);CHECK(listener!=INVALID_SOCKET);
    if(listener==INVALID_SOCKET)return 0;
    set_address(&where,other,0);
    if(bind(listener,(struct sockaddr*)&where,sizeof(where))!=0)
    {
        printf("native system link addressing: 127.0.0.2 is not on lo0; connect source not run\n");
        closesocket(listener);return 1;
    }
    CHECK(listen(listener,1)==0);
    length=sizeof(where);CHECK(getsockname(listener,(struct sockaddr*)&where,&length)==0);
    client=socket(AF_INET,SOCK_STREAM,IPPROTO_IP);CHECK(client!=INVALID_SOCKET);
    CHECK(connect(client,(struct sockaddr*)&where,sizeof(where))==0);
    length=sizeof(peer);CHECK(getsockname(client,(struct sockaddr*)&peer,&length)==0);
    CHECK(peer.sin_addr.s_addr==alias);
    length=sizeof(peer);accepted=accept(listener,(struct sockaddr*)&peer,&length);CHECK(accepted!=INVALID_SOCKET);
    /* the host sees the machine the connection came from, not itself */
    CHECK(peer.sin_addr.s_addr==alias);
    CHECK(closesocket(accepted)==0);CHECK(closesocket(client)==0);CHECK(closesocket(listener)==0);
    return 1;
}

/* debug.network_latency and its companions (port/linux/src/net_fault.c):
   what this machine receives is held back, lost or damaged. The settings
   are read once, so each of the three is its own run of this program
   (--net-fault <ms> <loss%> <corrupt%>). */
static int test_net_fault(double latency_ms,double loss,double corrupt)
{
    SOCKET sender,receiver;struct sockaddr_in any,bound,target,from;
    int length,index,got,differed=0,arrived=0;
    char incoming[NET_FAULT_PACKET_SIZE+64];
    static const char message[]="a datagram of a known length and content";
    receiver=socket(AF_INET,SOCK_DGRAM,IPPROTO_IP);CHECK(receiver!=INVALID_SOCKET);
    if(receiver==INVALID_SOCKET)return 0;
    set_address(&any,inet_addr("127.0.0.1"),0);
    CHECK(bind(receiver,(struct sockaddr*)&any,sizeof(any))==0);
    length=sizeof(bound);
    CHECK(getsockname(receiver,(struct sockaddr*)&bound,&length)==0);
    sender=socket(AF_INET,SOCK_DGRAM,IPPROTO_IP);CHECK(sender!=INVALID_SOCKET);
    set_address(&any,inet_addr("127.0.0.1"),0);
    CHECK(bind(sender,(struct sockaddr*)&any,sizeof(any))==0);
    set_address(&target,inet_addr("127.0.0.1"),bound.sin_port);
    for(index=0;index<20;++index)
        CHECK(sendto(sender,message,(int)sizeof(message),0,
            (struct sockaddr*)&target,sizeof(target))==(int)sizeof(message));
    if(latency_ms>0.0)
    {
        /* held back: there is nothing to read yet, and select agrees */
        length=sizeof(from);
        CHECK(recvfrom(receiver,incoming,sizeof(incoming),0,
            (struct sockaddr*)&from,&length)==SOCKET_ERROR&&
            WSAGetLastError()==WSAEWOULDBLOCK);
        usleep((unsigned)(latency_ms*1000.0)+300000u);
    }
    /* and now, once it is due, select says the socket is readable */
    if(loss<100.0)CHECK(receive_with_timeout(receiver,incoming,sizeof(incoming),&from)>=0);
    for(index=0;index<20;++index)
    {
        length=sizeof(from);
        got=recvfrom(receiver,incoming,sizeof(incoming),0,(struct sockaddr*)&from,&length);
        if(got<0)continue;
        ++arrived;
        if(got!=(int)sizeof(message)||memcmp(incoming,message,(size_t)got))differed=1;
    }
    if(loss>=100.0)CHECK(arrived==0);
    else CHECK(arrived>0);
    if(corrupt>=100.0)CHECK(differed);
    /* held back only: every one of them, unharmed */
    if(latency_ms>0.0&&loss<=0.0&&corrupt<=0.0)CHECK(arrived==19&&!differed);
    CHECK(closesocket(sender)==0);CHECK(closesocket(receiver)==0);
    return 1;
}

static int test_net_settings(unsigned long alias)
{
    SOCKET sender,receiver;struct sockaddr_in any,bound,target,from;
    int length;char incoming[8];static const char message[]="alias";
    unsigned long parsed,loopback;
    loopback=inet_addr("127.0.0.1");
    CHECK(parse_text("1.2.3.4",&parsed)&&parsed==inet_addr("1.2.3.4"));
    CHECK(parse_text(" 10.0.0.1 ",&parsed)&&parsed==inet_addr("10.0.0.1"));
    CHECK(parse_text("0.0.0.0",&parsed)&&parsed==0);
    CHECK(!parse_text("1.2.3",&parsed));
    CHECK(!parse_text("1.2.3.256",&parsed));
    CHECK(!parse_text("1.2.3.4.5",&parsed));
    CHECK(!parse_text("",&parsed));
    CHECK(net_settings_loopback_address()==loopback);
    /* INADDR_ANY becomes the network.address address */
    receiver=socket(AF_INET,SOCK_DGRAM,IPPROTO_IP);CHECK(receiver!=INVALID_SOCKET);
    if(receiver==INVALID_SOCKET)return 0;
    set_address(&any,INADDR_ANY,0);
    CHECK(bind(receiver,(struct sockaddr*)&any,sizeof(any))==0);
    length=sizeof(bound);
    CHECK(getsockname(receiver,(struct sockaddr*)&bound,&length)==0);
    CHECK(bound.sin_addr.s_addr==alias&&bound.sin_port!=0);
    sender=socket(AF_INET,SOCK_DGRAM,IPPROTO_IP);CHECK(sender!=INVALID_SOCKET);
    set_address(&any,INADDR_ANY,0);
    CHECK(bind(sender,(struct sockaddr*)&any,sizeof(any))==0);
    /* a datagram addressed to 127.0.0.1 reaches that address */
    set_address(&target,loopback,bound.sin_port);
    CHECK(sendto(sender,message,(int)sizeof(message),0,
        (struct sockaddr*)&target,sizeof(target))==(int)sizeof(message));
    CHECK(receive_with_timeout(receiver,incoming,sizeof(incoming),&from)==(int)sizeof(message));
    CHECK(!memcmp(incoming,message,sizeof(message)));
    /* and reads back as having come from 127.0.0.1, which is what the
       netcode above the adapter knows the other copy by */
    CHECK(from.sin_addr.s_addr==loopback);
    /* a broadcast goes to each address network.broadcast names */
    set_address(&target,INADDR_BROADCAST,bound.sin_port);
    CHECK(sendto(sender,message,(int)sizeof(message),0,
        (struct sockaddr*)&target,sizeof(target))==(int)sizeof(message));
    CHECK(receive_with_timeout(receiver,incoming,sizeof(incoming),&from)==(int)sizeof(message));
    CHECK(from.sin_addr.s_addr==loopback);
    CHECK(closesocket(sender)==0);CHECK(closesocket(receiver)==0);
    CHECK(test_connect_source(alias));
    return 1;
}

int main(int argc,char **argv)
{
    pthread_t thread;void*thread_result;
    if(argc==5&&!strcmp(argv[1],"--net-fault"))
    {
        probe_fault_latency=atof(argv[2]);
        probe_fault_loss=atof(argv[3]);
        probe_fault_corrupt=atof(argv[4]);
        CHECK(test_net_fault(probe_fault_latency,probe_fault_loss,probe_fault_corrupt));
        printf("native network fault %s ms %s%% lost %s%% damaged: checks=%d failures=%d\n",
            argv[2],argv[3],argv[4],checks,failures);
        return failures?1:0;
    }
    if(argc==2&&!strcmp(argv[1],"--net-settings"))
    {
        static char address_text[16];
        const unsigned char *octets=(const unsigned char *)&probe_alias;
        unsigned char hardware[6];
        if(!mac_xnet_local_address(&probe_alias,hardware))
        {
            printf("native system link addressing: no non-loopback interface; not run\n");
            return 0;
        }
        sprintf(address_text,"%u.%u.%u.%u",octets[0],octets[1],octets[2],octets[3]);
        probe_network_address=address_text;
        probe_network_broadcast=address_text;
        fprintf(stderr,"net settings: this machine is %s\n",address_text);
        CHECK(test_net_settings(probe_alias));
        printf("native system link addressing checks=%d failures=%d\n",checks,failures);
        return failures?1:0;
    }
    (void)argc;(void)argv;
    CHECK(sizeof(struct sockaddr_in)==16);
    CHECK(FD_SETSIZE==HALO_PORT_FD_SETSIZE);
    CHECK(sizeof(struct fd_set)==sizeof(unsigned int)+HALO_PORT_FD_SETSIZE*sizeof(unsigned int));
    CHECK(sizeof(struct timeval)==8);
    CHECK(WSAGetLastError()==0);WSASetLastError(WSAEINVAL);
    CHECK(pthread_create(&thread,NULL,thread_error,NULL)==0);
    CHECK(pthread_join(thread,&thread_result)==0&&thread_result==NULL);
    CHECK(WSAGetLastError()==WSAEINVAL);
    CHECK(test_tcp());CHECK(test_udp());CHECK(test_select_array_abi());CHECK(test_select_over_sixty_four());
    CHECK(test_byte_order());CHECK(test_inet_addr());CHECK(test_shutdown());
    CHECK(test_no_delay());CHECK(test_dirty_sockaddr_padding());CHECK(test_pending_connect());
    CHECK(socket(AF_UNIX,SOCK_STREAM,0)==INVALID_SOCKET&&WSAGetLastError()==WSAEAFNOSUPPORT);
    CHECK(socket(AF_INET,0x7fffffff,0)==INVALID_SOCKET&&WSAGetLastError()==WSAESOCKTNOSUPPORT);
    {
        SOCKET option_socket=socket(AF_INET,SOCK_DGRAM,IPPROTO_IP);int value,length=sizeof(value);
        CHECK(option_socket!=INVALID_SOCKET);
        CHECK(getsockopt(option_socket,SOL_SOCKET,0x7fff,(char*)&value,&length)==SOCKET_ERROR&&WSAGetLastError()==WSAENOPROTOOPT);
        /* but setting one neither side knows is let be rather than refused,
           as posix_socket_setsockopt has it: the Xbox's own options
           (SO_ENCRYPT and the rest) have nothing to do here, and a game that
           sets one must not see it fail */
        CHECK(setsockopt(option_socket,SOL_SOCKET,0x7fff,(const char*)&value,sizeof(value))==0);
        CHECK(closesocket(option_socket)==0);
    }
    CHECK(halo_ws_send(INVALID_SOCKET,"x",1,0)==SOCKET_ERROR&&WSAGetLastError()==WSAENOTSOCK);
    CHECK(bind(INVALID_SOCKET,NULL,0)==SOCKET_ERROR&&WSAGetLastError()==WSAENOTSOCK);
    CHECK(closesocket(INVALID_SOCKET)==SOCKET_ERROR&&WSAGetLastError()==WSAENOTSOCK);
    printf("native Winsock loopback checks=%d failures=%d\n",checks,failures);
    return failures?1:0;
}
