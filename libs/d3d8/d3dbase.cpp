/* Additional device methods adapted from RXDK-Libs, 2026 Team Resurgent.
 * GPL-3.0-or-later; see libs/d3d8/PROVENANCE.md. */
#include "device_layout.h"
#include "pixeljar.h"
#include "push_internal.h"
#include "resource_internal.h"
#include "display_internal.h"
#include "state_internal.h"
#include "math_internal.h"
#include "memory_internal.h"
#include "kernel_memory.h"
#include "visibility_internal.h"
#include "kernel_dispatcher.h"
#include "hardware_internal.h"
#include "surface_internal.h"
#pragma code_seg("D3D")
using namespace D3D;
void WINAPI D3DDevice_GetDirect3D(
    Direct3D **direct3d)
{
    *direct3d = (Direct3D *)1;
    return;
}
void WINAPI D3DDevice_GetDeviceCaps(
    D3DCAPS8 *caps)
{
    *caps = g_DeviceCaps;
    return;
}
void WINAPI D3DDevice_GetCreationParameters(
    D3DDEVICE_CREATION_PARAMETERS *parameters)
{
    CDevice *device = g_pDevice;
    memset(parameters, 0, sizeof(*parameters));
    parameters->DeviceType = D3DDEVTYPE_HAL;
    parameters->BehaviorFlags |= D3DCREATE_HARDWARE_VERTEXPROCESSING;
    if (device->m_StateFlags & 0x10)
    {
        parameters->BehaviorFlags |= D3DCREATE_PUREDEVICE;
    }
    return;
}
void WINAPI D3DDevice_SetGammaRamp(
    DWORD flags,
    const D3DGAMMARAMP *ramp)
{
    CDevice *device = g_pDevice;
    CMiniport *miniport = &device->m_Miniport;
    DWORD index = device->m_PresentCount & 1;
    D3DGAMMARAMP *destination = &miniport->m_GammaRamp[index];
    *destination = *ramp;
    miniport->m_GammaCurrentIndex = index;
    if (flags & 2)
    {
        miniport->DacProgramGammaRamp(destination);
    }
    else
    {
        miniport->m_GammaUpdated[index] = TRUE;
    }
    return;
}
void WINAPI D3DDevice_GetGammaRamp(
    D3DGAMMARAMP *ramp)
{
    CDevice *device = g_pDevice;
    *ramp = device->m_Miniport.m_GammaRamp[device->m_Miniport.m_GammaCurrentIndex];
    return;
}
void WINAPI D3DDevice_GetTransform(
    D3DTRANSFORMSTATETYPE state,
    D3DMATRIX *matrix)
{
    CDevice *device = g_pDevice;
    *matrix = device->m_Transform[state];
    return;
}
void WINAPI D3DDevice_GetViewport(
    D3DVIEWPORT8 *viewport)
{
    CDevice *device = g_pDevice;
    *viewport = device->m_Viewport;
    return;
}
void WINAPI D3DDevice_SetMaterial(
    const D3DMATERIAL8 *material)
{
    CDevice *device = g_pDevice;
    device->m_Material = *material;
    device->m_DirtyFlags |= 0x20;
    return;
}
void WINAPI D3DDevice_GetMaterial(
    D3DMATERIAL8 *material)
{
    CDevice *device = g_pDevice;
    *material = device->m_Material;
    return;
}
void WINAPI D3DDevice_SetBackMaterial(
    const D3DMATERIAL8 *material)
{
    CDevice *device = g_pDevice;
    device->m_BackMaterial = *material;
    device->m_DirtyFlags |= 0x20;
    return;
}
void WINAPI D3DDevice_GetBackMaterial(
    D3DMATERIAL8 *material)
{
    CDevice *device = g_pDevice;
    *material = device->m_BackMaterial;
    return;
}
void WINAPI D3DDevice_GetLight(
    DWORD index,
    D3DLIGHT8 *light)
{
    CDevice *device = g_pDevice;
    *light = device->m_pLights[index].Light8;
    return;
}
void WINAPI D3DDevice_GetLightEnable(
    DWORD index,
    BOOL *enabled)
{
    CDevice *device = g_pDevice;
    Light *requested = &device->m_pLights[index];
    Light *light = device->m_pActiveLights;
    while (light)
    {
        if (light == requested)
        {
            *enabled = TRUE;
            return;
        }
        light = light->pNext;
    }
    *enabled = FALSE;
    return;
}
void WINAPI D3DDevice_SetVerticalBlankCallback(
    D3DCALLBACK callback)
{
    g_pDevice->m_Miniport.m_pVerticalBlankCallback = callback;
    return;
}
void WINAPI D3DDevice_GetProjectionViewportMatrix(
    D3DMATRIX *matrix)
{
    *matrix = g_pDevice->m_ProjectionViewportTransform;
    return;
}
HRESULT WINAPI D3DDevice_GetModelView(
    D3DMATRIX *matrix)
{
    CDevice *device = g_pDevice;
    if (matrix)
    {
        *matrix = device->m_ModelViewTransform[0];
    }
    return !(device->m_DirtyFlags & 0x80000000);
}
void WINAPI D3DDevice_GetTile(
    DWORD index,
    D3DTILE *tile)
{
    *tile = g_pDevice->m_Tile[index];
    return;
}
void WINAPI D3DDevice_GetScissors(
    DWORD *count,
    BOOL *exclusive,
    D3DRECT *rectangles)
{
    CDevice *device = g_pDevice;
    if (count)
    {
        *count = device->m_ScissorsCount;
    }
    if (exclusive)
    {
        *exclusive = device->m_ScissorsExclusive;
    }
    if (rectangles)
    {
        memcpy(rectangles, device->m_ScissorsRects, device->m_ScissorsCount * sizeof(*rectangles));
    }
    return;
}

