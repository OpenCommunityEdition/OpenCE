#ifndef D3D8_RESOURCE_INTERNAL_H
#define D3D8_RESOURCE_INTERNAL_H
#include <xtl.h>
namespace D3D
{
BOOL WINAPI IsResourceSetInDevice(
    D3DResource *resource);
void WINAPI BlockOnResource(
    D3DResource *resource);
BYTE *WINAPI GetDataFromResource(
    D3DResource *resource);
void WINAPI InternalAddRef(
    D3DResource *resource);
void WINAPI InternalReleaseSurface(
    D3DResource *resource);
void WINAPI DestroyResource(
    D3DResource *resource);
__inline void WINAPI InternalRelease(
    D3DResource *resource)
{
    resource->Common -= 1 << D3DCOMMON_INTREFCOUNT_SHIFT;
    if (!(resource->Common & (D3DCOMMON_REFCOUNT_MASK | D3DCOMMON_INTREFCOUNT_MASK)))
    {
        DestroyResource(resource);
    }
    return;
}
}

#endif
