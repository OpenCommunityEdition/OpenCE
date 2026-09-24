#ifndef D3D8_XMETAL_INTERNAL_H
#define D3D8_XMETAL_INTERNAL_H
#include "device_layout.h"
/* January's PDB describes these header helpers as module-local procedures.
 * This is the actual raw packet layer used by the D3D convenience wrappers.
 * Count fields are disjoint from the method; payloads are NV2A commands.
 * Internal linkage permits ordinary VC7 private calling-convention optimization.
 */
extern "C"
{
static __inline void WINAPI XMETAL_EndPush(
    _XMETAL_PushBuffer *buffer,
    DWORD *put)
{
    buffer->m_pPut = put;
    return;
}
static __inline void WINAPI XMETAL_Push1(
    DWORD *push,
    DWORD method,
    DWORD value)
{
    push[0] = method + (1 << 18);
    push[1] = value;
    return;
}
static __inline void WINAPI XMETAL_Push2(
    DWORD *push,
    DWORD method,
    DWORD first,
    DWORD second)
{
    push[0] = method + (2 << 18);
    push[1] = first;
    push[2] = second;
    return;
}
static __inline void WINAPI XMETAL_PushCount(
    DWORD *push,
    DWORD method,
    DWORD count)
{
    push[0] = method + (count << 18);
    return;
}
}
#endif
