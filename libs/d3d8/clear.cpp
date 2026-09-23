/* NV2A clears, reconstructed against the January target and corroborated by
 * RXDK-Libs clear.cpp (2026 Team Resurgent), GPL-3.0-or-later.
 * See libs/d3d8/PROVENANCE.md and libs/d3d8/research/wave6_matching_notes.md.
 * Valid caller domain: finite Z in [0,1], allocated rectangles and valid
 * render/depth formats, with coordinates fitting the device's clip range. */
#include "device_layout.h"
#include "pixeljar.h"
#include "push_internal.h"
#include "math_internal.h"
#pragma code_seg("D3D")
using namespace D3D;

void WINAPI D3DDevice_Clear(
    DWORD count,
    const D3DRECT *rectangles,
    DWORD flags,
    D3DCOLOR color,
    float z,
    DWORD stencil)
{
    CDevice *device = g_pDevice;
    D3DRECT rectangle;
    DWORD *push;
    D3DSurface *target = device->m_pRenderTarget;
    D3DSurface *depth = device->m_pZBuffer;
    DWORD fillDepth = 0;
    DWORD savedFormat = 0;

    if (PixelJar::IsSwizzled(target))
    {
        savedFormat = PixelJar::GetSurfaceFormat(target, depth);
        DWORD linearFormat = (savedFormat & ~0x200UL) | 0x100UL;
        push = device->StartPush();
        Push1(push, 0x208, linearFormat);
        device->EndPush(push + 2);
    }

    if (flags & D3DCLEAR_TARGET)
    {
        switch (PixelJar::GetFormat(target))
        {
        case D3DFMT_R5G6B5:
        case D3DFMT_LIN_R5G6B5:
            color = ((color & 0x00f80000UL) >> 8) |
                    ((color & 0x0000fc00UL) >> 5) |
                    ((color & 0x000000f8UL) >> 3);
            break;
        case D3DFMT_X1R5G5B5:
        case D3DFMT_LIN_X1R5G5B5:
            color = ((color & 0x00f80000UL) >> 9) |
                    ((color & 0x0000f800UL) >> 6) |
                    ((color & 0x000000f8UL) >> 3);
            break;
        default:
            break;
        }
    }

    if (!depth)
    {
        flags &= ~(D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL);
        if (!flags)
        {
            // Original early exit is before SavedFormat restoration. Keep
            // the behavior visible; this is not a corrected driver build.
            return;
        }
    }

    if (flags & D3DCLEAR_ZBUFFER)
    {
        switch (PixelJar::GetFormat(depth))
        {
        case D3DFMT_D16:
        case D3DFMT_LIN_D16:
            {
                LONG value = FloatToLong(z * (float)D3DZ_MAX_D16);
                value = value > 0xffff ? 0xffff : value;
                value = value < 0 ? 0 : value;
                fillDepth = value;
            }
            break;
        case D3DFMT_D24S8:
        case D3DFMT_LIN_D24S8:
            {
                LONG value = FloatToLong(z * (float)D3DZ_MAX_D24S8);
                value = value > 0xffffff ? 0xffffff : value;
                value = value < 0 ? 0 : value;
                fillDepth = (DWORD)value << 8;
            }
            break;
        case D3DFMT_F16:
        case D3DFMT_LIN_F16:
            if (z == 0.0f)
            {
                fillDepth = 0;
            }
            else
            {
                double scaled = (double)z * D3DZ_MAX_F16;
                DWORD high;
                // Decode the little-endian double representation without
                // reading through an unrelated DWORD lvalue.
                memcpy(&high, (const BYTE *)&scaled + sizeof(DWORD), sizeof(high));
                fillDepth = ((high >> 8) - 0x3f8000) & 0xffff;
            }
            break;
        case D3DFMT_F24S8:
        case D3DFMT_LIN_F24S8:
            if (z == 0.0f)
            {
                fillDepth = 0;
            }
            else
            {
                double scaled = (double)z * D3DZ_MAX_F24S8;
                DWORD high;
                memcpy(&high, (const BYTE *)&scaled + sizeof(DWORD), sizeof(high));
                fillDepth = ((high - 0x38000000UL) << 4) & 0xffffff00UL;
            }
            break;
        default:
            break;
        }
    }

    INT leftClip = device->m_Viewport.X;
    INT topClip = device->m_Viewport.Y;
    INT rightClip = device->m_Viewport.X + device->m_Viewport.Width;
    INT bottomClip = device->m_Viewport.Y + device->m_Viewport.Height;
    if (!count)
    {
        rectangle.x1 = leftClip;
        rectangle.y1 = topClip;
        rectangle.x2 = rightClip;
        rectangle.y2 = bottomClip;
        rectangles = &rectangle;
        count = 1;
    }

    do
    {
        INT left = max(rectangles->x1, leftClip);
        INT top = max(rectangles->y1, topClip);
        INT right = min(rectangles->x2, rightClip);
        INT bottom = min(rectangles->y2, bottomClip);
        if (left < right && top < bottom)
        {
            left = Round(left * device->m_SuperSampleScaleX);
            right = Round(right * device->m_SuperSampleScaleX);
            top = Round(top * device->m_SuperSampleScaleY);
            bottom = Round(bottom * device->m_SuperSampleScaleY);
            push = device->StartPush();
            PushCount(push, 0x1d98, 2);
            push[1] = (DWORD)left | ((DWORD)(right - 1) << 16);
            push[2] = (DWORD)top | ((DWORD)(bottom - 1) << 16);
            PushCount(push + 3, 0x1d8c, 3);
            push[4] = fillDepth | stencil;
            push[5] = color;
            push[6] = flags;
            device->EndPush(push + 7);
        }
    } while (++rectangles, --count != 0);

    if (savedFormat)
    {
        push = device->StartPush();
        Push1(push, 0x208, savedFormat);
        device->EndPush(push + 2);
    }
    return;
}
