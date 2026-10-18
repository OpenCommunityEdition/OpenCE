/*
NET_FAULT.C

The held back, lost and damaged datagrams of debug.network_latency and its
companions. See net_fault.h for why this is not in xnet.c any more.

Nothing here includes a socket header, and an address is carried as opaque
bytes, so this compiles the same on either side.
*/

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "net_fault.h"

/* the platform layer's (port/linux/src/port_config.c) */
double config_real(char const *name);
void platform_log(char const *format, ...);

enum
{
	NET_FAULT_MAXIMUM_PACKETS = 4096,
	/* (large enough for any address the sockets give) */
	NET_FAULT_ADDRESS_SIZE = 128
};

struct net_fault_packet
{
	int socket;
	int length;
	int offset;
	int address_length;
	unsigned long time;
	char address[NET_FAULT_ADDRESS_SIZE];
	char data[NET_FAULT_PACKET_SIZE];
};

static struct
{
	int checked;
	unsigned long latency;
	int loss_percent;
	int corrupt_percent;
	int corrupt_stream_percent;
	unsigned long corrupt_after;
	int started;
	unsigned long started_time;
	unsigned corrupt_seed;
	struct net_fault_packet *packets;
	int first;
	int count;
} net_fault;

/* (the game's network threads share the queue) */
static pthread_mutex_t net_fault_lock = PTHREAD_MUTEX_INITIALIZER;

int net_fault_enabled(void)
{
	if (!net_fault.checked)
	{
		double latency = config_real("debug.network_latency");
		double loss = config_real("debug.network_loss");
		double corrupt = config_real("debug.network_corrupt");
		double corrupt_stream = config_real("debug.network_corrupt_stream");
		double corrupt_after = config_real("debug.network_corrupt_after");

		/* up to ten seconds, and shares */
		latency = !(latency > 0.0) ? 0.0 : latency > 10000.0 ? 10000.0 : latency;
		loss = !(loss > 0.0) ? 0.0 : loss > 100.0 ? 100.0 : loss;
		corrupt = !(corrupt > 0.0) ? 0.0 : corrupt > 100.0 ? 100.0 : corrupt;
		corrupt_stream = !(corrupt_stream > 0.0) ? 0.0 : corrupt_stream > 100.0 ? 100.0 : corrupt_stream;
		corrupt_after = !(corrupt_after > 0.0) ? 0.0 : corrupt_after > 86400.0 ? 86400.0 : corrupt_after;
		net_fault.latency = (unsigned long)latency;
		net_fault.loss_percent = (int)loss;
		net_fault.corrupt_percent = (int)corrupt;
		net_fault.corrupt_stream_percent = (int)corrupt_stream;
		net_fault.corrupt_after = (unsigned long)(corrupt_after * 1000.0);
		net_fault.corrupt_seed = (unsigned)(size_t)&net_fault.checked;
		net_fault.checked = 1;
		if (net_fault.latency || net_fault.loss_percent || net_fault.corrupt_percent ||
			net_fault.corrupt_stream_percent)
		{
			net_fault.packets = calloc(NET_FAULT_MAXIMUM_PACKETS, sizeof(*net_fault.packets));
			platform_log("network: receiving %lu ms late, losing %d%% of datagrams, damaging %d%% of them and %d%% of stream reads after %lu s (testing)",
				net_fault.latency, net_fault.loss_percent, net_fault.corrupt_percent,
				net_fault.corrupt_stream_percent, net_fault.corrupt_after / 1000);
		}
	}
	return net_fault.packets != NULL;
}

/* the first time a caller gives a time, which is when the game starts
sending and receiving */
static void net_fault_start(unsigned long now)
{
	if (!net_fault.started)
	{
		net_fault.started = 1;
		net_fault.started_time = now;
		net_fault.corrupt_seed ^= (unsigned)now;
	}
}

/* (the test's own numbers, so that the game's rand() goes as without it) */
static unsigned net_fault_random(void)
{
	net_fault.corrupt_seed = net_fault.corrupt_seed * 1103515245u + 12345u;
	return net_fault.corrupt_seed >> 8;
}

/* damages a packet at random: a few bytes changed, cut short, stretched
with random bytes (a datagram, up to the largest), or replaced throughout;
the length it has after */
static int net_fault_damage(char *data, int length, int datagram)
{
	int count, index;

	switch (net_fault_random() % 8)
	{
	case 0:
		/* (cut short, a datagram to nothing at times; a stream's read of
		nothing would be the stream closing) */
		if (length > 0)
			length = datagram ? (int)(net_fault_random() % (unsigned)(length + 1)) :
				1 + (int)(net_fault_random() % (unsigned)length);
		break;
	case 1:
		if (datagram)
		{
			int stretched = (int)(net_fault_random() % (unsigned)(NET_FAULT_PACKET_SIZE + 1));

			for (index = length; index < stretched; index++)
				data[index] = (char)net_fault_random();
			if (stretched > length)
				length = stretched;
		}
		else if (length > 0)
		{
			data[net_fault_random() % (unsigned)length] = (char)net_fault_random();
		}
		break;
	case 2:
		for (index = 0; index < length; index++)
			data[index] = (char)net_fault_random();
		break;
	case 3:
		/* (a run of bytes: a field, with its neighbours) */
		if (length > 0)
		{
			int start = (int)(net_fault_random() % (unsigned)length);
			int run = 1 + (int)(net_fault_random() % 16u);

			for (index = start; index < length && index < start + run; index++)
				data[index] = (char)net_fault_random();
		}
		break;
	case 4:
		/* (the extremes a size or an index is checked against) */
		if (length > 0)
		{
			static const unsigned char extremes[] = { 0x00, 0xFF, 0x7F, 0x80, 0x01, 0xFE };

			count = 1 + (int)(net_fault_random() % 4u);
			for (index = 0; index < count; index++)
				data[net_fault_random() % (unsigned)length] = (char)extremes[net_fault_random() % sizeof(extremes)];
		}
		break;
	default:
		/* (a few bits) */
		count = 1 + (int)(net_fault_random() % 8u);
		for (index = 0; index < count && length > 0; index++)
			data[net_fault_random() % (unsigned)length] ^= (char)(1u << (net_fault_random() % 8u));
		break;
	}
	return length;
}

