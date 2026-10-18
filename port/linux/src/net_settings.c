/*
NET_SETTINGS.C

network.address and network.broadcast, read once. See net_settings.h for
why these are here and not in xnet.c.

Nothing here includes a socket header, so the file compiles the same beside
glibc's Winsock namespace (port/linux/src/xnet.c) and beside the Mac's
privately renamed Darwin one (port/mac/engine_sockets.c).
*/

#include <string.h>

#include "net_settings.h"
#include "halo_endian.h"

/* 255.255.255.255, which is no machine's address */
#define NET_SETTINGS_BROADCAST 0xFFFFFFFFUL

/* the platform layer's (port/linux/src/port_config.c) */
const char *config_string(char const *name);
void platform_log(char const *format, ...);

static struct
{
	int read;
	int has_local_address;
	unsigned long local_address;
	int broadcast_count;
	unsigned long broadcast_targets[NET_SETTINGS_MAXIMUM_BROADCAST_TARGETS];
} net_settings;

int net_settings_parse_ipv4(
	const char *text,
	const char *end,
	unsigned long *address)
{
	unsigned long value = 0;
	int part;

	if (!text || !end || !address)
		return 0;
	while (text < end && (*text == ' ' || *text == '\t'))
		text++;
	while (end > text && (end[-1] == ' ' || end[-1] == '\t'))
		end--;
	for (part = 0; part < 4; part++)
	{
		unsigned long number = 0;
		int digits = 0;

		while (text < end && *text >= '0' && *text <= '9' && digits < 3)
		{
			number = number * 10 + (unsigned long)(*text++ - '0');
			digits++;
		}
		if (!digits || number > 255)
			return 0;
		value = value << 8 | number;
		if (part < 3)
		{
			if (text >= end || *text != '.')
				return 0;
			text++;
		}
	}
	if (text != end)
		return 0;
	*address = halo_be32(value);
	return 1;
}

unsigned long net_settings_loopback_address(void)
{
	return halo_be32(0x7F000001UL);
}

void net_settings_read(void)
{
	const char *text;

	if (net_settings.read)
		return;
	/* (set first: a setting that logs must not read the settings again) */
	net_settings.read = 1;
	text = config_string("network.address");
	if (text && *text)
	{
		unsigned long address;

		/* neither 0.0.0.0 nor 255.255.255.255 is a machine's address */
		if (net_settings_parse_ipv4(text, text + strlen(text), &address) && address != 0 &&
			address != NET_SETTINGS_BROADCAST)
		{
			net_settings.local_address = address;
			net_settings.has_local_address = 1;
		}
		else
		{
			platform_log("network.address \"%s\" is not a usable IPv4 address: ignored", text);
		}
	}
	text = config_string("network.broadcast");
	while (text && *text)
	{
		const char *end = text + strcspn(text, ",");
		unsigned long address;

		if (net_settings_parse_ipv4(text, end, &address) && address != 0)
		{
			if (net_settings.broadcast_count < NET_SETTINGS_MAXIMUM_BROADCAST_TARGETS)
				net_settings.broadcast_targets[net_settings.broadcast_count++] = address;
			else if (net_settings.broadcast_count++ == NET_SETTINGS_MAXIMUM_BROADCAST_TARGETS)
				platform_log("network.broadcast: only the first %d addresses are used",
					NET_SETTINGS_MAXIMUM_BROADCAST_TARGETS);
		}
		else if (end > text)
		{
			platform_log("network.broadcast: %.*s is not an IPv4 address: ignored",
				(int)(end - text), text);
		}
		text = *end ? end + 1 : end;
	}
	if (net_settings.broadcast_count > NET_SETTINGS_MAXIMUM_BROADCAST_TARGETS)
		net_settings.broadcast_count = NET_SETTINGS_MAXIMUM_BROADCAST_TARGETS;
	if (net_settings.has_local_address && !net_settings.broadcast_count)
	{
		platform_log("network.address without network.broadcast: sockets bound to one address "
			"receive no broadcasts, so this machine sees other machines' games and searches only if "
			"they list its address in their network.broadcast");
	}
}

int net_settings_local_address(
	unsigned long *address)
{
	net_settings_read();
	if (!net_settings.has_local_address || !address)
		return 0;
	*address = net_settings.local_address;
	return 1;
}

int net_settings_broadcast_targets(
	unsigned long *targets,
	int maximum_count)
{
	int count;

	net_settings_read();
	if (!targets || maximum_count <= 0)
		return 0;
	count = net_settings.broadcast_count < maximum_count ? net_settings.broadcast_count : maximum_count;
	memcpy(targets, net_settings.broadcast_targets, (size_t)count * sizeof(*targets));
	return count;
}
