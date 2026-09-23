/* State programming adapted from RXDK-Libs, 2026 Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Older target method numbers/state layout are proved by its disassembly.
 */
#include "push_internal.h"
#include "pixeljar.h"
#include "state_internal.h"
#include "math_internal.h"
#pragma code_seg("D3D")

namespace D3D
{
DWORD *__fastcall CommonSetControl0(
    CDevice *device,
    DWORD *push)
{
    DWORD control = 0x00100001;
    if (D3D__RenderState[D3DRS_YUVENABLE]) control |= 0x10000000;
    if (D3D__RenderState[D3DRS_ZENABLE] == D3DZB_USEW) control |= 0x00010000;
    if (device->m_pZBuffer)
    {
        D3DFORMAT format = PixelJar::GetFormat(device->m_pZBuffer);
        if (format == D3DFMT_F16 || format == D3DFMT_F24S8 ||
            format == D3DFMT_LIN_F16 || format == D3DFMT_LIN_F24S8)
        {
            control |= 0x1000;
        }
    }
    Push1(push, 0x290, control);
    return push + 2;
}
DWORD *__fastcall CommonSetOccludeZStencilEn(
    CDevice *device,
    DWORD *push)
{
    DWORD control = 0;
    if (D3D__RenderState[D3DRS_STENCILCULLENABLE])
    {
        control |= 2;
    }
    if (D3D__RenderState[D3DRS_OCCLUSIONCULLENABLE] &&
        (!D3D__RenderState[D3DRS_STENCILENABLE] ||
         D3D__RenderState[D3DRS_STENCILFAIL] == D3DSTENCILOP_KEEP))
    {
        control |= 1;
    }
    Push1(push, 0x1d84, control);
    return push + 2;
}
void __fastcall CommonSetAntiAliasingControl(
    CDevice *device)
{
    CommonSetPassthroughProgram(device);
    DWORD *push = device->StartPush();
    push = CommonSetViewport(device, push);
    Push1(push, 0x1d7c,
          (D3D__RenderState[D3DRS_MULTISAMPLEMASK] << 16) |
          (D3D__RenderState[D3DRS_MULTISAMPLEANTIALIAS] & 0xf));
    device->EndPush(push + 2);
    return;
}
void __fastcall CommonSetDebugRegisters(
    void)
{
    CDevice *device = g_pDevice;
    device->m_Miniport.m_DebugRegister[5] &= ~8UL;
    if (D3D__RenderState[D3DRS_DONOTCULLUNCOMPRESSED])
    {
        device->m_Miniport.m_DebugRegister[5] |= 8;
    }
    device->m_Miniport.m_DebugRegister[6] &= ~0x18100000UL;
    if (D3D__RenderState[D3DRS_ROPZCMPALWAYSREAD])
    {
        device->m_Miniport.m_DebugRegister[6] |= 0x100000;
    }
    if (D3D__RenderState[D3DRS_ROPZREAD])
    {
        device->m_Miniport.m_DebugRegister[6] |= 0x08000000;
    }
    DWORD *push = device->StartPush();
    Push1(push, NV097_WAIT_FOR_IDLE, 0);
    Push2(push + 2, 0x1d8c, 0x400094, device->m_Miniport.m_DebugRegister[5]);
    Push1(push + 5, NV097_NO_OPERATION, 0x320);
    Push2(push + 7, 0x1d8c, 0x400b80, device->m_Miniport.m_DebugRegister[6]);
    Push1(push + 10, NV097_NO_OPERATION, 0x320);
    device->EndPush(push + 12);
    return;
}
}
using namespace D3D;
void __fastcall D3D::CommonSetSurfaceFormat(
    CDevice *device)
{
    float xScale = 1.0f;
    float yScale = 1.0f;
    DWORD antiAlias = 0;
    D3DSurface *surface = device->m_pRenderTarget;
    if (surface == &device->m_FrameBufferSurfaces[0])
    {
        DWORD sampling = D3D__RenderState[D3DRS_MULTISAMPLETYPE];
        if (sampling & 0x3000)
        {
            xScale = (float)((sampling >> 4) & 15);
            yScale = (float)(sampling & 15);
            if (sampling & 0x1000)
            {
                xScale *= 0.5f;
                antiAlias = 0x1000;
                if (yScale > 1.0f)
                {
                    yScale *= 0.5f;
                    antiAlias = 0x2000;
                }
            }
        }
    }
    float scale = min(xScale, yScale);
    device->m_SuperSampleScaleX = xScale;
    device->m_SuperSampleScaleY = yScale;
    if (scale != device->m_SuperSampleScale)
    {
        device->m_SuperSampleScale = scale;
        device->m_SuperSampleLODBias = g_LODBias2x[Round(scale * 2.0f) - 2];
        device->m_DirtyFlags |= 1;
        D3DDevice_SetRenderStateNotInline(D3DRS_LINEWIDTH,
            D3D__RenderState[D3DRS_LINEWIDTH]);
    }
    DWORD *push = device->StartPush();
    Push1(push, 0x208, PixelJar::GetSurfaceFormat(surface, device->m_pZBuffer) | antiAlias);
    device->EndPush(push + 2);
    return;
}

