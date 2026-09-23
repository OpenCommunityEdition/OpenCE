#include "resource_internal.h"
#include "memory_internal.h"
#include "kernel_memory.h"
#include "push_internal.h"
#include "buffer_internal.h"
#pragma code_seg("D3D")
void WINAPI D3DIndexBuffer_GetDesc(
    D3DIndexBuffer *buffer,
    D3DINDEXBUFFER_DESC *desc)
{
    desc->Format = D3DFMT_INDEX16;
    desc->Type = D3DRTYPE_INDEXBUFFER;
    return;
}
void WINAPI D3DPalette_Lock(
    D3DPalette *palette,
    D3DCOLOR **colors,
    DWORD flags)
{
    if (!(flags & (D3DLOCK_NOOVERWRITE | D3DLOCK_READONLY)))
    {
        D3D::BlockOnResource(palette);
    }
    *colors = (D3DCOLOR *)D3D::GetDataFromResource(palette);
    return;
}
D3DPALETTESIZE WINAPI D3DPalette_GetSize(
    D3DPalette *palette)
{
    return (D3DPALETTESIZE)(palette->Common >> 30);
}

HRESULT WINAPI D3DDevice_CreateIndexBuffer(
    UINT length,
    DWORD usage,
    D3DFORMAT format,
    D3DPOOL pool,
    D3DIndexBuffer **result)
{
    D3DIndexBuffer *buffer = (D3DIndexBuffer *)LocalAlloc(0, length + sizeof(D3DIndexBuffer));
    if (!buffer)
    {
        return E_OUTOFMEMORY;
    }
    memset(buffer, 0, sizeof(*buffer));
    buffer->Common = 0x01010001;
    buffer->Data = (DWORD)(buffer + 1);
    *result = buffer;
    return S_OK;
}
HRESULT WINAPI D3DDevice_CreateVertexBuffer(
    UINT length,
    DWORD usage,
    DWORD fvf,
    D3DPOOL pool,
    D3DVertexBuffer **result)
{
    D3DVertexBuffer *buffer = (D3DVertexBuffer *)LocalAlloc(LMEM_ZEROINIT, sizeof(*buffer));
    if (!buffer)
    {
        return E_OUTOFMEMORY;
    }
    void *memory = MmAllocateContiguousMemoryEx(length, 0, 0x03ffb000, 0,
        PAGE_READWRITE | PAGE_WRITECOMBINE);
    if (!memory)
    {
        LocalFree(buffer);
        return E_OUTOFMEMORY;
    }
    buffer->Data = (DWORD)memory & 0x03ffffff;
    buffer->Common = D3DCOMMON_D3DCREATED | D3DCOMMON_TYPE_VERTEXBUFFER | 1;
    *result = buffer;
    return S_OK;
}
HRESULT WINAPI D3DDevice_CreatePalette(
    D3DPALETTESIZE size,
    D3DPalette **result)
{
    D3DPalette *palette = (D3DPalette *)LocalAlloc(LMEM_ZEROINIT, sizeof(*palette));
    if (!palette)
    {
        return E_OUTOFMEMORY;
    }
    void *memory = MmAllocateContiguousMemoryEx(D3D::g_PaletteSize[size], 0,
        0x03ffb000, 0, PAGE_READWRITE | PAGE_WRITECOMBINE);
    if (!memory)
    {
        LocalFree(palette);
        return E_OUTOFMEMORY;
    }
    DWORD paletteBits = (DWORD)size << D3DPALETTE_COMMON_PALETTESIZE_SHIFT;
    palette->Data = (DWORD)memory & 0x03ffffff;
    palette->Common = paletteBits | D3DCOMMON_D3DCREATED | D3DCOMMON_TYPE_PALETTE | 1;
    *result = palette;
    return S_OK;
}
void WINAPI D3DVertexBuffer_Lock(
    D3DVertexBuffer *buffer,
    UINT offset,
    UINT size,
    BYTE **data,
    DWORD flags)
{
    if (!(flags & D3DLOCK_NOFLUSH))
    {
        D3D::CDevice *device = D3D::g_pDevice;
        DWORD *push = device->StartPush();
        D3D::Push1(push, D3D::NV097_INVALIDATE_VERTEX_CACHE, 0);
        device->EndPush(push + 2);
    }
    if (!(flags & (D3DLOCK_NOOVERWRITE | D3DLOCK_READONLY)))
    {
        D3D::BlockOnResource(buffer);
    }
    *data = D3D::GetDataFromResource(buffer) + offset;
    return;
}
void WINAPI D3DVertexBuffer_GetDesc(
    D3DVertexBuffer *buffer,
    D3DVERTEXBUFFER_DESC *desc)
{
    desc->Format = D3DFMT_VERTEXDATA;
    desc->Type = buffer->GetType();
    return;
}
void *WINAPI D3DVertexBuffer_Lock2(
    D3DVertexBuffer *buffer,
    DWORD flags)
{
    if (!(flags & (D3DLOCK_NOOVERWRITE | D3DLOCK_READONLY)))
    {
        D3D::BlockOnResource(buffer);
    }
    return D3D::GetDataFromResource(buffer);
}

#pragma const_seg("D3D_RD")
namespace D3D
{
/* Exact original buffer.obj table, not a later SDK palette-size assumption. */
const int g_PaletteSize[] = { 256, 128, 64, 32 };
}
