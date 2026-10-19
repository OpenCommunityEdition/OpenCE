/* Darwin BSD socket adapter for the clean XDK Winsock ABI. Native headers are
   privately renamed before the XDK declarations are included; no Darwin
   sockaddr, fd_set, timeval or C-call-convention socket prototype crosses
   this boundary. */
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
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "halo_endian.h"

enum {
    MAC_AF_INET = AF_INET,
    MAC_SOCK_STREAM = SOCK_STREAM,
    MAC_SOCK_DGRAM = SOCK_DGRAM,
    MAC_IPPROTO_IP = IPPROTO_IP,
    MAC_IPPROTO_TCP = IPPROTO_TCP,
    MAC_TCP_NODELAY = TCP_NODELAY,
    MAC_SOL_SOCKET = SOL_SOCKET,
    MAC_SO_REUSEADDR = SO_REUSEADDR,
    MAC_SO_BROADCAST = SO_BROADCAST,
    MAC_SO_SNDBUF = SO_SNDBUF,
    MAC_SO_RCVBUF = SO_RCVBUF,
    MAC_SO_TYPE = SO_TYPE,
    MAC_MSG_OOB = MSG_OOB,
    MAC_MSG_PEEK = MSG_PEEK,
    MAC_MSG_DONTROUTE = MSG_DONTROUTE,
    MAC_MSG_WAITALL = MSG_WAITALL,
    MAC_MSG_TRUNC = MSG_TRUNC,
    MAC_FIONREAD = FIONREAD,
    MAC_SHUT_RD = SHUT_RD,
    MAC_SHUT_WR = SHUT_WR,
    MAC_SHUT_RDWR = SHUT_RDWR,
    MAC_BSD_FD_SETSIZE = FD_SETSIZE
};

static void mac_bsd_fd_zero(mac_bsd_fd_set *set) { FD_ZERO(set); }
static void mac_bsd_fd_add(int fd, mac_bsd_fd_set *set) { FD_SET(fd, set); }
static int mac_bsd_fd_has(int fd, const mac_bsd_fd_set *set) { return FD_ISSET(fd, (mac_bsd_fd_set *)set); }

#undef htonl
#undef htons
#undef inet_addr
#undef ntohl
#undef ntohs
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
#undef INADDR_ANY
#undef INADDR_BROADCAST
#undef INADDR_NONE
#undef SOL_SOCKET
#undef SO_REUSEADDR
#undef SO_BROADCAST
#undef SO_SNDBUF
#undef SO_RCVBUF
#undef MSG_OOB
#undef MSG_PEEK
#undef MSG_DONTROUTE
#undef MSG_WAITALL
#undef FIONREAD
#undef FIONBIO
#undef FD_SETSIZE

#include "engine_sockets.h"
#include "net_settings.h"
#include "net_fault.h"
#include "net_peer.h"
#undef accept
#undef bind
#undef closesocket
#undef connect
#undef getpeername
#undef getsockname
#undef getsockopt
#undef htonl
#undef htons
#undef inet_addr
#undef ioctlsocket
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

typedef char mac_xdk_sockaddr_size[(sizeof(struct sockaddr)==16)?1:-1];
typedef char mac_xdk_sockaddr_in_size[(sizeof(struct sockaddr_in)==16)?1:-1];
typedef char mac_xdk_fdset_abi[(FD_SETSIZE==HALO_PORT_FD_SETSIZE &&
    sizeof(struct fd_set)==sizeof(unsigned int)+HALO_PORT_FD_SETSIZE*sizeof(unsigned int))?1:-1];
typedef char mac_xdk_timeval_abi[(sizeof(struct timeval)==8)?1:-1];
typedef char mac_xdk_ulong_abi[(sizeof(unsigned long)==4)?1:-1];
typedef char mac_bsd_timeval_abi[(sizeof(struct mac_bsd_timeval)==8&&
    offsetof(struct mac_bsd_timeval,tv_sec)==0&&
    offsetof(struct mac_bsd_timeval,tv_usec)==4)?1:-1];
typedef char mac_xdk_ipv4_offsets[(offsetof(struct sockaddr_in,sin_family)==0&&
    offsetof(struct sockaddr_in,sin_port)==2&&offsetof(struct sockaddr_in,sin_addr)==4&&
    offsetof(struct sockaddr_in,sin_zero)==8)?1:-1];
typedef char mac_bsd_ipv4_abi[(sizeof(struct mac_bsd_sockaddr_in)==16&&
    offsetof(struct mac_bsd_sockaddr_in,sin_len)==0&&
    offsetof(struct mac_bsd_sockaddr_in,sin_family)==1&&
    offsetof(struct mac_bsd_sockaddr_in,sin_port)==2&&
    offsetof(struct mac_bsd_sockaddr_in,sin_addr)==4&&
    offsetof(struct mac_bsd_sockaddr_in,sin_zero)==8)?1:-1];

static pthread_once_t mac_wsa_once = PTHREAD_ONCE_INIT;
static pthread_key_t mac_wsa_key;

static void mac_wsa_key_create(void)
{
    if (pthread_key_create(&mac_wsa_key, NULL)) abort();
}

void WSAAPI WSASetLastError(int error)
{
    if (pthread_once(&mac_wsa_once, mac_wsa_key_create) ||
        pthread_setspecific(mac_wsa_key, (void *)(intptr_t)error)) abort();
}

int WSAAPI WSAGetLastError(void)
{
    if (pthread_once(&mac_wsa_once, mac_wsa_key_create)) abort();
    return (int)(intptr_t)pthread_getspecific(mac_wsa_key);
}

