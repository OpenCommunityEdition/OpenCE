/*
P2P.C

Internet play: machines that shared an invite reach each other's system
link games as if they were on one LAN, without a server of this project's.

- An invite is a link, halo://join/<host><token>: the hosting machine's
  random identifier (which its XNADDR also carries) and a random 16-byte
  token. A machine makes one when its game starts hosting (the game listens
  for connections), logs it, puts it on the clipboard, and offers it through
  Discord (p2p_discord.c). Nothing about a game is published anywhere else:
  without an invite there is no way to find or join it.
- Signalling (p2p_signal.c) goes through public MQTT brokers, on topics
  that are hashes of the token, with messages sealed with a key derived
  from it (p2p_crypto.c). A joiner offers the addresses it can be reached
  at; the host answers with its own and a key for their tunnel.
- The tunnel is one UDP socket. Each machine learns its public address from
  public STUN servers, and both then send to each other's addresses until
  packets get through (hole punching). There is no relay: two machines whose
  NATs both map every destination to a new port cannot connect. Every
  tunnel packet is sealed with the pair's key.
- Each peer gets a virtual address in 100.64.0.0/10, which the game sees
  (XNetXnAddrToInAddr maps the peer's XNADDR to it). xnet.c rewrites the
  game's destinations there to local stand-ins: a UDP socket here per peer
  and port forwards datagrams, and a TCP listener per peer and port takes
  the game's connections, carried reliably over the tunnel by KCP
  (port/third_party/kcp). Traffic arriving from a peer leaves these
  stand-ins, and xnet.c reports it as coming from the peer's address. The
  game's broadcasts also go to every peer, so a host's game shows up in its
  joiners' system link lists, and joining works as on a LAN.

The work happens on a thread of its own, under p2p_lock; the game's
threads only look up and create stand-ins.
*/

#include "platform.h"
#include "posix.h"
#include "port_config.h"
#include "p2p_internal.h"
#include "ikcp.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum
{
	TUNNEL_MAGIC = 0x68,
	TUNNEL_HEADER_SIZE = 1 + P2P_IDENTIFIER_SIZE,
	/* a tunnel packet's plaintext: a type, and at most the game's largest
	datagram (WSAStartup's iMaxUdpDg) with its ports */
	MAXIMUM_INNER_SIZE = 1400,
	MAXIMUM_PACKET_SIZE = TUNNEL_HEADER_SIZE + MAXIMUM_INNER_SIZE + P2P_SEAL_OVERHEAD,

	/* a host needs a UDP stand-in for two or three ports of every other
	machine, and a stream for each one's connection */
	MAXIMUM_PROXIES = 512,
	MAXIMUM_LISTENERS = 64,
	MAXIMUM_STREAMS = 160,
	MAXIMUM_STUN_SERVERS = 4,
	STREAM_BUFFER_SIZE = 16384,
	STREAM_CHUNK_SIZE = 1024,
	/* the most unacknowledged messages a stream sends ahead */
	STREAM_WINDOW = 128,
	KCP_MTU = 1200,

	/* milliseconds */
	LOOP_INTERVAL = 10,
	PUNCH_INTERVAL = 200,
	PING_INTERVAL = 1000,
	ENDPOINT_SWITCH_TIME = 3000,
	PEER_TIMEOUT = 20000,
	PUNCH_TIMEOUT = 30000,
	JOIN_TIMEOUT = 90000,
	STREAM_LINGER_TIME = 10000,
	STUN_RETRY_INTERVAL = 500,
	STUN_REFRESH_INTERVAL = 25000,
	STUN_ATTEMPTS = 6,

	/* where a running copy of the game takes invites from another one
	started to open a link (127.0.0.1) */
	HANDOFF_PORT = 47315,
};

enum
{
	_packet_ping = 1,
	_packet_pong,
	_packet_datagram,
	_packet_stream,
	_packet_bye,
};

/* the messages of a stream, inside KCP */
enum
{
	_stream_open = 'O',
	_stream_data = 'D',
	_stream_close = 'C',
};

enum
{
	/* a peer's stream whose open has not arrived */
	_stream_awaiting_open,
	/* connecting to the local game */
	_stream_connecting,
	_stream_open_state,
};

struct peer
{
	int used;
	unsigned char identifier[P2P_IDENTIFIER_SIZE];
	char name[2 * P2P_IDENTIFIER_SIZE + 1];
	unsigned char key[P2P_SHA256_SIZE];
	unsigned long virtual_address;
	int is_host;
	int connected;
	struct p2p_candidate candidates[P2P_MAXIMUM_CANDIDATES];
	int candidate_count;
	struct p2p_candidate endpoint;
	unsigned long offered_time;
	unsigned long heard_time;
	unsigned long endpoint_heard_time;
	unsigned long sent_time;
	unsigned long round_trip;
};

/* a UDP stand-in for one port of a peer */
struct proxy
{
	int socket;
	int peer;
	unsigned short remote_port;
	unsigned short local_port;
};

/* a TCP stand-in for one port of a peer: takes the game's connections */
struct listener
{
	int socket;
	int peer;
	unsigned short remote_port;
	unsigned short local_port;
};

/* one of the game's TCP connections, carried over the tunnel */
struct stream
{
	int used;
	int peer;
	IUINT32 conversation;
	ikcpcb *kcp;
	int state;
	/* the local end: the game's connection to a listener, or a connection
	to the game made for a peer's (whose port stands for the peer's) */
	int socket;
	unsigned short local_port;
	unsigned short remote_port;
	int local_closed;
	int remote_closed;
	unsigned long created_time;
	unsigned long closed_time;
	unsigned char pending[STREAM_BUFFER_SIZE];
	int pending_size;
};

struct stun_server
{
	char host[128];
	unsigned short port;
	unsigned long address;
	unsigned char transaction[12];
	int attempts;
	unsigned long sent_time;
	int has_mapped;
	struct p2p_candidate mapped;
};

static pthread_mutex_t p2p_lock = PTHREAD_MUTEX_INITIALIZER;

static struct
{
	int running;
	unsigned long local_address;
	int tunnel_socket;
	unsigned short tunnel_port;
	int handoff_socket;

	struct peer peers[P2P_MAXIMUM_PEERS];
	struct proxy proxies[MAXIMUM_PROXIES];
	struct listener listeners[MAXIMUM_LISTENERS];
	struct stream streams[MAXIMUM_STREAMS];
	/* recently finished streams, whose late packets are ignored */
	IUINT32 finished[16];
	int finished_next;

	struct stun_server stun[MAXIMUM_STUN_SERVERS];
	int stun_count;
	/* from the first time a game is hosted or joined */
	int stun_started;
	int reported_symmetric;

	/* hosting: while the game listens on hosting_socket */
	int hosting_socket;
	int hosting;
	int has_token;
	unsigned char token[P2P_TOKEN_SIZE];
	char invite[P2P_LINK_SIZE];
	int invite_copied;
	int reported_peer_count;

	/* joining: until the host is reached, or JOIN_TIMEOUT */
	int join_requested;
	int joining;
	unsigned char join_host[P2P_IDENTIFIER_SIZE];
	unsigned char join_token[P2P_TOKEN_SIZE];
	unsigned long join_time;

	char clipboard[P2P_LINK_SIZE];
	int has_clipboard;
} p2p = { 0, 0, -1, 0, -1, .hosting_socket = -1 };

static unsigned char identifier[P2P_IDENTIFIER_SIZE];
static int has_identifier;

/* ---------- helpers */