void WINAPI D3DDevice_SetRenderState_MultiSampleType(
    DWORD value)
{
    CDevice *device = g_pDevice;
    D3D__RenderState[D3DRS_MULTISAMPLETYPE] = value;
    D3DDevice_SetRenderTarget(device->m_pRenderTarget, device->m_pZBuffer);
    return;
}
void WINAPI D3DDevice_SetRenderState_PSTextureModes(
    DWORD value)
{
    CDevice *device = g_pDevice;
    device->m_PSShaderStageProgram = value;
    device->m_DirtyFlags |= 0x400;
    D3D__RenderState[D3DRS_PSTEXTUREMODES] = value;
    return;
}
void __fastcall D3DDevice_SetRenderState_Deferred(
    D3DRENDERSTATETYPE state,
    DWORD value)
{
    g_Device.m_DirtyFlags |= g_DirtyFromRenderState[state - D3DRS_SIMPLE_MAX];
    D3D__RenderState[state] = value;
    return;
}
void __fastcall D3DDevice_SetTextureState_Deferred(
    DWORD stage,
    D3DTEXTURESTAGESTATETYPE state,
    DWORD value)
{
    g_Device.m_DirtyFlags |= g_DirtyFromTextureState[state];
    D3D__TextureState[stage][state] = value;
    return;
}
void WINAPI D3DDevice_SetRenderStateNotInline(
    D3DRENDERSTATETYPE state,
    DWORD value)
{
    if (state < D3DRS_SIMPLE_MAX)
    {
        D3DDevice_SetRenderState_Simple(D3DSIMPLERENDERSTATEENCODE[state], value);
        D3D__RenderState[state] = value;
    }
    else if (state < D3DRS_DEFERRED_MAX)
    {
        D3DDevice_SetRenderState_Deferred(state, value);
    }
    else
    {
        g_ComplexRenderStateFunctionTable[state - D3DRS_DEFERRED_MAX](value);
    }
    return;
}
void WINAPI D3DDevice_SetRenderState_EdgeAntiAlias(
    DWORD value)
{
    CDevice *device = g_pDevice;
    DWORD *push = device->StartPush();
    Push2(push, NV097_SET_LINE_SMOOTH_ENABLE, value, value);
    device->EndPush(push + 3);
    D3D__RenderState[D3DRS_EDGEANTIALIAS] = value;
    return;
}
void WINAPI D3DDevice_SetRenderState_ShadowFunc(
    DWORD value)
{
    CDevice *device = g_pDevice;
    DWORD *push = device->StartPush();
    Push1(push, NV097_SET_SHADOW_DEPTH_FUNC, value - D3DCMP_NEVER);
    device->EndPush(push + 2);
    D3D__RenderState[D3DRS_SHADOWFUNC] = value;
    return;
}
void WINAPI D3DDevice_SetRenderState_FogColor(
    DWORD value)
{
    CDevice *device = g_pDevice;
    DWORD *push = device->StartPush();
    Push1(push, NV097_SET_FOG_COLOR, SwapRgb(value));
    device->EndPush(push + 2);
    D3D__RenderState[D3DRS_FOGCOLOR] = value;
    return;
}
void WINAPI D3DDevice_SetRenderState_CullMode(
    DWORD value)
{
    CDevice *device = g_pDevice;
    DWORD *push = device->StartPush();
    if (value == D3DCULL_NONE)
    {
        Push1(push, NV097_SET_CULL_FACE_ENABLE, FALSE);
        device->EndPush(push + 2);
    }
    else
    {
        Push1(push, NV097_SET_CULL_FACE_ENABLE, TRUE);
        DWORD cullFace = value == D3D__RenderState[D3DRS_FRONTFACE] ?
                         NV097_SET_CULL_FACE_V_FRONT : NV097_SET_CULL_FACE_V_BACK;
        Push1(push + 2, NV097_SET_CULL_FACE, cullFace);
        device->EndPush(push + 4);
    }
    D3D__RenderState[D3DRS_CULLMODE] = value;
    return;
}
void WINAPI D3DDevice_SetRenderState_FrontFace(
    DWORD value)
{
    CDevice *device = g_pDevice;
    DWORD *push = device->StartPush();
    Push1(push, NV097_SET_FRONT_FACE, value);
    device->EndPush(push + 2);
    D3D__RenderState[D3DRS_FRONTFACE] = value;
    D3DDevice_SetRenderState_CullMode(D3D__RenderState[D3DRS_CULLMODE]);
    return;
}
void WINAPI D3DDevice_SetRenderState_NormalizeNormals(
    DWORD value)
{
    CDevice *device = g_pDevice;
    DWORD *push = device->StartPush();
    Push1(push, NV097_SET_NORMALIZATION_ENABLE, value);
    device->EndPush(push + 2);
    device->m_DirtyFlags |= 2;
    D3D__RenderState[D3DRS_NORMALIZENORMALS] = value;
    return;
}
void WINAPI D3DDevice_SetRenderState_TextureFactor(
    DWORD value)
{
    CDevice *device = g_pDevice;
    if (!device->m_pPixelShader)
    {
        DWORD *push = device->StartPush();
        PushCount(push++, NV097_SET_COMBINER_FACTOR0, 16);
        for (DWORD i = 0; i < 16; i++)
        {
            *push++ = value;
        }
        device->EndPush(push);
    }
    D3D__RenderState[D3DRS_TEXTUREFACTOR] = value;
    return;
}
void WINAPI D3DDevice_SetRenderState_Dxt1NoiseEnable(
    DWORD value)
{
    CDevice *device = g_pDevice;
    BOOL noiseEnable = FALSE;
    if (PixelJar::GetBitsPerPixel(device->m_pRenderTarget) == 32)
    {
        noiseEnable = value;
    }
    if (noiseEnable != (device->m_StateFlags & 1))
    {
        device->m_StateFlags ^= 1;
        DWORD *push = device->StartPush();
        Push1(push, NV097_WAIT_FOR_IDLE, 0);
        /* January uses separate semaphore-offset argument and software method. */
        Push1(push + 2, 0x1d8c, noiseEnable);
        Push1(push + 4, NV097_NO_OPERATION, 0x31c);
        device->EndPush(push + 6);
    }
    D3D__RenderState[D3DRS_DXT1NOISEENABLE] = value;
    return;
}
void WINAPI D3DDevice_SetRenderState_YuvEnable(
    DWORD value)
{
    CDevice *device = g_pDevice;
    D3D__RenderState[D3DRS_YUVENABLE] = value;
    DWORD *push = device->StartPush();
    push = CommonSetControl0(device, push);
    device->EndPush(push);
    return;
}
void WINAPI D3DDevice_SetRenderState_OcclusionCullEnable(
    DWORD value)
{
    CDevice *device = g_pDevice;
    D3D__RenderState[D3DRS_OCCLUSIONCULLENABLE] = value;
    DWORD *push = device->StartPush();
    push = CommonSetOccludeZStencilEn(device, push);
    device->EndPush(push);
    return;
}
void WINAPI D3DDevice_SetRenderState_StencilCullEnable(
    DWORD value)
{
    CDevice *device = g_pDevice;
    D3D__RenderState[D3DRS_STENCILCULLENABLE] = value;
    DWORD *push = device->StartPush();
    push = CommonSetOccludeZStencilEn(device, push);
    device->EndPush(push);
    return;
}
void WINAPI D3DDevice_SetRenderState_RopZCmpAlwaysRead(
    DWORD value)
{
    D3D__RenderState[D3DRS_ROPZCMPALWAYSREAD] = value;
    CommonSetDebugRegisters();
    return;
}
void WINAPI D3DDevice_SetRenderState_RopZRead(
    DWORD value)
{
    D3D__RenderState[D3DRS_ROPZREAD] = value;
    CommonSetDebugRegisters();
    return;
}
void WINAPI D3DDevice_SetRenderState_DoNotCullUncompressed(
    DWORD value)
{
    D3D__RenderState[D3DRS_DONOTCULLUNCOMPRESSED] = value;
    CommonSetDebugRegisters();
    return;
}

