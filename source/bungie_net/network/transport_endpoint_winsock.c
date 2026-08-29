/*
TRANSPORT_ENDPOINT_WINSOCK.C

symbols in this file:
00071300 0060:
	_code_00071300 (0000)
00071360 0040:
	_code_00071360 (0000)
000713A0 0040:
	_code_000713a0 (0000)
000713E0 0080:
	_create_transport_endpoint (0000)
00071460 0060:
	_get_endpoint_type (0000)
000714C0 0100:
	_read_endpoint (0000)
000715C0 00f0:
	_write_endpoint (0000)
000716B0 00c0:
	_endpoint_readable (0000)
00071770 00a0:
	_endpoint_writeable (0000)
00071810 0040:
	_endpoint_connected (0000)
00071850 0040:
	_endpoint_listening (0000)
00071890 0040:
	_endpoint_blocking (0000)
000718D0 0040:
	_get_endpoint_error (0000)
00071910 0070:
	_endpoint_equivalent (0000)
00071980 0620:
	_winsock_error_to_string (0000)
00071FA0 0130:
	_code_00071fa0 (0000)
000720D0 0170:
	_get_endpoint_address (0000)
00072240 0110:
	_set_endpoint_blocking (0000)
00072350 0140:
	_bind_endpoint (0000)
00072490 01e0:
	_connect_endpoint (0000)
00072670 0080:
	_disconnect_endpoint (0000)
000726F0 0130:
	_code_000726f0 (0000)
00072820 0150:
	_connect_endpoint_async (0000)
00072970 00a0:
	_cancel_connect_process (0000)
00072A10 00b0:
	_listen_endpoint (0000)
00072AC0 00d0:
	_accept_endpoint (0000)
00072B90 0220:
	_read_from_endpoint (0000)
00072DB0 0180:
	_write_to_endpoint (0000)
00072F30 0080:
	_delete_transport_endpoint (0000)
00072FB0 0030:
	_reject_endpoint (0000)
002561C0 0007:
	??_C@_06FJHNOCKE@thread?$AA@ (0000)
002561C8 003f:
	??_C@_0DP@BJNHIBAC@c?3?2halo?2SOURCE?2bungie_net?2networ@ (0000)
00256208 0003:
	??_C@_02GBJNFGNA@ep?$AA@ (0000)
0025620C 001d:
	??_C@_0BN@LCGPPBOK@ep?5?$CG?$CG?5buffer?5?$CG?$CG?5?$CIlength?5?$DO?50?$CJ?$AA@ (0000)
0025622C 0025:
	??_C@_0CF@LILAICMM@ep?5?$CG?$CG?5?$CIep?9?$DOsocket?5?$CB?$DN?5INVALID_SOC@ (0000)
00256254 0016:
	??_C@_0BG@JGDABANF@winsock?5error?5?$CD?$CFd?3?5?$CFs?$AA@ (0000)
0025626C 0010:
	??_C@_0BA@MMFHLOKK@?$DMunknown?5error?$DO?$AA@ (0000)
0025627C 0016:
	??_C@_0BG@MLBCKNNP@WSA_QOS_GENERIC_ERROR?$AA@ (0000)
00256294 001b:
	??_C@_0BL@FIDPCPEO@WSA_QOS_TRAFFIC_CTRL_ERROR?$AA@ (0000)
002562B0 0013:
	??_C@_0BD@BLONOKBN@WSA_QOS_BAD_OBJECT?$AA@ (0000)
002562C4 0012:
	??_C@_0BC@NKDHAC@WSA_QOS_BAD_STYLE?$AA@ (0000)
002562D8 0017:
	??_C@_0BH@OPFHJNHJ@WSA_QOS_POLICY_FAILURE?$AA@ (0000)
002562F0 001a:
	??_C@_0BK@CCDCMKNF@WSA_QOS_ADMISSION_FAILURE?$AA@ (0000)
0025630C 001a:
	??_C@_0BK@IGGBDKMP@WSA_QOS_REQUEST_CONFIRMED?$AA@ (0000)
00256328 0015:
	??_C@_0BF@NCBEDIJO@WSA_QOS_NO_RECEIVERS?$AA@ (0000)
00256340 0013:
	??_C@_0BD@EICDPPFO@WSA_QOS_NO_SENDERS?$AA@ (0000)
00256354 0010:
	??_C@_0BA@FBDFNBLN@WSA_QOS_SENDERS?$AA@ (0000)
00256364 0012:
	??_C@_0BC@HLJBIMGI@WSA_QOS_RECEIVERS?$AA@ (0000)
00256378 000b:
	??_C@_0L@CBLHPKCP@WSANO_DATA?$AA@ (0000)
00256384 000f:
	??_C@_0P@HLNEBJNJ@WSANO_RECOVERY?$AA@ (0000)
00256394 000d:
	??_C@_0N@LHDAEOPK@WSATRY_AGAIN?$AA@ (0000)
002563A4 0012:
	??_C@_0BC@JCJCFNAO@WSAHOST_NOT_FOUND?$AA@ (0000)
002563B8 000c:
	??_C@_0M@NLNGHJCJ@WSAEREFUSED?$AA@ (0000)
002563C4 0010:
	??_C@_0BA@MEKAPNF@WSA_E_CANCELLED?$AA@ (0000)
002563D4 000e:
	??_C@_0O@LPJENGIE@WSA_E_NO_MORE?$AA@ (0000)
002563E4 0012:
	??_C@_0BC@CHACNBCC@WSATYPE_NOT_FOUND?$AA@ (0000)
002563F8 0015:
	??_C@_0BF@IEFJBMNO@WSASERVICE_NOT_FOUND?$AA@ (0000)
00256410 0012:
	??_C@_0BC@JMKCGGLG@WSASYSCALLFAILURE?$AA@ (0000)
00256424 0017:
	??_C@_0BH@MFNMMHCD@WSAEPROVIDERFAILEDINIT?$AA@ (0000)
0025643C 0014:
	??_C@_0BE@BDJFCAFA@WSAEINVALIDPROVIDER?$AA@ (0000)
00256450 0015:
	??_C@_0BF@CPFHOEAL@WSAEINVALIDPROCTABLE?$AA@ (0000)
00256468 000e:
	??_C@_0O@NAJLOKAM@WSAECANCELLED?$AA@ (0000)
00256478 000b:
	??_C@_0L@FEFCEDKC@WSAENOMORE?$AA@ (0000)
00256484 000b:
	??_C@_0L@EEHHIAEC@WSAEDISCON?$AA@ (0000)
00256490 0012:
	??_C@_0BC@CLCHBKPK@WSANOTINITIALISED?$AA@ (0000)
002564A4 0013:
	??_C@_0BD@PKACBPOA@WSAVERNOTSUPPORTED?$AA@ (0000)
002564B8 000f:
	??_C@_0P@MDJAKAKC@WSASYSNOTREADY?$AA@ (0000)
002564C8 000b:
	??_C@_0L@FBDKOKMM@WSAEREMOTE?$AA@ (0000)
002564D4 000a:
	??_C@_09GPMJAMEI@WSAESTALE?$AA@ (0000)
002564E0 000a:
	??_C@_09DHBOOMJK@WSAEDQUOT?$AA@ (0000)
002564EC 000a:
	??_C@_09IOPCJPLP@WSAEUSERS?$AA@ (0000)
002564F8 000c:
	??_C@_0M@CDIHOKBK@WSAEPROCLIM?$AA@ (0000)
00256504 000d:
	??_C@_0N@KFICKHOH@WSAENOTEMPTY?$AA@ (0000)
00256514 0010:
	??_C@_0BA@HKCFHLFG@WSAEHOSTUNREACH?$AA@ (0000)
00256524 000d:
	??_C@_0N@NJBKCOKK@WSAEHOSTDOWN?$AA@ (0000)
00256534 0010:
	??_C@_0BA@PCKIJCGI@WSAENAMETOOLONG?$AA@ (0000)
00256544 0009:
	??_C@_08PGIMLMFD@WSAELOOP?$AA@ (0000)
00256550 0010:
	??_C@_0BA@ECEEDLKJ@WSAECONNREFUSED?$AA@ (0000)
00256560 000d:
	??_C@_0N@BJEDIOFA@WSAETIMEDOUT?$AA@ (0000)
00256570 0010:
	??_C@_0BA@COKPFEMB@WSAETOOMANYREFS?$AA@ (0000)
00256580 000d:
	??_C@_0N@JOJKMOCF@WSAESHUTDOWN?$AA@ (0000)
00256590 000c:
	??_C@_0M@BJHJCMPN@WSAENOTCONN?$AA@ (0000)
0025659C 000b:
	??_C@_0L@KMBJICHC@WSAEISCONN?$AA@ (0000)
002565A8 000b:
	??_C@_0L@OOBBNNPD@WSAENOBUFS?$AA@ (0000)
002565B4 000e:
	??_C@_0O@CIPFFFJE@WSAECONNRESET?$AA@ (0000)
002565C4 0010:
	??_C@_0BA@HNCPIGIO@WSAECONNABORTED?$AA@ (0000)
002565D4 000d:
	??_C@_0N@KABONKCJ@WSAENETRESET?$AA@ (0000)
002565E4 000f:
	??_C@_0P@MMGFNLEA@WSAENETUNREACH?$AA@ (0000)
002565F4 000c:
	??_C@_0M@BMNODHFM@WSAENETDOWN?$AA@ (0000)
00256600 0011:
	??_C@_0BB@MBBPJMGK@WSAEADDRNOTAVAIL?$AA@ (0000)
00256614 000e:
	??_C@_0O@OPDDJBEB@WSAEADDRINUSE?$AA@ (0000)
00256624 0010:
	??_C@_0BA@DNDGDCNF@WSAEAFNOSUPPORT?$AA@ (0000)
00256634 0010:
	??_C@_0BA@KKILJDEG@WSAEPFNOSUPPORT?$AA@ (0000)
00256644 000e:
	??_C@_0O@IFNEEJHC@WSAEOPNOTSUPP?$AA@ (0000)
00256654 0013:
	??_C@_0BD@KNONLHOA@WSAESOCKTNOSUPPORT?$AA@ (0000)
00256668 0013:
	??_C@_0BD@EDJBJFH@WSAEPROTONOSUPPORT?$AA@ (0000)
0025667C 000f:
	??_C@_0P@KODINEND@WSAENOPROTOOPT?$AA@ (0000)
0025668C 000e:
	??_C@_0O@HCKGAENP@WSAEPROTOTYPE?$AA@ (0000)
0025669C 000c:
	??_C@_0M@PJKLHKI@WSAEMSGSIZE?$AA@ (0000)
002566A8 0010:
	??_C@_0BA@BLGPIJAD@WSAEDESTADDRREQ?$AA@ (0000)
002566B8 000c:
	??_C@_0M@MDGNMGJ@WSAENOTSOCK?$AA@ (0000)
002566C4 000c:
	??_C@_0M@EAGDEEAC@WSAEALREADY?$AA@ (0000)
002566D0 000f:
	??_C@_0P@FHEJEBJC@WSAEINPROGRESS?$AA@ (0000)
002566E0 000f:
	??_C@_0P@BLDHJENA@WSAEWOULDBLOCK?$AA@ (0000)
002566F0 000a:
	??_C@_09IJIBIDKO@WSAEMFILE?$AA@ (0000)
002566FC 000a:
	??_C@_09PDAAOFKK@WSAEINVAL?$AA@ (0000)
00256708 000a:
	??_C@_09BIHAGEEK@WSAEFAULT?$AA@ (0000)
00256714 000a:
	??_C@_09EKFHKCJJ@WSAEACCES?$AA@ (0000)
00256720 0009:
	??_C@_08LJKOCPDH@WSAEBADF?$AA@ (0000)
0025672C 0009:
	??_C@_08KEJFHKFF@WSAEINTR?$AA@ (0000)
00256738 000f:
	??_C@_0P@NPGBNDPH@WSA_IO_PENDING?$AA@ (0000)
00256748 0012:
	??_C@_0BC@FBGHLLOK@WSA_IO_INCOMPLETE?$AA@ (0000)
0025675C 0016:
	??_C@_0BG@FAAIAFCH@WSA_OPERATION_ABORTED?$AA@ (0000)
00256774 0017:
	??_C@_0BH@FPLHJKEA@WSA_WAIT_IO_COMPLETION?$AA@ (0000)
0025678C 0011:
	??_C@_0BB@IEEIFHNM@WSA_WAIT_TIMEOUT?$AA@ (0000)
002567A0 0016:
	??_C@_0BG@PLFAIPGK@WSA_INVALID_PARAMETER?$AA@ (0000)
002567B8 0010:
	??_C@_0BA@FAMENHPI@WSA_WAIT_FAILED?$AA@ (0000)
002567C8 0018:
	??_C@_0BI@MPMIPNBJ@WSA_MAXIMUM_WAIT_EVENTS?$AA@ (0000)
002567E0 0012:
	??_C@_0BC@KMEIGLBE@WSA_INVALID_EVENT?$AA@ (0000)
002567F4 0016:
	??_C@_0BG@GHKDMPOF@WSA_NOT_ENOUGH_MEMORY?$AA@ (0000)
0025680C 0013:
	??_C@_0BD@OCCEFHAI@WSA_INVALID_HANDLE?$AA@ (0000)
00256820 000e:
	??_C@_0O@HPGLJJN@ep?5?$CG?$CG?5address?$AA@ (0000)
00256830 000e:
	??_C@_0O@HEFAJMIG@input?9?$DOthread?$AA@ (0000)
00256840 000a:
	??_C@_09GPBDNHNE@input?9?$DOep?$AA@ (0000)
0025684C 0006:
	??_C@_05DFJCHPDH@input?$AA@ (0000)
00256854 0021:
	??_C@_0CB@MKIOMIKP@ep?5?$CG?$CG?5address?5?$CG?$CG?5process_ref_ptr@ (0000)
00256878 0034:
	??_C@_0DE@NJDDCKIE@?$CB?$CCunable?5to?5get?5mutex?5in?5cancel_@ (0000)
002568AC 0024:
	??_C@_0CE@KNDEECJL@input?5?$CG?$CG?5input?9?$DOep?5?$CG?$CG?5input?9?$DOthr@ (0000)
002568D0 0038:
	??_C@_0DI@BLIDBCNB@listening_endpoint?5?$CG?$CG?5?$CIlistening@ (0000)
00256908 0018:
	??_C@_0BI@JLDBFBGK@?$CBendpoint_connected?$CIep?$CJ?$AA@ (0000)
00256920 001d:
	??_C@_0BN@IMCCCKCG@err?5?$DN?$DN?5_transport_error_none?$AA@ (0000)
00256940 0020:
	??_C@_0CA@KHMMBKBD@ep?9?$DOtype?5?$DN?$DN?5_transport_type_udp?$AA@ (0000)
00256960 0029:
	??_C@_0CJ@GBBFPHGP@ep?5?$CG?$CG?5buffer?5?$CG?$CG?5src_addr?5?$CG?$CG?5?$CIlen@ (0000)
0025698C 002a:
	??_C@_0CK@OGLHOBHI@ep?5?$CG?$CG?5buffer?5?$CG?$CG?5?$CIlength?5?$DO?50?$CJ?5?$CG?$CG?5@ (0000)
0031CE38 020c:
	_bss_0031ce38 (0000)
*/