unsigned long p2p_now(void)
{
	return GetTickCount();
}

static int elapsed(unsigned long since, unsigned long time)
{
	return (long)(p2p_now() - since) >= (long)time;
}

unsigned long p2p_resolve(const char *host)
{
	unsigned long address;

	/* DNS can take seconds, which the game's threads must not wait for */
	pthread_mutex_unlock(&p2p_lock);
	address = posix_resolve_ipv4(host);
	pthread_mutex_lock(&p2p_lock);
	return address;
}

void p2p_register_url_scheme(const char *scheme, const char *description)
{
	if (config_real("debug.exit_after") > 0.0 || config_boolean("debug.hidden_window") ||
		config_boolean("debug.null_renderer"))
		return;
	posix_register_url_scheme(scheme, description);
}

void p2p_hex(const unsigned char *bytes, int size, char *text)
{
	static const char digits[] = "0123456789abcdef";
	int index;

	for (index = 0; index < size; index++)
	{
		text[index * 2] = digits[bytes[index] >> 4];
		text[index * 2 + 1] = digits[bytes[index] & 15];
	}
	text[size * 2] = 0;
}

static int hex_value(char digit)
{
	if (digit >= '0' && digit <= '9')
		return digit - '0';
	if (digit >= 'a' && digit <= 'f')
		return digit - 'a' + 10;
	if (digit >= 'A' && digit <= 'F')
		return digit - 'A' + 10;
	return -1;
}

static unsigned long network_long(unsigned long value)
{
	return __builtin_bswap32(value);
}

static unsigned short network_short(unsigned short value)
{
	return (unsigned short)(value << 8 | value >> 8);
}

static void put_short(unsigned char *bytes, unsigned short value)
{
	/* value is in network byte order already */
	memcpy(bytes, &value, 2);
}

static unsigned short get_short(const unsigned char *bytes)
{
	unsigned short value;

	memcpy(&value, bytes, 2);
	return value;
}

static void make_address(struct sockaddr_in *address, unsigned long ip, unsigned short port)
{
	memset(address, 0, sizeof(*address));
	address->sin_family = AF_INET;
	address->sin_port = port;
	address->sin_addr.s_addr = ip;
}

static const char *address_text(unsigned long ip, unsigned short port, char *text)
{
	unsigned long value = network_long(ip);

	sprintf(text, "%lu.%lu.%lu.%lu:%u", value >> 24, (value >> 16) & 255, (value >> 8) & 255, value & 255,
		network_short(port));
	return text;
}

/* a UDP or TCP socket bound to ip:port (0 for any), not blocking; its port
through bound_port; -1 on failure */
static int open_socket(int type, unsigned long ip, unsigned short port, unsigned short *bound_port)
{
	struct sockaddr_in address;
	int length = sizeof(address);
	int result = posix_socket(AF_INET, type, 0);

	if (result < 0)
		return -1;
	make_address(&address, ip, port);
	if (posix_socket_bind(result, &address, sizeof(address)) < 0 ||
		posix_socket_getsockname(result, &address, &length) < 0 ||
		posix_socket_set_nonblocking(result, 1) < 0)
	{
		posix_socket_close(result);
		return -1;
	}
	if (bound_port)
		*bound_port = address.sin_port;
	return result;
}

static int would_block(void)
{
	int error = posix_socket_last_error();

	return error == WSAEWOULDBLOCK || error == WSAEINPROGRESS;
}

const unsigned char *p2p_identifier(void)
{
	/* its own lock: the p2p thread asks while holding p2p_lock */
	static pthread_mutex_t identifier_lock = PTHREAD_MUTEX_INITIALIZER;

	pthread_mutex_lock(&identifier_lock);
	if (!has_identifier)
	{
		posix_random_bytes(identifier, sizeof(identifier));
		/* like a locally administered unicast MAC address, as XNADDR's
		abEnet holds one */
		identifier[0] = (unsigned char)((identifier[0] & 0xFC) | 0x02);
		has_identifier = 1;
	}
	pthread_mutex_unlock(&identifier_lock);
	return identifier;
}

/* ---------- peers */

static struct peer *find_peer(const unsigned char *peer_identifier)
{
	int index;

	for (index = 0; index < P2P_MAXIMUM_PEERS; index++)
	{
		if (p2p.peers[index].used && !memcmp(p2p.peers[index].identifier, peer_identifier, P2P_IDENTIFIER_SIZE))
			return &p2p.peers[index];
	}
	return NULL;
}

static struct peer *find_peer_by_address(unsigned long address)
{
	int index;

	for (index = 0; index < P2P_MAXIMUM_PEERS; index++)
	{
		if (p2p.peers[index].used && p2p.peers[index].virtual_address == address)
			return &p2p.peers[index];
	}
	return NULL;
}

static int is_virtual_address(unsigned long address)
{
	/* 100.64.0.0/10 */
	return (network_long(address) & 0xFFC00000) == 0x64400000;
}

/* an address in 100.64.0.0/10 from the identifier, not one another peer has */
static unsigned long virtual_address_for(const unsigned char *peer_identifier)
{
	unsigned char digest[P2P_SHA256_SIZE];
	unsigned long value;

	p2p_sha256(peer_identifier, P2P_IDENTIFIER_SIZE, digest);
	value = (unsigned long)digest[0] << 16 | (unsigned long)digest[1] << 8 | digest[2];
	for (;;)
	{
		unsigned long address = 0x64400000 | (value & 0x3FFFFF);

		/* no .0 or .255, which look like network and broadcast addresses */
		if ((address & 255) != 0 && (address & 255) != 255 && !find_peer_by_address(network_long(address)))
			return network_long(address);
		value++;
	}
}

static void peer_send_to(const struct peer *peer, const struct p2p_candidate *to, const unsigned char *inner,
	int size)
{
	unsigned char packet[MAXIMUM_PACKET_SIZE];
	struct sockaddr_in address;
	int sealed;

	if (p2p.tunnel_socket < 0 || size > MAXIMUM_INNER_SIZE)
		return;
	packet[0] = TUNNEL_MAGIC;
	memcpy(packet + 1, identifier, P2P_IDENTIFIER_SIZE);
	sealed = p2p_seal(peer->key, inner, size, packet + TUNNEL_HEADER_SIZE);
	make_address(&address, to->address, to->port);
	posix_socket_sendto(p2p.tunnel_socket, packet, TUNNEL_HEADER_SIZE + sealed, 0, &address, sizeof(address));
}

/* to a peer the tunnel has reached; dropped otherwise */
static void peer_send(struct peer *peer, const unsigned char *inner, int size)
{
	if (!peer->connected)
		return;
	peer_send_to(peer, &peer->endpoint, inner, size);
	peer->sent_time = p2p_now();
}

static void peer_ping(struct peer *peer, const struct p2p_candidate *to)
{
	unsigned char inner[5];
	unsigned long now = p2p_now();

	inner[0] = _packet_ping;
	memcpy(inner + 1, &now, 4);
	peer_send_to(peer, to, inner, sizeof(inner));
}

static void stream_free(struct stream *stream);
static void close_socket(int *socket);

/* everything carrying traffic for the peer (of this index) */
static void release_peer_links(int peer_index, int streams_only)
{
	int index;

	for (index = 0; index < MAXIMUM_STREAMS; index++)
	{
		if (p2p.streams[index].used && p2p.streams[index].peer == peer_index)
			stream_free(&p2p.streams[index]);
	}
	if (streams_only)
		return;
	for (index = 0; index < MAXIMUM_PROXIES; index++)
	{
		if (p2p.proxies[index].socket >= 0 && p2p.proxies[index].peer == peer_index)
			close_socket(&p2p.proxies[index].socket);
	}
	for (index = 0; index < MAXIMUM_LISTENERS; index++)
	{
		if (p2p.listeners[index].socket >= 0 && p2p.listeners[index].peer == peer_index)
			close_socket(&p2p.listeners[index].socket);
	}
}

