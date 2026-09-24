/* Reconstructed from January 2002 target code and genuine SDK declarations.
 * NV2A method payloads are GPU commands, not native instruction emission. */
#include "draw_internal.h"
#include "push_internal.h"
#pragma code_seg("D3D")
using namespace D3D;
void WINAPI D3DDevice_SetVertexData2f(
    int reg,
    float x,
    float y)
{
    CDevice *device = g_pDevice;
    DWORD *push = device->StartPush();
    PushCount(push, 0x1880 + reg * 8, 2);
    memcpy(push + 1, &x, sizeof(x));
    memcpy(push + 2, &y, sizeof(y));
    device->EndPush(push + 3);
    return;
}
void WINAPI D3DDevice_SetVertexData4f(
    int reg,
    float x,
    float y,
    float z,
    float w)
{
    CDevice *device = g_pDevice;
    DWORD *push = device->StartPush();
    DWORD method = reg == -1 ? 0x1518 : 0x1a00 + reg * 16;
    PushCount(push, method, 4);
    memcpy(push + 1, &x, sizeof(x));
    memcpy(push + 2, &y, sizeof(y));
    memcpy(push + 3, &z, sizeof(z));
    memcpy(push + 4, &w, sizeof(w));
    device->EndPush(push + 5);
    return;
}
void WINAPI D3DDevice_SetVertexData2s(
    int reg,
    short x,
    short y)
{
    CDevice *device = g_pDevice;
    DWORD *push = device->StartPush();
    Push1(push, 0x1900 + reg * 4, (WORD)x | ((DWORD)y << 16));
    device->EndPush(push + 2);
    return;
}
void WINAPI D3DDevice_SetVertexData4s(
    int reg,
    short x,
    short y,
    short z,
    short w)
{
    CDevice *device = g_pDevice;
    DWORD *push = device->StartPush();
    Push2(push, 0x1980 + reg * 8, (WORD)x | ((DWORD)y << 16),
        (WORD)z | ((DWORD)w << 16));
    device->EndPush(push + 3);
    return;
}
void WINAPI D3DDevice_SetVertexData4ub(
    int reg,
    BYTE x,
    BYTE y,
    BYTE z,
    BYTE w)
{
    CDevice *device = g_pDevice;
    DWORD *push = device->StartPush();
    Push1(push, 0x1940 + reg * 4, (((((DWORD)w << 8) | z) << 8) | y) << 8 | x);
    device->EndPush(push + 2);
    return;
}
void WINAPI D3DDevice_SetVertexDataColor(
    int reg,
    D3DCOLOR color)
{
    CDevice *device = g_pDevice;
    DWORD *push = device->StartPush();
    Push1(push, 0x1940 + reg * 4, SwapRgb(color));
    device->EndPush(push + 2);
    return;
}
void WINAPI D3DDevice_Begin(
    D3DPRIMITIVETYPE primitiveType)
{
    CDevice *device = g_pDevice;
    g_CurrentPrimitiveType = primitiveType;
    SetState();
    DWORD *push = device->StartPush();
    Push1(push, 0x17fc, primitiveType);
    device->EndPush(push + 2);
    device->m_StateFlags |= 0x800;
    return;
}
void WINAPI D3DDevice_End(
    void)
{
    CDevice *device = g_pDevice;
    g_CurrentPrimitiveType = 0;
    DWORD *push = device->StartPush();
    Push1(push, 0x17fc, 0);
    device->EndPush(push + 2);
    if (device->m_StateFlags & 0x1000)
        SetFence(1);
    device->m_StateFlags &= ~0x1800;
    return;
}

void WINAPI D3DDevice_DrawVertices(
    D3DPRIMITIVETYPE primitiveType,
    UINT startVertex,
    UINT vertexCount)
{
    CDevice *device = g_pDevice;
    device->SetStateVB(0);
    DWORD batchCount = ((vertexCount - 1) >> 8) + 1;
    DWORD *push = device->StartPush(batchCount + 5);
    Push1(push, 0x17fc, primitiveType);
    PushCount(push + 2, 0x40001810, batchCount);
    while (vertexCount > 256)
    {
        push[3] = startVertex | 0xff000000;
        push++;
        startVertex += 256;
        vertexCount -= 256;
    }
    push[3] = startVertex | ((vertexCount - 1) << 24);
    Push1(push + 4, 0x17fc, 0);
    device->EndPush(push + 6);
    return;
}
void WINAPI D3DDevice_PrimeVertexCache(
    UINT vertexCount,
    const WORD *indexData)
{
    CDevice *device = g_pDevice;
    device->SetStateVB(device->m_IndexBase);
    DWORD dwordCount = vertexCount >> 1;
    DWORD *push = device->StartPush();
    PushCount(push, 0x40001800, dwordCount);
    DWORD size = dwordCount * sizeof(DWORD);
    memcpy(push + 1, indexData, size);
    push += dwordCount + 1;
    if (vertexCount & 1)
    {
        Push1(push, 0x1808, indexData[dwordCount * 2]);
        push += 2;
    }
    device->EndPush(push);
    return;
}