HRESULT WINAPI D3DDevice_CreateTexture(
    UINT width,
    UINT height,
    UINT levels,
    DWORD usage,
    D3DFORMAT format,
    D3DPOOL pool,
    D3DTexture **texture)
{
    return CreateTexture(width, height, 1, levels, usage, format, false, false, (D3DBaseTexture **)texture);
}
HRESULT WINAPI D3DDevice_CreateVolumeTexture(
    UINT width,
    UINT height,
    UINT depth,
    UINT levels,
    DWORD usage,
    D3DFORMAT format,
    D3DPOOL pool,
    D3DVolumeTexture **texture)
{
    return CreateTexture(width, height, depth, levels, usage, format, false, true, (D3DBaseTexture **)texture);
}
HRESULT WINAPI D3DDevice_CreateCubeTexture(
    UINT edge,
    UINT levels,
    DWORD usage,
    D3DFORMAT format,
    D3DPOOL pool,
    D3DCubeTexture **texture)
{
    return CreateTexture(edge, edge, 1, levels, usage, format, true, false, (D3DBaseTexture **)texture);
}
HRESULT WINAPI D3DDevice_CreateRenderTarget(
    UINT width,
    UINT height,
    D3DFORMAT format,
    D3DMULTISAMPLE_TYPE multisample,
    BOOL lockable,
    D3DSurface **surface)
{
    return CreateStandAloneSurface(width, height, format, true, surface);
}
HRESULT WINAPI D3DDevice_CreateDepthStencilSurface(
    UINT width,
    UINT height,
    D3DFORMAT format,
    D3DMULTISAMPLE_TYPE multisample,
    D3DSurface **surface)
{
    return CreateStandAloneSurface(width, height, format, true, surface);
}
HRESULT WINAPI D3DDevice_CreateImageSurface(
    UINT width,
    UINT height,
    D3DFORMAT format,
    D3DSurface **surface)
{
    return CreateStandAloneSurface(width, height, format, false, surface);
}
void WINAPI D3DDevice_SetTransform(
    D3DTRANSFORMSTATETYPE State,
    const D3DMATRIX *pMatrix)
{
    DWORD state = (DWORD)State;
    CDevice *pDevice = g_pDevice;
    pDevice->m_Transform[state] = *pMatrix;
    pDevice->m_DirtyFlags |= g_TransformDirtyTable[state];
    if (State == D3DTS_PROJECTION)
    {
        if (pMatrix->_33 == pMatrix->_34 || pMatrix->_33 == 0.0f)
        {
            pDevice->m_WNear = 0.0f;
            pDevice->m_WFar = 1.0f;
        }
        else
        {
            pDevice->m_WNear = pMatrix->_44 - pMatrix->_43 / pMatrix->_33 * pMatrix->_34;
            pDevice->m_WFar = (pMatrix->_44 - pMatrix->_43) / (pMatrix->_33 - pMatrix->_34) * pMatrix->_34 + pMatrix->_44;
        }
        pDevice->m_InverseWFar = 1.0f / pDevice->m_WFar;
        pDevice->m_StateFlags &= ~2;
        if (pMatrix->_14 == 0.0f && pMatrix->_24 == 0.0f && pMatrix->_34 == 0.0f && pMatrix->_44 == 1.0f)
        {
            pDevice->m_StateFlags |= 2;
        }
        UpdateProjectionViewportTransform();
        CommonSetPassthroughProgram(pDevice);
    }
    return;
}
void WINAPI D3DDevice_MultiplyTransform(
    D3DTRANSFORMSTATETYPE State,
    const D3DMATRIX *pMatrix)
{
    __declspec(align(16)) D3DMATRIX result;
    __declspec(align(16)) D3DMATRIX temp = *pMatrix;
    DWORD state = (DWORD)State;
    CDevice *pDevice = g_pDevice;
    MatrixProduct4x4(&result, &temp, &pDevice->m_Transform[state]);
    D3DDevice_SetTransform(State, &result);
    return;
}
ULONG WINAPI D3DDevice_AddRef(
    void)
{
    return ++g_pDevice->m_cRefs;
}
ULONG WINAPI D3DDevice_Release(
    void)
{
    CDevice *device = g_pDevice;
    if (device->m_cRefs == 1)
    {
        device->UnInit();
        g_pPushBuffer = NULL;
        g_pDevice = NULL;
        memset(device, 0, sizeof(*device));
        return 0;
    }
    return --device->m_cRefs;
}
void WINAPI D3DDevice_BlockUntilIdle(
    void)
{
    KickOffAndWaitForIdle();
    return;
}
void WINAPI D3DDevice_BlockOnFence(
    DWORD fence)
{
    BlockOnTime(fence, FALSE);
    return;
}
void WINAPI D3DDevice_KickPushBuffer(
    void)
{
    g_pDevice->KickOff();
    return;
}
DWORD WINAPI D3DDevice_InsertFence(
    void)
{
    return SetFence(0);
}
void WINAPI D3DDevice_GetDisplayMode(
    D3DDISPLAYMODE *mode)
{
    CDevice *device = g_pDevice;
    mode->Width = device->m_DisplayWidth;
    mode->Height = device->m_DisplayHeight;
    mode->RefreshRate = device->m_Miniport.GetRefreshRate();
    mode->Flags = CMiniport::GetPresentFlagsFromAvInfo(device->m_Miniport.m_CurrentAvInfo);
    mode->Format = PixelJar::GetFormat(&device->m_FrameBufferSurfaces[1]);
    return;
}
void WINAPI D3DDevice_GetBackBuffer(
    INT BackBuffer,
    D3DBACKBUFFER_TYPE Type,
    D3DSurface **surface)
{
    CDevice *device = g_pDevice;
    if (BackBuffer == -1) { BackBuffer = 1; }
    else if (BackBuffer) { ++BackBuffer; }
    *surface = &device->m_FrameBufferSurfaces[BackBuffer];
    (*surface)->AddRef();
    return;
}
void WINAPI D3DDevice_GetRenderTarget(
    D3DSurface **surface)
{
    *surface = g_pDevice->m_pRenderTarget;
    if (*surface) { (*surface)->AddRef(); }
    return;
}
HRESULT WINAPI D3DDevice_GetDepthStencilSurface(
    D3DSurface **surface)
{
    *surface = g_pDevice->m_pZBuffer;
    if (*surface)
    {
        (*surface)->AddRef();
        return S_OK;
    }
    return D3DERR_NOTFOUND;
}
void WINAPI D3DDevice_GetTexture(
    DWORD Stage,
    D3DBaseTexture **texture)
{
    CDevice *device = g_pDevice;
    D3DBaseTexture **textures = device->m_Textures;
    if (textures[Stage]) { textures[Stage]->AddRef(); }
    *texture = textures[Stage];
    return;
}
void WINAPI D3DDevice_GetPalette(
    DWORD Stage,
    D3DPalette **palette)
{
    CDevice *device = g_pDevice;
    D3DPalette **palettes = device->m_Palettes;
    if (palettes[Stage]) { palettes[Stage]->AddRef(); }
    *palette = palettes[Stage];
    return;
}
void WINAPI D3DDevice_SetPalette(
    DWORD Stage,
    D3DPalette *palette)
{
    CDevice *device = g_pDevice;
    if (device->m_Palettes[Stage])
    {
        device->RecordResourceReadPush(device->m_Palettes[Stage]);
        InternalRelease(device->m_Palettes[Stage]);
    }
    device->m_Palettes[Stage] = palette;
    if (palette)
    {
        InternalAddRef(palette);
        DWORD size = palette->Common >> 28;
        DWORD *push = device->StartPush();
        Push1(push, 0x1b20 + Stage * 64, palette->Data | size);
        device->EndPush(push + 2);
    }
    return;
}
void WINAPI D3DDevice_SetIndices(
    D3DIndexBuffer *buffer,
    UINT base)
{
    CDevice *device = g_pDevice;
    if (buffer)
    {
        InternalAddRef(buffer);
        D3D__IndexData = (WORD *)buffer->Data;
    }
    else { D3D__IndexData = NULL; }
    if (device->m_pIndexBuffer) { InternalRelease(device->m_pIndexBuffer); }
    device->m_pIndexBuffer = buffer;
    device->m_IndexBase = base;
    return;
}
void WINAPI D3DDevice_GetIndices(
    D3DIndexBuffer **buffer,
    UINT *base)
{
    CDevice *device = g_pDevice;
    D3DIndexBuffer *current = device->m_pIndexBuffer;
    if (current)
    {
        current->AddRef();
        *buffer = current;
        *base = device->m_IndexBase;
    }
    else
    {
        *buffer = NULL;
        *base = 0;
    }
    return;
}
void WINAPI D3DDevice_Nop(
    void)
{
    CDevice *device = g_pDevice;
    DWORD *push = device->StartPush();
    Push1(push, NV097_NO_OPERATION, 0);
    device->EndPush(push + 2);
    return;
}
void WINAPI D3DDevice_FlushVertexCache(
    void)
{
    CDevice *device = g_pDevice;
    DWORD *push = device->StartPush();
    Push1(push, 0x1710, 0);
    device->EndPush(push + 2);
    return;
}