static void drop_peer(struct peer *peer, const char *reason)
{
	platform_log("Internet play: %s %s: %s", peer->is_host ? "host" : "player", peer->name, reason);
	if (peer->connected)
	{
		unsigned char bye = _packet_bye;

		peer_send(peer, &bye, 1);
	}
	release_peer_links((int)(peer - p2p.peers), 0);
	memset(peer, 0, sizeof(*peer));
}

static void add_candidates(struct peer *peer, const struct p2p_candidate *candidates, int count)
{
	int index;

	for (index = 0; index < count; index++)
	{
		int known;

		for (known = 0; known < peer->candidate_count; known++)
		{
			if (peer->candidates[known].address == candidates[index].address &&
				peer->candidates[known].port == candidates[index].port)
				break;
		}
		if (known < peer->candidate_count)
			continue;
		if (peer->candidate_count < P2P_MAXIMUM_CANDIDATES)
			peer->candidates[peer->candidate_count++] = candidates[index];
		else
			peer->candidates[P2P_MAXIMUM_CANDIDATES - 1] = candidates[index];
	}
}

void p2p_peer_offered(const unsigned char *peer_identifier, const unsigned char *key,
	const struct p2p_candidate *candidates, int count, int is_host)
{
	struct peer *peer = find_peer(peer_identifier);

	if (!memcmp(peer_identifier, identifier, P2P_IDENTIFIER_SIZE))
		return;
	if (peer && memcmp(peer->key, key, P2P_SHA256_SIZE))
	{
		/* a new session with the same machine: what the old one carried is
		gone */
		release_peer_links((int)(peer - p2p.peers), 1);
		memcpy(peer->key, key, P2P_SHA256_SIZE);
		peer->connected = 0;
		peer->candidate_count = 0;
		peer->offered_time = p2p_now();
	}
	if (!peer)
	{
		int index;

		for (index = 0; index < P2P_MAXIMUM_PEERS && p2p.peers[index].used; index++)
			;
		if (index == P2P_MAXIMUM_PEERS)
		{
			platform_log("Internet play: too many players; one more was turned away");
			return;
		}
		peer = &p2p.peers[index];
		memset(peer, 0, sizeof(*peer));
		peer->used = 1;
		memcpy(peer->identifier, peer_identifier, P2P_IDENTIFIER_SIZE);
		p2p_hex(peer_identifier, P2P_IDENTIFIER_SIZE, peer->name);
		memcpy(peer->key, key, P2P_SHA256_SIZE);
		peer->virtual_address = virtual_address_for(peer_identifier);
		peer->is_host = is_host;
		peer->offered_time = p2p_now();
		platform_log("Internet play: reaching %s %s", is_host ? "host" : "player", peer->name);
	}
	add_candidates(peer, candidates, count);
}

static void peer_heard(struct peer *peer, unsigned long address, unsigned short port)
{
	unsigned long now = p2p_now();
	int same = peer->endpoint.address == address && peer->endpoint.port == port;

	peer->heard_time = now;
	if (!peer->connected)
	{
		char text[32];

		peer->connected = 1;
		peer->endpoint.address = address;
		peer->endpoint.port = port;
		peer->endpoint_heard_time = now;
		platform_log("Internet play: connected to %s %s at %s", peer->is_host ? "host" : "player", peer->name,
			address_text(address, port, text));
		if (peer->is_host)
		{
			if (p2p.joining && !memcmp(p2p.join_host, peer->identifier, P2P_IDENTIFIER_SIZE))
			{
				p2p.joining = 0;
				p2p_signal_stop_joining();
			}
			platform_log("Internet play: the host's game is listed under Multiplayer, System Link");
		}
	}
	else if (same)
	{
		peer->endpoint_heard_time = now;
	}
	else if (elapsed(peer->endpoint_heard_time, ENDPOINT_SWITCH_TIME))
	{
		/* its address changed (a NAT's mapping, or a better path) */
		peer->endpoint.address = address;
		peer->endpoint.port = port;
		peer->endpoint_heard_time = now;
	}
}

static void update_peers(void)
{
	int index;

	for (index = 0; index < P2P_MAXIMUM_PEERS; index++)
	{
		struct peer *peer = &p2p.peers[index];

		if (!peer->used)
			continue;
		if (peer->connected)
		{
			if (elapsed(peer->heard_time, PEER_TIMEOUT))
				drop_peer(peer, "lost the connection");
			else if (elapsed(peer->sent_time, PING_INTERVAL))
			{
				peer_ping(peer, &peer->endpoint);
				peer->sent_time = p2p_now();
			}
		}
		else if (elapsed(peer->offered_time, PUNCH_TIMEOUT))
		{
			drop_peer(peer, "could not connect (both networks' NATs may be too strict for a direct "
				"connection; forwarding network.tunnel_port on one router helps)");
		}
		else if (elapsed(peer->sent_time, PUNCH_INTERVAL))
		{
			int candidate;

			for (candidate = 0; candidate < peer->candidate_count; candidate++)
				peer_ping(peer, &peer->candidates[candidate]);
			peer->sent_time = p2p_now();
		}
	}
}

/* ---------- STUN (RFC 5389): this machine's public address */

static void stun_send(struct stun_server *server)
{
	unsigned char request[20];
	struct sockaddr_in address;

	if (!server->address)
		return;
	memset(request, 0, sizeof(request));
	request[1] = 0x01; /* binding request */
	request[4] = 0x21; request[5] = 0x12; request[6] = 0xA4; request[7] = 0x42;
	posix_random_bytes(server->transaction, sizeof(server->transaction));
	memcpy(request + 8, server->transaction, sizeof(server->transaction));
	make_address(&address, server->address, server->port);
	posix_socket_sendto(p2p.tunnel_socket, request, sizeof(request), 0, &address, sizeof(address));
	server->sent_time = p2p_now();
	server->attempts++;
}

static void stun_setup(void)
{
	const char *text = config_string("network.stun_servers");

	while (*text && p2p.stun_count < MAXIMUM_STUN_SERVERS)
	{
		const char *end = text + strcspn(text, ",");
		struct stun_server *server = &p2p.stun[p2p.stun_count];
		const char *colon;
		int length;

		while (text < end && *text == ' ')
			text++;
		length = (int)(end - text);
		while (length > 0 && text[length - 1] == ' ')
			length--;
		if (length > 0 && length < (int)sizeof(server->host))
		{
			memset(server, 0, sizeof(*server));
			memcpy(server->host, text, (size_t)length);
			colon = strchr(server->host, ':');
			server->port = network_short(3478);
			if (colon)
			{
				server->port = network_short((unsigned short)atoi(colon + 1));
				server->host[colon - server->host] = 0;
			}
			p2p.stun_count++;
		}
		text = *end ? end + 1 : end;
	}
}