static int mac_socket_error(int error)
{
    switch (error)
    {
        case EINTR: return WSAEINTR;
        case EBADF: case ENOTSOCK: return WSAENOTSOCK;
        case EACCES: case EPERM: return WSAEACCES;
        case EFAULT: return WSAEFAULT;
        case EINVAL: return WSAEINVAL;
        case EDESTADDRREQ: return WSAEDESTADDRREQ;
        case EMFILE: case ENFILE: return WSAEMFILE;
        case EWOULDBLOCK: return WSAEWOULDBLOCK;
        case EINPROGRESS: return WSAEINPROGRESS;
        case EALREADY: return WSAEALREADY;
        case EMSGSIZE: return WSAEMSGSIZE;
        case EPROTOTYPE: return WSAEPROTOTYPE;
        case ENOPROTOOPT: return WSAENOPROTOOPT;
        case EPROTONOSUPPORT: return WSAEPROTONOSUPPORT;
        case EOPNOTSUPP: return WSAEOPNOTSUPP;
        case EAFNOSUPPORT: return WSAEAFNOSUPPORT;
        case EADDRINUSE: return WSAEADDRINUSE;
        case EADDRNOTAVAIL: return WSAEADDRNOTAVAIL;
        case ENETDOWN: return WSAENETDOWN;
        case ENETUNREACH: return WSAENETUNREACH;
        case ENETRESET: return WSAENETRESET;
        case ECONNABORTED: return WSAECONNABORTED;
        case ECONNRESET: case EPIPE: return WSAECONNRESET;
        case ENOBUFS: case ENOMEM: return WSAENOBUFS;
        case EISCONN: return WSAEISCONN;
        case ENOTCONN: return WSAENOTCONN;
        case ESHUTDOWN: return WSAESHUTDOWN;
        case ETIMEDOUT: return WSAETIMEDOUT;
        case ECONNREFUSED: return WSAECONNREFUSED;
        case EHOSTDOWN: return WSAEHOSTDOWN;
        case EHOSTUNREACH: return WSAEHOSTUNREACH;
        default: return WSAEINVAL;
    }
}

static int mac_socket_failure(int error)
{
    WSASetLastError(mac_socket_error(error));
    return SOCKET_ERROR;
}

static unsigned int mac_socket_handle(int fd)
{
    return fd < 0 ? INVALID_SOCKET : (unsigned int)fd;
}

static int mac_socket_fd(SOCKET socket, int *fd)
{
    if (socket > (SOCKET)INT_MAX) return 0;
    *fd = (int)socket;
    return 1;
}

static int mac_native_flags(int flags, int *native_flags)
{
    int result = 0;
    if (flags & ~(1 | 2 | 4 | 8)) return 0;
    if (flags & 1) result |= MAC_MSG_OOB;
    if (flags & 2) result |= MAC_MSG_PEEK;
    if (flags & 4) result |= MAC_MSG_DONTROUTE;
    if (flags & 8) result |= MAC_MSG_WAITALL;
    *native_flags = result;
    return 1;
}

static int mac_native_address(const struct sockaddr *address, int length,
    struct mac_bsd_sockaddr_storage *storage, struct mac_bsd_sockaddr **native,
    int *native_length)
{
    if (!address || length < (int)sizeof(struct sockaddr)) return WSAEFAULT;
    if (address->sa_family != AF_INET) return WSAEAFNOSUPPORT;
    if (length < (int)sizeof(struct sockaddr_in)) return WSAEFAULT;
    memset(storage, 0, sizeof(*storage));
    {
        const struct sockaddr_in *source = (const struct sockaddr_in *)address;
        struct mac_bsd_sockaddr_in *target = (struct mac_bsd_sockaddr_in *)storage;
        target->sin_len = sizeof(*target);
        target->sin_family = MAC_AF_INET;
        memcpy(&target->sin_port, &source->sin_port, sizeof(target->sin_port));
        memcpy(&target->sin_addr, &source->sin_addr, sizeof(target->sin_addr));
        /* sin_zero is left as the zeros storage was cleared to, and not taken
           from the caller. Winsock ignores those eight bytes, so the game
           leaves them as they were on the stack (bind_endpoint in
           source/bungie_net/network/transport_endpoint_winsock.c fills in
           only the family, the address and the port), and Darwin refuses a
           sockaddr_in whose sin_zero is not zero with EADDRNOTAVAIL. */
    }
    *native = (struct mac_bsd_sockaddr *)storage;
    *native_length = sizeof(struct mac_bsd_sockaddr_in);
    return 0;
}

static int mac_xdk_address(const struct mac_bsd_sockaddr *native,
    int native_length, struct sockaddr *address, int *length)
{
    int capacity;
    if (!address || !length || *length < 0) return WSAEFAULT;
    capacity = *length;
    if (capacity < 16 || native_length < 2) return WSAEFAULT;
    if (native->sa_family != MAC_AF_INET || native_length < (int)sizeof(struct mac_bsd_sockaddr_in))
        return WSAEAFNOSUPPORT;
    {
        const struct mac_bsd_sockaddr_in *source = (const struct mac_bsd_sockaddr_in *)native;
        struct sockaddr_in *result = (struct sockaddr_in *)address;
        result->sin_family = AF_INET;
        memcpy(&result->sin_port, &source->sin_port, sizeof(result->sin_port));
        memcpy(&result->sin_addr, &source->sin_addr, sizeof(result->sin_addr));
        /* and the eight bytes the caller reads are zeros, as Winsock's are */
        memset(result->sin_zero, 0, sizeof(result->sin_zero));
    }
    *length = 16;
    return 0;
}

/* port: the game's connections send every tick's messages at once, so they
   must not wait for Nagle to fill a segment. port/linux/src/xnet.c sets this
   on a stream socket it makes and on one it accepts; this did neither. */
static void mac_socket_no_delay(int fd, int type)
{
    int enabled = 1;
    if (type == MAC_SOCK_STREAM)
        setsockopt(fd, MAC_IPPROTO_TCP, MAC_TCP_NODELAY, &enabled, sizeof(enabled));
}

static int mac_socket_no_sigpipe(int fd)
{
    int enabled = 1;
    return setsockopt(fd, MAC_SOL_SOCKET, SO_NOSIGPIPE, &enabled, sizeof(enabled));
}

/* ---------- system link addressing (port/linux/src/net_settings.c)

   network.address lets several copies of the game share one machine: each
   binds its own address in place of INADDR_ANY, a destination of 127.0.0.1
   means that address, and traffic from it reads back as 127.0.0.1, so the
   netcode above it sees one ordinary loopback machine. network.broadcast
   names the addresses a broadcast goes to one by one, for the networks that
   carry no broadcast. port/linux/src/xnet.c does the same for the builds
   that compile it. */

/* INADDR_ANY becomes the network.address address */
static const struct sockaddr *mac_bind_address(const struct sockaddr *address,
    int address_length, struct sockaddr_in *storage)
{
    unsigned long local;
    if(address && address->sa_family==AF_INET && address_length>=(int)sizeof(*storage) &&
        ((const struct sockaddr_in *)address)->sin_addr.s_addr==INADDR_ANY &&
        net_settings_local_address(&local))
    {
        memcpy(storage,address,sizeof(*storage));
        storage->sin_addr.s_addr=local;
        return (const struct sockaddr *)storage;
    }
    return address;
}