void WINAPI D3DDevice_SetViewport(
    const D3DVIEWPORT8 *pViewport)
{
    D3DSurface *pSurface;
    CDevice *pDevice = g_pDevice;
    if (pDevice->m_pRenderTarget == &pDevice->m_FrameBufferSurfaces[0])
    {
        pSurface = &pDevice->m_FrameBufferSurfaces[1];
    }
    else { pSurface = pDevice->m_pRenderTarget; }
    DWORD left = max(pViewport->X, 0);
    DWORD top = max(pViewport->Y, 0);
    DWORD right = min(pViewport->X + pViewport->Width, (INT)PixelJar::GetWidth(pSurface));
    DWORD bottom = min(pViewport->Y + pViewport->Height, (INT)PixelJar::GetHeight(pSurface));
    pDevice->m_Viewport.X = left;
    pDevice->m_Viewport.Y = top;
    pDevice->m_Viewport.Width = right - left;
    pDevice->m_Viewport.Height = bottom - top;
    pDevice->m_Viewport.MinZ = pViewport->MinZ;
    pDevice->m_Viewport.MaxZ = pViewport->MaxZ;
    UpdateProjectionViewportTransform();
    D3DDevice_SetScissors(0, 0, NULL);
    DWORD *push = pDevice->StartPush();
    push = CommonSetViewport(pDevice, push);
    pDevice->EndPush(push);
    pDevice->m_DirtyFlags |= 1;
    return;
}
void WINAPI D3DDevice_SetTexture(
    DWORD stage,
    D3DBaseTexture *texture)
{
    DWORD *push;
    CDevice *device = g_pDevice;
    D3DBaseTexture *previous = device->m_Textures[stage];
    if (previous)
    {
        device->RecordResourceReadPush(previous);
        InternalRelease(previous);
    }
    device->m_Textures[stage] = texture;
    if (!texture)
    {
        push = device->StartPush();
        Push1(push, 0x1b0c + stage * 64, 0);
        device->EndPush(push + 2);
        device->m_TextureCubemapAndDimension[stage] = ~0UL;
        device->m_DirtyFlags |= 0x408;
    }
    else
    {
        InternalAddRef(texture);
        push = device->StartPush();
        Push2(push, 0x1b00 + stage * 64, texture->Data, texture->Format);
        push += 3;
        if (texture->Size != 0)
        {
            DWORD width = (texture->Size & D3DSIZE_WIDTH_MASK) + 1;
            DWORD height = ((texture->Size & D3DSIZE_HEIGHT_MASK) >> D3DSIZE_HEIGHT_SHIFT) + 1;
            DWORD pitch = PixelJar::GetPitch(texture);
            Push1(push, 0x1b10 + stage * 64, pitch << 16);
            Push1(push + 2, 0x1b1c + stage * 64, (width << 16) | height);
            push += 4;
        }
        DWORD dimensions = texture->Format & (D3DFORMAT_CUBEMAP | D3DFORMAT_DIMENSION_MASK);
        if (device->m_TextureCubemapAndDimension[stage] != dimensions)
        {
            device->m_TextureCubemapAndDimension[stage] = dimensions;
            /* January always reprograms CONTROL0 when these format bits change. */
            Push1(push, 0x1b0c + stage * 64, device->m_TextureControl0Enabled[stage]);
            push += 2;
            device->m_DirtyFlags |= 0x400;
            if (!previous) device->m_DirtyFlags |= 8;
        }
        device->EndPush(push);
    }
    return;
}
/* BUG (original): batches advance caller-supplied rectangle/point pointers by
 * 16 even when the final batch is shorter. Strict C pointer bounds therefore
 * require caller backing arrays rounded up to a 16-entry batch. The binary
 * proves the fixed increment; a corrected build would advance by batch.
 * Swizzled copies additionally require writable rectangle storage, as noted
 * at the actual original store below. Neither behavior is introduced to steer
 * the compiler. Normal matching tests do not establish arbitrary-input safety.
 */