/* ---------- headers */

#include "cseries/cseries.h"

#include "bungie_net/network/transport.h"

/* ---------- constants */

enum
{
	_transport_type_udp = 0x11,
	_transport_type_tcp,
	MAXIMUM_PENDING_CONNECTIONS = 32,
	MAXIMUM_SOCKETS_PER_SET = 64,
	MAXIMUM_ENDPOINT_THREADS = 64,
};

/* ---------- macros */

#define TRANSPORT_ENDPOINT_WINSOCK_FILE "c:\\halo\\SOURCE\\bungie_net\\network\\transport_endpoint_winsock.c"
#define INVALID_SOCKET ((long)-1)

/* ---------- structures */

struct transport_endpoint
{
	long socket;
	byte flags;
	char type;
	short error;
};

struct thread_reference;

struct endpoint_thread_reference
{
	struct thread_reference *thread;
	boolean dispose;
	byte pad[3];
};

struct transport_endpoint_winsock_globals
{
	void *unknown0;
	long unknown4;
	struct endpoint_thread_reference endpoint_threads[MAXIMUM_ENDPOINT_THREADS];
	long last_error;
};

struct winsock_fd_set
{
	unsigned long count;
	long sockets[MAXIMUM_SOCKETS_PER_SET];
};

struct winsock_timeval
{
	long seconds;
	long microseconds;
};

