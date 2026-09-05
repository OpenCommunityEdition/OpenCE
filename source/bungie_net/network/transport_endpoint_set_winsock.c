/*
TRANSPORT_ENDPOINT_SET_WINSOCK.C

symbols in this file:
00070420 0010:
	_code_00070420 (0000)
00070430 0040:
	_code_00070430 (0000)
00070470 0080:
	_transport_push_key (0000)
000704F0 0040:
	_transport_pop_key (0000)
00070530 0070:
	_transport_get_nonce (0000)
000705A0 0070:
	_transport_nonce_is_equal (0000)
00070610 0070:
	_transport_is_nonce (0000)
00070680 0050:
	_transport_client_stop (0000)
000706D0 0030:
	_transport_get_xnaddr (0000)
00070700 0040:
	_transport_get_key_id (0000)
00070740 0060:
	_transport_get_key (0000)
000707A0 01a0:
	_transport_initialize (0000)
00070940 0030:
	_transport_dispose (0000)
00070970 0010:
	_transport_network_available (0000)
00070980 0100:
	_create_endpoint_set (0000)
00070A80 0090:
	_delete_endpoint_set (0000)
00070B10 0030:
	_code_00070b10 (0000)
00070B40 0230:
	_poll_endpoint_set (0000)
00070D70 0150:
	_add_endpoint_to_set (0000)
00070EC0 00f0:
	_remove_endpoint_from_set (0000)
00070FB0 0070:
	_rewind_endpoint_set (0000)
00071020 0080:
	_get_next_endpoint_from_set (0000)
000710A0 0060:
	_count_endpoints_in_set (0000)
00071100 00a0:
	_transport_server_initialize (0000)
000711A0 00a0:
	_transport_server_terminate (0000)
00071240 00c0:
	_transport_client_start (0000)
00256000 0004:
	??_C@_03KCHOJKKI@set?$AA@ (0000)
00256008 0043:
	??_C@_0ED@JLJCEPLC@c?3?2halo?2SOURCE?2bungie_net?2networ@ (0000)
0025604C 000b:
	??_C@_0L@LJNLIMFC@0?5?$DN?$DN?5error?$AA@ (0000)
00256058 0015:
	??_C@_0BF@BILAEEIK@global_key_depth?5?$DO?50?$AA@ (0000)
00256070 001e:
	??_C@_0BO@NAEOGANL@bytes?5?$DN?$DN?5sizeof?$CIglobal_nonce?$CJ?$AA@ (0000)
00256090 000c:
	??_C@_0M@HAHFDDCN@dst?5?$CB?$DN?5NULL?$AA@ (0000)
0025609C 000c:
	??_C@_0M@MAJHCAP@src?5?$CB?$DN?5NULL?$AA@ (0000)
002560A8 0022:
	??_C@_0CC@EEMKELAP@XNET_STARTUP_BYPASS_SECURITY?5?$FLON@ (0000)
002560CC 0017:
	??_C@_0BH@JKFKBOIJ@d?3?2bypass_security?4txt?$AA@ (0000)
002560E4 0021:
	??_C@_0CB@GPCGELJA@xbox?5ethernet?5link?5is?5?$CFs?$CFs?$CFs?$CFs?$CFs@ (0000)
00256108 000e:
	??_C@_0O@GLMIBBEG@not?5connected?$AA@ (0000)
00256118 000a:
	??_C@_09JGOMOAGJ@connected?$AA@ (0000)
00256124 000d:
	??_C@_0N@IDEDDGND@?5at?5100?5Mbps?$AA@ (0000)
00256134 000c:
	??_C@_0M@PHBFBOED@?5at?510?5Mbps?$AA@ (0000)
00256140 0015:
	??_C@_0BF@IFMPKCKC@?5in?5full?9duplex?5mode?$AA@ (0000)
00256158 0015:
	??_C@_0BF@CEMKPCLG@?5in?5half?9duplex?5mode?$AA@ (0000)
00256170 0012:
	??_C@_0BC@JBFMBDMK@max_endpoints?5?$DO?50?$AA@ (0000)
00256184 0015:
	??_C@_0BF@KAELMCII@set?5?$CG?$CG?5set?9?$DOep_array?$AA@ (0000)
0025619C 000a:
	??_C@_09OCPCIGPM@ep?5?$CG?$CG?5set?$AA@ (0000)
002561A8 0016:
	??_C@_0BG@FPPJIENA@0?5?$DN?$DN?5global_key_depth?$AA@ (0000)
0031CE30 0008:
	_transport_initialized (0000)
	_global_client_active (0001)
	_global_key_depth (0004)
*/