/* These user-pointer paths submit the original attribute-run representation.
 * Integer byte cursors permit a signed/wrapping inter-run displacement without
 * forming an invalid intermediate C++ pointer. Each copied range must be backed
 * by the caller's valid vertex allocation, including DWORD attribute padding. */
void WINAPI D3DDevice_DrawVerticesUP(
    D3DPRIMITIVETYPE primitiveType,
    UINT vertexCount,
    const void *vertexData,
    UINT stride)
{
    CDevice *device = g_pDevice;
    device->SetStateUP();
    DWORD *push = device->StartPush();
    Push1(push, 0x17fc, primitiveType);
    device->EndPush(push + 2);
    device->m_StateFlags |= 0x800;
    ULONG_PTR source = (ULONG_PTR)vertexData + device->m_InlineStartOffset;
    DWORD attributeCount = device->m_InlineAttributeCount;
    InlineAttributeData *attributes = device->m_InlineAttributeData;
    attributes[attributeCount - 1].UP_Delta = stride + device->m_InlineDelta;
    DWORD limit = vertexCount <= 16 ? 16 : 2047 / device->m_InlineVertexDwords;
    do
    {
        DWORD batch = min(limit, vertexCount);
        DWORD words = device->m_InlineVertexDwords * batch;
        vertexCount -= batch;
        push = device->StartPush(words + 3);
        PushCount(push, 0x40001818, words);
        push++;
        do
        {
            for (DWORD i = 0; i < attributeCount; ++i)
            {
                DWORD bytes = attributes[i].UP_Count * sizeof(DWORD);
                memcpy(push, (const void *)source, bytes);
                push += attributes[i].UP_Count;
                source += bytes + attributes[i].UP_Delta;
            }
        } while (--batch != 0);
        if (vertexCount)
        {
            device->EndPush(push);
        }
    } while (vertexCount != 0);
    Push1(push, 0x17fc, 0);
    device->EndPush(push + 2);
    if (device->m_StateFlags & 0x1000)
    {
        SetFence(1);
    }
    device->m_StateFlags &= ~0x1800UL;
    return;
}

void WINAPI D3DDevice_DrawIndexedVerticesUP(
    D3DPRIMITIVETYPE primitiveType,
    UINT vertexCount,
    const void *indexData,
    const void *vertexData,
    UINT stride)
{
    const WORD *indices = (const WORD *)indexData;
    CDevice *device = g_pDevice;
    device->SetStateUP();
    DWORD *push = device->StartPush();
    Push1(push, 0x17fc, primitiveType);
    device->EndPush(push + 2);
    device->m_StateFlags |= 0x800;
    ULONG_PTR base = (ULONG_PTR)vertexData + device->m_InlineStartOffset;
    DWORD attributeCount = device->m_InlineAttributeCount;
    const InlineAttributeData *attributes = device->m_InlineAttributeData;
    DWORD limit = vertexCount <= 16 ? 16 : 2047 / device->m_InlineVertexDwords;
    do
    {
        DWORD batch = min(limit, vertexCount);
        DWORD words = device->m_InlineVertexDwords * batch;
        vertexCount -= batch;
        push = device->StartPush(words + 3);
        PushCount(push, 0x40001818, words);
        push++;
        do
        {
            ULONG_PTR source = base + (DWORD)*indices++ * stride;
            for (DWORD i = 0; i < attributeCount; ++i)
            {
                DWORD bytes = attributes[i].UP_Count * sizeof(DWORD);
                memcpy(push, (const void *)source, bytes);
                push += attributes[i].UP_Count;
                source += bytes + attributes[i].UP_Delta;
            }
        } while (--batch != 0);
        if (vertexCount)
        {
            device->EndPush(push);
        }
    } while (vertexCount != 0);
    Push1(push, 0x17fc, 0);
    device->EndPush(push + 2);
    if (device->m_StateFlags & 0x1000)
    {
        SetFence(1);
    }
    device->m_StateFlags &= ~0x1800UL;
    return;
}