/* a destination of 127.0.0.1 means the network.address address */
static const struct sockaddr *mac_outgoing_address(const struct sockaddr *address,
    int address_length, struct sockaddr_in *storage)
{
    unsigned long local;
    if(address && address->sa_family==AF_INET && address_length>=(int)sizeof(*storage) &&
        ((const struct sockaddr_in *)address)->sin_addr.s_addr==net_settings_loopback_address() &&
        net_settings_local_address(&local))
    {
        memcpy(storage,address,sizeof(*storage));
        storage->sin_addr.s_addr=local;
        return (const struct sockaddr *)storage;
    }
    return address;
}

/* port: traffic from an internet play peer's stand-in is really from the
   peer, and internet play says which peer and which port, as
   peer_incoming_address does in port/linux/src/xnet.c. Nonzero when the
   address has been rewritten to the peer's. */
static int mac_peer_incoming_address(int stream, struct sockaddr *address, const int *address_length)
{
    if(address && address_length && *address_length>=(int)sizeof(struct sockaddr_in) &&
        address->sa_family==AF_INET)
    {
        unsigned long ip=((struct sockaddr_in *)address)->sin_addr.s_addr;
        unsigned short port=((struct sockaddr_in *)address)->sin_port;

        if(net_peer_incoming(stream,&ip,&port))
        {
            ((struct sockaddr_in *)address)->sin_addr.s_addr=ip;
            ((struct sockaddr_in *)address)->sin_port=port;
            return 1;
        }
    }
    return 0;
}

/* port: a destination that is an internet play peer's goes to its stand-in
   (1); -1, with WSAEHOSTUNREACH, if it is a peer's that cannot be reached
   now; 0 if it is not a peer's. connecting: the socket (UDP) is connected to
   it, else -1. As peer_outgoing_address in port/linux/src/xnet.c. */
static int mac_peer_outgoing_address(int stream, int connecting, const struct sockaddr **address,
    int address_length, struct sockaddr_in *storage)
{
    unsigned long ip;
    unsigned short port;
    int result;

    if(!*address || (*address)->sa_family!=AF_INET || address_length<(int)sizeof(*storage))return 0;
    ip=((const struct sockaddr_in *)*address)->sin_addr.s_addr;
    port=((const struct sockaddr_in *)*address)->sin_port;
    result=net_peer_outgoing(stream,connecting,&ip,&port);
    if(result<0)WSASetLastError(WSAEHOSTUNREACH);
    if(result<=0)return result;
    memcpy(storage,*address,sizeof(*storage));
    storage->sin_addr.s_addr=ip;
    storage->sin_port=port;
    *address=(const struct sockaddr *)storage;
    return 1;
}

/* traffic from the network.address address comes from 127.0.0.1 */
static void mac_incoming_address(struct sockaddr *address, const int *address_length)
{
    unsigned long local;
    /* (both callers are datagram paths; only what is no peer's gets the
       system link translation below) */
    if(mac_peer_incoming_address(0,address,address_length))return;
    if(address && address_length && *address_length>=(int)sizeof(struct sockaddr_in) &&
        address->sa_family==AF_INET && net_settings_local_address(&local) &&
        ((struct sockaddr_in *)address)->sin_addr.s_addr==local)
    {
        ((struct sockaddr_in *)address)->sin_addr.s_addr=net_settings_loopback_address();
    }
}


/* HALO_MAC_SOCKET_TRACE=1 (2: every one): each socket's binds, connects and the first
   datagrams or stream reads and writes, and every failure other than
   would-block, on stderr (how a network game that goes quiet is diagnosed) */
static int mac_trace_enabled(void)
{
    static int state=-1;
    if(state<0){const char *text=getenv("HALO_MAC_SOCKET_TRACE");state=(text&&*text&&*text!='0')?(*text=='2'?2:1):0;}
    return state;
}

/* (appended with no stdio: the platform layer's prefix header redirects
   printf and its kin to the game's own, which a probe does not link) */
static char *mac_trace_text(char *at,const char *end,const char *text)
{
    while(*text&&at<end)*at++=*text++;
    return at;
}

static char *mac_trace_number(char *at,const char *end,long value)
{
    char digits[16];int count=0;
    unsigned long magnitude;
    if(value<0){if(at<end)*at++='-';magnitude=(unsigned long)(-value);}else magnitude=(unsigned long)value;
    do{digits[count++]=(char)('0'+magnitude%10);magnitude/=10;}while(magnitude&&count<(int)sizeof(digits));
    while(count&&at<end)*at++=digits[--count];
    return at;
}

static void mac_trace(int fd,const char *what,const struct mac_bsd_sockaddr *address,long result,int error)
{
    static unsigned long counts[256][2];
    int slot=(result<0)?1:0;
    unsigned long n=0;
    char line[160];char *at=line;const char *end=line+sizeof(line)-2;
    if(!mac_trace_enabled())return;
    if(result<0&&(error==EAGAIN||error==EWOULDBLOCK))return;
    if(fd>=0&&fd<256)
    {
        n=++counts[fd][slot];
        if(n>30&&(n%500)&&mac_trace_enabled()<2)return;
    }
    at=mac_trace_text(at,end,"sock ");
    at=mac_trace_number(at,end,fd);
    at=mac_trace_text(at,end," ");
    at=mac_trace_text(at,end,what);
    if(address&&address->sa_family==AF_INET)
    {
        const unsigned char *a=(const unsigned char *)&((const struct mac_bsd_sockaddr_in *)address)->sin_addr;
        const unsigned char *p=(const unsigned char *)&((const struct mac_bsd_sockaddr_in *)address)->sin_port;
        int octet;
        at=mac_trace_text(at,end," ");
        for(octet=0;octet<4;++octet){if(octet)at=mac_trace_text(at,end,".");at=mac_trace_number(at,end,a[octet]);}
        at=mac_trace_text(at,end,":");
        at=mac_trace_number(at,end,p[0]*256+p[1]);
    }
    at=mac_trace_text(at,end," -> ");
    at=mac_trace_number(at,end,result);
    if(result<0)at=mac_trace_text(at,end," (failed)");
    at=mac_trace_text(at,end," (#");
    at=mac_trace_number(at,end,(long)n);
    at=mac_trace_text(at,end,")");
    *at++='\n';
    (void)write(2,line,(size_t)(at-line));
}