/* ---------- headers */

#include "cseries/cseries.h"
#include "bungie_net/network/transport.h"
#include "bungie_net/network/transport_endpoint_winsock.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct transport_endpoint;

struct transport_endpoint_set
{
	fd_set sockets;
	struct transport_endpoint **ep_array;
	long max_endpoints;
	long last_endpoint_index;
	long current_endpoint_index;
	long needs_compaction;
};

typedef char winsock_fd_set_size_assert[
	sizeof(fd_set) == 0x104 ? 1 : -1];
typedef char transport_endpoint_set_ep_array_offset_assert[
	offsetof(struct transport_endpoint_set, ep_array) == 0x104 ? 1 : -1];
typedef char transport_endpoint_set_max_endpoints_offset_assert[
	offsetof(struct transport_endpoint_set, max_endpoints) == 0x108 ? 1 : -1];
typedef char transport_endpoint_set_last_endpoint_index_offset_assert[
	offsetof(struct transport_endpoint_set, last_endpoint_index) == 0x10C ? 1 : -1];
typedef char transport_endpoint_set_current_endpoint_index_offset_assert[
	offsetof(struct transport_endpoint_set, current_endpoint_index) == 0x110 ? 1 : -1];
typedef char transport_endpoint_set_needs_compaction_offset_assert[
	offsetof(struct transport_endpoint_set, needs_compaction) == 0x114 ? 1 : -1];
typedef char transport_endpoint_set_size_assert[
	sizeof(struct transport_endpoint_set) == 0x118 ? 1 : -1];

/* ---------- prototypes */

/* ---------- globals */

extern long global_key_depth;
extern XNADDR global_address;
extern XNKID global_key_id;
extern XNKEY global_key;

/* ---------- public code */

void transport_push_key(
	const XNKEY *key,
	const XNKID *key_id)
{
	int error;

	global_key = *key;
	global_key_id = *key_id;
	if (global_key_depth == 0)
	{
		error = XNetRegisterKey(&global_key_id, &global_key);
		match_assert(
			"c:\\halo\\SOURCE\\bungie_net\\network\\transport_endpoint_set_winsock.c",
			0x5C,
			0 == error);
	}
	global_key_depth++;
	return;
}

XNADDR *transport_get_xnaddr(
	XNADDR *address)
{
	*address = global_address;
	return address;
}

XNKID transport_get_key_id(
	void)
{
	match_assert(
		"c:\\halo\\SOURCE\\bungie_net\\network\\transport_endpoint_set_winsock.c",
		0xE0,
		global_key_depth > 0);
	return global_key_id;
}

XNKEY transport_get_key(
	void)
{
	match_assert(
		"c:\\halo\\SOURCE\\bungie_net\\network\\transport_endpoint_set_winsock.c",
		0xE7,
		global_key_depth > 0);
	return global_key;
}

void transport_pop_key(
	void)
{
	match_assert(
		"c:\\halo\\SOURCE\\bungie_net\\network\\transport_endpoint_set_winsock.c",
		0x66,
		global_key_depth > 0);
	global_key_depth--;
	if (global_key_depth == 0)
	{
		XNetUnregisterKey(&global_key_id);
	}
	return;
}

