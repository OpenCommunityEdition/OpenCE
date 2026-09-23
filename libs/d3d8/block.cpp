/* January state-block reconstruction; target PDB and COFF are authoritative.
 * Related RXDK block implementation: Copyright (C) 2026 Team Resurgent.
 * SPDX-License-Identifier: GPL-3.0-or-later. See retained reference provenance. */
#include "block_internal.h"
#include "vshader_internal.h"
#include "memory_internal.h"
#pragma code_seg("D3D")
#pragma const_seg("D3D_RD")
extern "C"
{
BYTE D3D__StateBlockDirty[(D3DSBD_MAX + 3) & ~3];
}
namespace D3D
{
void WINAPI ClearStateBlockFlags(
    void)
{
    CDevice *device = g_pDevice;
    ZeroMemory(D3D__StateBlockDirty, sizeof(D3D__StateBlockDirty));
    Light *light = device->m_pLights;
    for (DWORD i = 0; i < device->m_LightCount; ++i, ++light)
    {
        light->Flags &= ~(LIGHT_STATEBLOCK_SET | LIGHT_STATEBLOCK_ENABLED);
    }
    return;
}
extern "C" void WINAPI D3DDevice_BeginStateBlock(
    void)
{
    g_pDevice->m_StateFlags |= STATE_RECORDBLOCK;
    ClearStateBlockFlags();
    return;
}
extern "C" void WINAPI D3DDevice_Suspend(
    void)
{
    g_pDevice->SetStateVB(0);
    return;
}
extern "C" void WINAPI D3DDevice_ApplyStateBlock(
    DWORD token)
{
    DWORD i;
    CDevice *device = g_pDevice;
    StateBlock *block = (StateBlock *)token;
    DWORD *data = StateBlockData(block);
    for (i = 0; i < block->D0_Textures; ++i)
    {
        device->SetTexture(data[0], (D3DBaseTexture *)data[1]);
        data += 2;
    }
    if (block->D1_PixelShader)
    {
        device->SetPixelShader(data[0]);
        ++data;
    }
    if (block->D2_VertexShader)
    {
        device->SetVertexShader(data[0]);
        ++data;
    }
    if (block->D3_Index)
    {
        device->SetIndices((D3DIndexBuffer *)data[1], data[0]);
        data += 2;
    }
    for (i = 0; i < block->D4_Streams; ++i)
    {
        device->SetStreamSource(data[0], (D3DVertexBuffer *)data[2], data[1]);
        data += 3;
    }
    for (i = 0; i < block->D6_PixelShaderConstants; ++i)
    {
        device->SetPixelShaderConstant(data[0], data + 1, 1);
        data += 5;
    }
    for (i = 0; i < block->D7_VertexShaderConstants; ++i)
    {
        device->SetVertexShaderConstant(data[0] - 96, data + 1, 1);
        data += 5;
    }
    for (i = 0; i < block->D8_RenderStates; ++i)
    {
        DWORD state = data[0];
        DWORD value = data[1];
        if (D3D__RenderState[state] != value)
        {
            device->SetRenderStateNotInline((D3DRENDERSTATETYPE)state, value);
        }
        data += 2;
    }
    for (i = 0; i < block->D9_TextureStates; ++i)
    {
        DWORD stage = data[0];
        DWORD state = data[1];
        DWORD value = data[2];
        if (D3D__TextureState[stage][state] != value)
        {
            device->SetTextureStageStateNotInline(stage, (D3DTEXTURESTAGESTATETYPE)state, value);
        }
        data += 3;
    }
    for (i = 0; i < block->D10_Transforms; ++i)
    {
        device->SetTransform((D3DTRANSFORMSTATETYPE)data[0], (const D3DMATRIX *)(data + 1));
        data += 1 + sizeof(D3DMATRIX) / sizeof(DWORD);
    }
    if (block->D11_Viewport)
    {
        device->SetViewport((const D3DVIEWPORT8 *)data);
        data += sizeof(D3DVIEWPORT8) / sizeof(DWORD);
    }
    if (block->D12_Material)
    {
        device->SetMaterial((const D3DMATERIAL8 *)data);
        data += sizeof(D3DMATERIAL8) / sizeof(DWORD);
    }
    if (block->D13_BackMaterial)
    {
        device->SetBackMaterial((const D3DMATERIAL8 *)data);
        data += sizeof(D3DMATERIAL8) / sizeof(DWORD);
    }
    for (i = 0; i < block->D14_Lights; ++i)
    {
        device->SetLight(data[0], (const D3DLIGHT8 *)(data + 1));
        data += 1 + sizeof(D3DLIGHT8) / sizeof(DWORD);
    }
    for (i = 0; i < block->D15_LightEnables; ++i)
    {
        device->LightEnable(data[0], data[1]);
        data += 2;
    }
    return;
}
extern "C" void WINAPI D3DDevice_DeleteStateBlock(
    DWORD token)
{
    DWORD i;
    StateBlock *block = (StateBlock *)token;
    DWORD *data = StateBlockData(block);
    for (i = 0; i < block->D0_Textures; ++i)
    {
        D3DBaseTexture *texture = (D3DBaseTexture *)data[1];
        data += 2;
        if (texture != NULL)
        {
            texture->Release();
        }
    }
    if (block->D1_PixelShader)
    {
        DWORD handle = *data++;
        if (handle != 0)
        {
            D3DDevice::DeletePixelShader(handle);
        }
    }
    if (block->D2_VertexShader)
    {
        DWORD handle = *data++;
        if (handle & D3DFVF_RESERVED0)
        {
            D3DDevice::DeleteVertexShader(handle);
        }
    }
    if (block->D3_Index)
    {
        D3DIndexBuffer *indexBuffer = (D3DIndexBuffer *)data[1];
        data += 2;
        if (indexBuffer != NULL)
        {
            indexBuffer->Release();
        }
    }
    for (i = 0; i < block->D4_Streams; ++i)
    {
        D3DVertexBuffer *vertexBuffer = (D3DVertexBuffer *)data[2];
        data += 3;
        if (vertexBuffer != NULL)
        {
            vertexBuffer->Release();
        }
    }
    LocalFree(block);
    return;
}

extern "C" void WINAPI D3DDevice_CaptureStateBlock(
    DWORD token)
{
    DWORD i;
    CDevice *device = g_pDevice;
    StateBlock *block = (StateBlock *)token;
    DWORD *data = StateBlockData(block);
    for (i = 0; i < block->D0_Textures; ++i)
    {
        DWORD stage = data[0];
        D3DBaseTexture *texture = (D3DBaseTexture *)data[1];
        if (texture) texture->Release();
        texture = device->m_Textures[stage];
        if (texture) texture->AddRef();
        data[1] = (DWORD)texture;
        data += 2;
    }
    if (block->D1_PixelShader)
    {
        DWORD handle = data[0];
        if (handle) device->DeletePixelShader(handle);
        handle = (DWORD)device->m_pPixelShader;
        if (handle) ++device->m_pPixelShader->RefCount;
        data[0] = handle;
        ++data;
    }
    if (block->D2_VertexShader)
    {
        DWORD handle = data[0];
        if (handle & D3DFVF_RESERVED0) device->DeleteVertexShader(handle);
        handle = device->m_VertexShaderHandle;
        if (handle & D3DFVF_RESERVED0) ++device->m_pVertexShader->RefCount;
        data[0] = handle;
        ++data;
    }
    if (block->D3_Index)
    {
        D3DIndexBuffer *indexBuffer = (D3DIndexBuffer *)data[1];
        if (indexBuffer) indexBuffer->Release();
        indexBuffer = device->m_pIndexBuffer;
        if (indexBuffer) indexBuffer->AddRef();
        data[0] = device->m_IndexBase;
        data[1] = (DWORD)indexBuffer;
        data += 2;
    }
    for (i = 0; i < block->D4_Streams; ++i)
    {
        DWORD index = data[0];
        D3DVertexBuffer *vertexBuffer = (D3DVertexBuffer *)data[2];
        if (vertexBuffer) vertexBuffer->Release();
        vertexBuffer = g_Stream[index].pVertexBuffer;
        if (vertexBuffer) vertexBuffer->AddRef();
        data[1] = g_Stream[index].Stride;
        data[2] = (DWORD)vertexBuffer;
        data += 3;
    }
    for (i = 0; i < block->D6_PixelShaderConstants; ++i)
    {
        DWORD index = data[0];
        void *constantData = data + 1;
        memcpy(constantData, device->m_PixelShaderConstants[index], 4 * sizeof(FLOAT));
        data += 5;
    }
    for (i = 0; i < block->D7_VertexShaderConstants; ++i)
    {
        DWORD index = data[0];
        void *constantData = data + 1;
        memcpy(constantData, device->m_VertexShaderConstants[index], 4 * sizeof(FLOAT));
        data += 5;
    }
    for (i = 0; i < block->D8_RenderStates; ++i)
    {
        DWORD state = data[0];
        data[1] = D3D__RenderState[state];
        data += 2;
    }
    for (i = 0; i < block->D9_TextureStates; ++i)
    {
        DWORD stage = data[0];
        DWORD state = data[1];
        data[2] = D3D__TextureState[stage][state];
        data += 3;
    }
    for (i = 0; i < block->D10_Transforms; ++i)
    {
        DWORD state = data[0];
        memcpy((void *)(data + 1), &device->m_Transform[state], sizeof(device->m_Transform[state]));
        data += 1 + sizeof(D3DMATRIX) / sizeof(DWORD);
    }
    if (block->D11_Viewport)
    {
        memcpy(data, &device->m_Viewport, sizeof(D3DVIEWPORT8));
        data += sizeof(D3DVIEWPORT8) / sizeof(DWORD);
    }
    if (block->D12_Material)
    {
        memcpy(data, &device->m_Material, sizeof(D3DMATERIAL8));
        data += sizeof(D3DMATERIAL8) / sizeof(DWORD);
    }
    if (block->D13_BackMaterial)
    {
        memcpy(data, &device->m_BackMaterial, sizeof(D3DMATERIAL8));
        data += sizeof(D3DMATERIAL8) / sizeof(DWORD);
    }
    for (i = 0; i < block->D14_Lights; ++i)
    {
        DWORD index = data[0];
        memcpy(data + 1, &device->m_pLights[index].Light8, sizeof(D3DLIGHT8));
        data += 1 + sizeof(D3DLIGHT8) / sizeof(DWORD);
    }
    for (i = 0; i < block->D15_LightEnables; ++i)
    {
        device->GetLightEnable(data[0], (BOOL *)(data + 1));
        data += 2;
    }
    return;
}

/* Cursor is a byte count during sizing and an allocated address during writing.
 * Using ULONG_PTR avoids the historical NULL-based member/pointer arithmetic.
 * Each conversion to a writable record occurs only after allocation succeeded.
 * The dirty set and device state must not change concurrently between passes. */
HRESULT WINAPI RecordStateBlock(
    DWORD *token)
{
    DWORD i;
    DWORD j;
    BYTE *dirty;
    StateBlock *block = NULL;
    ULONG_PTR cursor = offsetof(StateBlock, Data);
    CDevice *device = g_pDevice;
    Light *lights = device->m_pLights;
    *token = 0;
    for (;;)
    {
        dirty = D3D__StateBlockDirty;
        for (i = 0; i < D3DTSS_MAXSTAGES; ++i)
        {
            if (*dirty++)
            {
                if (block)
                {
                    DWORD *data = (DWORD *)cursor;
                    D3DBaseTexture *texture = device->m_Textures[i];
                    ++block->D0_Textures;
                    data[0] = i;
                    data[1] = (DWORD)texture;
                    if (texture) texture->AddRef();
                }
                cursor += 2 * sizeof(DWORD);
            }
        }
        if (*dirty++)
        {
            if (block)
            {
                PixelShader *shader = device->m_pPixelShader;
                block->D1_PixelShader = TRUE;
                *(DWORD *)cursor = (DWORD)shader;
                if (shader) ++shader->RefCount;
            }
            cursor += sizeof(DWORD);
        }
        if (*dirty++)
        {
            if (block)
            {
                DWORD handle = device->m_VertexShaderHandle;
                block->D2_VertexShader = TRUE;
                *(DWORD *)cursor = handle;
                if (handle & D3DFVF_RESERVED0) ++device->m_pVertexShader->RefCount;
            }
            cursor += sizeof(DWORD);
        }
        if (*dirty++)
        {
            if (block)
            {
                DWORD *data = (DWORD *)cursor;
                D3DIndexBuffer *indexBuffer = device->m_pIndexBuffer;
                block->D3_Index = TRUE;
                data[0] = device->m_IndexBase;
                data[1] = (DWORD)indexBuffer;
                if (indexBuffer) indexBuffer->AddRef();
            }
            cursor += 2 * sizeof(DWORD);
        }
        for (i = 0; i < D3DVS_STREAMS_MAX_V1_0; ++i)
        {
            if (*dirty++)
            {
                if (block)
                {
                    DWORD *data = (DWORD *)cursor;
                    D3DVertexBuffer *vertexBuffer = g_Stream[i].pVertexBuffer;
                    ++block->D4_Streams;
                    data[0] = i;
                    data[1] = g_Stream[i].Stride;
                    data[2] = (DWORD)vertexBuffer;
                    if (vertexBuffer) vertexBuffer->AddRef();
                }
                cursor += 3 * sizeof(DWORD);
            }
        }
        for (i = 0; i < D3DPS_CONSTREG_MAX_DX8; ++i)
        {
            if (*dirty++ && device->m_pPixelShader)
            {
                if (block)
                {
                    DWORD *data = (DWORD *)cursor;
                    ++block->D6_PixelShaderConstants;
                    data[0] = i;
                    memcpy(data + 1, device->m_PixelShaderConstants[i], 4 * sizeof(FLOAT));
                }
                cursor += 5 * sizeof(DWORD);
            }
        }
        for (i = 0; i < D3DVS_CONSTREG_COUNT_XBOX; ++i)
        {
            if (*dirty++ && i >= 96)
            {
                if (block)
                {
                    DWORD *data = (DWORD *)cursor;
                    ++block->D7_VertexShaderConstants;
                    data[0] = i;
                    memcpy(data + 1, device->m_VertexShaderConstants[i], 4 * sizeof(FLOAT));
                }
                cursor += 5 * sizeof(DWORD);
            }
        }
        for (i = 0; i < D3DRS_MAX; ++i)
        {
            if (*dirty++)
            {
                if (device->m_pPixelShader || i < D3DRS_PSALPHAINPUTS0 ||
                    (i > D3DRS_PSINPUTTEXTURE && i != D3DRS_PSTEXTUREMODES))
                {
                    if (block)
                    {
                        DWORD *data = (DWORD *)cursor;
                        ++block->D8_RenderStates;
                        data[0] = i;
                        data[1] = D3D__RenderState[i];
                    }
                    cursor += 2 * sizeof(DWORD);
                }
            }
        }
        for (j = 0; j < D3DTSS_MAX; ++j)
        {
            for (i = 0; i < D3DTSS_MAXSTAGES; ++i)
            {
                if (*dirty++)
                {
                    if (block)
                    {
                        DWORD *data = (DWORD *)cursor;
                        ++block->D9_TextureStates;
                        data[0] = i;
                        data[1] = j;
                        data[2] = D3D__TextureState[i][j];
                    }
                    cursor += 3 * sizeof(DWORD);
                }
            }
        }
        for (i = 0; i < D3DTS_MAX; ++i)
        {
            if (*dirty++)
            {
                if (block)
                {
                    DWORD *data = (DWORD *)cursor;
                    ++block->D10_Transforms;
                    data[0] = i;
                    memcpy(data + 1, &device->m_Transform[i], sizeof(D3DMATRIX));
                }
                cursor += sizeof(DWORD) + sizeof(D3DMATRIX);
            }
        }
        if (*dirty++)
        {
            if (block)
            {
                block->D11_Viewport = TRUE;
                memcpy((void *)cursor, &device->m_Viewport, sizeof(D3DVIEWPORT8));
            }
            cursor += sizeof(D3DVIEWPORT8);
        }
        if (*dirty++)
        {
            if (block)
            {
                block->D12_Material = TRUE;
                memcpy((void *)cursor, &device->m_Material, sizeof(D3DMATERIAL8));
            }
            cursor += sizeof(D3DMATERIAL8);
        }
        if (*dirty++)
        {
            if (block)
            {
                block->D13_BackMaterial = TRUE;
                memcpy((void *)cursor, &device->m_BackMaterial, sizeof(D3DMATERIAL8));
            }
            cursor += sizeof(D3DMATERIAL8);
        }
        for (i = 0; i < device->m_LightCount; ++i)
        {
            if (lights[i].Flags & LIGHT_STATEBLOCK_SET)
            {
                if (block)
                {
                    DWORD *data = (DWORD *)cursor;
                    ++block->D14_Lights;
                    data[0] = i;
                    /* Original record path copies Light8 from the first light,
                     * whereas CaptureStateBlock uses the indexed light. Preserve
                     * this observed distinction; do not call it a corrected API. */
                    memcpy(data + 1, &lights->Light8, sizeof(D3DLIGHT8));
                }
                cursor += sizeof(DWORD) + sizeof(D3DLIGHT8);
            }
        }
        for (i = 0; i < device->m_LightCount; ++i)
        {
            if (lights[i].Flags & LIGHT_STATEBLOCK_ENABLED)
            {
                if (block)
                {
                    DWORD *data = (DWORD *)cursor;
                    ++block->D15_LightEnables;
                    data[0] = i;
                    device->GetLightEnable(i, (BOOL *)(data + 1));
                }
                cursor += 2 * sizeof(DWORD);
            }
        }
        if (block)
        {
            *token = (DWORD)block;
            return S_OK;
        }
        block = (StateBlock *)LocalAlloc(LMEM_FIXED, cursor);
        if (!block) return E_OUTOFMEMORY;
        ZeroMemory(block, offsetof(StateBlock, Data));
        cursor = (ULONG_PTR)StateBlockData(block);
    }
}

const BYTE g_PixelRenderStates[] =
{
    D3DRS_ZENABLE,
    D3DRS_FILLMODE,
    D3DRS_SHADEMODE,
    D3DRS_ZWRITEENABLE,
    D3DRS_ALPHATESTENABLE,
    D3DRS_SRCBLEND,
    D3DRS_DESTBLEND,
    D3DRS_ZFUNC,
    D3DRS_ALPHAREF,
    D3DRS_ALPHAFUNC,
    D3DRS_DITHERENABLE,
    D3DRS_FOGSTART,
    D3DRS_FOGEND,
    D3DRS_FOGDENSITY,
    D3DRS_EDGEANTIALIAS,
    D3DRS_ALPHABLENDENABLE,
    D3DRS_ZBIAS,
    D3DRS_STENCILENABLE,
    D3DRS_STENCILFAIL,
    D3DRS_STENCILZFAIL,
    D3DRS_STENCILPASS,
    D3DRS_STENCILFUNC,
    D3DRS_STENCILREF,
    D3DRS_STENCILMASK,
    D3DRS_STENCILWRITEMASK,
    D3DRS_TEXTUREFACTOR,
    D3DRS_WRAP0,
    D3DRS_WRAP1,
    D3DRS_WRAP2,
    D3DRS_WRAP3,
    D3DRS_COLORWRITEENABLE,
    D3DRS_BLENDOP,
};

const BYTE g_PixelTextureStates[] =
{
    D3DTSS_COLOROP,
    D3DTSS_COLORARG1,
    D3DTSS_COLORARG2,
    D3DTSS_ALPHAOP,
    D3DTSS_ALPHAARG1,
    D3DTSS_ALPHAARG2,
    D3DTSS_BUMPENVMAT00,
    D3DTSS_BUMPENVMAT01,
    D3DTSS_BUMPENVMAT10,
    D3DTSS_BUMPENVMAT11,
    D3DTSS_TEXCOORDINDEX,
    D3DTSS_ADDRESSU,
    D3DTSS_ADDRESSV,
    D3DTSS_BORDERCOLOR,
    D3DTSS_MAGFILTER,
    D3DTSS_MINFILTER,
    D3DTSS_MIPFILTER,
    D3DTSS_MIPMAPLODBIAS,
    D3DTSS_MAXMIPLEVEL,
    D3DTSS_MAXANISOTROPY,
    D3DTSS_BUMPENVLSCALE,
    D3DTSS_BUMPENVLOFFSET,
    D3DTSS_TEXTURETRANSFORMFLAGS,
    D3DTSS_ADDRESSW,
    D3DTSS_COLORARG0,
    D3DTSS_ALPHAARG0,
    D3DTSS_RESULTARG,
};

const BYTE g_VertexRenderStates[] =
{
    D3DRS_SHADEMODE,
    D3DRS_SPECULARENABLE,
    D3DRS_CULLMODE,
    D3DRS_FOGENABLE,
    D3DRS_FOGCOLOR,
    D3DRS_FOGTABLEMODE,
    D3DRS_FOGSTART,
    D3DRS_FOGEND,
    D3DRS_FOGDENSITY,
    D3DRS_RANGEFOGENABLE,
    D3DRS_AMBIENT,
    D3DRS_COLORVERTEX,
    D3DRS_LIGHTING,
    D3DRS_NORMALIZENORMALS,
    D3DRS_LOCALVIEWER,
    D3DRS_EMISSIVEMATERIALSOURCE,
    D3DRS_AMBIENTMATERIALSOURCE,
    D3DRS_DIFFUSEMATERIALSOURCE,
    D3DRS_SPECULARMATERIALSOURCE,
    D3DRS_VERTEXBLEND,
    D3DRS_POINTSIZE,
    D3DRS_POINTSIZE_MIN,
    D3DRS_POINTSPRITEENABLE,
    D3DRS_POINTSCALEENABLE,
    D3DRS_POINTSCALE_A,
    D3DRS_POINTSCALE_B,
    D3DRS_POINTSCALE_C,
    D3DRS_MULTISAMPLEANTIALIAS,
    D3DRS_MULTISAMPLEMASK,
    D3DRS_PATCHEDGESTYLE,
    D3DRS_PATCHSEGMENTS,
    D3DRS_POINTSIZE_MAX,
};

const BYTE g_VertexTextureStates[] =
{
    D3DTSS_TEXCOORDINDEX,
    D3DTSS_TEXTURETRANSFORMFLAGS,
};

extern "C" HRESULT WINAPI D3DDevice_CreateStateBlock(
    D3DSTATEBLOCKTYPE type,
    DWORD *token)
{
    DWORD i;
    DWORD j;
    CDevice *device = g_pDevice;
    if (type == D3DSBT_ALL)
    {
        memset(D3D__StateBlockDirty, 0xff, sizeof(D3D__StateBlockDirty));
        Light *lights = device->m_pLights;
        for (i = 0; i < device->m_LightCount; ++i)
        {
            if (lights[i].Flags & LIGHT_SET)
                lights[i].Flags |= LIGHT_STATEBLOCK_SET | LIGHT_STATEBLOCK_ENABLED;
        }
    }
    else
    {
        ClearStateBlockFlags();
        if (type == D3DSBT_PIXELSTATE)
        {
            D3D__StateBlockDirty[D3DSBD_PIXELSHADER] = TRUE;
            memset(D3D__StateBlockDirty + D3DSBD_PIXELSHADERCONSTANTS, 0xff, D3DPS_CONSTREG_MAX_DX8);
            for (i = 0; i < sizeof(g_PixelRenderStates); ++i)
                D3D__StateBlockDirty[D3DSBD_RENDERSTATES + g_PixelRenderStates[i]] = TRUE;
            for (i = 0; i < D3DTSS_MAXSTAGES; ++i)
                for (j = 0; j < sizeof(g_PixelTextureStates); ++j)
                    D3D__StateBlockDirty[D3DSBD_TEXTURESTATES + g_PixelTextureStates[j] * D3DTSS_MAXSTAGES + i] = TRUE;
        }
        else
        {
            D3D__StateBlockDirty[D3DSBD_VERTEXSHADER] = TRUE;
            memset(D3D__StateBlockDirty + D3DSBD_VERTEXSHADERCONSTANTS, 0xff, D3DVS_CONSTREG_COUNT_XBOX);
            for (i = 0; i < sizeof(g_VertexRenderStates); ++i)
                D3D__StateBlockDirty[D3DSBD_RENDERSTATES + g_VertexRenderStates[i]] = TRUE;
            for (i = 0; i < D3DTSS_MAXSTAGES; ++i)
                for (j = 0; j < sizeof(g_VertexTextureStates); ++j)
                    D3D__StateBlockDirty[D3DSBD_TEXTURESTATES + g_VertexTextureStates[j] * D3DTSS_MAXSTAGES + i] = TRUE;
            Light *lights = device->m_pLights;
            for (i = 0; i < device->m_LightCount; ++i)
            {
                if (lights[i].Flags & LIGHT_SET)
                    lights[i].Flags |= LIGHT_STATEBLOCK_SET | LIGHT_STATEBLOCK_ENABLED;
            }
        }
    }
    return RecordStateBlock(token);
}
extern "C" HRESULT WINAPI D3DDevice_EndStateBlock(
    DWORD *token)
{
    g_pDevice->m_StateFlags &= ~STATE_RECORDBLOCK;
    return RecordStateBlock(token);
}
extern "C" void WINAPI D3DDevice_Resume(
    BOOL reset)
{
    if (reset)
    {
        /* The original Resume path assumes state-block allocation succeeds.
         * This is not a checked recovery path for allocation failure. */
        DWORD token;
        D3DDevice_CreateStateBlock(D3DSBT_ALL, &token);
        D3DDevice_ApplyStateBlock(token);
        D3DDevice_DeleteStateBlock(token);
    }
    return;
}
}