static void stun_update(void)
{
	int index;

	if (!p2p.stun_started)
		return;
	for (index = 0; index < p2p.stun_count; index++)
	{
		struct stun_server *server = &p2p.stun[index];

		if (!server->address && !server->attempts)
		{
			/* looked up once, here on the p2p thread */
			server->address = p2p_resolve(server->host);
			if (!server->address)
			{
				platform_log("Internet play: cannot look up the STUN server %s", server->host);
				server->attempts = STUN_ATTEMPTS;
				continue;
			}
		}
		if (server->has_mapped)
		{
			/* also keeps the NAT's mapping of the tunnel alive */
			if (elapsed(server->sent_time, STUN_REFRESH_INTERVAL))
			{
				server->attempts = 0;
				stun_send(server);
			}
		}
		else if (server->attempts < STUN_ATTEMPTS && elapsed(server->sent_time, STUN_RETRY_INTERVAL))
		{
			stun_send(server);
		}
		else if (server->attempts >= STUN_ATTEMPTS && server->address && elapsed(server->sent_time, STUN_REFRESH_INTERVAL))
		{
			server->attempts = 0;
		}
	}
}

static void stun_received(const unsigned char *packet, int size)
{
	int index;
	int offset;

	if (size < 20 || packet[0] != 0x01 || packet[1] != 0x01)
		return;
	for (index = 0; index < p2p.stun_count; index++)
	{
		if (!memcmp(packet + 8, p2p.stun[index].transaction, 12))
			break;
	}
	if (index == p2p.stun_count)
		return;
	for (offset = 20; offset + 4 <= size; )
	{
		int type = packet[offset] << 8 | packet[offset + 1];
		int length = packet[offset + 2] << 8 | packet[offset + 3];
		const unsigned char *value = packet + offset + 4;

		if (offset + 4 + length > size)
			break;
		/* XOR-MAPPED-ADDRESS, or MAPPED-ADDRESS from an old server; IPv4 */
		if ((type == 0x0020 || type == 0x0001) && length >= 8 && value[1] == 0x01)
		{
			struct stun_server *server = &p2p.stun[index];
			unsigned char port[2], ip[4];
			int byte;

			memcpy(port, value + 2, 2);
			memcpy(ip, value + 4, 4);
			if (type == 0x0020)
			{
				for (byte = 0; byte < 2; byte++)
					port[byte] ^= packet[4 + byte];
				for (byte = 0; byte < 4; byte++)
					ip[byte] ^= packet[4 + byte];
			}
			if (!server->has_mapped)
			{
				char text[32];
				int other;

				memcpy(&server->mapped.address, ip, 4);
				memcpy(&server->mapped.port, port, 2);
				server->has_mapped = 1;
				platform_log("Internet play: this machine's public address is %s (from %s)",
					address_text(server->mapped.address, server->mapped.port, text), server->host);
				for (other = 0; other < p2p.stun_count; other++)
				{
					if (other != index && p2p.stun[other].has_mapped &&
						p2p.stun[other].mapped.port != server->mapped.port && !p2p.reported_symmetric)
					{
						platform_log("Internet play: this network's NAT gives each destination its own "
							"port, so it can only connect to machines behind more lenient ones");
						p2p.reported_symmetric = 1;
					}
				}
			}
			else
			{
				memcpy(&server->mapped.address, ip, 4);
				memcpy(&server->mapped.port, port, 2);
			}
			break;
		}
		offset += 4 + ((length + 3) & ~3);
	}
}

int p2p_local_candidates(struct p2p_candidate *candidates, int maximum_count)
{
	unsigned long lan = posix_local_ipv4_address();
	int count = 0;
	int index;

	if (lan && count < maximum_count)
	{
		candidates[count].address = lan;
		candidates[count++].port = p2p.tunnel_port;
	}
	for (index = 0; index < p2p.stun_count && count < maximum_count; index++)
	{
		int known;

		if (!p2p.stun[index].has_mapped)
			continue;
		for (known = 0; known < count; known++)
		{
			if (candidates[known].address == p2p.stun[index].mapped.address &&
				candidates[known].port == p2p.stun[index].mapped.port)
				break;
		}
		if (known == count)
			candidates[count++] = p2p.stun[index].mapped;
	}
	/* two copies of the game on one machine without a network */
	if (!count && maximum_count)
	{
		candidates[count].address = network_long(0x7F000001);
		candidates[count++].port = p2p.tunnel_port;
	}
	return count;
}

static int stun_settled(void)
{
	int index;

	for (index = 0; index < p2p.stun_count; index++)
	{
		if (!p2p.stun[index].has_mapped && p2p.stun[index].attempts < STUN_ATTEMPTS)
			return 0;
	}
	return 1;
}

/* ---------- stand-ins for peers' ports */

static void close_socket(int *socket)
{
	if (*socket >= 0)
		posix_socket_close(*socket);
	*socket = -1;
}

static struct proxy *find_proxy(int peer_index, unsigned short remote_port, int create)
{
	struct proxy *free_proxy = NULL;
	int index;

	for (index = 0; index < MAXIMUM_PROXIES; index++)
	{
		struct proxy *proxy = &p2p.proxies[index];

		if (proxy->socket < 0)
		{
			if (!free_proxy)
				free_proxy = proxy;
		}
		else if (proxy->peer == peer_index && proxy->remote_port == remote_port)
			return proxy;
	}
	if (!create || !free_proxy)
		return NULL;
	free_proxy->socket = open_socket(SOCK_DGRAM, p2p.local_address, 0, &free_proxy->local_port);
	if (free_proxy->socket < 0)
		return NULL;
	free_proxy->peer = peer_index;
	free_proxy->remote_port = remote_port;
	return free_proxy;
}

static struct listener *find_listener(int peer_index, unsigned short remote_port)
{
	struct listener *free_listener = NULL;
	int index;

	for (index = 0; index < MAXIMUM_LISTENERS; index++)
	{
		struct listener *listener = &p2p.listeners[index];

		if (listener->socket < 0)
		{
			if (!free_listener)
				free_listener = listener;
		}
		else if (listener->peer == peer_index && listener->remote_port == remote_port)
			return listener;
	}
	if (!free_listener)
		return NULL;
	free_listener->socket = open_socket(SOCK_STREAM, p2p.local_address, 0, &free_listener->local_port);
	if (free_listener->socket < 0)
		return NULL;
	if (posix_socket_listen(free_listener->socket, 8) < 0)
	{
		close_socket(&free_listener->socket);
		return NULL;
	}
	free_listener->peer = peer_index;
	free_listener->remote_port = remote_port;
	return free_listener;
}

int p2p_outgoing(int stream, unsigned long *address, unsigned short *port)
{
	struct peer *peer;
	int result = 0;

	if (!is_virtual_address(*address))
		return 0;
	pthread_mutex_lock(&p2p_lock);
	peer = p2p.running ? find_peer_by_address(*address) : NULL;
	if (peer && peer->connected)
	{
		int peer_index = (int)(peer - p2p.peers);

		if (stream)
		{
			struct listener *listener = find_listener(peer_index, *port);

			if (listener)
			{
				*address = p2p.local_address;
				*port = listener->local_port;
				result = 1;
			}
		}
		else
		{
			struct proxy *proxy = find_proxy(peer_index, *port, 1);

			if (proxy)
			{
				*address = p2p.local_address;
				*port = proxy->local_port;
				result = 1;
			}
		}
	}
	pthread_mutex_unlock(&p2p_lock);
	return result;
}