unsigned int WSAAPI halo_ws_socket(int family, int type, int protocol)
{
    int result;
    if (family != AF_INET) { WSASetLastError(WSAEAFNOSUPPORT); return INVALID_SOCKET; }
    if (type != SOCK_STREAM && type != SOCK_DGRAM) { WSASetLastError(WSAESOCKTNOSUPPORT); return INVALID_SOCKET; }
    result = socket(family, type, protocol);
    if (result < 0) { mac_socket_failure(errno); return INVALID_SOCKET; }
    mac_socket_no_delay(result, type);
    return mac_socket_handle(result);
}

/* (defined below, with mac_name_query which they read the socket's name
   through) */
static void mac_note_socket_port(int fd, int listening);
static unsigned short mac_socket_local_port(int fd);
/* the descriptors internet play has been told a port of (a send from a socket
   not bound yet gives it one, which a peer is reached through) */
static unsigned char mac_port_noted[256];

int WSAAPI halo_ws_bind(SOCKET socket, const struct sockaddr *address, int address_length)
{
    int fd, error, native_length; struct mac_bsd_sockaddr_storage storage; struct mac_bsd_sockaddr *native;
    struct sockaddr_in rewritten;
    if (!mac_socket_fd(socket, &fd)) return mac_socket_failure(ENOTSOCK);
    address = mac_bind_address(address, address_length, &rewritten);
    error = mac_native_address(address, address_length, &storage, &native, &native_length);
    if (error) { WSASetLastError(error); return SOCKET_ERROR; }
    {
        int result = bind(fd, native, native_length), saved = errno;
        mac_trace(fd, "bind", native, result, saved);
        errno = saved;
        if (result < 0)
            return mac_socket_failure(errno);
        /* port: the port the game just took is internet play's to know */
        mac_note_socket_port(fd, 0);
        return 0;
    }
}

int WSAAPI halo_ws_connect(SOCKET socket, const struct sockaddr *address, int address_length)
{
    int fd, error, native_length; struct mac_bsd_sockaddr_storage storage; struct mac_bsd_sockaddr *native;
    struct sockaddr_in rewritten;
    if (!mac_socket_fd(socket, &fd)) return mac_socket_failure(ENOTSOCK);
    {
        /* port: an internet play peer's TCP or UDP port goes to its stand-in
           (xnet.c's connect does the same) */
        int type = MAC_SOCK_STREAM;
        socklen_t type_length = sizeof(type);

        if (address && address->sa_family == AF_INET && address_length >= (int)sizeof(struct sockaddr_in) &&
            net_peer_is_peer(((const struct sockaddr_in *)address)->sin_addr.s_addr))
            getsockopt(fd, MAC_SOL_SOCKET, MAC_SO_TYPE, &type, &type_length);
        switch (mac_peer_outgoing_address(type == MAC_SOCK_STREAM, fd, &address, address_length, &rewritten))
        {
        case -1: return SOCKET_ERROR;
        case 0: address = mac_outgoing_address(address, address_length, &rewritten); break;
        }
    }
    error = mac_native_address(address, address_length, &storage, &native, &native_length);
    if (error) { WSASetLastError(error); return SOCKET_ERROR; }
    /* A connection from an unbound socket leaves from whichever address the
       route picks: to 127.0.0.2 from 127.0.0.1, which the host reads as
       itself (127.0.0.1 is this machine, mac_incoming_address), so two copies
       on one Mac were taken for one machine and the host sent the second's
       datagrams to its own client. With network.address it leaves from that
       address (port/linux/src/xnet.c does the same). */
    {
        unsigned long local;
        if (net_settings_local_address(&local))
        {
            struct mac_bsd_sockaddr_in bound;
            socklen_t bound_length = sizeof(bound);
            unsigned long bound_address = 0;
            if (getsockname(fd, (struct mac_bsd_sockaddr *)&bound, &bound_length) < 0)
                bound.sin_port = 0;
            else
                memcpy(&bound_address, &bound.sin_addr, sizeof(bound_address));
            if (bound.sin_port == 0 && bound_address == 0)
            {
                memset(&bound, 0, sizeof(bound));
                bound.sin_len = sizeof(bound);
                bound.sin_family = AF_INET;
                memcpy(&bound.sin_addr, &local, sizeof(local));
                (void)bind(fd, (struct mac_bsd_sockaddr *)&bound, sizeof(bound));
            }
        }
    }
    {
        int result = connect(fd, native, native_length), saved = errno;
        mac_trace(fd, "connect", native, result, saved);
        /* port: the port the system gave it (a connection under way has one
           too), which internet play is told as xnet.c's connect tells it */
        mac_note_socket_port(fd, 0);
        errno = saved;
        if (result >= 0) return 0;
    }
    {
        /* Winsock says a non-blocking connect that has not finished yet with
           WSAEWOULDBLOCK, and that is the one connect_endpoint waits on
           (source/bungie_net/network/transport_endpoint_winsock.c); every
           other error there is the connection refused. Darwin says
           EINPROGRESS, and Winsock's WSAEINPROGRESS means something else
           entirely: that a blocking call is already running on the socket.
           Mapping it as the other calls do made the host's own client fail
           to reach its own server. */
        if (errno == EINPROGRESS) { WSASetLastError(WSAEWOULDBLOCK); return SOCKET_ERROR; }
        return mac_socket_failure(errno);
    }
    return 0;
}

int WSAAPI halo_ws_listen(SOCKET socket, int backlog)
{
    int fd;
    if (!mac_socket_fd(socket, &fd)) return mac_socket_failure(ENOTSOCK);
    if (backlog < 0) return mac_socket_failure(EINVAL);
    if (listen(fd, backlog) < 0) return mac_socket_failure(errno);
    /* port: and it listens for connections while it hosts (internet play
       learns of a hosted game from this) */
    mac_note_socket_port(fd, 1);
    return 0;
}

