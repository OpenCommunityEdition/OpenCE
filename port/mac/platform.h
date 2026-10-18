/*
PLATFORM.H (the Mac's)

port/linux/src/platform.h is the shared platform layer's own header. It
reads the XDK headers (xtl.h, xbdm.h, d3d8perf.h) and declares that layer
whole: handles, asynchronous procedure calls, contiguous memory, the
renderer's page watch, paths and time. The Mac has its own window, files,
sockets and memory, and port/mac's units are compiled without the game's
XDK prefix, so reading that header here would pull in the whole SDK header
set for the sake of two declarations, and tie four more files to it.

This stands in its place for internet play's sources — p2p.c,
p2p_discord.c, p2p_lobby.c and p2p_signal.c. It mirrors only what those
four use, and deliberately nothing else.

How it takes the name. Those four say #include "platform.h", and a quoted
include is looked for in the including file's own directory first; they
live in port/linux/src, beside the real one, so no -I order can put this
header in front. Instead the Mac build compiles those four with
-include port/mac/platform.h, which reads this first, and this defines the
real header's include guard below, so that when their own line reaches it
the file is read and yields nothing. The XDK headers are never opened.

Mirrored from port/linux/src/platform.h:

	void platform_log(const char *format, ...)    36 uses in the four
	const char *platform_data_root(void)           3 uses in the four

and, for the mods (port/linux/src/port_mods.c), the three below it.

Nothing else in that header is named by them, which is why nothing else is
here. Two units in port/linux/src keep this honest on every Linux build and in
CI, where the Mac's own compiler cannot reach:
mac_platform_shim_check.c reads the real header and then this one, so a
prototype that has drifted apart is a compile error; and
mac_platform_suppress_check.c reads this one and then the real header, and
fails if the real header's body is no longer suppressed.

WSAEWOULDBLOCK and WSAEINPROGRESS are written out below rather than taken
from xdk_winsock.h. Including that header here would be the same mistake
in a smaller space: it declares Winsock's ntohl, htonl and timeval, which
collide with Darwin's own in sys/_endian.h and sys/resource.h unless the
XDK prefix's renaming has been done first, and port/mac's platform-layer
units do not do it. port/linux/src/posix_net.c and port/web/src/web_net.c
already write these two numbers out for the same reason. The check unit
named above compares them with xdk_winsock.h's, on Linux, where reading
that header costs nothing.
*/

#ifndef __HALO_MAC_PLATFORM_H
#define __HALO_MAC_PLATFORM_H

/* port/linux/src/platform.h's own guard. Defining it here is what makes
that header empty when internet play's sources include it by name: this
header has already answered for it. It is the one piece of this file that
knows anything about the original beyond the two prototypes, so if that
guard is ever renamed this stops suppressing it and the Mac build starts
reading the XDK headers again. port/linux/src/mac_platform_suppress_check.c
is the unit that fails when that happens. */
#define __HALO_LINUX_PLATFORM_H

/* The sockets p2p names directly: struct sockaddr_in, AF_INET,
SOCK_STREAM, SOCK_DGRAM, SOL_SOCKET, SO_SNDBUF and SO_RCVBUF. On Linux
these reach it through the XDK's Winsock declarations, which platform.h
reads; here they are Darwin's own, which is right rather than merely
convenient, because every socket call p2p makes goes to the posix_*
functions in port/linux/src/posix_net.c and those are BSD sockets
underneath. No byte-order function is in that list: p2p does its own.

Not taken when this header is read on Linux by the two check units in
port/linux/src, where these names belong to the XDK instead and declaring
both would be the collision this header exists to avoid. */
#if defined(__APPLE__)
#include <sys/socket.h>
#include <netinet/in.h>
#endif

/* The two Winsock errors p2p tests a connect that is still under way
against (p2p.c, p2p_signal.c). Winsock's own numbers, as xdk_winsock.h
gives them; mac_platform_shim_check.c is what holds them to it. */
#ifndef WSAEWOULDBLOCK
#define WSAEWOULDBLOCK 10035
#endif
#ifndef WSAEINPROGRESS
#define WSAEINPROGRESS 10036
#endif

/* prints the message and a newline to stderr. The Linux layer's prefixes
it with "halo-linux: "; the Mac's (port/mac/player_services.c) does not,
which is a difference in what is printed, not in how it is called. */
void platform_log(const char *format, ...) __attribute__((format(printf, 1, 2)));

/* the folder the game's data is under, without a trailing slash. Internet
play reads and writes join_link.txt here, which is how a launcher hands
the running game a halo:// invite. port/mac/platform.c answers it from the
Mac's own mac_engine_files_data_root. */
const char *platform_data_root(void);

/* whether an address is in the Xbox's contiguous memory, which the renderer
watches (port/mac/native_resource_memory.c), and the call that tells it a
write is coming. The mods' file reads go through both. */
int platform_is_contiguous(const void *address);
void memory_watch_prepare_write(void *address, unsigned long size);

/* asks the player a question in a box with a button for each label: the
button chosen, or -1 if it cannot ask. The Mac cannot yet (port/mac/platform.c),
so the mods that cannot be played together are all played, as a run nobody
watches plays them, and the Mods menu turns them off. */
int platform_ask(const char *title, const char *text, const char *const *labels, int count, int default_index);

#endif
