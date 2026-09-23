/* Pixel shader state adapted from RXDK-Libs, 2026 Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * See libs/d3d8/PROVENANCE.md. */
#include "device_layout.h"
#include "math_internal.h"
#include "push_internal.h"
#include "state_internal.h"
#pragma code_seg("D3D")
using namespace D3D;
void WINAPI D3DDevice_DeletePixelShader(
    DWORD handle)
{
    PixelShader *shader = (PixelShader *)handle;
    if (--shader->RefCount == 0 && shader->D3DOwned)
    {
        LocalFree(shader);
    }
    return;
}
void WINAPI D3DDevice_SetPixelShaderProgram(
    D3DPIXELSHADERDEF *definition)
{
    CDevice *device = g_pDevice;
    if (definition)
    {
        device->m_UserPixelShader.RefCount = 1;
        device->m_UserPixelShader.D3DOwned = 0;
        device->m_UserPixelShader.pPSDef = definition;
        D3DDevice_SetPixelShader((DWORD)&device->m_UserPixelShader);
    }
    else
    {
        D3DDevice_SetPixelShader(0);
    }
    return;
}
void WINAPI D3DDevice_GetPixelShader(
    DWORD *handle)
{
    *handle = (DWORD)g_pDevice->m_pPixelShader;
    return;
}
void WINAPI D3DDevice_GetPixelShaderFunction(
    DWORD handle,
    D3DPIXELSHADERDEF *definition)
{
    *definition = *((PixelShader *)handle)->pPSDef;
    return;
}
void WINAPI D3DDevice_GetPixelShaderConstant(
    DWORD index,
    void *data,
    DWORD count)
{
    float *colors = (float *)data;
    for (DWORD i = 0; i < count; ++i, ++index, colors += 4)
    {
        DWORD color = g_pDevice->m_pPixelShaderConstants[index];
        colors[3] = (float)((color >> 24) & 255) * (1.0f / 255.0f);
        colors[0] = (float)((color >> 16) & 255) * (1.0f / 255.0f);
        colors[1] = (float)((color >> 8) & 255) * (1.0f / 255.0f);
        colors[2] = (float)(color & 255) * (1.0f / 255.0f);
    }
    return;
}