unsigned int WSAAPI halo_ws_accept(SOCKET socket, struct sockaddr *address, int *address_length)
{
    int fd, result, error; socklen_t native_length; struct mac_bsd_sockaddr_storage storage;
    if (!mac_socket_fd(socket, &fd)) { mac_socket_failure(ENOTSOCK); return INVALID_SOCKET; }
    if ((address && !address_length) || (!address && address_length)) { mac_socket_failure(EFAULT); return INVALID_SOCKET; }
    if (address && *address_length < 16) { mac_socket_failure(EFAULT); return INVALID_SOCKET; }
    native_length = sizeof(storage);
    result = accept(fd, address ? (struct mac_bsd_sockaddr *)&storage : NULL,
        address ? &native_length : NULL);
    if (result < 0) { mac_socket_failure(errno); return INVALID_SOCKET; }
    if (address)
    {
        error = mac_xdk_address((struct mac_bsd_sockaddr *)&storage,(int)native_length,address,address_length);
        if (error) { close(result); WSASetLastError(error); return INVALID_SOCKET; }
        /* port: a connection off the tunnel is from the peer */
        mac_peer_incoming_address(1,address,address_length);
    }
    mac_socket_no_delay(result, MAC_SOCK_STREAM);
    return mac_socket_handle(result);
}

static int mac_name_query(int fd, int peer, struct sockaddr *address, int *address_length)
{
    int result, error; socklen_t native_length; struct mac_bsd_sockaddr_storage storage;
    if (!address || !address_length || *address_length < 16) return mac_socket_failure(EFAULT);
    native_length = sizeof(storage);
    result = peer ? getpeername(fd,(struct mac_bsd_sockaddr *)&storage,&native_length) :
        getsockname(fd,(struct mac_bsd_sockaddr *)&storage,&native_length);
    if (result < 0) return mac_socket_failure(errno);
    /* port: the far end the kernel reports for a connected socket is where
       the netcode's record of a machine comes from, and a machine recorded
       at the wrong address is sent its game somewhere else, so this belongs
       in the trace as much as the sends do. */
    mac_trace(fd, peer ? "getpeername" : "getsockname",
        (struct mac_bsd_sockaddr *)&storage, result, 0);
    error = mac_xdk_address((struct mac_bsd_sockaddr *)&storage,(int)native_length,address,address_length);
    if (error) { WSASetLastError(error); return SOCKET_ERROR; }
    return 0;
}

int WSAAPI halo_ws_getpeername(SOCKET socket, struct sockaddr *address, int *address_length)
{
    int fd;if(!mac_socket_fd(socket,&fd))return mac_socket_failure(ENOTSOCK);
    return mac_name_query(fd,1,address,address_length);
}

int WSAAPI halo_ws_getsockname(SOCKET socket, struct sockaddr *address, int *address_length)
{
    int fd;if(!mac_socket_fd(socket,&fd))return mac_socket_failure(ENOTSOCK);
    return mac_name_query(fd,0,address,address_length);
}

/* port: tells internet play the local port this socket has, as
   note_socket_port does in port/linux/src/xnet.c. A peer is reached through
   the port the game's socket is bound to, so a socket whose port internet
   play is not told of is one no peer can be reached through.

   The type is read with the native names rather than through
   halo_ws_getsockopt: SO_TYPE is not one of the options mac_option_name
   translates, and these are Darwin's own values, captured in the enum at
   the top of this file before the Winsock names shadow them. */
static void mac_note_socket_port(int fd, int listening)
{
    struct sockaddr_in bound;
    int length=(int)sizeof(bound);
    int type=MAC_SOCK_DGRAM;
    socklen_t type_length=sizeof(type);

    if(mac_name_query(fd,0,(struct sockaddr *)&bound,&length)!=0)return;
    if(bound.sin_family!=AF_INET || !bound.sin_port)return;
    if(fd>=0 && fd<256)mac_port_noted[fd]=1;
    getsockopt(fd,MAC_SOL_SOCKET,MAC_SO_TYPE,&type,&type_length);
    net_peer_socket_bound(fd,type==MAC_SOCK_STREAM,listening,
        bound.sin_addr.s_addr,bound.sin_port);
}

/* port: the port this socket is bound to, in network byte order, or 0 when
   it is not bound yet. Internet play sends from it. */
static unsigned short mac_socket_local_port(int fd)
{
    struct sockaddr_in bound;
    int length=(int)sizeof(bound);

    if(mac_name_query(fd,0,(struct sockaddr *)&bound,&length)!=0)return 0;
    if(bound.sin_family!=AF_INET)return 0;
    return bound.sin_port;
}

static int mac_option_name(int level, int option, int *native_level, int *native_option)
{
    if(level!=SOL_SOCKET)
    {
        /* (IPPROTO_IP and IPPROTO_TCP option numbers are the same on both,
           as translate_option in port/linux/src/posix_net.c has it) */
        *native_level=level;*native_option=option;return 1;
    }
    *native_level=MAC_SOL_SOCKET;
    switch(option)
    {
        case SO_REUSEADDR:*native_option=MAC_SO_REUSEADDR;return 1;
        case SO_BROADCAST:*native_option=MAC_SO_BROADCAST;return 1;
        case SO_SNDBUF:*native_option=MAC_SO_SNDBUF;return 1;
        case SO_RCVBUF:*native_option=MAC_SO_RCVBUF;return 1;
        default:return 0;
    }
}

int WSAAPI halo_ws_getsockopt(SOCKET socket, int level, int option, char *value, int *value_length)
{
    int fd,native_level,native_option; socklen_t native_length;
    if(!mac_socket_fd(socket,&fd))return mac_socket_failure(ENOTSOCK);
    if(!value||!value_length||*value_length<0)return mac_socket_failure(EFAULT);
    if(!mac_option_name(level,option,&native_level,&native_option))return mac_socket_failure(ENOPROTOOPT);
    native_length=(socklen_t)*value_length;
    if(getsockopt(fd,native_level,native_option,value,&native_length)<0)
        return mac_socket_failure(errno);
    if(native_length>(socklen_t)INT_MAX)return mac_socket_failure(EFAULT);
    *value_length=(int)native_length;return 0;
}

int WSAAPI halo_ws_setsockopt(SOCKET socket, int level, int option, const char *value, int value_length)
{
    int fd,native_level,native_option;
    if(!mac_socket_fd(socket,&fd))return mac_socket_failure(ENOTSOCK);
    if(value_length<0||(!value&&value_length))return mac_socket_failure(value_length<0?EINVAL:EFAULT);
    /* an option neither side knows is let be, not refused: the Xbox's own
       (SO_ENCRYPT and the rest) have nothing to do here, and
       posix_socket_setsockopt says so too */
    if(!mac_option_name(level,option,&native_level,&native_option)){WSASetLastError(0);return 0;}
    return setsockopt(fd,native_level,native_option,value,(socklen_t)value_length)<0 ?
        mac_socket_failure(errno) : 0;
}

/* port: milliseconds, wrapping as the GetTickCount xnet.c uses does, which
   is all net_fault.c compares */
