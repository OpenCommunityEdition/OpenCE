/* January AV-info and display-mode selection, corroborated by RXDK-Libs
 * (2026 Team Resurgent), GPL-3.0-or-later. Preserve pinned reference notices.
 * The table data and ABI/field choices are from the original January target. */
#include "display_internal.h"
#include "pixeljar.h"
#pragma code_seg("D3D")
namespace D3D
{
DWORD WINAPI CMiniport::GetPresentFlagsFromAvInfo(
    DWORD avInfo)
{
    DWORD flags = 0;
    if (avInfo & 0x10000) flags = 0x10;
    if (avInfo & 0x200000) flags |= 0x20;
    else if (avInfo & 0x1000000) flags |= 0xa0;
    else flags |= 0x40;
    if (avInfo & 0x2000000) flags |= 0x100;
    return flags;
}
DWORD WINAPI CMiniport::GetDisplayCapabilities(
    void)
{
    if (!D3D__AvInfo)
    {
        AvSendTVEncoderOption(NULL, 6, 0, &D3D__AvInfo);
    }
    return D3D__AvInfo;
}

}

#pragma const_seg("D3D_RD")
namespace D3D
{
const DWORD g_DisplayModeCount = sizeof(g_DisplayModes) / sizeof(g_DisplayModes[0]);
const DISPLAYMODE g_DisplayModes[191] =
{
#include "display_modes.inc"
};

const DISPLAYMODE *WINAPI GetTableForCurrentAvInfo(
    void)
{
    DWORD avInfo = CMiniport::GetDisplayCapabilities();
    DWORD region = avInfo & AV_STANDARD_MASK;
    DWORD pack = avInfo & AV_PACK_MASK;
    const DISPLAYMODE *mode;
    DWORD i;
    DWORD count = g_DisplayModeCount;
    // The table is nonempty, so the first loop assigns mode on every path.
    for (i = 0; i < count; ++i)
    {
        mode = &g_DisplayModes[i];
        if ((mode->AvInfo & AV_STANDARD_MASK) == region)
        {
            break;
        }
    }
    for (; i < count; ++i)
    {
        mode = &g_DisplayModes[i];
        DWORD modePack = mode->AvInfo & AV_PACK_MASK;
        if (!modePack || modePack == pack)
        {
            break;
        }
    }
    return mode;
}

void CMiniport::SetVideoMode(
    DWORD width,
    DWORD height,
    DWORD refresh,
    DWORD flags,
    D3DFORMAT format,
    DWORD presentationInterval,
    DWORD pitch)
{
    format = MapToLinearD3DFORMAT(format);
    DWORD depth = VideoBitsPerPixelOfD3DFORMAT(format);
    const DISPLAYMODE *mode = GetTableForCurrentAvInfo();
    DWORD avInfo = GetDisplayCapabilities();
    DWORD modePack = mode->AvInfo & AV_PACK_MASK;
    DWORD pack = avInfo & AV_PACK_MASK;
    DWORD avRefresh = avInfo & AV_REFRESH_MASK;
    DWORD hdtvModes = 0;
    DWORD field = flags & (D3DPRESENTFLAG_INTERLACED | D3DPRESENTFLAG_PROGRESSIVE);
    if (flags & D3DPRESENTFLAG_FIELD)
    {
        field &= ~D3DPRESENTFLAG_INTERLACED;
    }
    if (pack == AV_PACK_HDTV)
    {
        hdtvModes = avInfo & AV_HDTV_MODE_MASK;
    }
    // Zero means no refresh override. Only enter the selection for a
    // caller-specified rate; 50 and 60 are the supported explicit rates.
    if (refresh)
    {
        switch (refresh)
        {
        case 50:
            avRefresh = AV_FLAGS_50Hz;
            break;
        case 60:
            avRefresh = AV_FLAGS_60Hz;
            break;
        default:
            break;
        }
    }
    DWORD displayMode = 0;
    if (pack != AV_PACK_NONE)
    {
        for (;; ++mode)
        {
            if ((mode->AvInfo & AV_PACK_MASK) != modePack)
            {
                break;
            }
            if (pack == AV_PACK_HDTV && !(mode->AvInfo & hdtvModes) &&
                (mode->AvInfo & AV_HDTV_MODE_MASK))
            {
                continue;
            }
            if (mode->Width != width || mode->Height != height)
            {
                continue;
            }
            if (!(mode->AvInfo & AV_FLAGS_WIDESCREEN) != !(flags & D3DPRESENTFLAG_WIDESCREEN))
            {
                continue;
            }
            if (!(mode->AvInfo & AV_FLAGS_FIELD) != !(flags & D3DPRESENTFLAG_FIELD))
            {
                continue;
            }
            if (!(mode->AvInfo & AV_FLAGS_10x11PAR) != !(flags & D3DPRESENTFLAG_10X11PIXELASPECTRATIO))
            {
                continue;
            }
            if (!(mode->AvInfo & avRefresh))
            {
                continue;
            }
            if (field &&
                !((field & D3DPRESENTFLAG_INTERLACED) && (mode->AvInfo & AV_FLAGS_INTERLACED)) &&
                !((field & D3DPRESENTFLAG_PROGRESSIVE) && !(mode->AvInfo & AV_FLAGS_INTERLACED)))
            {
                continue;
            }
            displayMode = mode->DisplayMode;
            break;
        }
    }
    m_DisplayMode = displayMode;
    m_Format = format;
    m_PresentationInterval = presentationInterval;
    m_SurfacePitch = pitch;
    m_GammaUpdated[m_GammaCurrentIndex] = TRUE;
    m_CurrentAvInfo = mode->AvInfo;
    m_FirstFlip = TRUE;
    return;
}
}
#pragma const_seg()
