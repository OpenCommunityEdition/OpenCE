/* Pixel-container reconstruction against the January Halo binary.
 * Algorithms adapted from RXDK-Libs, 2026 Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Target-specific APIs and layouts differ from the reference.
 */
#include "device_layout.h"
#include "pixeljar.h"
#include "resource_internal.h"
#pragma code_seg("D3D")
namespace D3D
{
/* Period math helper and target both use BSF. Keep an explicit C return. */
DWORD __fastcall Log2(
    DWORD value)
{
    DWORD result;
    __asm
    {
        mov ecx, value
        bsf eax, ecx
        mov result, eax
    }
    return result;
}

D3DFORMAT WINAPI MapToLinearD3DFORMAT(
    D3DFORMAT format)
{
    switch (format)
    {
    case D3DFMT_A8R8G8B8: format = D3DFMT_LIN_A8R8G8B8; break;
    case D3DFMT_X8R8G8B8: format = D3DFMT_LIN_X8R8G8B8; break;
    case D3DFMT_R5G6B5: format = D3DFMT_LIN_R5G6B5; break;
    case D3DFMT_X1R5G5B5: format = D3DFMT_LIN_X1R5G5B5; break;
    case D3DFMT_A1R5G5B5: format = D3DFMT_LIN_A1R5G5B5; break;
    case D3DFMT_D16: format = D3DFMT_LIN_D16; break;
    case D3DFMT_D24S8: format = D3DFMT_LIN_D24S8; break;
    case D3DFMT_F16: format = D3DFMT_LIN_F16; break;
    case D3DFMT_F24S8: format = D3DFMT_LIN_F24S8; break;
    }
    return format;
}
bool WINAPI IsCompressedD3DFORMAT(
    DWORD format)
{
    bool compressed = false;
    switch (format)
    {
    case D3DFMT_DXT1:
    case D3DFMT_DXT2:
    case D3DFMT_DXT4:
        compressed = true;
        break;
    }
    return compressed;
}
DWORD WINAPI CalcPitch(
    DWORD width,
    DWORD texelSize)
{
    return (width * texelSize / 8 + D3DTEXTURE_PITCH_ALIGNMENT - 1) &
           ~(D3DTEXTURE_PITCH_ALIGNMENT - 1);
}
DWORD WINAPI VideoBitsPerPixelOfD3DFORMAT(
    DWORD format)
{
    DWORD bitsPerPixel;
    if (format == D3DFMT_X1R5G5B5 || format == D3DFMT_LIN_X1R5G5B5)
    {
        bitsPerPixel = 15;
    }
    else
    {
        bitsPerPixel = BitsPerPixelOfD3DFORMAT(format);
    }
    return bitsPerPixel;
}
DWORD WINAPI CalcTilePitch(
    DWORD width,
    D3DFORMAT format)
{
    DWORD pitch = CalcPitch(width, BitsPerPixelOfD3DFORMAT(format));
    DWORD i;
    for (i = 0; i < 16; i++)
    {
        if (pitch <= g_TilePitches[i])
        {
            pitch = g_TilePitches[i];
            break;
        }
    }
    return pitch;
}
namespace PixelJar
{
void WINAPI GetDepthBufferScale(
    D3DPixelContainer *pixel,
    float *scale)
{
    switch (GetFormat(pixel))
    {
    case D3DFMT_D16:
    case D3DFMT_LIN_D16:
        *scale = (float)D3DZ_MAX_D16;
        break;
    case D3DFMT_D24S8:
    case D3DFMT_LIN_D24S8:
        *scale = (float)D3DZ_MAX_D24S8;
        break;
    case D3DFMT_F16:
    case D3DFMT_LIN_F16:
        *scale = (float)D3DZ_MAX_F16;
        break;
    case D3DFMT_F24S8:
    case D3DFMT_LIN_F24S8:
        *scale = (float)D3DZ_MAX_F24S8;
        break;
    }
    return;
}
void WINAPI GetSize(
    D3DPixelContainer *pixel,
    DWORD level,
    DWORD *width,
    DWORD *height,
    DWORD *depth,
    DWORD *pitch,
    DWORD *slice)
{
    DWORD size = pixel->Size;
    if (!size)
    {
        long logWidth = GetLogWidth(pixel) - level;
        long logHeight = GetLogHeight(pixel) - level;
        long logDepth = GetLogDepth(pixel) - level;
        DWORD bitsPerPixel = GetBitsPerPixel(pixel);
        *height = 1 << (logHeight > 0 ? logHeight : 0);
        *depth = 1 << (logDepth > 0 ? logDepth : 0);
        *width = 1 << (logWidth > 0 ? logWidth : 0);
        long logMin = (long)MinimumTextureSizeOfD3DFORMAT(GetFormat(pixel));
        DWORD physicalWidth = 1 << max(logMin, logWidth);
        DWORD physicalHeight = 1 << max(logMin, logHeight);
        switch (GetFormat(pixel))
        {
        case D3DFMT_DXT1:
            *pitch = physicalWidth * 2;
            break;
        case D3DFMT_DXT2:
        case D3DFMT_DXT4:
            *pitch = physicalWidth * 4;
            break;
        default:
            *pitch = physicalWidth * bitsPerPixel / 8;
            break;
        }
        *slice = physicalWidth * physicalHeight * bitsPerPixel / 8;
    }
    else
    {
        *width = (size & D3DSIZE_WIDTH_MASK) + 1;
        *height = ((size & D3DSIZE_HEIGHT_MASK) >> D3DSIZE_HEIGHT_SHIFT) + 1;
        *depth = 1;
        *pitch = PitchFromSize(size);
        *slice = *pitch * *height;
    }
    return;
}
void WINAPI Get2DSurfaceDesc(
    D3DPixelContainer *pixel,
    UINT level,
    D3DSURFACE_DESC *desc)
{
    desc->Format = GetFormat(pixel);
    desc->Type = pixel->GetType();
    desc->Usage = 0;
    if (level == 0)
    {
        if (IsValidRenderTarget(pixel))
        {
            desc->Usage = D3DUSAGE_RENDERTARGET;
        }
        else if (IsValidDepthBuffer(pixel))
        {
            desc->Usage = D3DUSAGE_DEPTHSTENCIL;
        }
    }
    if (pixel->Data == g_pDevice->m_FrameBufferSurfaces[0].Data &&
        (D3D__RenderState[D3DRS_MULTISAMPLETYPE] & 0x3000))
    {
        desc->MultiSampleType = (D3DMULTISAMPLE_TYPE)D3D__RenderState[D3DRS_MULTISAMPLETYPE];
    }
    else
    {
        desc->MultiSampleType = D3DMULTISAMPLE_NONE;
    }
    DWORD width, height, depth, pitch, slice;
    GetSize(pixel, level, &width, &height, &depth, &pitch, &slice);
    desc->Width = width;
    desc->Height = height;
    desc->Size = slice;
    return;
}
void WINAPI Get3DSurfaceDesc(
    D3DPixelContainer *pixel,
    UINT level,
    D3DVOLUME_DESC *desc)
{
    desc->Format = GetFormat(pixel);
    desc->Type = pixel->GetType();
    desc->Usage = 0;
    DWORD width, height, depth, pitch, slice;
    GetSize(pixel, level, &width, &height, &depth, &pitch, &slice);
    desc->Width = width;
    desc->Height = height;
    desc->Depth = depth;
    desc->Size = slice * depth;
    return;
}
void WINAPI FindSurfaceWithinTexture(
    D3DPixelContainer *pixel,
    D3DCUBEMAP_FACES face,
    UINT level,
    BYTE **outData,
    DWORD *outRowPitch,
    DWORD *outSlicePitch,
    DWORD *outFormat,
    DWORD *outSize)
{
    BYTE *data = GetDataFromResource(pixel);
    DWORD texelSize = GetBitsPerPixel(pixel);
    DWORD rowPitch, slicePitch;
    DWORD size = pixel->Size;
    if (!size)
    {
        DWORD logWidth = GetLogWidth(pixel);
        DWORD logHeight = GetLogHeight(pixel);
        DWORD logDepth = GetLogDepth(pixel);
        DWORD logMin = MinimumTextureSizeOfD3DFORMAT(GetFormat(pixel));
        if (face != D3DCUBEMAP_FACE_POSITIVE_X)
        {
            DWORD levels = GetMipmapLevelCount(pixel);
            DWORD faceWidth = logWidth;
            DWORD faceHeight = logHeight;
            DWORD faceDepth = logDepth;
            DWORD faceSize = 0;
            for (; levels; levels--)
            {
                faceSize += (1 << (max(faceWidth, logMin) + max(faceHeight, logMin))) * texelSize / 8;
                if (faceWidth > 0) faceWidth--;
                if (faceHeight > 0) faceHeight--;
                if (faceDepth > 0) faceDepth--;
            }
            faceSize = (faceSize + D3DTEXTURE_CUBEFACE_ALIGNMENT - 1) &
                       ~(D3DTEXTURE_CUBEFACE_ALIGNMENT - 1);
            data += faceSize * face;
        }
        if (level)
        {
            DWORD levels = level;
            DWORD logSize;
            for (; levels; levels--)
            {
                logSize = max(logWidth, logMin) + max(logHeight, logMin) + logDepth;
                data += (1 << logSize) * texelSize / 8;
                if (logWidth > 0) logWidth--;
                if (logHeight > 0) logHeight--;
                if (logDepth > 0) logDepth--;
            }
        }
        DWORD format = pixel->Format & ~(D3DFORMAT_USIZE_MASK | D3DFORMAT_VSIZE_MASK | D3DFORMAT_PSIZE_MASK);
        format |= logWidth << D3DFORMAT_USIZE_SHIFT;
        format |= logHeight << D3DFORMAT_VSIZE_SHIFT;
        format |= logDepth << D3DFORMAT_PSIZE_SHIFT;
        logWidth = max(logWidth, logMin);
        logHeight = max(logHeight, logMin);
        switch (GetFormat(pixel))
        {
        case D3DFMT_DXT1: rowPitch = (1 << logWidth) * 2; break;
        case D3DFMT_DXT2:
        case D3DFMT_DXT4: rowPitch = (1 << logWidth) * 4; break;
        default: rowPitch = (1 << logWidth) * texelSize / 8; break;
        }
        slicePitch = (1 << (logWidth + logHeight)) * texelSize / 8;
        *outFormat = format;
        *outSize = pixel->Size;
    }
    else
    {
        DWORD textureHeight = ((size & D3DSIZE_HEIGHT_MASK) >> D3DSIZE_HEIGHT_SHIFT) + 1;
        rowPitch = PitchFromSize(size);
        slicePitch = rowPitch * textureHeight;
        *outFormat = pixel->Format;
        *outSize = pixel->Size;
    }
    *outData = data;
    *outRowPitch = rowPitch;
    *outSlicePitch = slicePitch;
    return;
}
static void WINAPI LockSurface(
    D3DPixelContainer *pixel,
    D3DCUBEMAP_FACES face,
    UINT level,
    DWORD flags,
    BYTE **data,
    DWORD *rowPitch,
    DWORD *slicePitch)
{
    if (!(flags & D3DLOCK_NOOVERWRITE))
    {
        BlockOnResource(pixel);
    }
    DWORD format, size;
    FindSurfaceWithinTexture(pixel, face, level, data, rowPitch, slicePitch, &format, &size);
    if (flags & D3DLOCK_TILED)
    {
        /* Target Xbox tiled framebuffer mapping. This is an address-space
         * conversion, not an aliasing/lifetime trick. */
        *data = (BYTE *)(((DWORD)*data & ~0xf0000000UL) | 0xf0000000UL);
    }
    return;
}
void WINAPI Lock2DSurface(
    D3DPixelContainer *pixel,
    D3DCUBEMAP_FACES face,
    UINT level,
    D3DLOCKED_RECT *locked,
    const RECT *rect,
    DWORD flags)
{
    DWORD rowPitch, slicePitch;
    BYTE *data;
    LockSurface(pixel, face, level, flags, &data, &rowPitch, &slicePitch);
    if (rect)
    {
        DWORD texelSize = GetBitsPerPixel(pixel);
        data += rect->top * rowPitch + rect->left * texelSize / 8;
    }
    locked->Pitch = rowPitch;
    locked->pBits = data;
    return;
}
void WINAPI Lock3DSurface(
    D3DPixelContainer *pixel,
    UINT level,
    D3DLOCKED_BOX *locked,
    const D3DBOX *box,
    DWORD flags)
{
    DWORD rowPitch, slicePitch;
    BYTE *data;
    LockSurface(pixel, D3DCUBEMAP_FACE_POSITIVE_X, level, flags, &data, &rowPitch, &slicePitch);
    if (box)
    {
        DWORD texelSize = GetBitsPerPixel(pixel);
        data += box->Front * slicePitch + box->Top * rowPitch + box->Left * texelSize / 8;
    }
    locked->RowPitch = rowPitch;
    locked->SlicePitch = slicePitch;
    locked->pBits = data;
    return;
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
    DWORD *outSize)
{
    DWORD size = 0;
    DWORD texelSize = BitsPerPixelOfD3DFORMAT(d3dFormat);
    DWORD logWidth, logHeight, logDepth, sizeWidth, sizeHeight;
    if ((g_TextureFormat[d3dFormat] & 1) || IsCompressedD3DFORMAT(d3dFormat))
    {
        logWidth = Log2(width);
        logHeight = Log2(height);
        logDepth = Log2(depth);
        sizeWidth = 0;
        sizeHeight = 0;
        DWORD logMin = MinimumTextureSizeOfD3DFORMAT(d3dFormat);
        if (levels == 0)
        {
            levels = max(logWidth, max(logHeight, logDepth)) + 1;
        }
        DWORD currentLevel;
        DWORD currentWidth = logWidth;
        DWORD currentHeight = logHeight;
        DWORD currentDepth = logDepth;
        DWORD logSize;
        for (currentLevel = levels; currentLevel; currentLevel--)
        {
            logSize = max(currentWidth, logMin) + max(currentHeight, logMin) + currentDepth;
            size += (1 << logSize) * texelSize / 8;
            if (currentWidth > 0) currentWidth--;
            if (currentHeight > 0) currentHeight--;
            if (currentDepth > 0) currentDepth--;
        }
        if (cubeMap)
        {
            size = (size + D3DTEXTURE_CUBEFACE_ALIGNMENT - 1) & ~(D3DTEXTURE_CUBEFACE_ALIGNMENT - 1);
            size *= 6;
        }
    }
    else
    {
        logWidth = logHeight = logDepth = 0;
        if (levels == 0) levels = 1;
        if (pitch == 0) pitch = CalcPitch(width, texelSize);
        sizeWidth = width;
        sizeHeight = height;
        size = pitch * height;
    }
    *outFormat = (cubeMap ? D3DFORMAT_CUBEMAP : 0)
               | (volume ? 3 : 2) << D3DFORMAT_DIMENSION_SHIFT
               | d3dFormat << D3DFORMAT_FORMAT_SHIFT
               | levels << D3DFORMAT_MIPMAP_SHIFT
               | logWidth << D3DFORMAT_USIZE_SHIFT
               | logHeight << D3DFORMAT_VSIZE_SHIFT
               | logDepth << D3DFORMAT_PSIZE_SHIFT
               | (videoMemory ? D3DFORMAT_DMACHANNEL_B : D3DFORMAT_DMACHANNEL_A)
               | D3DFORMAT_BORDERSOURCE_COLOR;
    if (sizeWidth)
    {
        *outSize = (sizeWidth - 1)
                 | (sizeHeight - 1) << D3DSIZE_HEIGHT_SHIFT
                 | ((pitch / D3DTEXTURE_PITCH_ALIGNMENT) - 1) << D3DSIZE_PITCH_SHIFT;
    }
    else
    {
        *outSize = 0;
    }
    return size;
}

}
}
