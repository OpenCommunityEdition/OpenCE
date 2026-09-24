#ifndef D3D8_CHECKSUM_INTERNAL_H
#define D3D8_CHECKSUM_INTERNAL_H
#include "device_layout.h"
namespace D3D
{
/* The test callback advances one of three color channels per invocation.
 * These original test-only globals are shared across the kernel wait boundary.
 * Callers serialize checksum requests; overlapping requests are unsupported. */
extern DWORD g_DoChecksum;
extern DWORD g_Channel;
extern DWORD g_Value[3];
}
extern "C" void __cdecl D3DTest_ChecksumCallback(
    DWORD frame);
extern "C" void WINAPI D3DTest_GetScreenChecksum(
    DWORD *red,
    DWORD *green,
    DWORD *blue);
#endif
