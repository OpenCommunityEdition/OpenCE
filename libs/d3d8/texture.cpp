#include "pixeljar.h"
#include "kernel_memory.h"
#pragma code_seg("D3D")
void WINAPI D3DTexture_GetLevelDesc(
    D3DTexture *texture,
    UINT level,
    D3DSURFACE_DESC *desc)
{
    D3D::PixelJar::Get2DSurfaceDesc(texture, level, desc);
    return;
}
void WINAPI D3DCubeTexture_GetLevelDesc(
    D3DCubeTexture *texture,
    UINT level,
    D3DSURFACE_DESC *desc)
{
    D3D::PixelJar::Get2DSurfaceDesc(texture, level, desc);
    return;
}
void WINAPI D3DVolumeTexture_GetLevelDesc(
    D3DVolumeTexture *texture,
    UINT level,
    D3DVOLUME_DESC *desc)
{
    D3D::PixelJar::Get3DSurfaceDesc(texture, level, desc);
    return;
}
void WINAPI D3DTexture_LockRect(
    D3DTexture *texture,
    UINT level,
    D3DLOCKED_RECT *locked,
    const RECT *rect,
    DWORD flags)
{
    D3D::PixelJar::Lock2DSurface(texture, D3DCUBEMAP_FACE_POSITIVE_X, level, locked, rect, flags);
    return;
}
void WINAPI D3DCubeTexture_LockRect(
    D3DCubeTexture *texture,
    D3DCUBEMAP_FACES face,
    UINT level,
    D3DLOCKED_RECT *locked,
    const RECT *rect,
    DWORD flags)
{
    D3D::PixelJar::Lock2DSurface(texture, face, level, locked, rect, flags);
    return;
}
void WINAPI D3DVolumeTexture_LockBox(
    D3DVolumeTexture *texture,
    UINT level,
    D3DLOCKED_BOX *locked,
    const D3DBOX *box,
    DWORD flags)
{
    D3D::PixelJar::Lock3DSurface(texture, level, locked, box, flags);
    return;
}
DWORD WINAPI D3DBaseTexture_GetLevelCount(
    D3DBaseTexture *texture)
{
    return (texture->Format & D3DFORMAT_MIPMAP_MASK) >> D3DFORMAT_MIPMAP_SHIFT;
}

namespace D3D
{
HRESULT WINAPI CreateTexture(
    DWORD width,
    DWORD height,
    DWORD depth,
    DWORD levels,
    DWORD usage,
    D3DFORMAT format,
    bool cubeMap,
    bool volume,
    D3DBaseTexture **result)
{
    DWORD encodedFormat, encodedSize;
    *result = NULL;
    DWORD allocationSize = PixelJar::EncodeFormat(width, height, depth, levels,
        format, 0, true, cubeMap, volume, &encodedFormat, &encodedSize);
    if (usage & 0x10000) encodedFormat &= ~D3DFORMAT_BORDERSOURCE_COLOR;
    D3DBaseTexture *texture = (D3DBaseTexture *)LocalAlloc(LMEM_ZEROINIT, sizeof(*texture));
    if (!texture) return E_OUTOFMEMORY;
    void *memory = MmAllocateContiguousMemoryEx(allocationSize, 0, 0x03ffb000,
        128, PAGE_READWRITE | PAGE_WRITECOMBINE);
    if (!memory)
    {
        LocalFree(texture);
        return E_OUTOFMEMORY;
    }
    texture->Data = (DWORD)memory & 0x03ffffff;
    texture->Common = 0x01040001;
    texture->Format = encodedFormat;
    texture->Size = encodedSize;
    texture->Lock = 0;
    *result = texture;
    return S_OK;
}
}

HRESULT WINAPI D3DTexture_GetSurfaceLevel(
    D3DTexture *texture,
    UINT level,
    D3DSurface **surface)
{
    BYTE *data;
    DWORD rowPitch, slicePitch, format, size;
    D3D::PixelJar::FindSurfaceWithinTexture(texture, D3DCUBEMAP_FACE_POSITIVE_X,
        level, &data, &rowPitch, &slicePitch, &format, &size);
    return D3D::CreateSurfaceOfTexture(format, size, texture, data, surface);
}
HRESULT WINAPI D3DCubeTexture_GetCubeMapSurface(
    D3DCubeTexture *texture,
    D3DCUBEMAP_FACES face,
    UINT level,
    D3DSurface **surface)
{
    BYTE *data;
    DWORD rowPitch, slicePitch, format, size;
    D3D::PixelJar::FindSurfaceWithinTexture(texture, face, level,
        &data, &rowPitch, &slicePitch, &format, &size);
    return D3D::CreateSurfaceOfTexture(format, size, texture, data, surface);
}
HRESULT WINAPI D3DVolumeTexture_GetVolumeLevel(
    D3DVolumeTexture *texture,
    UINT level,
    D3DVolume **volume)
{
    BYTE *data;
    DWORD rowPitch, slicePitch, format, size;
    D3D::PixelJar::FindSurfaceWithinTexture(texture, D3DCUBEMAP_FACE_POSITIVE_X,
        level, &data, &rowPitch, &slicePitch, &format, &size);
    return D3D::CreateVolumeOfTexture(format, texture, data, volume);
}
