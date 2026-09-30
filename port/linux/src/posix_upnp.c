/*
POSIX_UPNP.C

UPnP for internet play (posix.h; p2p.c): the router, asked with the UPnP
Internet Gateway Device protocol (port/third_party/miniupnpc), forwards a
UDP port of its internet address to the tunnel's socket here, so that a
peer whose NAT is too strict for hole punching can still reach this
machine there.

The router is looked for (an SSDP search on the local network, two seconds)
until one is found, and then kept; each forwarding asks for a lease of an
hour, which p2p.c renews (asking for the port it had), and falls back to a
permanent one where the router supports no other (it is removed when the
game exits). A router whose own internet address is a
private one (behind another NAT, such as a carrier's) cannot help, and is
not asked.

Built with the host's ABI, as the other posix_*.c (and, on Windows, with
the Windows SDK: port/windows/src/win32_upnp.c). One thread calls these at a
time.
*/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "posix.h"

#include "miniupnpc.h"
#include "upnpcommands.h"
#include "upnperrors.h"

enum
{
	/* milliseconds the router is looked for */
	DISCOVERY_DELAY = 2000,
	/* ports tried when the router already forwards the same one elsewhere */
	PORT_ATTEMPTS = 8,
};

#define FORWARDING_DESCRIPTION "Halo internet play"
#define LEASE_SECONDS "3600"

static struct
{
	int found;
	struct UPNPUrls urls;
	struct IGDdatas data;
	char lan_address[64];
	/* the router's answer: no leases but permanent ones */
	int permanent_only;
} upnp;

static unsigned short swap_short(unsigned short value)
{
	return (unsigned short)(value << 8 | value >> 8);
}

/* the router, looked for until it is found (the machine may have moved to
another network since); 1 if there is one */
static int upnp_find_router(char *error, int error_size)
{
	struct UPNPDev *devices;
	char wan_address[64];
	int discover_error = 0;
	int result;

	if (upnp.found)
		return 1;
	devices = upnpDiscover(DISCOVERY_DELAY, NULL, NULL, 0, 0, 2, &discover_error);
	if (!devices)
	{
		snprintf(error, (size_t)error_size, "no UPnP router on this network");
		return 0;
	}
	result = UPNP_GetValidIGD(devices, &upnp.urls, &upnp.data, upnp.lan_address, sizeof(upnp.lan_address),
		wan_address, sizeof(wan_address));
	freeUPNPDevlist(devices);
	switch (result)
	{
	case UPNP_CONNECTED_IGD:
		upnp.found = 1;
		return 1;
	case UPNP_PRIVATEIP_IGD:
		snprintf(error, (size_t)error_size,
			"the router's internet address (%s) is a private one: another NAT is beyond it", wan_address);
		break;
	case UPNP_DISCONNECTED_IGD:
		snprintf(error, (size_t)error_size, "the UPnP router is not connected to the internet");
		break;
	default:
		snprintf(error, (size_t)error_size, "no UPnP router on this network");
		break;
	}
	if (result)
		FreeUPNPUrls(&upnp.urls);
	return 0;
}

static int upnp_add(const char *external_port, const char *internal_port)
{
	int result;

	if (!upnp.permanent_only)
	{
		result = UPNP_AddPortMapping(upnp.urls.controlURL, upnp.data.first.servicetype, external_port,
			internal_port, upnp.lan_address, FORWARDING_DESCRIPTION, "UDP", NULL, LEASE_SECONDS);
		/* 725 OnlyPermanentLeasesSupported */
		if (result != 725)
			return result;
		upnp.permanent_only = 1;
	}
	return UPNP_AddPortMapping(upnp.urls.controlURL, upnp.data.first.servicetype, external_port, internal_port,
		upnp.lan_address, FORWARDING_DESCRIPTION, "UDP", NULL, "0");
}

int posix_upnp_forward_udp(unsigned short port, unsigned short preferred_port, posix_ulong *external_address,
	unsigned short *external_port, char *error, int error_size)
{
	char internal_text[8];
	char external_text[8];
	char address_text[64];
	unsigned int parts[4];
	unsigned short try_port = swap_short(preferred_port ? preferred_port : port);
	int attempt;
	int result = 0;

	if (!upnp_find_router(error, error_size))
		return 0;
	address_text[0] = 0;
	if (UPNP_GetExternalIPAddress(upnp.urls.controlURL, upnp.data.first.servicetype, address_text) != UPNPCOMMAND_SUCCESS ||
		sscanf(address_text, "%u.%u.%u.%u", &parts[0], &parts[1], &parts[2], &parts[3]) != 4 ||
		parts[0] > 255 || parts[1] > 255 || parts[2] > 255 || parts[3] > 255 || parts[0] == 0)
	{
		snprintf(error, (size_t)error_size, "the router did not say its internet address");
		return 0;
	}
	snprintf(internal_text, sizeof(internal_text), "%u", (unsigned int)swap_short(port));
	for (attempt = 0; attempt < PORT_ATTEMPTS; attempt++)
	{
		snprintf(external_text, sizeof(external_text), "%u", (unsigned int)try_port);
		result = upnp_add(external_text, internal_text);
		/* 718 ConflictInMappingEntry: another machine has that port; 724
		SamePortValuesRequired: this one only */
		if (result != 718 || attempt + 1 == PORT_ATTEMPTS)
			break;
		posix_random_bytes(&try_port, sizeof(try_port));
		try_port = (unsigned short)(49152 + try_port % 16384);
	}
	if (result != UPNPCOMMAND_SUCCESS)
	{
		snprintf(error, (size_t)error_size, "the router refused to forward a port (%d: %s)", result,
			strupnperror(result) ? strupnperror(result) : "unknown");
		return 0;
	}
	*external_address = (posix_ulong)(parts[0] | parts[1] << 8 | parts[2] << 16 | (posix_ulong)parts[3] << 24);
	*external_port = swap_short(try_port);
	return 1;
}

void posix_upnp_stop_forwarding_udp(unsigned short external_port)
{
	char external_text[8];

	if (!upnp.found)
		return;
	snprintf(external_text, sizeof(external_text), "%u", (unsigned int)swap_short(external_port));
	UPNP_DeletePortMapping(upnp.urls.controlURL, upnp.data.first.servicetype, external_text, "UDP", NULL);
}
