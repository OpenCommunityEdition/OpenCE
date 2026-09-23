#ifndef HALO_D3D8_SURFACE_INTERNAL_H
#define HALO_D3D8_SURFACE_INTERNAL_H
#include <xtl.h>
namespace D3D
{
/* The January contiguous allocation reserves a 64-byte prefix for its header. */
enum { SURFACE_CONTIGUOUS_HEADER_BYTES = 64 };
void WINAPI InitializeSurface(
    D3DSurface *surface,
    DWORD format,
    DWORD size,
    void *memory);
HRESULT WINAPI CreateSurfaceWithContiguousHeader(
    DWORD width,
    DWORD height,
    D3DFORMAT format,
    D3DSurface **result);
}
#endif
