/* Device initialization adapted from RXDK-Libs, 2026 Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * The original January code/PDB determines context handles, layouts and order.
 */
#include "dxgcreate_internal.h"
#include "push_internal.h"
#include "lazy_internal.h"
#include "math_internal.h"
#include "resource_internal.h"
#include "kernel_memory.h"
#include "display_internal.h"
#include "pusher_state.h"
#include "miniport_constants.h"
#include "vshader_internal.h"
#include "surface_internal.h"
#include "pixeljar.h"
#pragma code_seg("D3D")
namespace D3D
{
#include "initial_states.inc"
DWORD CDevice::m_PushBufferSize;
DWORD CDevice::m_PushSegmentSize;
void WINAPI InitializeKelvin(
    void)
{
    CDevice *device = g_pDevice;
    DWORD *push = device->StartPush();
    PushCount(push, 0x180, 3);
    push[1] = NV_DD_DMA_PUSHER_SYNC_NOTIFIER_CONTEXT_DMA_TO_MEMORY;
    push[2] = NV_DD_DMA_CONTEXT_DMA_IN_VIDEO_MEMORY;
    push[3] = NV_DD_DMA_CONTEXT_DMA_IN_VIDEO_MEMORY;
    PushCount(push + 4, 0x190, 6);
    push[5] = NV_DD_DMA_CONTEXT_DMA_TO_VIDEO_MEMORY;
    push[6] = D3D_COLOR_CONTEXT_DMA_IN_VIDEO_MEMORY;
    push[7] = D3D_ZETA_CONTEXT_DMA_IN_VIDEO_MEMORY;
    push[8] = NV_DD_DMA_CONTEXT_DMA_IN_VIDEO_MEMORY;
    push[9] = NV_DD_DMA_CONTEXT_DMA_IN_VIDEO_MEMORY;
    push[10] = D3D_SEMAPHORE_CONTEXT_DMA_IN_MEMORY;
    PushCount(push + 11, 0x1a8, 1);
    push[12] = D3D_CONTEXT_IN_CACHED_MEMORY;
    Push1(push + 13, 0x1d6c, 0);
    device->EndPush(push + 15);
    push = device->StartPush();
    Push1(push, 0x9fc, 1);
    PushCount(push + 2, 0xa50, 4);
    push[3] = NV_FLOAT_ZERO;
    push[4] = NV_FLOAT_ZERO;
    push[5] = NV_FLOAT_ZERO;
    push[6] = NV_FLOAT_ONE;
    Push1(push + 7, 0x16bc, TRUE);
    Push1(push + 9, 0x1e78, 0x210000);
    Push1(push + 11, 0x1d80, TRUE);
    Push1(push + 13, 0x1e68, NV_FLOAT_POSITIVE_INFINITY);
    Push1(push + 15, 0x1d78, TRUE);
    device->EndPush(push + 17);
    device->SetShaderConstantMode(D3DSCM_96CONSTANTS);
    return;
}
void WINAPI InitializeHardware(
    void)
{
    CDevice *device = g_pDevice;
    DWORD *push = device->StartPush();
    Push1(push, 0x4000, D3D_MEMORY_TO_MEMORY_COPY);
    Push1(push + 2, 0x6000, D3D_RECTANGLE_COPY);
    Push1(push + 4, 0x8000, D3D_RECTANGLE_COPY_SURFACES);
    Push1(push + 6, 0x0000, D3D_KELVIN_PRIMITIVE);
    push += 8;
    Push1(push, 0x4180, D3D_MEMCOPY_NOTIFIER_CONTEXT_DMA_TO_MEMORY);
    Push1(push + 2, 0x619c, D3D_RECTANGLE_COPY_SURFACES);
    Push1(push + 4, 0x62fc, 3); /* SRCCOPY operation. */
    Push2(push + 6, 0x8184, NV_DD_DMA_CONTEXT_DMA_IN_VIDEO_MEMORY,
          D3D_COPY_CONTEXT_DMA_IN_VIDEO_MEMORY);
    device->EndPush(push + 9);
    InitializeKelvin();
    return;
}
void WINAPI InitializeD3dState(
    void)
{
    CDevice *device = g_pDevice;
    DWORD i;
    for (i = 0; i < D3DTS_MAX; i++)
        device->SetTransform((D3DTRANSFORMSTATETYPE)i, &g_IdentityMatrix);
    for (i = D3DRS_PS_MAX; i < D3DRS_MAX; i++)
    {
        if (i != D3DRS_MULTISAMPLETYPE)
            device->SetRenderStateNotInline((D3DRENDERSTATETYPE)i,
                                            g_InitialRenderStates[i - D3DRS_PS_MAX]);
    }
    device->SetRenderStateNotInline(D3DRS_ZENABLE, device->m_pAutoDepthBuffer != NULL);
    for (DWORD stage = 0; stage < D3DTSS_MAXSTAGES; stage++)
    {
        for (i = 0; i < D3DTSS_MAX; i++)
            device->SetTextureStageStateNotInline(stage, (D3DTEXTURESTAGESTATETYPE)i,
                                                 g_InitialTextureStates[i]);
        device->SetTextureStageStateNotInline(stage, D3DTSS_TEXCOORDINDEX, stage);
    }
    device->SetTextureStageStateNotInline(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    device->SetTextureStageStateNotInline(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    return;
}
void CDevice::FreeFrameBuffers(
    void)
{
    if (AvGetSavedDataAddress() == NULL)
        AvSendTVEncoderOption(m_Miniport.m_RegisterBase, AV_OPTION_BLANK_SCREEN, TRUE, NULL);
    if (m_pRenderTarget != NULL)
    {
        RecordSurfaceWritePush(m_pRenderTarget);
        InternalReleaseSurface(m_pRenderTarget);
        m_pRenderTarget = NULL;
    }
    if (m_pZBuffer != NULL)
    {
        RecordSurfaceWritePush(m_pZBuffer);
        InternalReleaseSurface(m_pZBuffer);
        m_pZBuffer = NULL;
    }
    if (m_pAutoDepthBuffer != NULL)
    {
        SetTile(1, NULL);
        MmFreeContiguousMemory(m_pAutoDepthBuffer);
        m_pAutoDepthBuffer = NULL;
        ZeroMemory(&m_AutoDepthSurface, sizeof(m_AutoDepthSurface));
    }
    if (m_pFrameBufferBase != NULL)
    {
        SetTile(0, NULL);
        MmFreeContiguousMemory(m_pFrameBufferBase);
        m_pFrameBufferBase = NULL;
        ZeroMemory(m_FrameBufferSurfaces, sizeof(m_FrameBufferSurfaces));
    }
    if (m_pAntiAliasBufferBase != NULL)
    {
        MmFreeContiguousMemory(m_pAntiAliasBufferBase);
        m_pAntiAliasBufferBase = NULL;
    }
    return;
}

HRESULT WINAPI PresentationParametersParameterCheck(
    D3DPRESENT_PARAMETERS *parameters)
{
    DWORD sampleType = parameters->MultiSampleType;
    if (sampleType != D3DMULTISAMPLE_NONE && sampleType != 0)
    {
        if ((sampleType & 0x000f0000UL) > D3DMULTISAMPLE_PREFILTER_FORMAT_A8R8G8B8)
            return D3DERR_INVALIDCALL;
        if (((sampleType >> 4) & 15) < 1 || (sampleType & 15) < 1
            || ((sampleType >> 4) & 15) > 3 || (sampleType & 15) > 3)
            return D3DERR_INVALIDCALL;
    }
    DWORD interval = parameters->FullScreen_PresentationInterval;
    if (interval > D3DPRESENT_INTERVAL_TWO && interval != D3DPRESENT_INTERVAL_IMMEDIATE)
        return D3DERR_INVALIDCALL;
    D3DSWAPEFFECT effect = parameters->SwapEffect;
    if (effect != D3DSWAPEFFECT_DISCARD && effect != D3DSWAPEFFECT_FLIP && effect != 0)
        return D3DERR_INVALIDCALL;
    if (parameters->BackBufferCount > 2)
        return D3DERR_INVALIDCALL;
    return S_OK;
}

/* The January owner frees the push allocation directly. It does not use the
 * later SDK's UninitializePushBuffer path. Completion requires GPU progress. */
void CDevice::UnInit(
    void)
{
    DWORD i;
    if (m_Pusher.m_pPut != NULL)
    {
        KickOffAndWaitForIdle();
        *m_Pusher.m_pPut = ((DWORD)m_pPushBase & 0x0fffffffUL) + 1;
        m_Pusher.m_pPut = m_pPushBase;
        KickOff();
        while (((DWORD)HwGet() & 0x0fffffffUL)
            != ((DWORD)m_pPushBase & 0x0fffffffUL))
        {
            BusyLoop();
        }
    }
    while (m_Miniport.IsFlipPending()) { }
    FreeFrameBuffers();
    if (m_pIndexBuffer != NULL)
        InternalRelease(m_pIndexBuffer);
    for (i = 0; i < D3DTSS_MAXSTAGES; i++)
    {
        if (m_Textures[i] != NULL)
            InternalRelease(m_Textures[i]);
        if (m_Palettes[i] != NULL)
            InternalRelease(m_Palettes[i]);
    }
    for (i = 0; i < 16; i++)
    {
        if (g_Stream[i].pVertexBuffer != NULL)
            InternalRelease(g_Stream[i].pVertexBuffer);
    }
    for (i = 0; i < 16; i++)
    {
        if (m_ReportAllocations[i] != NULL)
            MmFreeContiguousMemory(m_ReportAllocations[i]);
    }
    if (m_pCachedContiguousMemoryBase != NULL)
        MmFreeContiguousMemory(m_pCachedContiguousMemoryBase);
    if (m_pPushBase != NULL)
        MmFreeContiguousMemory(m_pPushBase);
    if (m_pLights != NULL)
        LocalFree(m_pLights);
    if (m_Miniport.m_RegisterBase != NULL)
    {
        m_Miniport.DisableInterrupts();
        m_Miniport.ShutdownEngines();
    }
    ZeroMemory(g_Stream, sizeof(g_Stream));
    return;
}

/* January stores surface headers inside CDevice and allocates the pixel planes
 * separately. Creation/Reset own cleanup of any partial allocations on failure.
 * Width/height/counts and multisample encodings must be valid checked API values. */
HRESULT CDevice::InitializeFrameBuffers(
    D3DPRESENT_PARAMETERS *parameters)
{
    DWORD format;
    DWORD size;
    D3DTILE tile;
    DWORD i;
    DWORD backBufferCount = max(1, parameters->BackBufferCount);
    D3DFORMAT postfilterFormat = MapToLinearD3DFORMAT(parameters->BackBufferFormat);
    D3DFORMAT autoDepthStencilFormat = MapToLinearD3DFORMAT(parameters->AutoDepthStencilFormat);
    DWORD backBufferWidth = parameters->BackBufferWidth;
    DWORD backBufferHeight = parameters->BackBufferHeight;
    m_PresentationInterval = parameters->FullScreen_PresentationInterval;
    m_DisplayWidth = backBufferWidth;
    m_DisplayHeight = backBufferHeight;
    DWORD frameCount = backBufferCount + 1;
    m_FrameBufferCount = frameCount;

    /* Preserve January's low-word render-state storage. Its later prefilter
     * selector consequently has only the retained state bits available. */
    D3D__RenderState[D3DRS_MULTISAMPLETYPE] = parameters->MultiSampleType & 0xffff;
    BOOL doAntiAlias = D3D__RenderState[D3DRS_MULTISAMPLETYPE] & 0x3000;
    if (!doAntiAlias)
        D3D__RenderState[D3DRS_MULTISAMPLETYPE] = D3DMULTISAMPLE_NONE;
    D3DFORMAT prefilterFormat = postfilterFormat;
    switch (D3D__RenderState[D3DRS_MULTISAMPLETYPE] & 0xf0000)
    {
    case D3DMULTISAMPLE_PREFILTER_FORMAT_X1R5G5B5:
        prefilterFormat = D3DFMT_LIN_X1R5G5B5;
        break;
    case D3DMULTISAMPLE_PREFILTER_FORMAT_R5G6B5:
        prefilterFormat = D3DFMT_LIN_R5G6B5;
        break;
    case D3DMULTISAMPLE_PREFILTER_FORMAT_X8R8G8B8:
        prefilterFormat = D3DFMT_LIN_X8R8G8B8;
        break;
    case D3DMULTISAMPLE_PREFILTER_FORMAT_A8R8G8B8:
        prefilterFormat = D3DFMT_LIN_A8R8G8B8;
        break;
    }
    DWORD width = backBufferWidth * ((D3D__RenderState[D3DRS_MULTISAMPLETYPE] >> 4) & 15);
    DWORD height = backBufferHeight * (D3D__RenderState[D3DRS_MULTISAMPLETYPE] & 15);
    DWORD frameSize = PixelJar::EncodeFormat(width, height, 1, 1,
        prefilterFormat, CalcTilePitch(width, prefilterFormat), true, false, false,
        &format, &size);
    frameCount = doAntiAlias ? 1 : frameCount;
    DWORD allocationSize = (frameCount * frameSize + D3DTILE_ALIGNMENT - 1)
        & ~(D3DTILE_ALIGNMENT - 1);
    BYTE *memory = (BYTE *)MmAllocateContiguousMemoryEx(allocationSize, 0,
        0x03ffb000, D3DTILE_ALIGNMENT, PAGE_READWRITE | PAGE_WRITECOMBINE);
    if (memory == NULL)
        return E_OUTOFMEMORY;
    m_pFrameBufferBase = memory;
    for (i = 0; i < frameCount; i++)
    {
        InitializeSurface(&m_FrameBufferSurfaces[i], format, size, memory);
        memory += frameSize;
    }
    /* Original 1920-wide display path leaves these allocations untiled. */
    if (width != 1920)
    {
        /* Ignored depth fields are initialized as well: do not copy
         * indeterminate members through the SetTile value assignment. */
        tile.ZOffset = 0;
        tile.ZStartTag = 0;
        tile.Flags = 0;
        tile.pMemory = m_pFrameBufferBase;
        tile.Size = allocationSize;
        tile.Pitch = PixelJar::GetPitch(&m_FrameBufferSurfaces[0]);
        SetTile(0, &tile);
    }
    if (parameters->EnableAutoDepthStencil)
    {
        frameSize = PixelJar::EncodeFormat(width, height, 1, 1,
            autoDepthStencilFormat, CalcTilePitch(width, autoDepthStencilFormat),
            true, false, false, &format, &size);
        allocationSize = (frameSize + D3DTILE_ALIGNMENT - 1) & ~(D3DTILE_ALIGNMENT - 1);
        memory = (BYTE *)MmAllocateContiguousMemoryEx(allocationSize, 0,
            0x03ffb000, D3DTILE_ALIGNMENT, PAGE_READWRITE | PAGE_WRITECOMBINE);
        if (memory == NULL)
            return E_OUTOFMEMORY;
        m_pAutoDepthBuffer = memory;
        InitializeSurface(&m_AutoDepthSurface, format, size, memory);
        if (width != 1920)
        {
            tile.Flags = D3DTILE_FLAGS_ZBUFFER | D3DTILE_FLAGS_ZCOMPRESS;
            tile.pMemory = memory;
            tile.Size = allocationSize;
            tile.Pitch = PixelJar::GetPitch(&m_AutoDepthSurface);
            tile.ZOffset = 0;
            tile.ZStartTag = 0;
            if (BitsPerPixelOfD3DFORMAT(autoDepthStencilFormat) == 32)
                tile.Flags |= D3DTILE_FLAGS_Z32BITS;
            SetTile(1, &tile);
        }
    }
    if (doAntiAlias)
    {
        frameSize = PixelJar::EncodeFormat(backBufferWidth, backBufferHeight, 1, 1,
            postfilterFormat, 0, true, false, false, &format, &size);
        memory = (BYTE *)MmAllocateContiguousMemoryEx(backBufferCount * frameSize, 0,
            0x03ffb000, D3DTILE_ALIGNMENT, PAGE_READWRITE | PAGE_WRITECOMBINE);
        if (memory == NULL)
            return E_OUTOFMEMORY;
        m_pAntiAliasBufferBase = memory;
        for (i = 1; i <= backBufferCount; i++)
        {
            InitializeSurface(&m_FrameBufferSurfaces[i], format, size, memory);
            memory += frameSize;
        }
    }
    DWORD *push = StartPush();
    DWORD flipLimit = doAntiAlias ? 2 : m_FrameBufferCount;
    PushCount(push, 0x120, 3);
    push[1] = 0;
    push[2] = 1;
    push[3] = flipLimit;
    EndPush(push + 4);
    return S_OK;
}
}