static unsigned long mac_socket_now(void)
{
    struct mac_bsd_timeval at;
    if(gettimeofday(&at,NULL)!=0)return 0;
    return (unsigned long)at.tv_sec*1000UL+(unsigned long)(at.tv_usec/1000);
}

/* port: debug.network_latency and its companions
   (port/linux/src/net_fault.c): what this machine receives is held back,
   lost and damaged, to test the netcode as over the internet and to test
   that nothing another machine sends can crash the game. The reads here are
   this platform's; what becomes of them is shared with xnet.c, which does
   the same around its own. */
static int mac_fault_receive(int fd, char *buffer, int length, int native_flags,
    struct sockaddr *from, int *from_length, int datagram)
{
    char scratch[NET_FAULT_PACKET_SIZE];
    struct mac_bsd_sockaddr_storage storage;
    int taken,truncated=0,stored_length;
    for(;;)
    {
        mac_bsd_fd_set readable;struct mac_bsd_timeval nothing;
        ssize_t result;socklen_t from_native=sizeof(storage);
        mac_bsd_fd_zero(&readable);mac_bsd_fd_add(fd,&readable);
        nothing.tv_sec=0;nothing.tv_usec=0;
        /* (only what is there now: some of the game's sockets block) */
        if(select(fd+1,&readable,NULL,NULL,&nothing)<=0)break;
        if(datagram)
        {
            struct iovec vector;struct msghdr message;
            memset(&message,0,sizeof(message));
            vector.iov_base=scratch;vector.iov_len=sizeof(scratch);
            message.msg_iov=&vector;message.msg_iovlen=1;
            message.msg_name=&storage;message.msg_namelen=from_native;
            result=recvmsg(fd,&message,native_flags);
            from_native=message.msg_namelen;
        }
        else result=recv(fd,scratch,sizeof(scratch),native_flags);
        if(result<0)
        {
            /* (an error but would-block is the caller's now) */
            if(errno!=EWOULDBLOCK&&errno!=EAGAIN)return mac_socket_failure(errno);
            break;
        }
        /* (a stream closing is at once) */
        if(!datagram&&result==0)return 0;
        net_fault_keep(fd,scratch,(int)result,datagram?(const void *)&storage:NULL,
            datagram?(int)from_native:0,datagram,mac_socket_now());
    }
    stored_length=(int)sizeof(storage);
    taken=net_fault_take(fd,buffer,length,datagram?(void *)&storage:NULL,
        datagram?&stored_length:NULL,datagram,mac_socket_now(),&truncated);
    if(taken<0)return mac_socket_failure(EWOULDBLOCK);
    if(datagram&&from&&from_length)
    {
        int error=mac_xdk_address((struct mac_bsd_sockaddr *)&storage,stored_length,from,from_length);
        if(error){WSASetLastError(error);return SOCKET_ERROR;}
        mac_incoming_address(from,from_length);
    }
    if(truncated)return mac_socket_failure(EMSGSIZE);
    return taken;
}

int WSAAPI halo_ws_recv(SOCKET socket, char *buffer, int length, int flags)
{
    int fd,native_flags; ssize_t result;
    if(!mac_socket_fd(socket,&fd))return mac_socket_failure(ENOTSOCK);
    if(length<0)return mac_socket_failure(EINVAL);
    if(length&&!buffer)return mac_socket_failure(EFAULT);
    if(!mac_native_flags(flags,&native_flags))return mac_socket_failure(EOPNOTSUPP);
    if(net_fault_enabled())return mac_fault_receive(fd,buffer,length,native_flags,NULL,NULL,0);
    result=recv(fd,buffer,(size_t)length,native_flags);
    {int saved=errno;mac_trace(fd,"recv",NULL,(long)result,saved);errno=saved;}
    return result<0?mac_socket_failure(errno):(int)result;
}

int WSAAPI halo_ws_send(unsigned int socket, const char *buffer, int length, int flags)
{
    int fd,native_flags; ssize_t result;
    if(!mac_socket_fd(socket,&fd))return mac_socket_failure(ENOTSOCK);
    if(length<0)return mac_socket_failure(EINVAL);
    if(length&&!buffer)return mac_socket_failure(EFAULT);
    if(!mac_native_flags(flags,&native_flags))return mac_socket_failure(EOPNOTSUPP);
    if(mac_socket_no_sigpipe(fd)<0)return mac_socket_failure(errno);
    result=send(fd,buffer,(size_t)length,native_flags);
    {int saved=errno;mac_trace(fd,"send",NULL,(long)result,saved);errno=saved;}
    return result<0?mac_socket_failure(errno):(int)result;
}

int WSAAPI halo_ws_recvfrom(SOCKET socket, char *buffer, int length, int flags,
    struct sockaddr *from, int *from_length)
{
    int fd,native_flags,error; ssize_t result; socklen_t native_length;
    struct iovec vector; struct msghdr message;
    struct mac_bsd_sockaddr_storage storage;
    if(!mac_socket_fd(socket,&fd))return mac_socket_failure(ENOTSOCK);
    if(length<0)return mac_socket_failure(EINVAL);
    if((length&&!buffer)||((from&&!from_length)||(!from&&from_length)))return mac_socket_failure(EFAULT);
    if(from&&*from_length<16)return mac_socket_failure(EFAULT);
    if(!mac_native_flags(flags,&native_flags))return mac_socket_failure(EOPNOTSUPP);
    if(net_fault_enabled())return mac_fault_receive(fd,buffer,length,native_flags,from,from_length,1);
    memset(&message,0,sizeof(message));
    vector.iov_base=buffer;vector.iov_len=(size_t)length;
    message.msg_iov=&vector;message.msg_iovlen=1;
    if(from){message.msg_name=&storage;message.msg_namelen=sizeof(storage);}
    result=recvmsg(fd,&message,native_flags);
    {int saved=errno;mac_trace(fd,"recvfrom",from?(struct mac_bsd_sockaddr*)&storage:NULL,(long)result,saved);errno=saved;}
    if(result<0)return mac_socket_failure(errno);
    if((message.msg_flags&MAC_MSG_TRUNC)||result>length)return mac_socket_failure(EMSGSIZE);
    if(from)
    {
        native_length=message.msg_namelen;
        error=mac_xdk_address((struct mac_bsd_sockaddr*)&storage,(int)native_length,from,from_length);
        if(error){WSASetLastError(error);return SOCKET_ERROR;}
        mac_incoming_address(from,from_length);
    }
    return (int)result;
}

