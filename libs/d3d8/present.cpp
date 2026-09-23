/* January anti-alias presentation reconstruction. Related state-management
 * reference: Team-Resurgent/RXDK-Libs, GPL-3.0-or-later, pinned in provenance.
 * The older AntiAliased* ABI and behavior here come from the supplied target.
 * Copyright (C) 2026 Team Resurgent; SPDX-License-Identifier: GPL-3.0-or-later */
#define D3DCOMPILE_NOTINLINE 1
#include "present_internal.h"
#include "draw_internal.h"
#include "push_internal.h"
#include "pixeljar.h"
#include "vshader_internal.h"
#include "state_internal.h"
#include "pusher_state.h"
#include "display_internal.h"
#include "kernel_memory.h"
#pragma code_seg("D3D")
#pragma const_seg("D3D_RD")
namespace D3D
{
DWORD g_FlipAddress;
static const float KELVIN_BORDER = 0.53125f;

const DWORD g_AntiAliasedRenderStates[] =
{
    D3DRS_FILLMODE, D3DFILL_SOLID,
    D3DRS_BACKFILLMODE, D3DFILL_SOLID,
    D3DRS_CULLMODE, D3DCULL_NONE,
    D3DRS_DITHERENABLE, TRUE,
    D3DRS_ALPHATESTENABLE, FALSE,
    D3DRS_ALPHABLENDENABLE, FALSE,
    D3DRS_FOGENABLE, FALSE,
    D3DRS_EDGEANTIALIAS, FALSE,
    D3DRS_STENCILENABLE, FALSE,
    D3DRS_LIGHTING, FALSE,
    D3DRS_MULTISAMPLEMASK, 0xffffffffUL,
    D3DRS_LOGICOP, D3DLOGICOP_NONE,
    D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALL,
    D3DRS_YUVENABLE, FALSE,
    D3DRS_SPECULARENABLE, FALSE,
    D3DRS_ZBIAS, 0,
};
const DWORD g_AntiAliasedTextureStates[] =
{
    D3DTSS_COLOROP, D3DTOP_SELECTARG1,
    D3DTSS_COLORARG1, D3DTA_TEXTURE,
    D3DTSS_ALPHAOP, D3DTOP_DISABLE,
    D3DTSS_TEXCOORDINDEX, 0,
    D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP,
    D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP,
    D3DTSS_COLORKEYOP, D3DTCOLORKEYOP_DISABLE,
    D3DTSS_COLORSIGN, 0,
    D3DTSS_ALPHAKILL, D3DTALPHAKILL_DISABLE,
    D3DTSS_MINFILTER, D3DTEXF_LINEAR,
    D3DTSS_MAGFILTER, D3DTEXF_LINEAR,
};

void WINAPI AntiAliasedSetState(
    AntiAliasedSavedState *saved,
    D3DSurface *destination,
    D3DSurface *source,
    DWORD filter)
{
    DWORD i;
    CDevice *device = g_pDevice;
    for (i = 0; i < 16; ++i)
    {
        device->SetRenderState((D3DRENDERSTATETYPE)g_AntiAliasedRenderStates[2 * i],
                              g_AntiAliasedRenderStates[2 * i + 1]);
    }
    for (i = 0; i < 11; ++i)
    {
        device->SetTextureStageState(0, (D3DTEXTURESTAGESTATETYPE)g_AntiAliasedTextureStates[2 * i],
                                    g_AntiAliasedTextureStates[2 * i + 1]);
    }
    device->SetPixelShader(0);
    device->SetVertexShader(D3DFVF_XYZRHW | D3DFVF_TEX1);
    device->SetRenderTarget(destination, NULL);
    device->SetTextureStageState(0, D3DTSS_MINFILTER, filter);
    device->SetTextureStageState(0, D3DTSS_MAGFILTER, filter);
    device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    /* Copy the common pixel-container representation, not a fictional derived
     * texture object at the surface address. The temporary stays live through
     * restoration; only the resource type bits are replaced. */
    memcpy(&saved->SrcTexture, source, sizeof(saved->SrcTexture));
    saved->SrcTexture.Common = (saved->SrcTexture.Common & ~D3DCOMMON_TYPE_MASK)
                            | D3DCOMMON_TYPE_TEXTURE;
    device->SetTexture(0, &saved->SrcTexture);
    SetState();
    return;
}
void WINAPI AntiAliasedSaveState(
    AntiAliasedSavedState *saved)
{
    CDevice *device = g_pDevice;
    DWORD i;
    saved->pRenderTarget = device->m_pRenderTarget;
    saved->pRenderTarget->AddRef();
    saved->pZBuffer = device->m_pZBuffer;
    if (saved->pZBuffer) saved->pZBuffer->AddRef();
    saved->pTexture = device->m_Textures[0];
    if (saved->pTexture) saved->pTexture->AddRef();
    saved->pPixelShader = device->m_pPixelShader;
    saved->VertexShaderHandle = device->m_VertexShaderHandle;
    saved->Viewport = device->m_Viewport;
    saved->Stage1ColorOp = D3D__TextureState[1][D3DTSS_COLOROP];
    for (i = 0; i < 16; ++i)
    {
        saved->RenderState[i] = D3D__RenderState[g_AntiAliasedRenderStates[2 * i]];
    }
    for (i = 0; i < 11; ++i)
    {
        saved->TextureState[i] = D3D__TextureState[0][g_AntiAliasedTextureStates[2 * i]];
    }
    if (saved->pPixelShader)
    {
        for (i = D3DRS_PS_MIN; i < D3DRS_PS_MAX; ++i)
            saved->PixelShaderState[i - D3DRS_PS_MIN] = D3D__RenderState[i];
        saved->PSTextureModes = D3D__RenderState[D3DRS_PSTEXTUREMODES];
    }
    memcpy(saved->VertexShaders, device->m_VertexShaderProgramSlots, sizeof(saved->VertexShaders));
    return;
}
void WINAPI AntiAliasedRestoreState(
    AntiAliasedSavedState *saved)
{
    CDevice *device = g_pDevice;
    DWORD i;
    device->SetRenderTarget(saved->pRenderTarget, saved->pZBuffer);
    saved->pRenderTarget->Release();
    if (saved->pZBuffer) saved->pZBuffer->Release();
    device->SetTexture(0, saved->pTexture);
    if (saved->pTexture) saved->pTexture->Release();
    device->SetPixelShader((DWORD)saved->pPixelShader);
    device->SetVertexShader(saved->VertexShaderHandle);
    device->SetViewport(&saved->Viewport);
    BOOL shadowed = !(device->m_StateFlags & 0x10);
    device->SetTextureStageState(1, D3DTSS_COLOROP, saved->Stage1ColorOp);
    for (i = 0; i < 16; ++i)
    {
        if (shadowed || g_AntiAliasedRenderStates[2 * i] >= D3DRS_SIMPLE_MAX)
            device->SetRenderState((D3DRENDERSTATETYPE)g_AntiAliasedRenderStates[2 * i], saved->RenderState[i]);
    }
    for (i = 0; i < 11; ++i)
        device->SetTextureStageState(0, (D3DTEXTURESTAGESTATETYPE)g_AntiAliasedTextureStates[2 * i], saved->TextureState[i]);
    if (shadowed)
    {
        if (saved->pPixelShader)
        {
            for (i = D3DRS_PS_MIN; i < D3DRS_PS_MAX; ++i)
                device->SetRenderState((D3DRENDERSTATETYPE)i, saved->PixelShaderState[i - D3DRS_PS_MIN]);
            device->SetRenderState(D3DRS_PSTEXTUREMODES, saved->PSTextureModes);
        }
        RestoreVertexShaders(saved->VertexShaders, sizeof(saved->VertexShaders) / sizeof(DWORD));
    }
    return;
}
void WINAPI AliasedFlip(
    void)
{
    CDevice *device = g_pDevice;
    DWORD i = device->m_FrameBufferCount - 1;
    DWORD data = device->m_FrameBufferSurfaces[i].Data;
    DWORD lock = device->m_FrameBufferSurfaces[i].Lock;
    for (; i != 0; --i)
    {
        device->m_FrameBufferSurfaces[i].Data = device->m_FrameBufferSurfaces[i - 1].Data;
        device->m_FrameBufferSurfaces[i].Lock = device->m_FrameBufferSurfaces[i - 1].Lock;
    }
    device->m_FrameBufferSurfaces[0].Data = data;
    device->m_FrameBufferSurfaces[0].Lock = lock;
    DWORD *push = device->StartPush();
    Push1(push, 0x12c, 0);
    Push1(push + 2, NV097_NO_OPERATION, 0);
    DWORD flipAddress = device->m_FrameBufferSurfaces[1].Data;
    g_FlipAddress = flipAddress;
    Push1(push + 4, NV097_WAIT_FOR_IDLE, 0);
    Push1(push + 6, 0x1d8c, flipAddress);
    if (device->m_PresentationInterval == D3DPRESENT_INTERVAL_IMMEDIATE)
        Push1(push + 8, NV097_NO_OPERATION, 0x300);
    else
        Push1(push + 8, NV097_NO_OPERATION, 0x304);
    push += 10;
    if (device->m_pRenderTarget == &device->m_FrameBufferSurfaces[0])
    {
        Push1(push, 0x130, 0);
        Push1(push + 2, 0x210, device->m_FrameBufferSurfaces[0].Data);
        push += 4;
    }
    else
    {
        device->m_StateFlags |= 0x100;
    }
    device->EndPush(push);
    return;
}
}