int p2p_incoming(int stream, unsigned long *address, unsigned short *port)
{
	int result = 0;
	int index;

	if (!p2p.running || *address != p2p.local_address)
		return 0;
	pthread_mutex_lock(&p2p_lock);
	if (stream)
	{
		for (index = 0; index < MAXIMUM_LISTENERS && !result; index++)
		{
			struct listener *listener = &p2p.listeners[index];

			if (listener->socket >= 0 && listener->local_port == *port)
			{
				*address = p2p.peers[listener->peer].virtual_address;
				*port = listener->remote_port;
				result = 1;
			}
		}
		for (index = 0; index < MAXIMUM_STREAMS && !result; index++)
		{
			struct stream *entry = &p2p.streams[index];

			if (entry->used && entry->local_port && entry->local_port == *port)
			{
				*address = p2p.peers[entry->peer].virtual_address;
				*port = entry->remote_port;
				result = 1;
			}
		}
	}
	else
	{
		for (index = 0; index < MAXIMUM_PROXIES; index++)
		{
			struct proxy *proxy = &p2p.proxies[index];

			if (proxy->socket >= 0 && proxy->local_port == *port)
			{
				*address = p2p.peers[proxy->peer].virtual_address;
				*port = proxy->remote_port;
				result = 1;
				break;
			}
		}
	}
	pthread_mutex_unlock(&p2p_lock);
	return result;
}

int p2p_broadcast_targets(unsigned short port, unsigned long *addresses, unsigned short *ports, int maximum_count)
{
	int count = 0;
	int index;

	if (!p2p.running)
		return 0;
	pthread_mutex_lock(&p2p_lock);
	for (index = 0; index < P2P_MAXIMUM_PEERS && count < maximum_count; index++)
	{
		struct proxy *proxy;

		if (!p2p.peers[index].used || !p2p.peers[index].connected)
			continue;
		proxy = find_proxy(index, port, 1);
		if (proxy)
		{
			addresses[count] = p2p.local_address;
			ports[count++] = proxy->local_port;
		}
	}
	pthread_mutex_unlock(&p2p_lock);
	return count;
}

int p2p_peer_address(const unsigned char *peer_identifier, unsigned long *address)
{
	struct peer *peer;
	int result = 0;

	if (!p2p.running)
		return 0;
	pthread_mutex_lock(&p2p_lock);
	peer = find_peer(peer_identifier);
	if (peer && peer->connected)
	{
		*address = peer->virtual_address;
		result = 1;
	}
	pthread_mutex_unlock(&p2p_lock);
	return result;
}

/* a datagram from the game to a peer */
static void proxy_readable(struct proxy *proxy)
{
	unsigned char inner[MAXIMUM_INNER_SIZE];
	int count;

	for (count = 0; count < 64; count++)
	{
		struct sockaddr_in from;
		int from_length = sizeof(from);
		int size = posix_socket_recvfrom(proxy->socket, inner + 5, sizeof(inner) - 5, 0, &from, &from_length);

		if (size < 0)
			break;
		/* only the game's own sockets use a stand-in */
		if (from.sin_addr.s_addr != p2p.local_address && from.sin_addr.s_addr != network_long(0x7F000001))
			continue;
		inner[0] = _packet_datagram;
		put_short(inner + 1, from.sin_port);
		put_short(inner + 3, proxy->remote_port);
		peer_send(&p2p.peers[proxy->peer], inner, size + 5);
	}
}

/* a datagram from a peer to the game */
static void datagram_received(struct peer *peer, const unsigned char *inner, int size)
{
	struct proxy *proxy;
	struct sockaddr_in to;

	if (size < 5)
		return;
	proxy = find_proxy((int)(peer - p2p.peers), get_short(inner + 1), 1);
	if (!proxy)
		return;
	make_address(&to, p2p.local_address, get_short(inner + 3));
	posix_socket_sendto(proxy->socket, inner + 5, size - 5, 0, &to, sizeof(to));
}

/* ---------- streams */

static int kcp_output(const char *buffer, int size, ikcpcb *kcp, void *user)
{
	struct stream *stream = user;
	unsigned char inner[MAXIMUM_INNER_SIZE];

	(void)kcp;
	if (size + 1 > (int)sizeof(inner))
		return -1;
	inner[0] = _packet_stream;
	memcpy(inner + 1, buffer, (size_t)size);
	peer_send(&p2p.peers[stream->peer], inner, size + 1);
	return 0;
}

static struct stream *stream_new(int peer_index, IUINT32 conversation)
{
	int index;

	for (index = 0; index < MAXIMUM_STREAMS; index++)
	{
		struct stream *stream = &p2p.streams[index];

		if (stream->used)
			continue;
		/* all but the buffer, which pending_size (after it) says is empty */
		memset(stream, 0, offsetof(struct stream, pending));
		stream->pending_size = 0;
		stream->kcp = ikcp_create(conversation, stream);
		if (!stream->kcp)
			return NULL;
		stream->used = 1;
		stream->peer = peer_index;
		stream->conversation = conversation;
		stream->socket = -1;
		stream->created_time = p2p_now();
		ikcp_setoutput(stream->kcp, kcp_output);
		ikcp_nodelay(stream->kcp, 1, LOOP_INTERVAL, 2, 1);
		ikcp_wndsize(stream->kcp, 256, 256);
		ikcp_setmtu(stream->kcp, KCP_MTU);
		return stream;
	}
	return NULL;
}

static void stream_free(struct stream *stream)
{
	close_socket(&stream->socket);
	if (stream->kcp)
		ikcp_release(stream->kcp);
	p2p.finished[p2p.finished_next++ % 16] = stream->conversation;
	memset(stream, 0, offsetof(struct stream, pending));
	stream->pending_size = 0;
	stream->socket = -1;
}

static void stream_message(struct stream *stream, unsigned char type, const void *data, int size)
{
	unsigned char message[1 + STREAM_CHUNK_SIZE];

	message[0] = type;
	memcpy(message + 1, data, (size_t)size);
	ikcp_send(stream->kcp, (const char *)message, size + 1);
}

static void stream_local_closed(struct stream *stream)
{
	close_socket(&stream->socket);
	if (!stream->local_closed)
	{
		stream->local_closed = 1;
		stream->closed_time = p2p_now();
		if (!stream->remote_closed)
			stream_message(stream, _stream_close, NULL, 0);
	}
}

/* the game connected to a stand-in listener */
static void listener_readable(struct listener *listener)
{
	for (;;)
	{
		struct sockaddr_in from;
		int from_length = sizeof(from);
		int socket = posix_socket_accept(listener->socket, &from, &from_length);
		struct stream *stream;
		IUINT32 conversation;
		unsigned char open[4];

		if (socket < 0)
			return;
		posix_socket_set_nonblocking(socket, 1);
		posix_random_bytes(&conversation, sizeof(conversation));
		stream = stream_new(listener->peer, conversation | 1);
		if (!stream)
		{
			posix_socket_close(socket);
			continue;
		}
		stream->socket = socket;
		stream->state = _stream_open_state;
		put_short(open, listener->remote_port);
		put_short(open + 2, from.sin_port);
		stream_message(stream, _stream_open, open, sizeof(open));
	}
}

/* a peer's connection opened: connect to the game for it */
static void stream_opened(struct stream *stream, const unsigned char *data, int size)
{
	struct sockaddr_in to;

	if (stream->state != _stream_awaiting_open || size < 4)
		return;
	stream->remote_port = get_short(data + 2);
	stream->socket = open_socket(SOCK_STREAM, p2p.local_address, 0, &stream->local_port);
	make_address(&to, p2p.local_address, get_short(data));
	if (stream->socket < 0 || (posix_socket_connect(stream->socket, &to, sizeof(to)) < 0 && !would_block()))
	{
		stream->local_port = 0;
		stream_local_closed(stream);
		return;
	}
	stream->state = _stream_connecting;
}

