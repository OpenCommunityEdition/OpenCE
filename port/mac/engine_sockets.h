#ifndef HALO_MAC_ENGINE_SOCKETS_H
#define HALO_MAC_ENGINE_SOCKETS_H
/* Use the clean XDK's Winsock prototypes and structures, namespaced away from
   Darwin's BSD calls exactly as engine units see them in the native prefix. */
#ifndef __stdcall
#define __stdcall
#endif
#include "halo_port_limits.h"
#if defined(FD_SETSIZE) && FD_SETSIZE != HALO_PORT_FD_SETSIZE
#error Native game and platform headers disagree about the Winsock fd_set capacity
#endif
#ifndef FD_SETSIZE
#define FD_SETSIZE HALO_PORT_FD_SETSIZE
#endif
#include "halo_linux_winsock_names.h"
#include "xdk_win32.h"
#include "xdk_winsock.h"
#endif