int WSAAPI halo_ws_sendto(SOCKET socket, const char *buffer, int length, int flags,
    const struct sockaddr *to, int to_length)
{
    int fd,native_flags,error; ssize_t result;
    struct mac_bsd_sockaddr_storage storage; struct mac_bsd_sockaddr *native=NULL;
    struct sockaddr_in rewritten;
    if(!mac_socket_fd(socket,&fd))return mac_socket_failure(ENOTSOCK);
    if(length<0||((length&&!buffer))||((!to&&to_length)||(to&&to_length<0)))return mac_socket_failure(EFAULT);
    if(!mac_native_flags(flags,&native_flags))return mac_socket_failure(EOPNOTSUPP);
    if(mac_socket_no_sigpipe(fd)<0)return mac_socket_failure(errno);
    /* port: an internet play peer's stand-in address goes on the tunnel
       rather than on the wire, from this socket's own port, as the sendto
       in port/linux/src/xnet.c does it. A socket with no port yet has
       nothing to send from: the system's send below binds it, and the next
       datagram to that peer goes on the tunnel. */
    if(to && to->sa_family==AF_INET && to_length>=(int)sizeof(struct sockaddr_in) &&
        net_peer_is_peer(((const struct sockaddr_in *)to)->sin_addr.s_addr))
    {
        unsigned short source_port=mac_socket_local_port(fd);

        if(source_port)
        {
            switch(net_peer_send(source_port,((const struct sockaddr_in *)to)->sin_addr.s_addr,
                ((const struct sockaddr_in *)to)->sin_port,buffer,length))
            {
                case 1:return length;
                case -1:WSASetLastError(WSAEHOSTUNREACH);return SOCKET_ERROR;
                default:break;
            }
        }
    }
    /* a broadcast goes to each of network.broadcast's addresses instead, one
       datagram each, and counts as sent if any of them was */
    if(to && to->sa_family==AF_INET && to_length>=(int)sizeof(struct sockaddr_in) &&
        ((const struct sockaddr_in *)to)->sin_addr.s_addr==INADDR_BROADCAST)
    {
        unsigned long targets[NET_SETTINGS_MAXIMUM_BROADCAST_TARGETS];
        int count=net_settings_broadcast_targets(targets,NET_SETTINGS_MAXIMUM_BROADCAST_TARGETS);
        if(count)
        {
            int index,sent,sent_result=0,sent_errno=0,native_length;
            memcpy(&rewritten,to,sizeof(rewritten));
            for(index=0;index<count;++index)
            {
                rewritten.sin_addr.s_addr=targets[index];
                error=mac_native_address((const struct sockaddr *)&rewritten,(int)sizeof(rewritten),
                    &storage,&native,&native_length);
                if(error){WSASetLastError(error);return SOCKET_ERROR;}
                sent=(int)sendto(fd,buffer,(size_t)length,native_flags,native,native_length);
                if(sent>=0||index==0){sent_result=sent;if(sent<0)sent_errno=errno;}
            }
            /* port: and to every internet play peer, on the tunnel. After
               the sends above, which bind the socket if it was not, so that
               it has a port to send from. */
            {
                unsigned short source_port=mac_socket_local_port(fd);

                if(source_port)
                {
                    mac_note_socket_port(fd,0);
                    if(net_peer_broadcast(source_port,rewritten.sin_port,buffer,length)>0 &&
                        sent_result<0)
                    {
                        sent_result=length;sent_errno=0;
                    }
                }
            }
            return sent_result<0?mac_socket_failure(sent_errno):sent_result;
        }
    }
    if(to)
    {
        /* port: a peer's address the tunnel did not take (no port yet) goes to its stand-in */
        switch(mac_peer_outgoing_address(0,-1,&to,to_length,&rewritten))
        {
            case -1:return SOCKET_ERROR;
            case 0:to=mac_outgoing_address(to,to_length,&rewritten);break;
        }
    }
    if(to){int native_length;error=mac_native_address(to,to_length,&storage,&native,&native_length);if(error){WSASetLastError(error);return SOCKET_ERROR;}to_length=native_length;}
    result=sendto(fd,buffer,(size_t)length,native_flags,native,to?to_length:0);
    {int saved=errno;mac_trace(fd,"sendto",native,(long)result,saved);errno=saved;}
    /* port: the port the send gave an unbound socket (one that failed binds it too) */
    if(fd>=0 && fd<256 && !mac_port_noted[fd]){int saved=errno;mac_note_socket_port(fd,0);errno=saved;}
    return result<0?mac_socket_failure(errno):(int)result;
}

static int mac_fdset_to_bsd(const struct fd_set *source, mac_bsd_fd_set *target, int *max_fd)
{
    unsigned int i;int fd;
    mac_bsd_fd_zero(target);
    if(!source)return 0;
    if(source->fd_count>sizeof(source->fd_array)/sizeof(source->fd_array[0]))return WSAEINVAL;
    for(i=0;i<source->fd_count;++i)
    {
        if(!mac_socket_fd(source->fd_array[i],&fd)||fd>=MAC_BSD_FD_SETSIZE)return WSAENOTSOCK;
        mac_bsd_fd_add(fd,target);if(fd>*max_fd)*max_fd=fd;
    }
    return 0;
}

static void mac_fdset_from_bsd(struct fd_set *target,const struct fd_set *source,const mac_bsd_fd_set *ready)
{
    unsigned int i,count=0;int fd;
    if(!target)return;
    for(i=0;i<source->fd_count;++i)
        if(mac_socket_fd(source->fd_array[i],&fd)&&mac_bsd_fd_has(fd,ready))target->fd_array[count++]=source->fd_array[i];
    target->fd_count=count;
}

