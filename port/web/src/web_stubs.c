/*
WEB_STUBS.C

What the platform layer links against that a browser does not have:
internet play's router (UPnP, port/linux/src/posix_upnp.c), which stays off
in the web build (port/web/src/web_main.c).
*/

#include <string.h>

#include "posix.h"

int posix_upnp_forward_udp(unsigned short port, unsigned short preferred_port, posix_ulong *external_address,
	unsigned short *external_port, char *error, int error_size)
{
	(void)port;
	(void)preferred_port;
	(void)external_address;
	(void)external_port;
	if (error && error_size > 0)
	{
		strncpy(error, "a browser cannot reach the router", (size_t)error_size - 1);
		error[error_size - 1] = '\0';
	}
	return 0;
}

void posix_upnp_stop_forwarding_udp(unsigned short external_port)
{
	(void)external_port;
}

/* ---------- MSVC (scenario.c): a compiler barrier, an intrinsic clang knows
only on x86 and ARM */

void _ReadWriteBarrier(void)
{
	__atomic_signal_fence(__ATOMIC_SEQ_CST);
}