struct transport_endpoint_set *create_endpoint_set(
	short max_endpoints)
{
	struct transport_endpoint_set *set;

	match_assert(
		"c:\\halo\\SOURCE\\bungie_net\\network\\transport_endpoint_set_winsock.c",
		0x196,
		transport_initialized);
	match_assert(
		"c:\\halo\\SOURCE\\bungie_net\\network\\transport_endpoint_set_winsock.c",
		0x197,
		max_endpoints > 0);
	set = match_malloc(
		"c:\\halo\\SOURCE\\bungie_net\\network\\transport_endpoint_set_winsock.c",
		0x199,
		sizeof(*set));
	if (set)
	{
		if (max_endpoints <= FD_SETSIZE)
		{
			set->needs_compaction = FALSE;
			FD_ZERO(&set->sockets);
			set->ep_array = debug_malloc(
				max_endpoints * sizeof(*set->ep_array),
				TRUE,
				"c:\\halo\\SOURCE\\bungie_net\\network\\transport_endpoint_set_winsock.c",
				0x1A2);
			if (set->ep_array)
			{
				set->max_endpoints = max_endpoints;
				set->last_endpoint_index = NONE;
				set->current_endpoint_index = 0;
			}
			else
			{
				match_free(
					"c:\\halo\\SOURCE\\bungie_net\\network\\transport_endpoint_set_winsock.c",
					0x1AA,
					set);
				set = NULL;
			}
		}
		else
		{
			match_free(
				"c:\\halo\\SOURCE\\bungie_net\\network\\transport_endpoint_set_winsock.c",
				0x1B0,
				set);
			set = NULL;
		}
	}
	return set;
}

short delete_endpoint_set(
	struct transport_endpoint_set *set)
{
	match_assert(
		"c:\\halo\\SOURCE\\bungie_net\\network\\transport_endpoint_set_winsock.c",
		0x1BB,
		set && set->ep_array);
	match_assert(
		"c:\\halo\\SOURCE\\bungie_net\\network\\transport_endpoint_set_winsock.c",
		0x1BC,
		transport_initialized);
	match_free(
		"c:\\halo\\SOURCE\\bungie_net\\network\\transport_endpoint_set_winsock.c",
		0x1BE,
		set->ep_array);
	match_free(
		"c:\\halo\\SOURCE\\bungie_net\\network\\transport_endpoint_set_winsock.c",
		0x1BF,
		set);
	return _transport_error_none;
}

void rewind_endpoint_set(
	struct transport_endpoint_set *set)
{
	match_assert(
		"c:\\halo\\SOURCE\\bungie_net\\network\\transport_endpoint_set_winsock.c",
		0x26D,
		set);
	match_assert(
		"c:\\halo\\SOURCE\\bungie_net\\network\\transport_endpoint_set_winsock.c",
		0x26E,
		transport_initialized);
	set->current_endpoint_index = 0;
	return;
}

struct transport_endpoint *get_next_endpoint_from_set(
	struct transport_endpoint_set *set)
{
	struct transport_endpoint *endpoint = NULL;

	match_assert(
		"c:\\halo\\SOURCE\\bungie_net\\network\\transport_endpoint_set_winsock.c",
		0x27A,
		set);
	match_assert(
		"c:\\halo\\SOURCE\\bungie_net\\network\\transport_endpoint_set_winsock.c",
		0x27B,
		transport_initialized);
	if (set->current_endpoint_index <= set->last_endpoint_index)
	{
		endpoint = set->ep_array[set->current_endpoint_index++];
	}
	return endpoint;
}

long count_endpoints_in_set(
	struct transport_endpoint_set *set)
{
	match_assert(
		"c:\\halo\\SOURCE\\bungie_net\\network\\transport_endpoint_set_winsock.c",
		0x289,
		set);
	match_assert(
		"c:\\halo\\SOURCE\\bungie_net\\network\\transport_endpoint_set_winsock.c",
		0x28A,
		transport_initialized);
	return set->last_endpoint_index + 1;
}

/* ---------- private code */