static void stream_received(struct peer *peer, const unsigned char *data, int size)
{
	int peer_index = (int)(peer - p2p.peers);
	IUINT32 conversation;
	struct stream *stream = NULL;
	int index;

	if (size < 24)
		return;
	conversation = ikcp_getconv(data);
	for (index = 0; index < MAXIMUM_STREAMS; index++)
	{
		if (p2p.streams[index].used && p2p.streams[index].peer == peer_index &&
			p2p.streams[index].conversation == conversation)
		{
			stream = &p2p.streams[index];
			break;
		}
	}
	if (!stream)
	{
		for (index = 0; index < 16; index++)
		{
			if (p2p.finished[index] == conversation)
				return;
		}
		stream = stream_new(peer_index, conversation);
		if (!stream)
			return;
		stream->state = _stream_awaiting_open;
	}
	ikcp_input(stream->kcp, (const char *)data, size);
}

static void stream_flush_pending(struct stream *stream)
{
	while (stream->pending_size > 0 && stream->socket >= 0 && stream->state == _stream_open_state)
	{
		int sent = posix_socket_send(stream->socket, stream->pending, stream->pending_size, 0);

		if (sent < 0)
		{
			if (!would_block())
				stream_local_closed(stream);
			return;
		}
		memmove(stream->pending, stream->pending + sent, (size_t)(stream->pending_size - sent));
		stream->pending_size -= sent;
	}
}

static void stream_update(struct stream *stream)
{
	unsigned char message[1 + STREAM_CHUNK_SIZE];

	ikcp_update(stream->kcp, p2p_now());
	/* what the peer sent */
	for (;;)
	{
		int size = ikcp_peeksize(stream->kcp);

		if (size <= 0 || size > (int)sizeof(message))
			break;
		if (size - 1 > STREAM_BUFFER_SIZE - stream->pending_size)
			break;
		size = ikcp_recv(stream->kcp, (char *)message, size);
		if (size <= 0)
			break;
		switch (message[0])
		{
		case _stream_open:
			stream_opened(stream, message + 1, size - 1);
			break;
		case _stream_data:
			if (stream->socket >= 0)
			{
				memcpy(stream->pending + stream->pending_size, message + 1, (size_t)(size - 1));
				stream->pending_size += size - 1;
			}
			break;
		case _stream_close:
			stream->remote_closed = 1;
			if (!stream->closed_time)
				stream->closed_time = p2p_now();
			break;
		}
	}
	stream_flush_pending(stream);
	if (stream->remote_closed && !stream->pending_size && stream->socket >= 0)
		stream_local_closed(stream);
	/* finished: both ends closed and everything delivered, or the tunnel
	gave up on it */
	if ((stream->local_closed && stream->remote_closed && !ikcp_waitsnd(stream->kcp)) ||
		(stream->closed_time && elapsed(stream->closed_time, STREAM_LINGER_TIME)) ||
		(stream->state == _stream_awaiting_open && elapsed(stream->created_time, STREAM_LINGER_TIME)) ||
		stream->kcp->state == (IUINT32)-1)
	{
		stream_free(stream);
	}
}

static void stream_readable(struct stream *stream)
{
	unsigned char buffer[STREAM_CHUNK_SIZE];
	int count;

	for (count = 0; count < 8 && ikcp_waitsnd(stream->kcp) < STREAM_WINDOW && stream->socket >= 0; count++)
	{
		int size = posix_socket_recv(stream->socket, buffer, sizeof(buffer), 0);

		if (size == 0 || (size < 0 && !would_block()))
		{
			stream_local_closed(stream);
			return;
		}
		if (size < 0)
			return;
		stream_message(stream, _stream_data, buffer, size);
	}
}

static void stream_writeable(struct stream *stream)
{
	if (stream->state == _stream_connecting)
		stream->state = _stream_open_state;
	stream_flush_pending(stream);
}

/* ---------- the tunnel */

static void tunnel_received(const unsigned char *packet, int size, const struct sockaddr_in *from)
{
	unsigned char inner[MAXIMUM_INNER_SIZE + P2P_SEAL_OVERHEAD];
	struct peer *peer;
	int inner_size;

	if (size >= 20 && packet[4] == 0x21 && packet[5] == 0x12 && packet[6] == 0xA4 && packet[7] == 0x42)
	{
		stun_received(packet, size);
		return;
	}
	if (size < TUNNEL_HEADER_SIZE + P2P_SEAL_OVERHEAD + 1 || packet[0] != TUNNEL_MAGIC ||
		size - TUNNEL_HEADER_SIZE > (int)sizeof(inner))
		return;
	peer = find_peer(packet + 1);
	if (!peer)
		return;
	inner_size = p2p_open(peer->key, packet + TUNNEL_HEADER_SIZE, size - TUNNEL_HEADER_SIZE, inner);
	if (inner_size < 1)
		return;
	peer_heard(peer, from->sin_addr.s_addr, from->sin_port);
	switch (inner[0])
	{
	case _packet_ping:
		if (inner_size >= 5)
		{
			struct p2p_candidate to;

			inner[0] = _packet_pong;
			to.address = from->sin_addr.s_addr;
			to.port = from->sin_port;
			peer_send_to(peer, &to, inner, 5);
		}
		break;
	case _packet_pong:
		if (inner_size >= 5)
		{
			unsigned long sent;

			memcpy(&sent, inner + 1, 4);
			peer->round_trip = p2p_now() - sent;
		}
		break;
	case _packet_datagram:
		datagram_received(peer, inner, inner_size);
		break;
	case _packet_stream:
		stream_received(peer, inner + 1, inner_size - 1);
		break;
	case _packet_bye:
		drop_peer(peer, "left");
		break;
	}
}

static void tunnel_readable(void)
{
	unsigned char packet[2048];
	int count;

	for (count = 0; count < 256; count++)
	{
		struct sockaddr_in from;
		int from_length = sizeof(from);
		int size = posix_socket_recvfrom(p2p.tunnel_socket, packet, sizeof(packet), 0, &from, &from_length);

		if (size < 0)
			break;
		tunnel_received(packet, size, &from);
	}
}

/* ---------- invites */