void WINAPI D3DDevice_SetRenderState_LogicOp(
    DWORD value)
{
    CDevice *device = g_pDevice;
    DWORD *push = device->StartPush();
    if (value == D3DLOGICOP_NONE)
    {
        Push1(push, 0x17bc, FALSE);
        push += 2;
    }
    else
    {
        Push2(push, 0x17bc, TRUE, value);
        push += 3;
    }
    device->EndPush(push);
    D3D__RenderState[D3DRS_LOGICOP] = value;
    return;
}
void WINAPI D3DDevice_SetRenderState_FillMode(
    DWORD value)
{
    CDevice *device = g_pDevice;
    DWORD *push = device->StartPush();
    DWORD back = D3D__RenderState[D3DRS_TWOSIDEDLIGHTING] ?
                 D3D__RenderState[D3DRS_BACKFILLMODE] : value;
    Push2(push, 0x38c, value, back);
    device->EndPush(push + 3);
    D3D__RenderState[D3DRS_FILLMODE] = value;
    return;
}
void WINAPI D3DDevice_SetRenderState_BackFillMode(
    DWORD value)
{
    D3D__RenderState[D3DRS_BACKFILLMODE] = value;
    D3DDevice_SetRenderState_FillMode(D3D__RenderState[D3DRS_FILLMODE]);
    return;
}
void WINAPI D3DDevice_SetRenderState_TwoSidedLighting(
    DWORD value)
{
    CDevice *device = g_pDevice;
    device->m_DirtyFlags |= 0x20;
    D3D__RenderState[D3DRS_TWOSIDEDLIGHTING] = value;
    D3DDevice_SetRenderState_FillMode(D3D__RenderState[D3DRS_FILLMODE]);
    return;
}
void WINAPI D3DDevice_SetRenderState_VertexBlend(
    DWORD value)
{
    CDevice *device = g_pDevice;
    device->m_DirtyFlags |= 2;
    DWORD *push = device->StartPush();
    Push1(push, 0x328, value);
    device->EndPush(push + 2);
    D3D__RenderState[D3DRS_VERTEXBLEND] = value;
    return;
}
void WINAPI D3DDevice_SetRenderState_ZEnable(
    DWORD value)
{
    CDevice *device = g_pDevice;
    DWORD *push = device->StartPush();
    Push1(push, 0x30c, value != D3DZB_FALSE && device->m_pZBuffer != NULL);
    device->EndPush(push + 2);
    DWORD previous = D3D__RenderState[D3DRS_ZENABLE];
    D3D__RenderState[D3DRS_ZENABLE] = value;
    if (previous == D3DZB_USEW || value == D3DZB_USEW)
    {
        UpdateProjectionViewportTransform();
        CommonSetPassthroughProgram(device);
        DWORD *push = device->StartPush();
        push = CommonSetControl0(device, push);
        device->EndPush(push);
    }
    return;
}
void WINAPI D3DDevice_SetRenderState_StencilEnable(
    DWORD value)
{
    CDevice *device = g_pDevice;
    DWORD *push = device->StartPush();
    push = CommonSetOccludeZStencilEn(device, push);
    Push1(push, 0x32c, value && device->m_pZBuffer != NULL);
    device->EndPush(push + 2);
    D3D__RenderState[D3DRS_STENCILENABLE] = value;
    return;
}
void WINAPI D3DDevice_SetRenderState_StencilFail(
    DWORD value)
{
    CDevice *device = g_pDevice;
    DWORD *push = device->StartPush();
    push = CommonSetOccludeZStencilEn(device, push);
    Push1(push, 0x370, value);
    device->EndPush(push + 2);
    D3D__RenderState[D3DRS_STENCILFAIL] = value;
    return;
}
void WINAPI D3DDevice_SetRenderState_MultiSampleAntiAlias(
    DWORD value)
{
    CDevice *device = g_pDevice;
    D3D__RenderState[D3DRS_MULTISAMPLEANTIALIAS] = value;
    CommonSetAntiAliasingControl(device);
    return;
}
void WINAPI D3DDevice_SetRenderState_MultiSampleMask(
    DWORD value)
{
    CDevice *device = g_pDevice;
    D3D__RenderState[D3DRS_MULTISAMPLEMASK] = value;
    CommonSetAntiAliasingControl(device);
    return;
}

