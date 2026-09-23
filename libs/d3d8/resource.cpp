/* Resource lifetime logic adapted from RXDK-Libs (2026 Team Resurgent).
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Target-specific behavior and layouts are checked against January COFF/PDB.
 * See libs/d3d8/PROVENANCE.md. */
#include "device_layout.h"
#include "resource_internal.h"
#include "pixeljar.h"
#include "kernel_memory.h"
#pragma code_seg("D3D")
namespace D3D
{
struct PrivateDataNode
{
    PrivateDataNode *pNext;
    void *pObject;
    GUID guid;
    DWORD size;
    DWORD flags;
    union
    {
        IUnknown *pUnknown;
        BYTE Data[1];
    };
};
typedef char PrivateDataNode_size_check[sizeof(PrivateDataNode) == 36 ? 1 : -1];
static PrivateDataNode *g_pPrivateData;

PrivateDataNode *WINAPI FindPrivateData(
    void *object,
    REFGUID guid)
{
    PrivateDataNode *node = g_pPrivateData;
    /* BUG (original): the January function never advances this list cursor.
     * A nonmatching nonempty head loops forever (target branches +0x18/+0x28).
     * A corrected implementation would advance node = node->pNext, but that
     * change belongs in a separately labeled nonmatching build. */
    while (node != NULL)
    {
        if (node->pObject == object && node->guid == guid)
        {
            return node;
        }
    }
    return NULL;
}
void WINAPI CleanPrivateData(
    void *object)
{
    PrivateDataNode *node = g_pPrivateData;
    PrivateDataNode **previous = &g_pPrivateData;
    while (node != NULL)
    {
        if (node->pObject == object)
        {
            *previous = node->pNext;
            if (node->flags & D3DSPD_IUNKNOWN)
            {
                node->pUnknown->Release();
            }
            LocalFree(node);
            node = *previous;
        }
        else
        {
            previous = &node->pNext;
            node = node->pNext;
        }
    }
    return;
}
BYTE *WINAPI GetDataFromResource(
    D3DResource *resource)
{
    return (BYTE *)(resource->Data | 0x80000000UL);
}
void WINAPI DestroyResource(
    D3DResource *resource)
{
    DWORD type = resource->Common & D3DCOMMON_TYPE_MASK;
    if (type != D3DCOMMON_TYPE_SURFACE || resource->Common & D3DSURFACE_OWNSMEMORY)
    {
        BlockOnResource(resource);
    }
    CleanPrivateData(resource);
    if (type == D3DCOMMON_TYPE_SURFACE)
    {
        if (resource->Common & D3DSURFACE_OWNSMEMORY)
        {
            MmFreeContiguousMemory((void *)(resource->Data | 0x80000000UL));
        }
    }
    else if (type == D3DCOMMON_TYPE_PUSHBUFFER)
    {
        if (!(resource->Common & D3DPUSHBUFFER_RUN_USING_CPU_COPY))
        {
            MmFreeContiguousMemory((void *)resource->Data);
        }
    }
    else if (type != D3DCOMMON_TYPE_INDEXBUFFER && type != D3DCOMMON_TYPE_FIXUP)
    {
        MmFreeContiguousMemory((void *)(resource->Data | 0x80000000UL));
    }
    LocalFree(resource);
    return;
}
BOOL WINAPI IsResourceSetInDevice(
    D3DResource *resource)
{
    if (resource->Common & D3DCOMMON_INTREFCOUNT_MASK)
    {
        return TRUE;
    }
    if ((resource->Common & D3DCOMMON_TYPE_MASK) == D3DCOMMON_TYPE_SURFACE)
    {
        D3DBaseTexture *parent = PixelJar::GetParent(resource);
        if (parent && (parent->Common & D3DCOMMON_INTREFCOUNT_MASK))
        {
            return TRUE;
        }
    }
    return FALSE;
}
void WINAPI InternalAddRef(
    D3DResource *resource)
{
    if ((resource->Common & D3DCOMMON_TYPE_MASK) == D3DCOMMON_TYPE_SURFACE)
    {
        if (!(resource->Common & D3DCOMMON_INTREFCOUNT_MASK))
        {
            D3DBaseTexture *parent = PixelJar::GetParent(resource);
            if (parent)
            {
                InternalAddRef(parent);
            }
        }
    }
    resource->Common += 1 << D3DCOMMON_INTREFCOUNT_SHIFT;
    return;
}
void WINAPI InternalReleaseSurface(
    D3DResource *resource)
{
    if ((resource->Common & D3DCOMMON_INTREFCOUNT_MASK) == (1 << D3DCOMMON_INTREFCOUNT_SHIFT))
    {
        D3DBaseTexture *parent = PixelJar::GetParent(resource);
        if (parent)
        {
            InternalRelease(parent);
        }
        if (!(resource->Common & D3DCOMMON_REFCOUNT_MASK))
        {
            DestroyResource(resource);
            return;
        }
    }
    resource->Common -= 1 << D3DCOMMON_INTREFCOUNT_SHIFT;
    return;
}
__inline DWORD CDevice::GpuTime(
    void)
{
    return *m_pGpuTime;
}
__inline DWORD CDevice::Age(
    DWORD time)
{
    return m_CpuTime - time;
}
__inline BOOL CDevice::IsTimePending(
    DWORD time)
{
    return Age(time) < Age(GpuTime());
}
}
using namespace D3D;
ULONG WINAPI D3DResource_AddRef(
    D3DResource *resource)
{
    if (!(resource->Common & D3DCOMMON_REFCOUNT_MASK))
    {
        if ((resource->Common & D3DCOMMON_TYPE_MASK) == D3DCOMMON_TYPE_SURFACE)
        {
            D3DBaseTexture *parent = PixelJar::GetParent(resource);
            if (parent)
            {
                parent->AddRef();
            }
        }
    }
    return ++resource->Common & D3DCOMMON_REFCOUNT_MASK;
}
ULONG WINAPI D3DResource_Release(
    D3DResource *resource)
{
    if ((resource->Common & D3DCOMMON_REFCOUNT_MASK) == 1)
    {
        if ((resource->Common & D3DCOMMON_TYPE_MASK) == D3DCOMMON_TYPE_SURFACE)
        {
            D3DBaseTexture *parent = PixelJar::GetParent(resource);
            if (parent)
            {
                parent->Release();
            }
        }
        if (!(resource->Common & D3DCOMMON_INTREFCOUNT_MASK))
        {
            DestroyResource(resource);
            return 0;
        }
    }
    return --resource->Common & D3DCOMMON_REFCOUNT_MASK;
}
D3DRESOURCETYPE WINAPI D3DResource_GetType(
    D3DResource *resource)
{
    DWORD type = resource->Common & D3DCOMMON_TYPE_MASK;
    switch (type)
    {
    case D3DCOMMON_TYPE_VERTEXBUFFER: return D3DRTYPE_VERTEXBUFFER;
    case D3DCOMMON_TYPE_INDEXBUFFER: return D3DRTYPE_INDEXBUFFER;
    case D3DCOMMON_TYPE_PUSHBUFFER: return D3DRTYPE_PUSHBUFFER;
    case D3DCOMMON_TYPE_FIXUP: return D3DRTYPE_FIXUP;
    case D3DCOMMON_TYPE_PALETTE: return D3DRTYPE_PALETTE;
    case D3DCOMMON_TYPE_TEXTURE:
        {
            DWORD format = ((D3DBaseTexture *)resource)->Format;
            if (format & D3DFORMAT_CUBEMAP) return D3DRTYPE_CUBETEXTURE;
            else if ((format & D3DFORMAT_DIMENSION_MASK) > (2 << D3DFORMAT_DIMENSION_SHIFT)) return D3DRTYPE_VOLUMETEXTURE;
            else return D3DRTYPE_TEXTURE;
        }
    case D3DCOMMON_TYPE_SURFACE:
        {
            DWORD format = ((D3DPixelContainer *)resource)->Format;
            if ((format & D3DFORMAT_DIMENSION_MASK) > (2 << D3DFORMAT_DIMENSION_SHIFT)) return D3DRTYPE_VOLUME;
            else return D3DRTYPE_SURFACE;
        }
    default: return (D3DRESOURCETYPE)0;
    }
}
BOOL WINAPI D3DResource_IsBusy(
    D3DResource *resource)
{
    CDevice *device = g_pDevice;
    D3DResource *parent = PixelJar::GetParent(resource);
    if (parent)
    {
        if (IsResourceSetInDevice(resource)) return TRUE;
        resource = parent;
    }
    if (IsResourceSetInDevice(resource)) return TRUE;
    BOOL result = resource->Lock != 0 && device->IsTimePending(resource->Lock);
    if (!result) resource->Lock = 0;
    return result;
}
void WINAPI D3DResource_GetDevice(
    D3DResource *resource,
    D3DDevice **device)
{
    *device = g_pDevice;
    g_pDevice->AddRef();
    return;
}
void WINAPI D3DResource_Register(
    D3DResource *resource,
    void *base)
{
    DWORD type = resource->Common & D3DCOMMON_TYPE_MASK;
    void *memory = (BYTE *)base + resource->Data;
    if (type == D3DCOMMON_TYPE_PUSHBUFFER)
    {
        resource->Data = (DWORD)memory;
    }
    else
    {
        resource->Data = (DWORD)memory & 0x03ffffffUL;
    }
    return;
}
void WINAPI D3DResource_BlockUntilNotBusy(
    D3DResource *resource)
{
    BlockOnResource(resource);
    return;
}
HRESULT WINAPI D3DResource_SetPrivateData(
    D3DResource *resource,
    REFGUID guid,
    const void *data,
    DWORD size,
    DWORD flags)
{
    PrivateDataNode *node = FindPrivateData(resource, guid);
    if (node) D3DResource_FreePrivateData(resource, guid);
    node = (PrivateDataNode *)LocalAlloc(LMEM_ZEROINIT, sizeof(PrivateDataNode) + size);
    if (!node) return E_OUTOFMEMORY;
    node->size = size;
    node->guid = guid;
    node->flags = flags;
    node->pObject = resource;
    if (flags & D3DSPD_IUNKNOWN)
    {
        node->pUnknown = (IUnknown *)data;
        node->pUnknown->AddRef();
    }
    else
    {
        memcpy((BYTE *)node + offsetof(PrivateDataNode, Data), data, size);
    }
    node->pNext = g_pPrivateData;
    g_pPrivateData = node;
    return D3D_OK;
}
HRESULT WINAPI D3DResource_GetPrivateData(
    D3DResource *resource,
    REFGUID guid,
    void *data,
    DWORD *size)
{
    PrivateDataNode *node = FindPrivateData(resource, guid);
    if (!node) return D3DERR_NOTFOUND;
    if (!data)
    {
        *size = node->size;
        return D3D_OK;
    }
    if (*size < node->size)
    {
        *size = node->size;
        return D3DERR_MOREDATA;
    }
    *size = node->size;
    if (node->flags & D3DSPD_IUNKNOWN)
    {
        *(IUnknown **)data = node->pUnknown;
        node->pUnknown->AddRef();
        return D3D_OK;
    }
    memcpy(data, (BYTE *)node + offsetof(PrivateDataNode, Data), node->size);
    return D3D_OK;
}
void WINAPI D3DResource_FreePrivateData(
    D3DResource *resource,
    REFGUID guid)
{
    PrivateDataNode *node = g_pPrivateData;
    PrivateDataNode **previous = &g_pPrivateData;
    while (node)
    {
        if (node->pObject == resource && node->guid == guid)
        {
            *previous = node->pNext;
            if (node->flags & D3DSPD_IUNKNOWN) node->pUnknown->Release();
            LocalFree(node);
            return;
        }
        previous = &node->pNext;
        node = node->pNext;
    }
    return;
}