/* the host identifier and token in an invite link or code within text */
static int parse_invite(const char *text, unsigned char *host, unsigned char *token)
{
	unsigned char bytes[P2P_IDENTIFIER_SIZE + P2P_TOKEN_SIZE];
	const char *start = NULL;
	const char *search;
	int index;

	for (search = text; *search && !start; search++)
	{
		static const char prefix[] = "halo://join/";
		int length;

		for (length = 0; prefix[length] && search[length] &&
			(search[length] | 0x20) == prefix[length]; length++)
			;
		if (!prefix[length])
			start = search + length;
	}
	if (!start)
	{
		/* a bare code: the text is its digits alone, spaces around them
		allowed */
		start = text;
		while (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n')
			start++;
		for (index = 0; index < (int)sizeof(bytes) * 2; index++)
		{
			if (hex_value(start[index]) < 0)
				return 0;
		}
		for (search = start + index; *search; search++)
		{
			if (*search != ' ' && *search != '\t' && *search != '\r' && *search != '\n')
				return 0;
		}
	}
	for (index = 0; index < (int)sizeof(bytes); index++)
	{
		int high = hex_value(start[index * 2]);
		int low = high < 0 ? -1 : hex_value(start[index * 2 + 1]);

		if (low < 0)
			return 0;
		bytes[index] = (unsigned char)(high << 4 | low);
	}
	if (hex_value(start[sizeof(bytes) * 2]) >= 0)
		return 0;
	memcpy(host, bytes, P2P_IDENTIFIER_SIZE);
	memcpy(token, bytes + P2P_IDENTIFIER_SIZE, P2P_TOKEN_SIZE);
	return 1;
}

/* under p2p_lock */
static int join_invite(const char *text)
{
	unsigned char host[P2P_IDENTIFIER_SIZE], token[P2P_TOKEN_SIZE];
	struct peer *peer;

	if (!parse_invite(text, host, token))
		return 0;
	if (!memcmp(host, identifier, P2P_IDENTIFIER_SIZE))
		return 1;
	peer = find_peer(host);
	if (peer && peer->connected)
	{
		platform_log("Internet play: already connected to that invite's host");
		return 1;
	}
	if ((p2p.joining || p2p.join_requested) && !memcmp(host, p2p.join_host, sizeof(host)) &&
		!memcmp(token, p2p.join_token, sizeof(token)))
		return 1;
	memcpy(p2p.join_host, host, sizeof(host));
	memcpy(p2p.join_token, token, sizeof(token));
	p2p.join_requested = 1;
	return 1;
}

int p2p_join_invite(const char *text)
{
	int result;

	p2p_identifier();
	pthread_mutex_lock(&p2p_lock);
	result = join_invite(text);
	pthread_mutex_unlock(&p2p_lock);
	if (result && !p2p.running)
		platform_log("Internet play is off (network.online in config.toml): the invite is ignored");
	return result;
}

void p2p_invite_received(const char *text)
{
	if (!join_invite(text))
		platform_log("Internet play: that is not an invite");
}

static void update_joining(void)
{
	char name[2 * P2P_IDENTIFIER_SIZE + 1];

	if (p2p.join_requested)
	{
		/* the offer carries the public address, if there is one */
		p2p.stun_started = 1;
		if (!stun_settled())
			return;
		p2p.join_requested = 0;
		p2p.joining = 1;
		p2p.join_time = p2p_now();
		p2p_hex(p2p.join_host, P2P_IDENTIFIER_SIZE, name);
		platform_log("Internet play: joining %s's game", name);
		p2p_signal_start();
		p2p_signal_join(p2p.join_host, p2p.join_token);
	}
	else if (p2p.joining && elapsed(p2p.join_time, JOIN_TIMEOUT))
	{
		p2p.joining = 0;
		p2p_signal_stop_joining();
		platform_log("Internet play: no answer from the invite's host; it may have stopped hosting or quit");
	}
}

static int connected_player_count(void)
{
	int count = 0;
	int index;

	for (index = 0; index < P2P_MAXIMUM_PEERS; index++)
		count += p2p.peers[index].used && p2p.peers[index].connected && !p2p.peers[index].is_host;
	return count;
}

static void update_hosting(void)
{
	int want = p2p.hosting_socket >= 0;

	if (want && !p2p.hosting)
	{
		char text[2 * (P2P_IDENTIFIER_SIZE + P2P_TOKEN_SIZE) + 1];

		/* one invite for the whole run, so a link keeps working from game
		to game */
		if (!p2p.has_token)
		{
			unsigned char bytes[P2P_IDENTIFIER_SIZE + P2P_TOKEN_SIZE];

			posix_random_bytes(p2p.token, sizeof(p2p.token));
			p2p.has_token = 1;
			memcpy(bytes, identifier, P2P_IDENTIFIER_SIZE);
			memcpy(bytes + P2P_IDENTIFIER_SIZE, p2p.token, P2P_TOKEN_SIZE);
			p2p_hex(bytes, sizeof(bytes), text);
			snprintf(p2p.invite, sizeof(p2p.invite), "halo://join/%s", text);
		}
		p2p.hosting = 1;
		p2p.stun_started = 1;
		p2p_signal_start();
		p2p_signal_host(p2p.token);
		platform_log("Internet play: hosting. Invite players with this link (it only works while this "
			"copy of the game runs): %s", p2p.invite);
		if (!p2p.invite_copied)
		{
			memcpy(p2p.clipboard, p2p.invite, sizeof(p2p.clipboard));
			p2p.has_clipboard = 1;
			p2p.invite_copied = 1;
		}
		p2p.reported_peer_count = -1;
	}
	else if (!want && p2p.hosting)
	{
		p2p.hosting = 0;
		p2p_signal_stop_hosting();
		p2p_discord_set_hosting(NULL, 0, 0);
	}
	if (p2p.hosting && p2p.reported_peer_count != connected_player_count())
	{
		p2p.reported_peer_count = connected_player_count();
		/* the host and the machines the tunnel takes */
		p2p_discord_set_hosting(p2p.invite + strlen("halo://join/"), p2p.reported_peer_count + 1,
			P2P_MAXIMUM_PEERS + 1);
	}
}

void p2p_socket_listening(int socket)
{
	if (!p2p.running)
		return;
	pthread_mutex_lock(&p2p_lock);
	p2p.hosting_socket = socket;
	pthread_mutex_unlock(&p2p_lock);
}

void p2p_socket_closed(int socket)
{
	if (!p2p.running)
		return;
	pthread_mutex_lock(&p2p_lock);
	if (p2p.hosting_socket == socket)
		p2p.hosting_socket = -1;
	pthread_mutex_unlock(&p2p_lock);
}

const char *p2p_take_clipboard_text(void)
{
	static char text[P2P_LINK_SIZE];
	const char *result = NULL;

	if (!p2p.running)
		return NULL;
	pthread_mutex_lock(&p2p_lock);
	if (p2p.has_clipboard)
	{
		memcpy(text, p2p.clipboard, sizeof(text));
		p2p.has_clipboard = 0;
		result = text;
	}
	pthread_mutex_unlock(&p2p_lock);
	return result;
}

/* ---------- invites from elsewhere */

/* the first command line argument holding an invite */
static int command_line_invite(char *text, int size)
{
	int index;

	for (index = 1; posix_command_line_argument(index, text, (posix_ulong)size); index++)
	{
		unsigned char host[P2P_IDENTIFIER_SIZE], token[P2P_TOKEN_SIZE];

		if (parse_invite(text, host, token))
			return 1;
	}
	return 0;
}

int p2p_hand_off_invite(void)
{
#ifdef HALO_ANDROID
	return 0;
#else
	char invite[256];
	char message[300];
	struct sockaddr_in to;
	int socket;
	int attempt;
	int result = 0;

	if (!command_line_invite(invite, sizeof(invite)))
		return 0;
	socket = open_socket(SOCK_DGRAM, network_long(0x7F000001), 0, NULL);
	if (socket < 0)
		return 0;
	snprintf(message, sizeof(message), "halo-invite %s", invite);
	make_address(&to, network_long(0x7F000001), network_short(HANDOFF_PORT));
	for (attempt = 0; attempt < 3 && !result; attempt++)
	{
		int read[1] = { socket };
		int read_count = 1, write_count = 0, error_count = 0;
		char reply[16];

		posix_socket_sendto(socket, message, (int)strlen(message), 0, &to, sizeof(to));
		if (posix_socket_select(read, &read_count, NULL, &write_count, NULL, &error_count, 0, 150000, 0) > 0 &&
			posix_socket_recv(socket, reply, sizeof(reply), 0) == 2 && !memcmp(reply, "ok", 2))
		{
			result = 1;
		}
	}
	posix_socket_close(socket);
	if (result)
		platform_log("Internet play: passed the invite to the copy of the game already running");
	return result;
#endif
}

static void handoff_readable(void)
{
	char message[300];
	struct sockaddr_in from;
	int from_length = sizeof(from);
	int size = posix_socket_recvfrom(p2p.handoff_socket, message, sizeof(message) - 1, 0, &from, &from_length);

	if (size < 12 || from.sin_addr.s_addr != network_long(0x7F000001) || memcmp(message, "halo-invite ", 12))
		return;
	message[size] = 0;
	posix_socket_sendto(p2p.handoff_socket, "ok", 2, 0, &from, sizeof(from));
	p2p_invite_received(message + 12);
}

#ifdef HALO_ANDROID
/* the app's activity writes a link it was opened with here */
static void poll_invite_file(void)
{
	static unsigned long checked_time;
	char path[512];
	char text[256];
	FILE *file;
	size_t size;

	if (!elapsed(checked_time, 1000))
		return;
	checked_time = p2p_now();
	snprintf(path, sizeof(path), "%s/join_link.txt", platform_data_root());
	file = fopen(path, "rb");
	if (!file)
		return;
	size = fread(text, 1, sizeof(text) - 1, file);
	fclose(file);
	remove(path);
	text[size] = 0;
	p2p_invite_received(text);
}
#endif

/* ---------- the thread */

static void *p2p_thread(void *unused)
{
	enum { MAXIMUM_SOCKETS = 2 + MAXIMUM_PROXIES + MAXIMUM_LISTENERS + MAXIMUM_STREAMS + 16 };
	int read[MAXIMUM_SOCKETS], write[MAXIMUM_SOCKETS];

	(void)unused;
	pthread_mutex_lock(&p2p_lock);
	for (;;)
	{
		int read_count = 0, write_count = 0, error_count = 0;
		/* KCP's clock needs a pass every LOOP_INTERVAL while it carries
		streams; otherwise the thread can sleep longer */
		int wait = LOOP_INTERVAL * 5;
		int index;

		/* what to wait for */
		read[read_count++] = p2p.tunnel_socket;
		if (p2p.handoff_socket >= 0)
			read[read_count++] = p2p.handoff_socket;
		for (index = 0; index < MAXIMUM_PROXIES; index++)
		{
			if (p2p.proxies[index].socket >= 0)
				read[read_count++] = p2p.proxies[index].socket;
		}
		for (index = 0; index < MAXIMUM_LISTENERS; index++)
		{
			if (p2p.listeners[index].socket >= 0)
				read[read_count++] = p2p.listeners[index].socket;
		}
		for (index = 0; index < MAXIMUM_STREAMS; index++)
		{
			struct stream *stream = &p2p.streams[index];

			if (stream->used)
				wait = LOOP_INTERVAL;
			if (!stream->used || stream->socket < 0)
				continue;
			/* (not while the tunnel's window is full, which stream_readable
			waits out, or the wait would return at once) */
			if (stream->state == _stream_open_state && ikcp_waitsnd(stream->kcp) < STREAM_WINDOW)
				read[read_count++] = stream->socket;
			if (stream->state == _stream_connecting || stream->pending_size)
				write[write_count++] = stream->socket;
		}
		p2p_signal_select_sets(read, &read_count, write, &write_count,
			MAXIMUM_SOCKETS - (read_count > write_count ? read_count : write_count));

		pthread_mutex_unlock(&p2p_lock);
		if (posix_socket_select(read, &read_count, write, &write_count, NULL, &error_count, 0,
			wait * 1000, 0) < 0)
		{
			read_count = write_count = 0;
		}
		pthread_mutex_lock(&p2p_lock);

		/* what is ready; the lists now hold only ready sockets */
		for (index = 0; index < read_count; index++)
		{
			int socket = read[index];
			int entry;

			if (socket == p2p.tunnel_socket)
			{
				tunnel_readable();
				continue;
			}
			if (socket == p2p.handoff_socket)
			{
				handoff_readable();
				continue;
			}
			for (entry = 0; entry < MAXIMUM_PROXIES; entry++)
			{
				if (p2p.proxies[entry].socket == socket)
					proxy_readable(&p2p.proxies[entry]);
			}
			for (entry = 0; entry < MAXIMUM_LISTENERS; entry++)
			{
				if (p2p.listeners[entry].socket == socket)
					listener_readable(&p2p.listeners[entry]);
			}
			for (entry = 0; entry < MAXIMUM_STREAMS; entry++)
			{
				if (p2p.streams[entry].used && p2p.streams[entry].socket == socket)
					stream_readable(&p2p.streams[entry]);
			}
		}
		for (index = 0; index < write_count; index++)
		{
			int entry;

			for (entry = 0; entry < MAXIMUM_STREAMS; entry++)
			{
				if (p2p.streams[entry].used && p2p.streams[entry].socket == write[index])
					stream_writeable(&p2p.streams[entry]);
			}
		}
		/* a connection to the game that failed is in neither list */
		for (index = 0; index < MAXIMUM_STREAMS; index++)
		{
			struct stream *stream = &p2p.streams[index];

			if (stream->used && stream->state == _stream_connecting && elapsed(stream->created_time, 5000))
				stream_local_closed(stream);
		}
		p2p_signal_update(read, read_count, write, write_count);

		for (index = 0; index < MAXIMUM_STREAMS; index++)
		{
			if (p2p.streams[index].used)
				stream_update(&p2p.streams[index]);
		}
		update_peers();
		stun_update();
		update_hosting();
		update_joining();
		p2p_discord_update();
#ifdef HALO_ANDROID
		poll_invite_file();
#endif
	}
	return NULL;
}

void p2p_initialize(unsigned long local_address)
{
	char invite[256];
	pthread_t thread;
	int index;

	p2p_identifier();
	if (p2p.running || !config_boolean("network.online"))
		return;
	for (index = 0; index < MAXIMUM_PROXIES; index++)
		p2p.proxies[index].socket = -1;
	for (index = 0; index < MAXIMUM_LISTENERS; index++)
		p2p.listeners[index].socket = -1;
	for (index = 0; index < MAXIMUM_STREAMS; index++)
		p2p.streams[index].socket = -1;
	p2p.local_address = local_address;
	p2p.tunnel_socket = open_socket(SOCK_DGRAM, 0,
		network_short((unsigned short)config_integer("network.tunnel_port")), &p2p.tunnel_port);
	if (p2p.tunnel_socket < 0)
	{
		platform_log("Internet play: cannot open its socket (network.tunnel_port %ld in use?); it is off",
			config_integer("network.tunnel_port"));
		return;
	}
	{
		/* all of a host's traffic, up to a 128-machine game's, goes through
		this one socket: buffers as large as the game's own sockets' (at
		least 1 MB, transport_endpoint_winsock.c) */
		int size = 1 << 20;

		posix_socket_setsockopt(p2p.tunnel_socket, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
		posix_socket_setsockopt(p2p.tunnel_socket, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
	}
#ifndef HALO_ANDROID
	/* the first copy of the game takes the invites later ones are opened
	with */
	p2p.handoff_socket = open_socket(SOCK_DGRAM, network_long(0x7F000001), network_short(HANDOFF_PORT), NULL);
	p2p_register_url_scheme("halo", "Halo: Combat Evolved invite");
#endif
	stun_setup();
	if (pthread_create(&thread, NULL, p2p_thread, NULL) != 0)
	{
		close_socket(&p2p.tunnel_socket);
		close_socket(&p2p.handoff_socket);
		return;
	}
	pthread_detach(thread);
	p2p.running = 1;
	if (command_line_invite(invite, sizeof(invite)))
		p2p_join_invite(invite);
}