/* ---------- prototypes */

struct transport_endpoint *accept_endpoint(
	struct transport_endpoint *listening_endpoint);
long __stdcall closesocket(
	long socket);
void code_000713a0(
	void);
void delete_transport_endpoint(
	struct transport_endpoint *endpoint);
void dispose_thread(
	struct thread_reference *thread);
long __stdcall listen(
	long socket,
	long backlog);
long __stdcall select(
	long ignored,
	struct winsock_fd_set *readable,
	struct winsock_fd_set *writeable,
	struct winsock_fd_set *exceptions,
	struct winsock_timeval *timeout);
long __stdcall WSAGetLastError(
	void);
long __stdcall __WSAFDIsSet(
	long socket,
	struct winsock_fd_set *set);
char const *winsock_error_to_string(
	long error);

/* ---------- globals */

struct transport_endpoint_winsock_globals bss_0031ce38 = {0};

/* ---------- public code */

void code_000713a0(
	void)
{
	long endpoint_thread_index = 0;

	do
	{
		struct endpoint_thread_reference *endpoint_thread =
			&bss_0031ce38.endpoint_threads[endpoint_thread_index];

		if (endpoint_thread->thread && endpoint_thread->dispose)
		{
			dispose_thread(endpoint_thread->thread);
			endpoint_thread->thread = NULL;
			endpoint_thread->dispose = FALSE;
		}

		endpoint_thread_index++;
	}
	while (endpoint_thread_index < MAXIMUM_ENDPOINT_THREADS);

	return;
}