namespace D3D
{
DWORD *WINAPI AntiAliasedFilterBlt(
    DWORD *push,
    DWORD xScale,
    DWORD yScale,
    const RECT *rectangle)
{
    CDevice *device = g_pDevice;
    DWORD left = 0;
    DWORD top = 0;
    DWORD right = PixelJar::GetWidth(device->m_pRenderTarget);
    DWORD bottom = PixelJar::GetHeight(device->m_pRenderTarget);
    if (rectangle)
    {
        left = max((LONG)left, rectangle->left);
        right = min((LONG)right, rectangle->right);
        top = max((LONG)top, rectangle->top);
        bottom = min((LONG)bottom, rectangle->bottom);
    }
    if (left < right && top < bottom)
    {
        PushCount(push++, 0x1760, 16);
        for (DWORD i = 0; i < 16; ++i) *push++ = SIZEANDTYPE_DISABLED;
        Push1(push, 0x1760, 0x22);
        Push1(push + 2, 0x1784, 0x22);
        Push1(push + 4, 0x17fc, 5);
        PushCount(push + 6, 0x40001818, 12);
        DWORD filter = D3D__TextureState[0][D3DTSS_MINFILTER];
        float uAdjust = 0.0f;
        float vAdjust = 0.0f;
        if (filter == D3DTEXF_QUINCUNX || filter == D3DTEXF_GAUSSIANCUBIC)
        {
            if (xScale == 2) uAdjust = 0.5f;
            if (yScale == 2) vAdjust = 0.5f;
        }
        /* The command payload is a 12-float triangle. memcpy writes float
         * representations without dereferencing an incompatible DWORD lvalue.
         * Bounds: caller reserves at least 38 words for a nonempty rectangle. */
        float value = (float)left - KELVIN_BORDER;
        memcpy(push + 7, &value, sizeof(value));
        value = (float)top - KELVIN_BORDER;
        memcpy(push + 8, &value, sizeof(value));
        value = (float)(left * xScale) + uAdjust;
        memcpy(push + 9, &value, sizeof(value));
        value = (float)(top * yScale) + vAdjust;
        memcpy(push + 10, &value, sizeof(value));
        value = (float)right * 4.0f - KELVIN_BORDER;
        memcpy(push + 11, &value, sizeof(value));
        value = (float)top * 4.0f - KELVIN_BORDER;
        memcpy(push + 12, &value, sizeof(value));
        value = (float)(right * xScale) * 4.0f + uAdjust;
        memcpy(push + 13, &value, sizeof(value));
        value = (float)(top * yScale) * 4.0f + vAdjust;
        memcpy(push + 14, &value, sizeof(value));
        value = (float)left * 4.0f - KELVIN_BORDER;
        memcpy(push + 15, &value, sizeof(value));
        value = (float)bottom * 4.0f - KELVIN_BORDER;
        memcpy(push + 16, &value, sizeof(value));
        value = (float)(left * xScale) * 4.0f + uAdjust;
        memcpy(push + 17, &value, sizeof(value));
        value = (float)(bottom * yScale) * 4.0f + vAdjust;
        memcpy(push + 18, &value, sizeof(value));
        Push1(push + 19, 0x17fc, 0);
        push += 21;
        device->m_DirtyFlags |= 0x7ff;
    }
    return push;
}
void WINAPI AntiAliasedCopy(
    const RECT *rectangle)
{
    CDevice *device = g_pDevice;
    if (device->m_FrameBufferCount > 2 &&
        device->m_PresentationInterval != D3DPRESENT_INTERVAL_IMMEDIATE)
    {
        DWORD data = device->m_FrameBufferSurfaces[2].Data;
        DWORD lock = device->m_FrameBufferSurfaces[2].Lock;
        device->m_FrameBufferSurfaces[2].Data = device->m_FrameBufferSurfaces[1].Data;
        device->m_FrameBufferSurfaces[2].Lock = device->m_FrameBufferSurfaces[1].Lock;
        device->m_FrameBufferSurfaces[1].Data = data;
        device->m_FrameBufferSurfaces[1].Lock = lock;
    }
    AntiAliasedSavedState savedState;
    AntiAliasedSaveState(&savedState);
    DWORD sample = D3D__RenderState[D3DRS_MULTISAMPLETYPE];
    DWORD xScale = (sample >> 4) & 15;
    DWORD yScale = sample & 15;
    D3DSurface *destination = &device->m_FrameBufferSurfaces[1];
    D3DSurface *source = &device->m_FrameBufferSurfaces[0];
    DWORD filter = D3DTEXF_LINEAR;
    if (sample & 0x100) filter = D3DTEXF_QUINCUNX;
    else if (sample & 0x200) filter = D3DTEXF_GAUSSIANCUBIC;
    DWORD flipAddress = destination->Data;
    g_FlipAddress = flipAddress;
    AntiAliasedSetState(&savedState, destination, source, filter);
    DWORD *push = device->StartPush();
    if (device->m_PresentationInterval == D3DPRESENT_INTERVAL_IMMEDIATE)
    {
        push = AntiAliasedFilterBlt(push, xScale, yScale, rectangle);
        if (device->m_PresentCount == 1)
        {
            Push1(push, NV097_WAIT_FOR_IDLE, 0);
            push += 2;
        }
        Push1(push, 0x1d8c, flipAddress);
        Push1(push + 2, NV097_NO_OPERATION, 0x300);
        push += 4;
    }
    else if (device->m_FrameBufferCount < 3)
    {
        Push1(push, 0x12c, 0);
        Push1(push + 2, NV097_NO_OPERATION, 0);
        Push1(push + 4, NV097_WAIT_FOR_IDLE, 0);
        Push1(push + 6, 0x1d8c, flipAddress);
        Push1(push + 8, NV097_NO_OPERATION, 0x304);
        Push1(push + 10, 0x130, 0);
        push = AntiAliasedFilterBlt(push + 12, xScale, yScale, rectangle);
    }
    else
    {
        Push1(push, 0x130, 0);
        push = AntiAliasedFilterBlt(push + 2, xScale, yScale, rectangle);
        Push1(push, NV097_WAIT_FOR_IDLE, 0);
        Push1(push + 2, 0x1d8c, flipAddress);
        Push1(push + 4, NV097_NO_OPERATION, 0x304);
        Push1(push + 6, 0x12c, 0);
        Push1(push + 8, NV097_NO_OPERATION, 0);
        push += 10;
    }
    device->EndPush(push);
    AntiAliasedRestoreState(&savedState);
    return;
}
extern "C" void WINAPI D3DDevice_Present(
    const RECT *sourceRect,
    const RECT *destRect,
    void *destWindowOverride,
    void *dirtyRegion)
{
    CDevice *device = g_pDevice;
    SetFence(2);
    while (device->m_PresentCount - device->m_Miniport.m_VBlankFlipCount >= 2)
        BusyLoop();
    ++device->m_PresentCount;
    if (D3D__RenderState[D3DRS_MULTISAMPLETYPE] & 0x3000)
        AntiAliasedCopy(sourceRect);
    else
        AliasedFlip();
    device->KickOff();
    device->RecordSurfaceWritePush(&device->m_FrameBufferSurfaces[1]);
    if (device->m_Miniport.m_FirstFlip)
    {
        DWORD pitch = device->m_Miniport.m_SurfacePitch;
        DWORD mode = device->m_Miniport.m_DisplayMode;
        DWORD format = device->m_Miniport.m_Format;
        void *registerBase = device->m_Miniport.m_RegisterBase;
        KickOffAndWaitForIdle();
        if (device->m_Miniport.m_CurrentAvInfo & AV_FLAGS_FIELD) pitch >>= 1;
        DWORD step = 0;
        do
        {
            device->BlockUntilVerticalBlank();
            step = AvSetDisplayMode(registerBase, step, mode, format, pitch, g_FlipAddress);
        } while (step);
        if (device->m_Miniport.m_CurrentAvInfo & AV_FLAGS_FIELD)
            AvSendTVEncoderOption(device->m_Miniport.m_RegisterBase, 11, 0, NULL);
        DWORD field;
        AvSendTVEncoderOption(device->m_Miniport.m_RegisterBase, 15, 0, &field);
        if (!((device->m_Miniport.m_VBlankCount ^ field) & 1))
            ++device->m_Miniport.m_VBlankCount;
        void *saved = AvGetSavedDataAddress();
        if (saved)
        {
            DWORD bytes = MmQueryAllocationSize(saved);
            MmPersistContiguousMemory(saved, bytes, FALSE);
            MmFreeContiguousMemory(saved);
            AvSetSavedDataAddress(NULL);
        }
        AvSendTVEncoderOption(registerBase, AV_OPTION_BLANK_SCREEN, 0, NULL);
        device->m_Miniport.m_FirstFlip = FALSE;
    }
    return;
}
}
