#ifndef D3D8_PUSH_INTERNAL_H
#define D3D8_PUSH_INTERNAL_H
/* Target dynamic method expressions fold the count with ADD, not OR.
 * Valid method fields do not overlap the count field.
 * NV2A FIFO packet operations. The method word and data words are GPU
 * commands, never native x86 instruction bytes. */
#include "device_layout.h"
#include "xmetal_internal.h"
extern "C" DWORD *WINAPI XMETAL_StartPush(
    _XMETAL_PushBuffer *buffer);
extern "C" DWORD *WINAPI XMETAL_StartPushCount(
    _XMETAL_PushBuffer *buffer,
    DWORD count);
extern "C" _XMETAL_PushBuffer *g_pPushBuffer;
namespace D3D
{
void WINAPI DumpMatrixTransposed(
    DWORD *push,
    DWORD method,
    const D3DMATRIX *matrix);
void WINAPI KickOffAndWaitForIdle(
    void);
void WINAPI BlockOnTime(
    DWORD time,
    BOOL makeSpace);
DWORD WINAPI SetFence(
    DWORD flags);
__inline DWORD *CDevice::StartPush(
    void)
{
    return XMETAL_StartPush(
        &m_Pusher);
}
__inline DWORD *CDevice::StartPush(
    DWORD count)
{
    return XMETAL_StartPushCount(
        &m_Pusher,
        count);
}
__inline void CDevice::EndPush(
    DWORD *put)
{
    XMETAL_EndPush(&m_Pusher, put);
    return;
}
static __inline void WINAPI PushCount(
    DWORD *push,
    DWORD method,
    DWORD count)
{
    XMETAL_PushCount(push, method, count);
    return;
}
static __inline void WINAPI Push1(
    DWORD *push,
    DWORD method,
    DWORD value)
{
    XMETAL_Push1(push, method, value);
    return;
}
static __inline void WINAPI Push2(
    DWORD *push,
    DWORD method,
    DWORD first,
    DWORD second)
{
    XMETAL_Push2(push, method, first, second);
    return;
}
/* January's eight hardware FIFO subchannels, authenticated by the original PDB. */
enum SubChannel
{
    SUBCH_3D = 0,
    SUBCH_SW = 1,
    SUBCH_MEMCOPY = 2,
    SUBCH_RECTCOPY = 3,
    SUBCH_RECTCOPYSURFACES = 4,
    SUBCH_UNUSED0 = 5,
    SUBCH_UNUSED1 = 6,
    SUBCH_UNUSED2 = 7
};
static __inline void WINAPI PushCount(
    DWORD *push,
    SubChannel subchannel,
    DWORD method,
    DWORD count)
{
    push[0] = method + ((subchannel + (count << 5)) << 13);
    return;
}
static __inline void WINAPI Push1(
    DWORD *push,
    SubChannel subchannel,
    DWORD method,
    DWORD value)
{
    push[0] = method + ((subchannel + (1 << 5)) << 13);
    push[1] = value;
    return;
}
static __inline void WINAPI Push3(
    DWORD *push,
    SubChannel subchannel,
    DWORD method,
    DWORD first,
    DWORD second,
    DWORD third)
{
    push[0] = method + ((subchannel + (3 << 5)) << 13);
    push[1] = first;
    push[2] = second;
    push[3] = third;
    return;
}
static __inline void WINAPI Push4(
    DWORD *push,
    SubChannel subchannel,
    DWORD method,
    DWORD first,
    DWORD second,
    DWORD third,
    DWORD fourth)
{
    push[0] = method + ((subchannel + (4 << 5)) << 13);
    push[1] = first;
    push[2] = second;
    push[3] = third;
    push[4] = fourth;
    return;
}
/* NV2A matrix packets have sixteen transposed words, or twelve words of
 * an already-computed inverse. Copies preserve the float representation. */
static __inline void WINAPI PushMatrixTransposed(
    DWORD *push,
    DWORD method,
    const D3DMATRIX *matrix)
{
    DumpMatrixTransposed(push, method + (16 << 18), matrix);
    return;
}
static __inline void WINAPI PushInverseModelViewMatrix(
    DWORD *push,
    DWORD method,
    const D3DMATRIX *matrix)
{
    PushCount(push, method, 12);
    memcpy(push + 1, matrix, 12 * sizeof(DWORD));
    return;
}
static __inline DWORD WINAPI SwapRgb(
    DWORD color)
{
    return (color & 0xff00ff00UL) | ((color & 0xff) << 16) | ((color >> 16) & 0xff);
}
/* IEEE-754 encodings used as constant GPU payload words. These are not
 * host instruction encodings; dynamic float payloads use representation copies. */
enum NvFloatWord
{
    NV_FLOAT_ZERO = 0x00000000,
    NV_FLOAT_ONE = 0x3f800000,
    NV_FLOAT_POSITIVE_INFINITY = 0x7f800000,
    NV_FLOAT_INFINITE_LIGHT_RANGE = 0x7149f2ca /* 1e30f */
};
/* NV097 method numbers, confirmed at all corresponding target writes. */
enum
{
    NV097_NO_OPERATION = 0x100,
    NV097_INVALIDATE_VERTEX_CACHE = 0x1710,
    NV097_WAIT_FOR_IDLE = 0x110,
    NV097_SET_FOG_COLOR = 0x2a8,
    NV097_SET_CULL_FACE_ENABLE = 0x308,
    NV097_SET_LINE_SMOOTH_ENABLE = 0x320,
    NV097_SET_CULL_FACE = 0x39c,
    NV097_SET_FRONT_FACE = 0x3a0,
    NV097_SET_NORMALIZATION_ENABLE = 0x3a4,
    NV097_SET_LINE_WIDTH = 0x3b8,
    NV097_SET_COMBINER_FACTOR0 = 0xa60,
    NV097_SET_SHADOW_DEPTH_FUNC = 0x1e6c,
    NV097_SET_CULL_FACE_V_FRONT = 0x404,
    NV097_SET_CULL_FACE_V_BACK = 0x405
};
static __inline void WINAPI Push4f(
    DWORD *push,
    DWORD method,
    float x,
    float y,
    float z,
    float w)
{
    /* GPU command memory contains float payloads, not x86 instructions. */
    push[0] = method + (4 << 18);
    ((float *)(push + 1))[0] = x;
    ((float *)(push + 1))[1] = y;
    ((float *)(push + 1))[2] = z;
    ((float *)(push + 1))[3] = w;
    return;
}
__inline void CDevice::RecordResourceReadPush(
    D3DResource *resource)
{
    resource->Lock = m_CpuTime;
    return;
}
__inline void CDevice::RecordSurfaceWritePush(
    D3DResource *resource)
{
    D3DBaseTexture *parent = ((D3DSurface *)resource)->Parent;
    if (parent) resource = parent;
    resource->Lock = m_CpuTime;
    return;
}
}
#endif
