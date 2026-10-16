/*
WEBRTC_NATIVE.C

A native build's WebRTC (port/linux/src/p2p_webrtc.c, with posix_dtls.c and
Mbed TLS), built for this computer around one connection to a browser:
port/web/tests/webrtc_native.mjs runs it with a browser's data channel, as
internet play's signalling would tell each about the other, and the browser
sends it messages of every size, which it sends back.

It prints, on its standard output, what the browser is told: its address,
the tunnel's port, its ICE credentials (from the session's secret, the first
argument in hexadecimal) and its certificate's hash; and every second the
messages received. On its standard input it takes the browser's
certificate's hash ("browser <hex>"), which opens the connection, and
"quit".
HARNESS_LOSS=<percent> loses that share of the datagrams each way.
HARNESS_DROP=<n>[,<n>...] loses the nth DATA chunks the browser sends (from
1; the records p2p_webrtc.c reads, by linking it with
-Wl,--wrap=posix_dtls_receive): only those, whatever else their packets
carry.
*/

#include "posix.h"
#include "p2p_internal.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

static int udp = -1;
static unsigned short tunnel_port;
static in_addr_t local_address;
static int connection = -1;
static long received_count, received_bytes;
static int loss;
static const char *drops;
static long data_count;

/* ---------- what p2p_webrtc.c calls (p2p.c's, the platform layer's) */

void posix_random_bytes(void *buffer, posix_ulong size)
{
	if (getrandom(buffer, size, 0) != (ssize_t)size)
		abort();
}