/* drops what is held for socket, and what has been read (socket -1),
keeping the rest in order */
static void net_fault_drop(int socket)
{
	int index, kept = 0;

	for (index = 0; index < net_fault.count; index++)
	{
		struct net_fault_packet *packet =
			&net_fault.packets[(net_fault.first + index) % NET_FAULT_MAXIMUM_PACKETS];

		if (packet->socket == socket || packet->socket == -1)
			continue;
		if (kept != index)
			net_fault.packets[(net_fault.first + kept) % NET_FAULT_MAXIMUM_PACKETS] = *packet;
		kept++;
	}
	net_fault.count = kept;
}

void net_fault_socket_closed(int socket)
{
	if (!net_fault.packets)
		return;
	pthread_mutex_lock(&net_fault_lock);
	net_fault_drop(socket);
	pthread_mutex_unlock(&net_fault_lock);
}

int net_fault_due(int socket, unsigned long now)
{
	int result = 0;
	int index;

	if (!net_fault.packets)
		return 0;
	pthread_mutex_lock(&net_fault_lock);
	for (index = 0; index < net_fault.count; index++)
	{
		struct net_fault_packet const *packet =
			&net_fault.packets[(net_fault.first + index) % NET_FAULT_MAXIMUM_PACKETS];

		if (packet->socket == socket)
		{
			result = now - packet->time >= net_fault.latency;
			break;
		}
	}
	pthread_mutex_unlock(&net_fault_lock);
	return result;
}

int net_fault_keep(int socket, const void *data, int length,
	const void *address, int address_length, int datagram, unsigned long now)
{
	struct net_fault_packet *packet;
	int kept = 0;

	if (!net_fault.packets || length < 0 || length > NET_FAULT_PACKET_SIZE)
		return 0;
	pthread_mutex_lock(&net_fault_lock);
	net_fault_start(now);
	/* (full: what was read, behind what was not, goes) */
	if (net_fault.count == NET_FAULT_MAXIMUM_PACKETS)
		net_fault_drop(-1);
	if (net_fault.count < NET_FAULT_MAXIMUM_PACKETS &&
		!(datagram && net_fault.loss_percent &&
			(int)(net_fault_random() % 100u) < net_fault.loss_percent))
	{
		packet = &net_fault.packets[(net_fault.first + net_fault.count) % NET_FAULT_MAXIMUM_PACKETS];
		memcpy(packet->data, data, (size_t)length);
		/* (a stream's reads by their own share: a damaged stream is dropped,
		and the game must go on for its datagrams to be tested) */
		if ((datagram ? net_fault.corrupt_percent : net_fault.corrupt_stream_percent) &&
			now - net_fault.started_time >= net_fault.corrupt_after &&
			(int)(net_fault_random() % 100u) <
				(datagram ? net_fault.corrupt_percent : net_fault.corrupt_stream_percent))
		{
			length = net_fault_damage(packet->data, length, datagram);
		}
		packet->socket = socket;
		packet->length = length;
		packet->offset = 0;
		packet->address_length = 0;
		if (address && address_length > 0)
		{
			packet->address_length = address_length < NET_FAULT_ADDRESS_SIZE ?
				address_length : NET_FAULT_ADDRESS_SIZE;
			memcpy(packet->address, address, (size_t)packet->address_length);
		}
		packet->time = now;
		net_fault.count++;
		kept = 1;
	}
	pthread_mutex_unlock(&net_fault_lock);
	return kept;
}

int net_fault_take(int socket, void *buffer, int length,
	void *address, int *address_length, int datagram, unsigned long now,
	int *truncated)
{
	int index;
	int result = -1;

	if (truncated)
		*truncated = 0;
	if (!net_fault.packets)
		return -1;
	pthread_mutex_lock(&net_fault_lock);
	net_fault_start(now);
	for (index = 0; index < net_fault.count; index++)
	{
		struct net_fault_packet *packet =
			&net_fault.packets[(net_fault.first + index) % NET_FAULT_MAXIMUM_PACKETS];
		int size;

		if (packet->socket != socket)
			continue;
		if (now - packet->time < net_fault.latency)
			break;
		size = packet->length - packet->offset;
		/* (a datagram larger than the buffer: its start, and WSAEMSGSIZE, as
		Winsock does) */
		if (datagram && size > length && truncated)
			*truncated = 1;
		if (size > length)
			size = length;
		if (size > 0)
			memcpy(buffer, packet->data + packet->offset, (size_t)size);
		if (datagram && address && address_length)
		{
			int copied = packet->address_length < *address_length ?
				packet->address_length : *address_length;

			memcpy(address, packet->address, (size_t)copied);
			*address_length = copied;
		}
		packet->offset += size;
		if (datagram || packet->offset >= packet->length)
			packet->socket = -1;
		/* (the spent ones at the front go) */
		while (net_fault.count > 0 && net_fault.packets[net_fault.first].socket == -1)
		{
			net_fault.first = (net_fault.first + 1) % NET_FAULT_MAXIMUM_PACKETS;
			net_fault.count--;
		}
		result = size;
		break;
	}
	pthread_mutex_unlock(&net_fault_lock);
	return result;
}
