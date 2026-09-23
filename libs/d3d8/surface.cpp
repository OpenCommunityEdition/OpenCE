#include "pixeljar.h"
#include "resource_internal.h"
#include "kernel_memory.h"
#include "surface_internal.h"
#pragma code_seg("D3D")
void WINAPI D3DSurface_GetDesc(
    D3DSurface *surface,
    D3DSURFACE_DESC *desc)
{
    D3D::PixelJar::Get2DSurfaceDesc(surface, 0, desc);
    return;
}
void WINAPI D3DSurface_LockRect(
    D3DSurface *surface,
    D3DLOCKED_RECT *locked,
    const RECT *rect,
    DWORD flags)
{
    D3D::PixelJar::Lock2DSurface(surface, D3DCUBEMAP_FACE_POSITIVE_X, 0, locked, rect, flags);
    return;
}
void WINAPI D3DVolume_GetDesc(
    D3DVolume *volume,
    D3DVOLUME_DESC *desc)
{
    D3D::PixelJar::Get3DSurfaceDesc(volume, 0, desc);
    return;
}
void WINAPI D3DVolume_LockBox(
    D3DVolume *volume,
    D3DLOCKED_BOX *locked,
    const D3DBOX *box,
    DWORD flags)
{
    D3D::PixelJar::Lock3DSurface(volume, 0, locked, box, flags);
    return;
}


namespace D3D
{
HRESULT WINAPI CreateSurfaceOfTexture(
    DWORD format,
    DWORD size,
    D3DBaseTexture *texture,
    void *data,
    D3DSurface **result)
{
    D3DSurface *surface = (D3DSurface *)LocalAlloc(LMEM_ZEROINIT, sizeof(*surface));
    if (!surface) return E_OUTOFMEMORY;
    surface->Data = (DWORD)data & 0x03ffffff;
    surface->Common = 0x01050001;
    surface->Format = format;
    surface->Size = size;
    surface->Parent = texture;
    surface->Lock = 0;
    texture->AddRef();
    *result = surface;
    return S_OK;
}
HRESULT WINAPI CreateVolumeOfTexture(
    DWORD format,
    D3DBaseTexture *texture,
    void *data,
    D3DVolume **result)
{
    D3DVolume *volume = (D3DVolume *)LocalAlloc(LMEM_ZEROINIT, sizeof(*volume));
    if (!volume) return E_OUTOFMEMORY;
    volume->Data = (DWORD)data & 0x03ffffff;
    volume->Common = 0x01050001;
    volume->Format = format;
    volume->Size = 0;
    volume->Parent = texture;
    volume->Lock = 0;
    texture->AddRef();
    *result = volume;
    return S_OK;
}
HRESULT WINAPI CreateStandAloneSurface(
    DWORD width,
    DWORD height,
    D3DFORMAT format,
    bool renderTarget,
    D3DSurface **result)
{
    DWORD encodedFormat, encodedSize;
    DWORD allocationSize = PixelJar::EncodeFormat(width, height, 1, 1, format, 0,
        renderTarget, false, false, &encodedFormat, &encodedSize);
    D3DSurface *surface = (D3DSurface *)LocalAlloc(LMEM_ZEROINIT, sizeof(*surface));
    if (!surface) return E_OUTOFMEMORY;
    void *memory = MmAllocateContiguousMemoryEx(allocationSize, 0, 0x03ffb000, 64,
        PAGE_READWRITE | PAGE_WRITECOMBINE);
    if (!memory)
    {
        LocalFree(surface);
        return E_OUTOFMEMORY;
    }
    surface->Data = (DWORD)memory & 0x03ffffff;
    surface->Common = 0x81050001;
    surface->Format = encodedFormat;
    surface->Size = encodedSize;
    surface->Parent = NULL;
    surface->Lock = 0;
    *result = surface;
    return S_OK;
}
}

namespace D3D
{
void WINAPI InitializeSurface(
    D3DSurface *surface,
    DWORD format,
    DWORD size,
    void *memory)
{
    surface->Data = (DWORD)memory & 0x03ffffff;
    surface->Size = size;
    surface->Common = D3DCOMMON_D3DCREATED | D3DCOMMON_TYPE_SURFACE | 1;
    surface->Format = format;
    surface->Parent = NULL;
    surface->Lock = 0;
    return;
}
HRESULT WINAPI CreateSurfaceWithContiguousHeader(
    DWORD width,
    DWORD height,
    D3DFORMAT format,
    D3DSurface **result)
{
    DWORD encodedFormat, encodedSize;
    DWORD allocationSize = PixelJar::EncodeFormat(width, height, 1, 1,
        format, 0, true, false, false, &encodedFormat, &encodedSize);
    /* The original allocation reserves an aligned 64-byte header prefix.
     * The returned surface occupies only the first sizeof(D3DSurface) bytes. */
    D3DSurface *surface = (D3DSurface *)MmAllocateContiguousMemoryEx(
        allocationSize + SURFACE_CONTIGUOUS_HEADER_BYTES, 0, 0x03ffb000,
        D3DSURFACE_ALIGNMENT, PAGE_READWRITE | PAGE_WRITECOMBINE);
    if (!surface)
    {
        return E_OUTOFMEMORY;
    }
    void *pixels = (BYTE *)surface + SURFACE_CONTIGUOUS_HEADER_BYTES;
    surface->Common = D3DCOMMON_D3DCREATED | D3DCOMMON_TYPE_SURFACE | D3DSURFACE_OWNSMEMORY | 1;
    surface->Data = (DWORD)pixels & 0x03ffffff;
    surface->Format = encodedFormat;
    surface->Size = encodedSize;
    surface->Parent = NULL;
    surface->Lock = 0;
    *result = surface;
    return S_OK;
}
}
HRESULT WINAPI D3DSurface_GetContainer(
    D3DSurface *surface,
    D3DBaseTexture **container)
{
    *container = surface->Parent;
    if (*container)
    {
        (*container)->AddRef();
    }
    return *container ? S_OK : E_FAIL;
}
HRESULT WINAPI D3DVolume_GetContainer(
    D3DVolume *volume,
    D3DBaseTexture **container)
{
    *container = volume->Parent;
    if (*container)
    {
        (*container)->AddRef();
    }
    return *container ? S_OK : E_FAIL;
}