struct transport_endpoint *create_transport_endpoint(
	long type)
{
	struct transport_endpoint *ep = NULL;

	match_assert(TRANSPORT_ENDPOINT_WINSOCK_FILE, 0xCE, transport_initialized);
	code_000713a0();

	if (type == _transport_type_udp || type == _transport_type_tcp)
	{
		ep = match_malloc(
			TRANSPORT_ENDPOINT_WINSOCK_FILE,
			0xD4,
			sizeof(*ep));
		if (ep)
		{
			ep->error = _transport_error_none;
			ep->type = (char)type;
			ep->socket = INVALID_SOCKET;
			ep->flags = 0;
		}
	}

	return ep;
}

long get_endpoint_type(
	struct transport_endpoint const *ep)
{
	match_assert(TRANSPORT_ENDPOINT_WINSOCK_FILE, 0x12C, ep);
	match_assert(TRANSPORT_ENDPOINT_WINSOCK_FILE, 0x12D, transport_initialized);

	return ep->type;
}

long endpoint_connected(
	struct transport_endpoint const *ep)
{
	match_assert(TRANSPORT_ENDPOINT_WINSOCK_FILE, 0x426, ep);

	return ep->flags & 1;
}

boolean endpoint_writeable(
	struct transport_endpoint *ep,
	word timeout)
{
	struct winsock_fd_set writeable;
	struct winsock_timeval timeval;

	match_assert(
		TRANSPORT_ENDPOINT_WINSOCK_FILE,
		0x417,
		ep && (ep->socket != INVALID_SOCKET));

	timeval.seconds = 0;
	timeval.microseconds = timeout * MILLISECONDS_PER_SECOND;
	writeable.sockets[0] = ep->socket;
	writeable.count = 1;

	if (select(1, NULL, &writeable, NULL, &timeval) > 0 &&
		__WSAFDIsSet(ep->socket, &writeable))
		return TRUE;

	return FALSE;
}