void WINAPI D3DDevice_CopyRects(
    D3DSurface *sourceSurface,
    const RECT *sourceRects,
    UINT count,
    D3DSurface *destinationSurface,
    const POINT *destinationPoints)
{
    const DWORD maxRectangles = 16;
    RECT rectangles[maxRectangles];
    POINT points[maxRectangles];
    DWORD swizzledColumns, swizzledRows;
    DWORD bytesPerPixel = PixelJar::GetBitsPerPixel(sourceSurface) / 8;
    DWORD sourcePitch = PixelJar::GetPitch(sourceSurface);
    DWORD destinationPitch = PixelJar::GetPitch(destinationSurface);
    BOOL swizzled = PixelJar::IsSwizzled(sourceSurface) || PixelJar::IsCompressed(sourceSurface);
    CDevice *device = g_pDevice;
    if (count == 0) count = 1;
    if (swizzled)
    {
        DWORD slice = PixelJar::GetSlice(sourceSurface);
        bytesPerPixel = 1;
        if (slice <= 8128)
        {
            sourcePitch = (slice + 63) & ~63;
            destinationPitch = sourcePitch;
            swizzledColumns = slice;
            swizzledRows = 1;
        }
        else
        {
            sourcePitch = 4096;
            destinationPitch = 4096;
            swizzledColumns = 4096;
            swizzledRows = slice / 4096;
        }
    }
    do
    {
        RECT *rects;
        POINT *destinations;
        DWORD batch = min(maxRectangles, count);
        count -= batch;
        if (sourceRects)
        {
            rects = (RECT *)sourceRects;
            sourceRects += maxRectangles;
        }
        else
        {
            rects = rectangles;
            for (DWORD i = 0; i < batch; i++)
            {
                rectangles[i].top = 0;
                rectangles[i].bottom = PixelJar::GetHeight(sourceSurface);
                rectangles[i].left = 0;
                rectangles[i].right = PixelJar::GetWidth(sourceSurface);
            }
        }
        if (destinationPoints)
        {
            destinations = (POINT *)destinationPoints;
            destinationPoints += maxRectangles;
        }
        else
        {
            destinations = points;
            for (DWORD i = 0; i < batch; i++)
            {
                points[i].x = rects[i].left;
                points[i].y = rects[i].top;
            }
        }
        if (swizzled)
        {
            /* BUG (original): the target writes through the const-qualified
             * input rectangle pointer. This matching entry requires mutable
             * backing storage when explicit swizzled rectangles are supplied.
             * A corrected build would use a local rectangle copy. */
            for (DWORD i = 0; i < batch; i++)
            {
                rects[i].left = 0;
                rects[i].top = 0;
                rects[i].right = swizzledColumns;
                rects[i].bottom = swizzledRows;
            }
        }
        GPUCopyVideoRectangle(bytesPerPixel, sourceSurface->Data, sourcePitch,
            destinationSurface->Data, destinationPitch, batch, rects, destinations);
    } while (count);
    device->RecordSurfaceWritePush(sourceSurface);
    device->RecordSurfaceWritePush(destinationSurface);
    return;
}
void WINAPI D3DDevice_SetScissors(
    DWORD count,
    BOOL exclusive,
    const D3DRECT *rects)
{
    const D3DRECT *current;
    D3DRECT viewport;
    DWORD i;
    CDevice *device = g_pDevice;
    if (count == 0)
    {
        viewport.x1 = device->m_Viewport.X;
        viewport.y1 = device->m_Viewport.Y;
        viewport.x2 = viewport.x1 + device->m_Viewport.Width;
        viewport.y2 = viewport.y1 + device->m_Viewport.Height;
        exclusive = FALSE;
        rects = &viewport;
        count = 1;
    }
    DWORD *push = device->StartPush();
    Push1(push, 0x2b4, exclusive);
    push += 2;
    float xScale = device->m_SuperSampleScaleX;
    float yScale = device->m_SuperSampleScaleY;
    for (current = rects, i = 0; i < count; i++, current++)
    {
        Push1(push, 0x2c0 + i * 4,
            Round(xScale * current->x1) | ((Round(xScale * current->x2) - 1) << 16));
        Push1(push + 2, 0x2e0 + i * 4,
            Round(yScale * current->y1) | ((Round(yScale * current->y2) - 1) << 16));
        push += 4;
    }
    device->EndPush(push);
    memcpy(&device->m_ScissorsRects[0], rects, count * sizeof(D3DRECT));
    device->m_ScissorsCount = count;
    device->m_ScissorsExclusive = exclusive;
    return;
}

