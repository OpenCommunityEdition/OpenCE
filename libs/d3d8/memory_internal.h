#ifndef D3D8_MEMORY_INTERNAL_H
#define D3D8_MEMORY_INTERNAL_H
#include <xtl.h>
#include "kernel_memory.h"
#include "xmetal_memory_internal.h"
void *WINAPI D3D_AllocContiguousMemory(
    DWORD size,
    DWORD alignment);
void WINAPI D3D_FreeContiguousMemory(
    void *memory);
extern "C" void *WINAPI D3D_AllocNoncontiguousMemory(
    DWORD size);
extern "C" HLOCAL WINAPI D3D_FreeNoncontiguousMemory(
    void *memory);
namespace D3D
{
/* PDB-private January allocator used by the pixel/resource creation family.
 * The top 20 KiB remain reserved for GPU instance memory. */
static __inline void *WINAPI AllocateContiguousMemory(
    DWORD size,
    DWORD alignment)
{
    return MmAllocateContiguousMemoryEx(size, 0, 0x03ffb000,
        alignment, PAGE_READWRITE | PAGE_WRITECOMBINE);
}
static __inline void WINAPI FreeContiguousMemory(
    void *memory)
{
    MmFreeContiguousMemory(memory);
    return;
}
void WINAPI GPUCopyMemory(
    DWORD sourceOffset,
    DWORD destinationOffset,
    DWORD size);
void WINAPI GPUCopyVideoRectangle(
    DWORD bytesPerPixel,
    DWORD sourceOffset,
    DWORD sourcePitch,
    DWORD destinationOffset,
    DWORD destinationPitch,
    DWORD count,
    RECT *rectangles,
    POINT *points);
}
#endif