long endpoint_blocking(
	struct transport_endpoint const *ep)
{
	match_assert(TRANSPORT_ENDPOINT_WINSOCK_FILE, 0x436, ep);

	return ~(ep->flags >> 4) & 1;
}

short get_endpoint_error(
	struct transport_endpoint const *ep)
{
	match_assert(TRANSPORT_ENDPOINT_WINSOCK_FILE, 0x43E, ep);

	return ep->error;
}

long endpoint_equivalent(
	struct transport_endpoint const *a,
	struct transport_endpoint const *b)
{
	match_assert(TRANSPORT_ENDPOINT_WINSOCK_FILE, 0x447, a);
	match_assert(TRANSPORT_ENDPOINT_WINSOCK_FILE, 0x448, b);

	if (a->socket != -1 && a->socket == b->socket)
		return TRUE;

	return FALSE;
}

void disconnect_endpoint(
	struct transport_endpoint *ep)
{
	match_assert(TRANSPORT_ENDPOINT_WINSOCK_FILE, 0x221, ep);
	match_assert(TRANSPORT_ENDPOINT_WINSOCK_FILE, 0x222, transport_initialized);

	if (ep->socket != INVALID_SOCKET)
	{
		if (closesocket(ep->socket) != 0)
			winsock_error_to_string(WSAGetLastError());

		ep->socket = INVALID_SOCKET;
	}

