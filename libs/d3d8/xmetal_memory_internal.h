#ifndef D3D8_XMETAL_MEMORY_INTERNAL_H
#define D3D8_XMETAL_MEMORY_INTERNAL_H
#include "device_layout.h"
/* Original module-local XMETAL address helpers. In this retail build the
 * contiguous-memory diagnostic has no instructions other than its return.
 * The physical map is the January 64 MiB map, not a later SDK aperture. */
extern "C"
{
static __inline void WINAPI XMETAL_DebugVerifyContiguous(
    void *memory)
{
    return;
}
/* Original retail physical-address diagnostic is also an empty check. */
static __inline void WINAPI XMETAL_DebugVerifyPhysical(
    DWORD address)
{
    return;
}
static __inline void *WINAPI XMETAL_MapToContiguousAddress(
    DWORD address)
{
    XMETAL_DebugVerifyPhysical(address);
    return (void *)(address | 0x80000000UL);
}
static __inline void *WINAPI XMETAL_MapToVideoAddress(
    DWORD address)
{
    XMETAL_DebugVerifyPhysical(address);
    return (void *)(address | 0x80000000UL);
}
static __inline DWORD WINAPI XMETAL_MapToPhysicalOffset(
    void *memory)
{
    XMETAL_DebugVerifyContiguous(memory);
    return (DWORD)memory & 0x03ffffffUL;
}
}
#endif
