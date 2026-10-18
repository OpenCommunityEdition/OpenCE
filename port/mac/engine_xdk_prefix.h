#ifndef HALO_MAC_ENGINE_XDK_PREFIX_H
#define HALO_MAC_ENGINE_XDK_PREFIX_H
/* Clean declarations only: native Mac calls use the platform's C ABI on both
   PPC and Intel. This provides no Xbox platform implementations or SEH. */
#include "engine_prefix.h"
#include "halo_port_limits.h"
/* Keep every game translation unit on the same Winsock fd_set capacity.
   This must precede any clean XDK declarations that include xdk_win32.h. */
#if defined(FD_SETSIZE) && FD_SETSIZE != HALO_PORT_FD_SETSIZE
#error Native game and platform headers disagree about the Winsock fd_set capacity
#endif
#ifndef FD_SETSIZE
#define FD_SETSIZE HALO_PORT_FD_SETSIZE
#endif
struct memory_status;
#include "cseries.h"
#define __stdcall
#define __fastcall
#define __cdecl
#define __forceinline __inline__
#define DECLSPEC_SELECTANY __attribute__((weak))
/* Native builds validate device ownership and the supported resource
   subset before reaching the shared Xbox-format allocation routines. */
#define D3DDevice_CreateTexture mac_engine_device_create_texture
#define D3DDevice_CreateCubeTexture mac_engine_device_create_cube_texture
#define D3DDevice_CreateVolumeTexture mac_engine_device_create_volume_texture
#undef htonl
#undef htons
#undef ntohl
#undef ntohs
#include "halo_linux_winsock_names.h"
/* GCC 4 rejects even identical duplicate typedefs. Keep the clean PDB aliases
   distinct while reading its declarations; both remain unsigned-char types. */
#define boolean halo_mac_xdk_boolean
#define byte halo_mac_xdk_byte
#include "xtl.h"
#undef byte
#undef boolean
/* The clean inline wrapper is included by each native engine unit. */
#pragma weak D3DDevice_SetRenderState
#pragma weak D3DXMatrixIdentity
#pragma weak D3DXMatrixIdentity
#endif