	ep->flags &= ~1;
	return;
}

short listen_endpoint(
	struct transport_endpoint *ep)
{
	long error = _transport_error_none;

	match_assert(TRANSPORT_ENDPOINT_WINSOCK_FILE, 0x2B0, ep);
	match_assert(TRANSPORT_ENDPOINT_WINSOCK_FILE, 0x2B1, transport_initialized);

	if (ep->socket != INVALID_SOCKET)
	{
		if (listen(ep->socket, MAXIMUM_PENDING_CONNECTIONS) == 0)
		{
			ep->flags |= 2;
		}
		else
		{
			winsock_error_to_string(WSAGetLastError());
			error = _transport_error_listen_failed;
		}
	}
	else
		error = _transport_error_bad_endpoint;

	ep->error = (short)error;
	return (short)error;
}

void delete_transport_endpoint(
	struct transport_endpoint *ep)
{
	match_assert(TRANSPORT_ENDPOINT_WINSOCK_FILE, 0xE4, ep);
	match_assert(TRANSPORT_ENDPOINT_WINSOCK_FILE, 0xE5, transport_initialized);

	disconnect_endpoint(ep);
	match_free(TRANSPORT_ENDPOINT_WINSOCK_FILE, 0xE8, ep);
	code_000713a0();
	return;
}

short reject_endpoint(
	struct transport_endpoint *listening_endpoint)
{
	struct transport_endpoint *endpoint = accept_endpoint(listening_endpoint);

	if (endpoint)
		delete_transport_endpoint(endpoint);

	return 0;
}

/* ---------- private code */
