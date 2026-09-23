#ifndef D3D8_MEMORY_INTERNAL_H
#define D3D8_MEMORY_INTERNAL_H
#include <xtl.h>
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
