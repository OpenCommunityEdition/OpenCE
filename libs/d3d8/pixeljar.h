/* Pixel helpers adapted from RXDK-Libs, 2026 Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * See libs/d3d8/PROVENANCE.md.
 */
#ifndef D3D8_PIXELJAR_H
#define D3D8_PIXELJAR_H
#include <xtl.h>
namespace D3D
{
DWORD __fastcall Log2(
    DWORD value);
extern const BYTE g_TextureFormat[];
extern const DWORD g_TilePitches[16];
D3DFORMAT WINAPI MapToLinearD3DFORMAT(
    D3DFORMAT format);
bool WINAPI IsCompressedD3DFORMAT(
    DWORD format);
DWORD WINAPI CalcPitch(
    DWORD width,
    DWORD texelSize);
DWORD WINAPI VideoBitsPerPixelOfD3DFORMAT(
    DWORD format);
DWORD WINAPI CalcTilePitch(
    DWORD width,
    D3DFORMAT format);
__inline DWORD WINAPI BitsPerPixelOfD3DFORMAT(
    DWORD format)
{
    return g_TextureFormat[format] & 0x3c;
}
__inline DWORD WINAPI MinimumTextureSizeOfD3DFORMAT(
    DWORD format)
{
    return IsCompressedD3DFORMAT(format) ? 2 : 0;
}
namespace PixelJar
{
__inline BOOL WINAPI IsSwizzled(
    D3DPixelContainer *pixel)
{
    return g_TextureFormat[(pixel->Format & D3DFORMAT_FORMAT_MASK) >> D3DFORMAT_FORMAT_SHIFT] & 1;
}
__inline bool WINAPI IsCompressed(
    D3DPixelContainer *pixel)
{
    return IsCompressedD3DFORMAT((pixel->Format & D3DFORMAT_FORMAT_MASK) >> D3DFORMAT_FORMAT_SHIFT);
}
__inline bool WINAPI IsSurface(
    D3DResource *resource)
{
    return (resource->Common & D3DCOMMON_TYPE_MASK) == D3DCOMMON_TYPE_SURFACE;
}
__inline D3DBaseTexture *WINAPI GetParent(
    D3DResource *resource)
{
    if (IsSurface(resource))
    {
        return ((D3DSurface *)resource)->Parent;
    }
    return NULL;
}
__inline D3DFORMAT WINAPI GetFormat(
    D3DPixelContainer *pixel)
{
    return (D3DFORMAT)((pixel->Format & D3DFORMAT_FORMAT_MASK) >> D3DFORMAT_FORMAT_SHIFT);
}
__inline DWORD WINAPI GetLogWidth(
    D3DPixelContainer *pixel)
{
    return (pixel->Format & D3DFORMAT_USIZE_MASK) >> D3DFORMAT_USIZE_SHIFT;
}
__inline DWORD WINAPI GetLogHeight(
    D3DPixelContainer *pixel)
{
    return (pixel->Format & D3DFORMAT_VSIZE_MASK) >> D3DFORMAT_VSIZE_SHIFT;
}
__inline DWORD WINAPI GetLogDepth(
    D3DPixelContainer *pixel)
{
    return (pixel->Format & D3DFORMAT_PSIZE_MASK) >> D3DFORMAT_PSIZE_SHIFT;
}
__inline DWORD WINAPI GetBitsPerPixel(
    D3DPixelContainer *pixel)
{
    return BitsPerPixelOfD3DFORMAT(GetFormat(pixel));
}
__inline BOOL WINAPI IsValidRenderTarget(
    D3DPixelContainer *pixel)
{
    return g_TextureFormat[GetFormat(pixel)] & 0x80;
}
__inline BOOL WINAPI IsValidDepthBuffer(
    D3DPixelContainer *pixel)
{
    return g_TextureFormat[GetFormat(pixel)] & 0x40;
}
__inline DWORD WINAPI GetMipmapLevelCount(
    D3DPixelContainer *pixel)
{
    return (pixel->Format & D3DFORMAT_MIPMAP_MASK) >> D3DFORMAT_MIPMAP_SHIFT;
}
__inline DWORD WINAPI PitchFromSize(
    DWORD size)
{
    return (((size & D3DSIZE_PITCH_MASK) >> D3DSIZE_PITCH_SHIFT) + 1) * D3DTEXTURE_PITCH_ALIGNMENT;
}
/* Original header ABI is stdcall (PDB/decorated target definitions).
 * These are genuine layout accessors used by viewport and copy operations. */
__inline DWORD WINAPI GetWidth(
    D3DPixelContainer *pixel)
{
    DWORD size = pixel->Size;
    if (!size) { return 1UL << GetLogWidth(pixel); }
    return (size & D3DSIZE_WIDTH_MASK) + 1;
}
__inline DWORD WINAPI GetHeight(
    D3DPixelContainer *pixel)
{
    DWORD size = pixel->Size;
    if (!size) { return 1UL << GetLogHeight(pixel); }
    return ((size & D3DSIZE_HEIGHT_MASK) >> D3DSIZE_HEIGHT_SHIFT) + 1;
}
__inline DWORD WINAPI GetDepth(
    D3DPixelContainer *pixel)
{
    if (!pixel->Size) { return 1UL << GetLogDepth(pixel); }
    return 1;
}
__inline DWORD WINAPI GetPitch(
    D3DPixelContainer *pixel)
{
    DWORD size = pixel->Size;
    if (!size)
    {
        DWORD width = GetWidth(pixel);
        D3DFORMAT format = GetFormat(pixel);
        switch (format)
        {
        case D3DFMT_DXT1: return width * 2;
        case D3DFMT_DXT2:
        case D3DFMT_DXT4: return width * 4;
        default: return width * BitsPerPixelOfD3DFORMAT(format) / 8;
        }
    }
    return PitchFromSize(
        size);
}
__inline DWORD WINAPI GetSlice(
    D3DPixelContainer *pixel)
{
    DWORD bits = GetBitsPerPixel(pixel);
    DWORD size = pixel->Size;
    DWORD logMin = MinimumTextureSizeOfD3DFORMAT(GetFormat(pixel));
    if (!size)
    {
        DWORD logWidth = max(logMin, GetLogWidth(pixel));
        DWORD logHeight = max(logMin, GetLogHeight(pixel));
        return (1UL << (logWidth + logHeight)) * bits / 8;
    }
    return (((size & D3DSIZE_HEIGHT_MASK) >> D3DSIZE_HEIGHT_SHIFT) + 1) * PitchFromSize(
        size);
}
DWORD WINAPI EncodeFormat(
    DWORD width,
    DWORD height,
    DWORD depth,
    DWORD levels,
    D3DFORMAT d3dFormat,
    DWORD pitch,
    bool videoMemory,
    bool cubeMap,
    bool volume,
    DWORD *outFormat,
    DWORD *outSize);
void WINAPI GetSize(
    D3DPixelContainer *pixel,
    DWORD level,
    DWORD *width,
    DWORD *height,
    DWORD *depth,
    DWORD *pitch,
    DWORD *slice);
DWORD WINAPI GetSurfaceFormat(
    D3DPixelContainer *renderTarget,
    D3DPixelContainer *zBuffer);
void WINAPI GetDepthBufferScale(
    D3DPixelContainer *pixel,
    float *scale);
void WINAPI Get2DSurfaceDesc(
    D3DPixelContainer *pixel,
    UINT level,
    D3DSURFACE_DESC *desc);
void WINAPI Get3DSurfaceDesc(
    D3DPixelContainer *pixel,
    UINT level,
    D3DVOLUME_DESC *desc);
void WINAPI Lock2DSurface(
    D3DPixelContainer *pixel,
    D3DCUBEMAP_FACES face,
    UINT level,
    D3DLOCKED_RECT *locked,
    const RECT *rect,
    DWORD flags);
void WINAPI Lock3DSurface(
    D3DPixelContainer *pixel,
    UINT level,
    D3DLOCKED_BOX *locked,
    const D3DBOX *box,
    DWORD flags);
void WINAPI FindSurfaceWithinTexture(
    D3DPixelContainer *pixel,
    D3DCUBEMAP_FACES face,
    UINT level,
    BYTE **data,
    DWORD *rowPitch,
    DWORD *slicePitch,
    DWORD *format,
    DWORD *size);
}
HRESULT WINAPI CreateTexture(
    DWORD width,
    DWORD height,
    DWORD depth,
    DWORD levels,
    DWORD usage,
    D3DFORMAT format,
    bool cubeMap,
    bool volume,
    D3DBaseTexture **texture);
HRESULT WINAPI CreateStandAloneSurface(
    DWORD width,
    DWORD height,
    D3DFORMAT format,
    bool renderTarget,
    D3DSurface **surface);
HRESULT WINAPI CreateSurfaceOfTexture(
    DWORD format,
    DWORD size,
    D3DBaseTexture *texture,
    void *data,
    D3DSurface **surface);
HRESULT WINAPI CreateVolumeOfTexture(
    DWORD format,
    D3DBaseTexture *texture,
    void *data,
    D3DVolume **volume);
}
#endif