BYTE *WINAPI D3D::GetVisibilityAddress(
    DWORD index)
{
    CDevice *device = g_pDevice;
    DWORD page = index >> 8;
    if (device->m_ReportAllocations[page] == NULL)
    {
        device->m_ReportAllocations[page] = MmAllocateContiguousMemoryEx(
            4096, 0, 0x00ffffff, 0, PAGE_READWRITE);
        if (device->m_ReportAllocations[page] == NULL)
            return NULL;
    }
    VisibilityReport *report = (VisibilityReport *)
        ((BYTE *)device->m_ReportAllocations[page] + (index & 255) * 16);
    if (!(device->m_StateFlags & 4))
        report->Status = 0xffffffff;
    return (BYTE *)report;
}
void WINAPI D3DDevice_BeginVisibilityTest(
    void)
{
    CDevice *device = g_pDevice;
    DWORD *push = device->StartPush();
    Push2(push, 0x17c8, 1, 1);
    device->EndPush(push + 3);
    return;
}
HRESULT WINAPI D3DDevice_EndVisibilityTest(
    DWORD index)
{
    BYTE *address = GetVisibilityAddress(index);
    if (address == NULL)
        return E_OUTOFMEMORY;
    CDevice *device = g_pDevice;
    DWORD *push = device->StartPush();
    Push2(push, 0x17cc, 0, ((DWORD)address & 0x02ffffff) | 0x01000000);
    device->EndPush(push + 3);
    device->KickOff();
    return S_OK;
}
HRESULT WINAPI D3DDevice_GetVisibilityTestResult(
    DWORD index,
    UINT *result,
    ULONGLONG *timeStamp)
{
    CDevice *device = g_pDevice;
    VisibilityReport *report = (VisibilityReport *)
        ((BYTE *)device->m_ReportAllocations[index >> 8] + (index & 255) * 16);
    if (report->Status == 0xffffffff)
        return D3DERR_TESTINCOMPLETE;
    *result = report->Samples;
    if (timeStamp)
        *timeStamp = report->TimeStamp;
    return S_OK;
}