void WINAPI D3DDevice_SetRenderState_LineWidth(
    DWORD Value)
{
    CDevice *device = g_pDevice;
    float width;
    memcpy(&width, &Value, sizeof(width));
    DWORD scaled = FloatToLong(width * device->m_SuperSampleScale * 8.0f + 0.5f);
    if (scaled > 511) { scaled = 511; }
    DWORD *push = device->StartPush();
    Push1(push, 0x380, scaled);
    device->EndPush(push + 2);
    D3D__RenderState[D3DRS_LINEWIDTH] = Value;
    return;
}
void WINAPI D3DDevice_SetRenderState_ZBias(
    DWORD Value)
{
    BOOL enabled = Value != 0;
    float bias = -(float)Value;
    float scale = bias * 0.25f;
    DWORD payload;
    memcpy(&payload, &scale, sizeof(payload));
    D3DDevice_SetRenderStateNotInline(D3DRS_POLYGONOFFSETZSLOPESCALE, payload);
    memcpy(&payload, &bias, sizeof(payload));
    D3DDevice_SetRenderStateNotInline(D3DRS_POLYGONOFFSETZOFFSET, payload);
    D3DDevice_SetRenderStateNotInline(D3DRS_POINTOFFSETENABLE, enabled);
    D3DDevice_SetRenderStateNotInline(D3DRS_WIREFRAMEOFFSETENABLE, enabled);
    D3DDevice_SetRenderStateNotInline(D3DRS_SOLIDOFFSETENABLE, enabled);
    D3D__RenderState[D3DRS_ZBIAS] = Value;
    return;
}
void WINAPI D3DDevice_SetTextureState_TexCoordIndex(
    DWORD Stage,
    DWORD Value)
{
    CDevice *device = g_pDevice;
    D3D__TextureState[Stage][D3DTSS_TEXCOORDINDEX] = Value;
    DWORD needsInverse = FALSE;
    DWORD mode;
    switch (Value & 0xffff0000)
    {
    case D3DTSS_TCI_PASSTHRU:
        device->m_DirtyFlags |= 0x180;
        mode = 0;
        break;
    case D3DTSS_TCI_CAMERASPACENORMAL:
        needsInverse = TRUE;
        mode = 0x8511;
        break;
    case D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR:
        needsInverse = TRUE;
        mode = 0x8512;
        break;
    case D3DTSS_TCI_CAMERASPACEPOSITION:
        mode = 0x2400;
        break;
    case D3DTSS_TCI_OBJECT:
        mode = 0x2401;
        break;
    default:
        /* Target default, including sphere mapping. */
        needsInverse = TRUE;
        mode = 0x2402;
        break;
    }
    if (!device->m_TexGenInverseNeeded && needsInverse) { device->m_DirtyFlags |= 2; }
    device->m_TexGenInverseNeeded &= ~(1 << Stage);
    device->m_TexGenInverseNeeded |= needsInverse << Stage;
    DWORD *push = device->StartPush();
    PushCount(push, 0x3c0 + Stage * 16, 3);
    push[1] = mode;
    push[2] = mode;
    push[3] = mode;
    device->EndPush(push + 4);
    device->m_DirtyFlags |= 0x194;
    return;
}
void WINAPI D3DDevice_SetTextureState_BumpEnv(
    DWORD Stage,
    D3DTEXTURESTAGESTATETYPE Type,
    DWORD Value)
{
    CDevice *device = g_pDevice;
    DWORD unit = device->m_pPixelShader ? Stage : Stage + 1;
    if (unit & 3)
    {
        DWORD *push = device->StartPush();
        Push1(push, 0x1b28 + unit * 64 + 4 * (Type - D3DTSS_BUMPENVMAT00), Value);
        device->EndPush(push + 2);
    }
    D3D__TextureState[Stage][Type] = Value;
    return;
}
void WINAPI D3DDevice_SetTextureState_BorderColor(
    DWORD Stage,
    DWORD Value)
{
    CDevice *device = g_pDevice;
    DWORD *push = device->StartPush();
    Push1(push, 0x1b24 + Stage * 64, Value);
    device->EndPush(push + 2);
    D3D__TextureState[Stage][D3DTSS_BORDERCOLOR] = Value;
    return;
}
void WINAPI D3DDevice_SetTextureState_ColorKeyColor(
    DWORD Stage,
    DWORD Value)
{
    CDevice *device = g_pDevice;
    DWORD *push = device->StartPush();
    Push1(push, 0xae0 + Stage * 4, Value);
    device->EndPush(push + 2);
    D3D__TextureState[Stage][D3DTSS_COLORKEYCOLOR] = Value;
    return;
}
void WINAPI D3DDevice_SetTextureStageStateNotInline(
    DWORD Stage,
    D3DTEXTURESTAGESTATETYPE Type,
    DWORD Value)
{
    CDevice *device = g_pDevice;
    if (Type < D3DTSS_DEFERRED_MAX)
    {
        DWORD dirty = g_DirtyFromTextureState[Type];
        D3D__TextureState[Stage][Type] = Value;
        device->m_DirtyFlags |= dirty;
    }
    else if (Type == D3DTSS_TEXCOORDINDEX) { D3DDevice_SetTextureState_TexCoordIndex(Stage, Value); }
    else if (Type == D3DTSS_BORDERCOLOR) { D3DDevice_SetTextureState_BorderColor(Stage, Value); }
    else if (Type == D3DTSS_COLORKEYCOLOR) { D3DDevice_SetTextureState_ColorKeyColor(Stage, Value); }
    else if (Type >= D3DTSS_BUMPENVMAT00 && Type <= D3DTSS_BUMPENVLOFFSET)
    { D3DDevice_SetTextureState_BumpEnv(Stage, Type, Value); }
    return;
}
namespace D3D
{
DWORD *__fastcall CommonSetTextureBumpEnv(
    CDevice *device,
    DWORD *push)
{
    DWORD stage = device->m_pPixelShader ? 1 : 0;
    for (DWORD unit = 1; unit <= 3; unit++)
    {
        PushCount(push, 0x1b28 + unit * 64, 6);
        memcpy(push + 1, &D3D__TextureState[stage][D3DTSS_BUMPENVMAT00], 6 * sizeof(DWORD));
        push += 7;
        stage++;
    }
    return push;
}
}