void WINAPI D3DDevice_SetPixelShader(
    DWORD handle)
{
    PixelShader *shader = (PixelShader *)handle;
    CDevice *device = g_pDevice;
    PixelShader *oldShader = device->m_pPixelShader;
    device->m_pPixelShader = shader;
    if (!shader)
    {
        device->m_DirtyFlags |= 0x408;
        if (device->m_ShaderUsesSpecFog) device->m_DirtyFlags |= 0x40;
        D3DDevice_SetRenderState_TextureFactor(D3D__RenderState[D3DRS_TEXTUREFACTOR]);
        DWORD *push = device->StartPush();
        push = CommonSetTextureBumpEnv(device, push);
        Push1(push, 0x1e78, 0x00210000);
        device->EndPush(push + 2);
        return;
    }
    device->m_ShaderUsesSpecFog = shader->pPSDef->PSFinalCombinerInputsABCD | shader->pPSDef->PSFinalCombinerInputsEFG;
    device->m_ShaderAdjustsTexMode = shader->pPSDef->PSFinalCombinerConstants & 0x100;
    D3DPIXELSHADERDEF *definition = shader->pPSDef;
    device->m_PSShaderStageProgram = definition->PSTextureModes;
    device->m_DirtyFlags |= 0x400;
    DWORD *push = device->StartPush();
    if (!oldShader) push = CommonSetTextureBumpEnv(device, push);
    PushCount(push, 0x260, 8);
    memcpy(push + 1, definition->PSAlphaInputs, 8 * sizeof(DWORD));
    PushCount(push + 9, 0xa60, 32);
    memcpy(push + 10, (const BYTE *)definition + offsetof(D3DPIXELSHADERDEF, PSConstant0), 32 * sizeof(DWORD));
    Push1(push + 42, 0x17f8, definition->PSCompareMode);
    PushCount(push + 44, 0x1e20, 2);
    memcpy(push + 45, (const BYTE *)definition + offsetof(D3DPIXELSHADERDEF, PSFinalCombinerConstant0), 2 * sizeof(DWORD));
    PushCount(push + 47, 0x1e40, 9);
    memcpy(push + 48, (const BYTE *)definition + offsetof(D3DPIXELSHADERDEF, PSRGBOutputs), 9 * sizeof(DWORD));
    PushCount(push + 57, 0x1e74, 2);
    memcpy(push + 58, (const BYTE *)definition + offsetof(D3DPIXELSHADERDEF, PSDotMapping), 2 * sizeof(DWORD));
    if (device->m_ShaderUsesSpecFog)
    {
        PushCount(push + 60, 0x288, 2);
        memcpy(push + 61, (const BYTE *)definition + offsetof(D3DPIXELSHADERDEF, PSFinalCombinerInputsABCD), 2 * sizeof(DWORD));
        device->EndPush(push + 63);
    }
    else device->EndPush(push + 60);
    if (!(device->m_StateFlags & 0x10))
    {
        memcpy(D3D__RenderState, definition, sizeof(*definition) - 3 * sizeof(DWORD));
        D3D__RenderState[D3DRS_PSTEXTUREMODES] = definition->PSTextureModes;
    }
    return;
}
namespace D3D
{
__inline UCHAR WINAPI Float2UChar(
    float value)
{
    if (value > 1.0f) value = 1.0f;
    else if (value < 0.0f) value = 0.0f;
    return (UCHAR)FloatToLong(value * 255.0f + 0.5f);
}
}
void WINAPI D3DDevice_SetPixelShaderConstant(
    DWORD index,
    const void *data,
    DWORD count)
{
    CDevice *device = g_pDevice;
    D3DPIXELSHADERDEF *definition = device->m_pPixelShader->pPSDef;
    const float *colors = (const float *)data;
    for (UINT i = 0; i < count; i++, index++, colors += 4)
    {
        DWORD color = ((DWORD)Float2UChar(colors[3]) << 24) |
                      ((DWORD)Float2UChar(colors[0]) << 16) |
                      ((DWORD)Float2UChar(colors[1]) << 8) |
                      (DWORD)Float2UChar(colors[2]);
        device->m_pPixelShaderConstants[index] = color;
        UINT r;
        for (r = 0; r < 8; r++)
        {
            if (((definition->PSC0Mapping >> (4 * r)) & 15) == index)
                D3DDevice_SetRenderStateNotInline((D3DRENDERSTATETYPE)(D3DRS_PSCONSTANT0_0 + r), color);
        }
        for (r = 0; r < 8; r++)
        {
            if (((definition->PSC1Mapping >> (4 * r)) & 15) == index)
                D3DDevice_SetRenderStateNotInline((D3DRENDERSTATETYPE)(D3DRS_PSCONSTANT1_0 + r), color);
        }
        for (r = 0; r < 2; r++)
        {
            if (((definition->PSFinalCombinerConstants >> (4 * r)) & 15) == index)
                D3DDevice_SetRenderStateNotInline((D3DRENDERSTATETYPE)(D3DRS_PSFINALCOMBINERCONSTANT0 + r), color);
        }
        if (index == 0)
        {
            DWORD *push = device->StartPush();
            PushCount(push, 0x181c, 3);
            memcpy(push + 1, colors, 3 * sizeof(DWORD));
            device->EndPush(push + 4);
        }
    }
    return;
}

HRESULT WINAPI D3DDevice_CreatePixelShader(
    const D3DPIXELSHADERDEF *definition,
    DWORD *handle)
{
    PixelShader *shader = (PixelShader *)LocalAlloc(
        LMEM_FIXED, sizeof(PixelShader) + sizeof(D3DPIXELSHADERDEF));
    if (!shader)
    {
        return E_OUTOFMEMORY;
    }
    shader->pPSDef = (D3DPIXELSHADERDEF *)(shader + 1);
    shader->RefCount = 1;
    shader->D3DOwned = 1;
    *shader->pPSDef = *definition;
    *handle = (DWORD)shader;
    return S_OK;
}