int WSAAPI halo_ws_select(int ignored, struct fd_set *read_set, struct fd_set *write_set,
    struct fd_set *except_set, const struct timeval *timeout)
{
    mac_bsd_fd_set read_native,write_native,except_native;struct mac_bsd_timeval native_timeout;
    mac_bsd_fd_set *r=NULL,*w=NULL,*e=NULL;int max_fd=-1,result,error;
    struct fd_set read_copy,write_copy,except_copy;
    (void)ignored;
    if(!read_set&&!write_set&&!except_set)return mac_socket_failure(EINVAL);
    if(read_set){read_copy=*read_set;error=mac_fdset_to_bsd(&read_copy,&read_native,&max_fd);if(error){WSASetLastError(error);return SOCKET_ERROR;}r=&read_native;}
    if(write_set){write_copy=*write_set;error=mac_fdset_to_bsd(&write_copy,&write_native,&max_fd);if(error){WSASetLastError(error);return SOCKET_ERROR;}w=&write_native;}
    if(except_set){except_copy=*except_set;error=mac_fdset_to_bsd(&except_copy,&except_native,&max_fd);if(error){WSASetLastError(error);return SOCKET_ERROR;}e=&except_native;}
    if(timeout)
    {
        if(timeout->tv_sec<0||timeout->tv_usec<0||timeout->tv_usec>=1000000)return mac_socket_failure(EINVAL);
        native_timeout.tv_sec=timeout->tv_sec;native_timeout.tv_usec=timeout->tv_usec;
    }
    result=select(max_fd+1,r,w,e,timeout?&native_timeout:NULL);
    if(result<0)return mac_socket_failure(errno);
    if(read_set)
    {
        mac_fdset_from_bsd(read_set,&read_copy,r);
        /* port: what is held back and due is there to read, as xnet.c has it
           after its own select (debug.network_latency) */
        if(net_fault_enabled())
        {
            unsigned int asked,at;int fd;unsigned long now=mac_socket_now();
            for(asked=0;asked<read_copy.fd_count;++asked)
            {
                int present=0;
                for(at=0;at<read_set->fd_count;++at)
                    present|=read_set->fd_array[at]==read_copy.fd_array[asked];
                if(!present&&read_set->fd_count<HALO_PORT_FD_SETSIZE&&
                    mac_socket_fd(read_copy.fd_array[asked],&fd)&&net_fault_due(fd,now))
                {
                    read_set->fd_array[read_set->fd_count++]=read_copy.fd_array[asked];
                    ++result;
                }
            }
        }
    }
    if(write_set)mac_fdset_from_bsd(write_set,&write_copy,w);
    if(except_set)mac_fdset_from_bsd(except_set,&except_copy,e);
    return result;
}

int __WSAFDIsSet(unsigned int socket, struct fd_set *set)
{
    unsigned int i;
    if(!set||set->fd_count>sizeof(set->fd_array)/sizeof(set->fd_array[0]))return 0;
    for(i=0;i<set->fd_count;++i)if(set->fd_array[i]==socket)return 1;
    return 0;
}

int WSAAPI halo_ws_ioctlsocket(SOCKET socket, long command, unsigned long *argument)
{
    int fd,flags;int available;
    if(!mac_socket_fd(socket,&fd))return mac_socket_failure(ENOTSOCK);
    if(!argument)return mac_socket_failure(EFAULT);
    if((unsigned long)command==FIONBIO)
    {
        flags=fcntl(fd,F_GETFL,0);if(flags<0)return mac_socket_failure(errno);
        if(*argument)flags|=O_NONBLOCK;else flags&=~O_NONBLOCK;
        return fcntl(fd,F_SETFL,flags)<0?mac_socket_failure(errno):0;
    }
    if((unsigned long)command==FIONREAD)
    {
        available=0;if(ioctl(fd,MAC_FIONREAD,&available)<0)return mac_socket_failure(errno);
        *argument=(unsigned long)available;return 0;
    }
    return mac_socket_failure(EOPNOTSUPP);
}

int WSAAPI halo_ws_closesocket(unsigned int socket)
{
    int fd;
    if(!mac_socket_fd(socket,&fd))return mac_socket_failure(ENOTSOCK);
    {
        /* port: internet play's bookkeeping, as xnet.c's closesocket does
           it: a datagram socket gives its port back, so that a later socket
           of that number does not inherit the peers reached through it. Read
           before the close and never allowed to fail it - reading it and
           then failing the close on an error left the descriptor open. */
        struct sockaddr_in bound;
        int length=(int)sizeof(bound);
        int type=MAC_SOCK_STREAM;
        socklen_t type_length=sizeof(type);
        int datagram_port=0;

        getsockopt(fd,MAC_SOL_SOCKET,MAC_SO_TYPE,&type,&type_length);
        if(type==MAC_SOCK_DGRAM && mac_name_query(fd,0,(struct sockaddr *)&bound,&length)==0 &&
            bound.sin_family==AF_INET)
            datagram_port=bound.sin_port;
        /* (this gives net_fault the socket as well) */
        net_peer_socket_closing(fd,datagram_port);
        if(fd>=0 && fd<256)mac_port_noted[fd]=0;
    }
    return close(fd)<0?mac_socket_failure(errno):0;
}

/* Network byte order is big-endian, so these are the identity on PowerPC and
   a swap on Intel. port/linux/src/xnet.c swaps whatever the host is, which is
   right only for the little-endian builds that compile it. */

unsigned long WSAAPI halo_ws_htonl(unsigned long value)
{
    return halo_be32(value);
}

unsigned long WSAAPI halo_ws_ntohl(unsigned long value)
{
    return halo_be32(value);
}

unsigned short WSAAPI halo_ws_htons(unsigned short value)
{
    return halo_be16(value);
}

unsigned short WSAAPI halo_ws_ntohs(unsigned short value)
{
    return halo_be16(value);
}

/* The dotted quad and nothing else, as xnet.c reads it, in network order. */
unsigned long WSAAPI halo_ws_inet_addr(const char *text)
{
    unsigned long parts[4];
    int count=0;
    if(!text)return INADDR_NONE;
    while(count<4)
    {
        unsigned long value=0;int digits=0;
        while(*text>='0'&&*text<='9'){value=value*10+(unsigned long)(*text++-'0');digits++;}
        if(!digits||value>255)return INADDR_NONE;
        parts[count++]=value;
        if(*text!='.')break;
        text++;
    }
    if(count!=4||*text)return INADDR_NONE;
    return halo_be32((parts[0]<<24)|(parts[1]<<16)|(parts[2]<<8)|parts[3]);
}

int WSAAPI halo_ws_shutdown(SOCKET socket, int how)
{
    int fd,native;
    if(!mac_socket_fd(socket,&fd))return mac_socket_failure(ENOTSOCK);
    switch(how)
    {
        case 0: native=MAC_SHUT_RD; break;
        case 1: native=MAC_SHUT_WR; break;
        case 2: native=MAC_SHUT_RDWR; break;
        default: return mac_socket_failure(EINVAL);
    }
    return shutdown(fd,native)<0?mac_socket_failure(errno):0;
}
