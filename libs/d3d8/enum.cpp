/* Adapted from RXDK-Libs (2026 Team Resurgent), GPL-3.0-or-later.
 * January PDB, original code and SDK define the target-specific layout/API.
 * See libs/d3d8/PROVENANCE.md. */
#include "display_internal.h"
#include "pixeljar.h"
#pragma code_seg("D3D")
namespace D3D
{
__inline BOOL CheckDisplayFormat(
    D3DFORMAT format)
{
    return g_TextureFormat[format] & 0x80;
}
}
using namespace D3D;
Direct3D *WINAPI Direct3DCreate8(
    UINT SDKVersion)
{
    return (Direct3D *)1;
}
HRESULT WINAPI Direct3D_GetAdapterIdentifier(
    UINT Adapter,
    DWORD Flags,
    D3DADAPTER_IDENTIFIER8 *pIdentifier)
{
    if (Adapter != 0) { return D3DERR_INVALIDCALL; }
    *pIdentifier = g_AdapterIdentifier;
    return S_OK;
}
UINT WINAPI Direct3D_GetAdapterModeCount(
    UINT Adapter)
{
    UINT Count = 0;
    const DISPLAYMODE *pMode = GetTableForCurrentAvInfo();
    DWORD SystemAvInfo = CMiniport::GetDisplayCapabilities();
    ULONG ModeAvPack = pMode->AvInfo & AV_PACK_MASK;
    ULONG AvPack = SystemAvInfo & AV_PACK_MASK;
    ULONG AvRefresh = SystemAvInfo & AV_REFRESH_MASK;
    ULONG AvHDTVModes = SystemAvInfo & AV_HDTV_MODE_MASK;
    for (;; pMode++)
    {
        if ((pMode->AvInfo & AV_PACK_MASK) != ModeAvPack) { break; }
        if ((AvPack != AV_PACK_HDTV || !(pMode->AvInfo & (AV_FLAGS_HDTV_720p | AV_FLAGS_HDTV_1080i)))
            && (pMode->AvInfo & AV_FLAGS_WIDESCREEN) && !(SystemAvInfo & AV_FLAGS_WIDESCREEN)) { continue; }
        if (!(pMode->AvInfo & AvRefresh)) { continue; }
        if (AvPack == AV_PACK_HDTV && !(pMode->AvInfo & AvHDTVModes)
            && (pMode->AvInfo & AV_HDTV_MODE_MASK) != 0) { continue; }
        Count++;
    }
    return Count * 4;
}
HRESULT WINAPI Direct3D_EnumAdapterModes(
    UINT Adapter,
    UINT Mode,
    D3DDISPLAYMODE *pModeRet)
{
    DWORD Format = Mode % 4;
    Mode /= 4;
    const DISPLAYMODE *pMode = GetTableForCurrentAvInfo();
    DWORD SystemAvInfo = CMiniport::GetDisplayCapabilities();
    ULONG ModeAvPack = pMode->AvInfo & AV_PACK_MASK;
    ULONG AvPack = SystemAvInfo & AV_PACK_MASK;
    ULONG AvRefresh = SystemAvInfo & AV_REFRESH_MASK;
    ULONG AvHDTVModes = SystemAvInfo & AV_HDTV_MODE_MASK;
    for (;; pMode++)
    {
        if ((pMode->AvInfo & AV_PACK_MASK) != ModeAvPack) { return D3DERR_INVALIDCALL; }
        if ((AvPack != AV_PACK_HDTV || !(pMode->AvInfo & (AV_FLAGS_HDTV_720p | AV_FLAGS_HDTV_1080i)))
            && (pMode->AvInfo & AV_FLAGS_WIDESCREEN) && !(SystemAvInfo & AV_FLAGS_WIDESCREEN)) { continue; }
        if (!(pMode->AvInfo & AvRefresh)) { continue; }
        if (AvPack == AV_PACK_HDTV && !(pMode->AvInfo & AvHDTVModes)
            && (pMode->AvInfo & AV_HDTV_MODE_MASK) != 0) { continue; }
        if (!Mode--)
        {
            pModeRet->Width = pMode->Width;
            pModeRet->Height = pMode->Height;
            pModeRet->RefreshRate = (pMode->AvInfo & AV_FLAGS_60Hz) ? 60 : 50;
            pModeRet->Flags = CMiniport::GetPresentFlagsFromAvInfo(pMode->AvInfo);
            switch (Format)
            {
            case 0: pModeRet->Format = D3DFMT_LIN_X8R8G8B8; break;
            case 1: pModeRet->Format = D3DFMT_LIN_R5G6B5; break;
            case 2: pModeRet->Format = D3DFMT_LIN_X1R5G5B5; break;
            case 3: pModeRet->Format = D3DFMT_LIN_A8R8G8B8; break;
            }
            return S_OK;
        }
    }
}
HRESULT WINAPI Direct3D_GetAdapterDisplayMode(
    UINT Adapter,
    D3DDISPLAYMODE *pMode)
{
    if (Adapter != 0) { return D3DERR_INVALIDCALL; }
    CDevice *pDevice = g_pDevice;
    if (pDevice == NULL)
    {
        memset(pMode, 0, sizeof(*pMode));
    }
    else
    {
        pMode->Width = pDevice->m_DisplayWidth;
        pMode->Height = pDevice->m_DisplayHeight;
        pMode->RefreshRate = pDevice->m_Miniport.GetRefreshRate();
        pMode->Format = PixelJar::GetFormat(&pDevice->m_FrameBufferSurfaces[1]);
        pMode->Flags = CMiniport::GetPresentFlagsFromAvInfo(pDevice->m_Miniport.m_CurrentAvInfo);
    }
    return S_OK;
}
HRESULT WINAPI Direct3D_CheckDeviceType(
    UINT Adapter,
    D3DDEVTYPE CheckType,
    D3DFORMAT DisplayFormat,
    D3DFORMAT BackBufferFormat,
    BOOL Windowed)
{
    if (Adapter != 0) { return D3DERR_INVALIDCALL; }
    if (CheckType != D3DDEVTYPE_HAL) { return D3DERR_INVALIDDEVICE; }
    if (Windowed == TRUE) { return D3DERR_NOTAVAILABLE; }
    if (!CheckDisplayFormat(DisplayFormat)) { return D3DERR_NOTAVAILABLE; }
    if (!CheckDisplayFormat(BackBufferFormat)) { return D3DERR_NOTAVAILABLE; }
    return S_OK;
}
HRESULT WINAPI Direct3D_CheckDeviceFormat(
    UINT Adapter,
    D3DDEVTYPE DeviceType,
    D3DFORMAT AdapterFormat,
    DWORD Usage,
    D3DRESOURCETYPE RType,
    D3DFORMAT CheckFormat)
{
    if (Adapter != 0) { return D3DERR_INVALIDCALL; }
    if (DeviceType != D3DDEVTYPE_HAL) { return D3DERR_INVALIDDEVICE; }
    if (!CheckDisplayFormat(AdapterFormat)) { return D3DERR_NOTAVAILABLE; }
    if (Usage & D3DUSAGE_DEPTHSTENCIL)
    {
        if (!(g_TextureFormat[CheckFormat] & 0x40)) { return D3DERR_NOTAVAILABLE; }
    }
    else if (Usage & D3DUSAGE_RENDERTARGET)
    {
        if (!CheckDisplayFormat(CheckFormat)) { return D3DERR_NOTAVAILABLE; }
    }
    else
    {
        if (BitsPerPixelOfD3DFORMAT(CheckFormat) == 0) { return D3DERR_NOTAVAILABLE; }
    }
    return S_OK;
}
HRESULT WINAPI Direct3D_CheckDeviceMultiSampleType(
    UINT Adapter,
    D3DDEVTYPE DeviceType,
    D3DFORMAT RenderTargetFormat,
    BOOL Windowed,
    D3DMULTISAMPLE_TYPE MultiSampleType)
{
    if (Adapter != 0) { return D3DERR_INVALIDCALL; }
    if (DeviceType != D3DDEVTYPE_HAL) { return D3DERR_INVALIDDEVICE; }
    if (Windowed == TRUE) { return D3DERR_NOTAVAILABLE; }
    if (!CheckDisplayFormat(RenderTargetFormat)) { return D3DERR_NOTAVAILABLE; }
    if (MultiSampleType != D3DMULTISAMPLE_NONE && MultiSampleType != 0)
    {
        if ((MultiSampleType & 0xf0000) > D3DMULTISAMPLE_PREFILTER_FORMAT_A8R8G8B8)
        { return D3DERR_INVALIDCALL; }
        if (((MultiSampleType >> 4) & 15) < 1 || (MultiSampleType & 15) < 1
            || ((MultiSampleType >> 4) & 15) > 3 || (MultiSampleType & 15) > 3)
        { return D3DERR_NOTAVAILABLE; }
    }
    return S_OK;
}
HRESULT WINAPI Direct3D_CheckDepthStencilMatch(
    UINT Adapter,
    D3DDEVTYPE DeviceType,
    D3DFORMAT AdapterFormat,
    D3DFORMAT RenderTargetFormat,
    D3DFORMAT DepthStencilFormat)
{
    if (Adapter != 0) { return D3DERR_INVALIDCALL; }
    if (DeviceType != D3DDEVTYPE_HAL) { return D3DERR_INVALIDDEVICE; }
    if (!CheckDisplayFormat(RenderTargetFormat)) { return D3DERR_NOTAVAILABLE; }
    if (!(g_TextureFormat[DepthStencilFormat] & 0x40)) { return D3DERR_NOTAVAILABLE; }
    return S_OK;
}
HRESULT WINAPI Direct3D_GetDeviceCaps(
    UINT Adapter,
    D3DDEVTYPE DeviceType,
    D3DCAPS8 *pCaps)
{
    if (Adapter != 0) { return D3DERR_INVALIDCALL; }
    if (DeviceType != D3DDEVTYPE_HAL) { return D3DERR_INVALIDDEVICE; }
    D3DDevice_GetDeviceCaps(pCaps);
    return S_OK;
}