unsigned long p2p_now(void)
{
	struct timespec now;

	clock_gettime(CLOCK_MONOTONIC, &now);
	return (unsigned long)(now.tv_sec * 1000 + now.tv_nsec / 1000000);
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

int p2p_local_candidates(struct p2p_candidate *candidates, int maximum_count)
{
	(void)maximum_count;
	candidates[0].address = local_address;
	candidates[0].port = tunnel_port;
	return 1;
}

void p2p_peer_name(int peer, char *name)
{
	(void)peer;
	strcpy(name, "browser");
}

void platform_log(const char *format, ...)
{
	va_list arguments;

	va_start(arguments, format);
	vfprintf(stderr, format, arguments);
	va_end(arguments);
	fputc('\n', stderr);
}

static int lost(void)
{
	unsigned int value;

	posix_random_bytes(&value, sizeof(value));
	return (int)(value % 100) < loss;
}

void p2p_tunnel_send(unsigned long address, unsigned short port, const void *data, int size)
{
	struct sockaddr_in to;

	if (lost())
		return;
	memset(&to, 0, sizeof(to));
	to.sin_family = AF_INET;
	to.sin_addr.s_addr = (in_addr_t)address;
	to.sin_port = port;
	sendto(udp, data, (size_t)size, 0, (struct sockaddr *)&to, sizeof(to));
}

/* the browser's SCTP packets as p2p_webrtc.c reads them, less the DATA
chunks HARNESS_DROP names */
int __real_posix_dtls_receive(int handle, posix_ulong now, void *buffer, int size);

static int dropped(long number)
{
	const char *list = drops;

	while (list && *list)
	{
		if (atol(list) == number)
			return 1;
		list = strchr(list, ',');
		if (list)
			list++;
	}
	return 0;
}

static unsigned int crc32c(const unsigned char *data, int size)
{
	unsigned int crc = 0xFFFFFFFF;
	int index, bit;

	for (index = 0; index < size; index++)
	{
		crc ^= data[index];
		for (bit = 0; bit < 8; bit++)
			crc = crc & 1 ? 0x82F63B78 ^ crc >> 1 : crc >> 1;
	}
	return ~crc;
}

int __wrap_posix_dtls_receive(int handle, posix_ulong now, void *buffer, int size)
{
	unsigned char *packet = buffer;
	int result = __real_posix_dtls_receive(handle, now, buffer, size);
	int offset = 12;
	int changed = 0;
	unsigned int crc;

	if (result <= 0 || !drops)
		return result;
	while (offset + 4 <= result)
	{
		int length = packet[offset + 2] << 8 | packet[offset + 3];
		int padded = (length + 3) & ~3;

		if (length < 4 || offset + length > result)
			break;
		if (offset + padded > result)
			padded = result - offset;
		/* (a DATA chunk) */
		if (packet[offset] == 0 && dropped(++data_count))
		{
			fprintf(stderr, "DATA chunk %ld lost\n", data_count);
			memmove(packet + offset, packet + offset + padded, (size_t)(result - offset - padded));
			result -= padded;
			changed = 1;
			continue;
		}
		offset += padded;
	}
	if (changed)
	{
		memset(packet + 8, 0, 4);
		crc = crc32c(packet, result);
		packet[8] = (unsigned char)crc;
		packet[9] = (unsigned char)(crc >> 8);
		packet[10] = (unsigned char)(crc >> 16);
		packet[11] = (unsigned char)(crc >> 24);
	}
	return result;
}

/* a message from the browser: back to it */
void p2p_tunnel_packet(const unsigned char *packet, int size, unsigned long address, unsigned short port)
{
	(void)address;
	(void)port;
	received_count++;
	received_bytes += size;
	p2p_webrtc_send(connection, packet, size);
}

/* ---------- the harness */

static void unhex(const char *text, unsigned char *bytes, int size)
{
	int index;

	for (index = 0; index < size; index++)
	{
		unsigned int value = 0;

		sscanf(text + index * 2, "%2x", &value);
		bytes[index] = (unsigned char)value;
	}
}

int main(int argc, char **argv)
{
	unsigned char secret[P2P_SHA256_SIZE];
	unsigned char fingerprint[P2P_FINGERPRINT_SIZE];
	char ufrag[P2P_ICE_UFRAG_SIZE], password[P2P_ICE_PASSWORD_SIZE], text[2 * P2P_FINGERPRINT_SIZE + 1];
	struct sockaddr_in address;
	socklen_t length = sizeof(address);
	unsigned long reported = 0;

	if (argc < 2 || strlen(argv[1]) != 2 * P2P_SHA256_SIZE)
	{
		fprintf(stderr, "usage: webrtc_native <secret in hex>\n");
		return 2;
	}
	loss = getenv("HARNESS_LOSS") ? atoi(getenv("HARNESS_LOSS")) : 0;
	drops = getenv("HARNESS_DROP");
	if (drops && !*drops)
		drops = NULL;
	unhex(argv[1], secret, sizeof(secret));
	/* the address this computer sends from (a browser may not take a
	loopback one): a UDP socket connected anywhere names it, sending nothing */
	local_address = htonl(INADDR_LOOPBACK);
	udp = socket(AF_INET, SOCK_DGRAM, 0);
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr("192.0.2.1");
	address.sin_port = htons(9);
	if (udp >= 0 && connect(udp, (struct sockaddr *)&address, sizeof(address)) == 0 &&
		getsockname(udp, (struct sockaddr *)&address, &length) == 0)
	{
		local_address = address.sin_addr.s_addr;
	}
	if (udp >= 0)
		close(udp);
	length = sizeof(address);
	udp = socket(AF_INET, SOCK_DGRAM, 0);
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	if (udp < 0 || bind(udp, (struct sockaddr *)&address, sizeof(address)) != 0 ||
		getsockname(udp, (struct sockaddr *)&address, &length) != 0)
	{
		perror("socket");
		return 1;
	}
	tunnel_port = address.sin_port;
	p2p_webrtc_credentials(secret, ufrag, password);
	if (!posix_dtls_fingerprint(fingerprint))
	{
		fprintf(stderr, "no certificate\n");
		return 1;
	}
	p2p_hex(fingerprint, sizeof(fingerprint), text);
	printf("address %s\nport %u\nufrag %s\npassword %s\nfingerprint %s\n",
		inet_ntoa(*(struct in_addr *)&local_address), ntohs(tunnel_port), ufrag, password, text);
	fflush(stdout);
	for (;;)
	{
		struct pollfd waits[2] = { { udp, POLLIN, 0 }, { 0, POLLIN, 0 } };
		unsigned char datagram[2048];

		poll(waits, 2, 5);
		if (waits[1].revents & (POLLIN | POLLHUP))
		{
			char line[256];
			int size = 0;

			while (size < (int)sizeof(line) - 1 && read(0, line + size, 1) == 1 && line[size] != '\n')
				size++;
			line[size] = 0;
			if (!strncmp(line, "browser ", 8) && strlen(line) >= 8 + 2 * P2P_FINGERPRINT_SIZE)
			{
				struct p2p_webrtc remote;

				memset(&remote, 0, sizeof(remote));
				remote.kind = _p2p_webrtc_browser;
				unhex(line + 8, remote.fingerprint, P2P_FINGERPRINT_SIZE);
				connection = p2p_webrtc_offered(-1, 0, secret, &remote, NULL, 0, 0);
				printf("connection %d\n", connection);
				fflush(stdout);
			}
			else if (!strcmp(line, "quit") || !size)
				break;
		}
		if (waits[0].revents & POLLIN)
		{
			struct sockaddr_in from;
			socklen_t from_length = sizeof(from);
			ssize_t size = recvfrom(udp, datagram, sizeof(datagram), 0, (struct sockaddr *)&from, &from_length);

			if (size > 0 && !lost())
				p2p_webrtc_received(datagram, (int)size, from.sin_addr.s_addr, from.sin_port);
		}
		p2p_webrtc_update();
		if (p2p_now() - reported >= 1000)
		{
			reported = p2p_now();
			printf("received %ld %ld\n", received_count, received_bytes);
			fflush(stdout);
		}
	}
	return 0;
}