HRESULT WINAPI D3DDevice_Reset(
    D3DPRESENT_PARAMETERS *parameters)
{
    CDevice *device = g_pDevice;
    KickOffAndWaitForIdle();
    while (device->m_Miniport.IsFlipPending())
    {
        /* The original API waits for the final queued display flip. */
    }
    device->FreeFrameBuffers();
    HRESULT result = device->InitializeFrameBuffers(parameters);
    if (FAILED(result))
    {
        device->FreeFrameBuffers();
        return result;
    }
    device->m_Miniport.SetVideoMode(parameters->BackBufferWidth,
        parameters->BackBufferHeight, parameters->FullScreen_RefreshRateInHz,
        parameters->Flags, parameters->BackBufferFormat,
        parameters->FullScreen_PresentationInterval,
        PixelJar::GetPitch(&device->m_FrameBufferSurfaces[1]));
    D3DDevice_SetRenderTarget(&device->m_FrameBufferSurfaces[0],
        device->m_pAutoDepthBuffer ? &device->m_AutoDepthSurface : NULL);
    D3DDevice_Clear(0, NULL, D3DCLEAR_STENCIL | D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
    return S_OK;
}

void WINAPI D3DDevice_SetRenderTarget(
    D3DSurface *renderTarget,
    D3DSurface *zBuffer)
{
    CDevice *device = g_pDevice;
    if (renderTarget == NULL)
        renderTarget = device->m_pRenderTarget;
    DWORD width = PixelJar::GetWidth(renderTarget);
    DWORD height = PixelJar::GetHeight(renderTarget);
    DWORD renderPitch = PixelJar::GetPitch(renderTarget);
    if (renderTarget == &device->m_FrameBufferSurfaces[0])
    {
        DWORD sampling = D3D__RenderState[D3DRS_MULTISAMPLETYPE];
        if ((sampling & 0x3000) && (sampling & 0x1000))
        {
            width >>= 1;
            if ((sampling & 15) > 1)
                height >>= 1;
        }
    }
    DWORD depthPitch = zBuffer ? PixelJar::GetPitch(zBuffer) : renderPitch;
    InternalAddRef(renderTarget);
    if (device->m_pRenderTarget)
    {
        device->RecordSurfaceWritePush(device->m_pRenderTarget);
        InternalReleaseSurface(device->m_pRenderTarget);
    }
    device->m_pRenderTarget = renderTarget;
    if (device->m_pZBuffer)
    {
        device->RecordSurfaceWritePush(device->m_pZBuffer);
        InternalReleaseSurface(device->m_pZBuffer);
    }
    device->m_pZBuffer = zBuffer;
    if (zBuffer)
    {
        InternalAddRef(zBuffer);
        PixelJar::GetDepthBufferScale(zBuffer, &device->m_ZScale);
    }
    DWORD *push = device->StartPush();
    DWORD renderOffset = renderTarget->Data;
    DWORD depthOffset = zBuffer ? zBuffer->Data : 0;
    DWORD pitch = (renderPitch & 0xffff) | (depthPitch << 16);
    /* Target-evidenced NV2A surface switch synchronization: each register
     * update is bracketed by NOP and idle methods, and the sequence is repeated
     * twice. These are hardware command words, not native instruction data. */
    for (DWORD i = 0; i < 2; i++)
    {
        Push1(push, NV097_NO_OPERATION, 0);
        Push1(push + 2, 0x20c, pitch);
        Push1(push + 4, NV097_NO_OPERATION, 0);
        Push1(push + 6, NV097_WAIT_FOR_IDLE, 0);
        push += 8;
        Push1(push, NV097_NO_OPERATION, 0);
        Push1(push + 2, 0x210, renderOffset);
        Push1(push + 4, NV097_NO_OPERATION, 0);
        Push1(push + 6, NV097_WAIT_FOR_IDLE, 0);
        push += 8;
        Push1(push, NV097_NO_OPERATION, 0);
        Push1(push + 2, 0x214, depthOffset);
        Push1(push + 4, NV097_NO_OPERATION, 0);
        Push1(push + 6, NV097_WAIT_FOR_IDLE, 0);
        push += 8;
    }
    Push2(push, 0x200, width << 16, height << 16);
    push = CommonSetControl0(device, push + 3);
    Push1(push, 0x30c, D3D__RenderState[D3DRS_ZENABLE] && device->m_pZBuffer);
    Push1(push + 2, 0x32c, D3D__RenderState[D3DRS_STENCILENABLE] && device->m_pZBuffer);
    push += 4;
    if ((device->m_StateFlags & 0x100) && renderTarget == &device->m_FrameBufferSurfaces[0])
    {
        device->m_StateFlags &= ~0x100;
        Push1(push, 0x130, 0);
        push += 2;
    }
    device->EndPush(push);
    CommonSetSurfaceFormat(device);
    CommonSetPassthroughProgram(device);
    D3DVIEWPORT8 viewport;
    viewport.X = 0;
    viewport.Y = 0;
    viewport.Width = width;
    viewport.Height = height;
    viewport.MinZ = 0.0f;
    viewport.MaxZ = 1.0f;
    D3DDevice_SetViewport(&viewport);
    D3DDevice_SetRenderStateNotInline(D3DRS_DXT1NOISEENABLE,
        D3D__RenderState[D3DRS_DXT1NOISEENABLE]);
    return;
}
HRESULT WINAPI D3DDevice_GetVertexBlendModelView(
    UINT count,
    D3DMATRIX *modelViews,
    D3DMATRIX *projectionViewport)
{
    CDevice *device = g_pDevice;
    if (modelViews)
        memcpy(modelViews, device->m_ModelViewTransform, count * sizeof(D3DMATRIX));
    if (projectionViewport)
        *projectionViewport = device->m_ProjectionViewport;
    return (device->m_DirtyFlags & 0x80000000) ? S_OK : S_FALSE;
}
void WINAPI D3DDevice_SetModelView(
    const D3DMATRIX *modelView,
    const D3DMATRIX *inverseModelView,
    const D3DMATRIX *composite)
{
    CDevice *device = g_pDevice;
    if (composite == NULL)
    {
        device->m_DirtyFlags &= ~0x80000000;
        device->m_DirtyFlags |= 2;
        return;
    }
    device->m_DirtyFlags &= ~2;
    device->m_DirtyFlags |= 0x80000000;
    DWORD *push = device->StartPush();
    DumpMatrixTransposed(push, 0x400480, modelView);
    DumpMatrixTransposed(push + 17, 0x400680, composite);
    push += 34;
    if (inverseModelView)
    {
        PushCount(push, 0x580, 12);
        memcpy(push + 1, inverseModelView, 12 * sizeof(float));
        push += 13;
    }
    device->EndPush(push);
    if (!(device->m_StateFlags & 0x10))
        device->m_ModelViewTransform[0] = *modelView;
    return;
}

/* Display controls assume an initialized device; field sampling is actual GPIO
 * I/O and register reads below are genuine volatile MMIO, not byte steering. */
void WINAPI D3DDevice_BlockUntilVerticalBlank(
    void)
{
    CDevice *device = g_pDevice;
    KeClearEvent(&device->m_Miniport.m_VerticalBlankEvent);
    KeWaitForSingleObject(&device->m_Miniport.m_VerticalBlankEvent,
        UserRequest, UserMode, FALSE, NULL);
    return;
}
void WINAPI D3DDevice_SetFlickerFilter(
    DWORD filter)
{
    AvSendTVEncoderOption(g_pDevice->m_Miniport.m_RegisterBase,
        AV_OPTION_FLICKER_FILTER, filter, NULL);
    return;
}
void WINAPI D3DDevice_SetSoftDisplayFilter(
    BOOL enable)
{
    AvSendTVEncoderOption(g_pDevice->m_Miniport.m_RegisterBase,
        AV_OPTION_SOFT_DISPLAY_FILTER, enable, NULL);
    return;
}
void WINAPI D3DDevice_GetRasterStatus(
    D3DRASTER_STATUS *status)
{
    CDevice *device = g_pDevice;
    DWORD line = REG_RD32(device->m_NvBase, NV2A_RASTER_POSITION)
        & NV2A_RASTER_POSITION_MASK;
    if (line != 0 && line < device->m_DisplayHeight)
    {
        status->InVBlank = FALSE;
        status->ScanLine = line;
    }
    else
    {
        status->InVBlank = TRUE;
        status->ScanLine = 0;
    }
    return;
}
BOOL WINAPI D3DDevice_IsBusy(
    void)
{
    CDevice *device = g_pDevice;
    _HWREG *registerBase = device->m_NvBase;
    return (((DWORD)device->HwGet() & 0x0fffffff) !=
        ((DWORD)device->m_pKickOff & 0x0fffffff)) ||
        REG_RD32(registerBase, NV2A_GRAPHICS_STATUS) != 0;
}
void WINAPI D3DDevice_GetDisplayFieldStatus(
    D3DFIELD_STATUS *status)
{
    CDevice *device = g_pDevice;
    status->VBlankCount = device->m_Miniport.m_VBlankCount;
    if (device->m_Miniport.m_CurrentAvInfo &
        (AV_FLAGS_INTERLACED | AV_FLAGS_FIELD))
    {
        status->Field = (BYTE)device->m_Miniport.IsOddField() ? D3DFIELD_ODD : D3DFIELD_EVEN;
    }
    else
    {
        status->Field = D3DFIELD_PROGRESSIVE;
    }
    return;
}
void WINAPI D3DDevice_SetTile(
    DWORD index,
    D3DTILE *tileInfo)
{
    CDevice *device = g_pDevice;
    D3DTILE tile;
    if (tileInfo != NULL && tileInfo->pMemory != NULL)
    {
        tile = *tileInfo;
        if (!(tile.Flags & D3DTILE_FLAGS_ZCOMPRESS))
        {
            tile.ZOffset = 0;
            tile.ZStartTag = 0;
        }
        device->m_Tile[index] = tile;
        device->m_Miniport.CreateTile(index, (DWORD)tile.pMemory & 0x03ffffff,
            tile.Size, tile.Pitch, tile.Flags, tile.ZStartTag, tile.ZOffset);
    }
    else
    {
        device->m_Miniport.DestroyTile(index, device->m_Tile[index].ZOffset);
        memset(&device->m_Tile[index], 0, sizeof(D3DTILE));
    }
    return;
}
/* Preserves the original partial-bank iteration: i is compared with the batch
 * count, not start+count. These APIs assume a valid tag partition/range and an
 * idle GPU. Nonzero starting indices can transfer fewer words than requested;
 * changing that historical quirk intentionally changes the target bytes. */
void WINAPI D3DDevice_SetTileCompressionTagBits(
    DWORD partition,
    DWORD address,
    DWORD *data,
    DWORD count)
{
    _HWREG *registerBase = g_pDevice->m_NvBase;
    DWORD dataAddress = address % 16;
    address /= 16;
    while (count)
    {
        REG_WR32(registerBase, NV2A_TAG_INDEX,
            (((NV2A_TAG_PARTITION_SELECT + partition) & NV2A_TAG_SELECT_MASK) << 16)
            | ((address++ & NV2A_TAG_ADDRESS_MASK) << 6));
        DWORD dwordCount = min(count, 16 - dataAddress);
        count -= dwordCount;
        for (DWORD i = dataAddress; i < dwordCount; i++)
        {
            REG_WR32(registerBase, NV2A_TAG_DATA + i * sizeof(DWORD), *data++);
        }
        dataAddress = 0;
    }
    return;
}
void WINAPI D3DDevice_GetTileCompressionTagBits(
    DWORD partition,
    DWORD address,
    DWORD *data,
    DWORD count)
{
    _HWREG *registerBase = g_pDevice->m_NvBase;
    DWORD dataAddress = address % 16;
    address /= 16;
    while (count)
    {
        REG_WR32(registerBase, NV2A_TAG_INDEX,
            (((NV2A_TAG_PARTITION_SELECT + partition) & NV2A_TAG_SELECT_MASK) << 16)
            | ((address++ & NV2A_TAG_ADDRESS_MASK) << 6));
        DWORD dwordCount = min(count, 16 - dataAddress);
        count -= dwordCount;
        for (DWORD i = dataAddress; i < dwordCount; i++)
        {
            *data++ = REG_RD32(registerBase, NV2A_TAG_DATA + i * sizeof(DWORD));
        }
        dataAddress = 0;
    }
    return;
}

/* As with GetTileCompressionTagBits, the original first partial-bank bound is
 * retained. ZStartTag/ZEndTag must describe a valid aligned hardware-tag range. */
DWORD WINAPI D3DDevice_GetTileCompressionTags(
    DWORD startTag,
    DWORD endTag)
{
    KickOffAndWaitForIdle();
    _HWREG *registerBase = g_pDevice->m_NvBase;
    DWORD compressedTags = 0;
    DWORD dwordsPerPartition = (endTag - startTag) / (4 * 32);
    DWORD partitionAddressStart = startTag / (4 * 32);
    for (DWORD partition = 0; partition < 4; partition++)
    {
        DWORD dwordsToRead = dwordsPerPartition;
        DWORD dataStart = partitionAddressStart % 16;
        for (DWORD addressOffset = 0; dwordsToRead; addressOffset++)
        {
            REG_WR32(registerBase, NV2A_TAG_INDEX,
                (((NV2A_TAG_PARTITION_SELECT + partition) & NV2A_TAG_SELECT_MASK) << 16)
                | (((partitionAddressStart / 16 + addressOffset) & NV2A_TAG_ADDRESS_MASK) << 6));
            DWORD dwordCount = min(dwordsToRead, 16 - dataStart);
            dwordsToRead -= dwordCount;
            for (DWORD i = dataStart; i < dwordCount; i++)
            {
                DWORD data = REG_RD32(registerBase,
                    NV2A_TAG_DATA + i * sizeof(DWORD));
                while (data)
                {
                    compressedTags++;
                    data &= data - 1;
                }
            }
            dataStart = 0;
        }
    }
    return compressedTags;
}

/* The source must contain the destination mip chain. Compressed logarithmic
 * dimensions stop at their minimum block size; faces occupy 128-byte strides. */
void WINAPI D3DDevice_UpdateTexture(
    D3DBaseTexture *source,
    D3DBaseTexture *destination)
{
    DWORD sourceMips = PixelJar::GetMipmapLevelCount(source);
    DWORD size;
    DWORD sourceSize;
    DWORD sourceOffset = 0;
    if (sourceMips == 1)
    {
        DWORD depth = PixelJar::GetDepth(source);
        size = PixelJar::GetSlice(destination) * depth;
        sourceSize = size;
    }
    else
    {
        DWORD texelSize = PixelJar::GetBitsPerPixel(source);
        DWORD logMin = MinimumTextureSizeOfD3DFORMAT(PixelJar::GetFormat(source));
        size = 0;
        DWORD destinationWidth = PixelJar::GetLogWidth(destination);
        DWORD destinationHeight = PixelJar::GetLogHeight(destination);
        DWORD destinationDepth = PixelJar::GetLogDepth(destination);
        DWORD sourceWidth = PixelJar::GetLogWidth(source);
        DWORD sourceHeight = PixelJar::GetLogHeight(source);
        DWORD sourceDepth = PixelJar::GetLogDepth(source);
        DWORD currentWidth = destinationWidth;
        DWORD currentHeight = destinationHeight;
        DWORD currentDepth = destinationDepth;
        for (DWORD i = PixelJar::GetMipmapLevelCount(destination); i != 0; i--)
        {
            size += (1UL << (currentWidth + currentHeight + currentDepth)) * texelSize / 8;
            if (currentWidth > logMin) currentWidth--;
            if (currentHeight > logMin) currentHeight--;
            if (currentDepth > logMin) currentDepth--;
        }
        sourceSize = 0;
        currentWidth = sourceWidth;
        currentHeight = sourceHeight;
        currentDepth = sourceDepth;
        for (DWORD i = sourceMips; i != 0; i--)
        {
            if (currentWidth == destinationWidth && currentHeight == destinationHeight &&
                currentDepth == destinationDepth)
            {
                sourceOffset = sourceSize;
            }
            sourceSize += (1UL << (currentWidth + currentHeight + currentDepth)) * texelSize / 8;
            if (currentWidth > logMin) currentWidth--;
            if (currentHeight > logMin) currentHeight--;
            if (currentDepth > logMin) currentDepth--;
        }
    }
    if (source->Format & D3DFORMAT_CUBEMAP)
    {
        DWORD sourceStride = (sourceSize + 127) & ~127UL;
        DWORD destinationStride = (size + 127) & ~127UL;
        for (DWORD i = 0; i < 6; i++)
        {
            DWORD sourceData = source->Data + i * sourceStride;
            DWORD destinationData = destination->Data + i * destinationStride;
            GPUCopyMemory(sourceData + sourceOffset, destinationData, size);
        }
    }
    else
    {
        DWORD sourceData = source->Data;
        DWORD destinationData = destination->Data;
        GPUCopyMemory(sourceData + sourceOffset, destinationData, size);
    }
    return;
}
HRESULT WINAPI D3DDevice_LightEnable(
    DWORD index,
    BOOL enable)
{
    CDevice *device = g_pDevice;
    if (index >= device->m_LightCount || !(device->m_pLights[index].Flags & 1))
    {
        D3DLIGHT8 light;
        memset(&light, 0, sizeof(light));
        light.Type = D3DLIGHT_DIRECTIONAL;
        light.Direction.z = 1.0f;
        light.Diffuse.r = 1.0f;
        light.Diffuse.g = 1.0f;
        light.Diffuse.b = 1.0f;
        HRESULT result = D3DDevice_SetLight(index, &light);
        if (result != S_OK) return result;
    }
    Light *light = &device->m_pLights[index];
    light->Flags |= 4;
    /* A real link pointer replaces the historical fabricated previous Light
     * object before m_pActiveLights. No before-object typed member access. */
    Light **link = &device->m_pActiveLights;
    while (*link != NULL)
    {
        if (*link == light)
        {
            *link = light->pNext;
            break;
        }
        link = &(*link)->pNext;
    }
    if (enable)
    {
        light->pNext = device->m_pActiveLights;
        device->m_pActiveLights = light;
    }
    device->m_DirtyFlags |= 0x20;
    return S_OK;
}
void WINAPI D3DDevice_SetVertexBlendModelView(
    UINT count,
    const D3DMATRIX *modelViews,
    const D3DMATRIX *inverseModelViews,
    const D3DMATRIX *projectionViewport)
{
    CDevice *device = g_pDevice;
    if (modelViews == NULL)
    {
        device->m_DirtyFlags &= ~0x80000000UL;
        device->m_DirtyFlags |= 2;
        return;
    }
    device->m_DirtyFlags &= ~2UL;
    device->m_DirtyFlags |= 0x80000000UL;
    DWORD *push = device->StartPush(count * 34 + 17);
    for (UINT i = 0; i < count; i++)
    {
        DumpMatrixTransposed(push, 0x400480 + i * 0x40, &modelViews[i]);
        push += 17;
    }
    if (inverseModelViews != NULL)
    {
        for (UINT i = 0; i < count; i++)
        {
            push[0] = 0x300580 + i * 0x40;
            memcpy(push + 1, &inverseModelViews[i], 12 * sizeof(DWORD));
            push += 13;
        }
    }
    DumpMatrixTransposed(push, 0x400680, projectionViewport);
    device->EndPush(push + 17);
    if (!(device->m_StateFlags & 0x10))
    {
        memcpy(device->m_ModelViewTransform, modelViews, count * sizeof(D3DMATRIX));
        device->m_ProjectionViewport = *projectionViewport;
    }
    return;
}
HRESULT WINAPI D3DDevice_PersistDisplay(
    void)
{
    CDevice *device = g_pDevice;
    D3DSurface *copy = (D3DSurface *)AvGetSavedDataAddress();
    if (copy != NULL)
    {
        MmFreeContiguousMemory(copy);
        AvSetSavedDataAddress(NULL);
    }
    if (device->m_cRefs == 0) return E_FAIL;
    while (device->m_Miniport.IsFlipPending())
    {
    }
    D3DSurface *surface = &device->m_FrameBufferSurfaces[1];
    D3DFORMAT format = PixelJar::GetFormat(surface);
    DWORD width = PixelJar::GetWidth(surface);
    DWORD height = PixelJar::GetHeight(surface);
    HRESULT result = CreateSurfaceWithContiguousHeader(width, height, format, &copy);
    if (FAILED(result)) return result;
    D3DDevice_CopyRects(surface, NULL, 0, copy, NULL);
    KickOffAndWaitForIdle();
    void *registerBase = device->m_Miniport.m_RegisterBase;
    DWORD displayMode = device->m_Miniport.m_DisplayMode;
    DWORD step = 0;
    do
    {
        step = AvSetDisplayMode(registerBase, step, displayMode,
            MapToLinearD3DFORMAT(format), PixelJar::GetPitch(copy), copy->Data);
    } while (step);
    DWORD allocationSize = MmQueryAllocationSize(copy);
    MmPersistContiguousMemory(copy, allocationSize, TRUE);
    AvSetSavedDataAddress(copy);
    return S_OK;
}
