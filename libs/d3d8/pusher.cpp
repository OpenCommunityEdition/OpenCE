/* Transposed matrix packet from January target. GPU payload writes only. */
#include <xmmintrin.h>
#include "push_internal.h"
#include "pusher_state.h"
#include "resource_internal.h"
#include "pixeljar.h"
#include "kernel_dispatcher.h"
#pragma code_seg("D3D")
namespace D3D
{
void WINAPI DumpMatrixTransposed(
    DWORD *push,
    DWORD method,
    const D3DMATRIX *matrix)
{
    push[0] = method;
    memcpy(push + 1, &matrix->m[0][0], sizeof(float));
    memcpy(push + 2, &matrix->m[1][0], sizeof(float));
    memcpy(push + 3, &matrix->m[2][0], sizeof(float));
    memcpy(push + 4, &matrix->m[3][0], sizeof(float));
    memcpy(push + 5, &matrix->m[0][1], sizeof(float));
    memcpy(push + 6, &matrix->m[1][1], sizeof(float));
    memcpy(push + 7, &matrix->m[2][1], sizeof(float));
    memcpy(push + 8, &matrix->m[3][1], sizeof(float));
    memcpy(push + 9, &matrix->m[0][2], sizeof(float));
    memcpy(push + 10, &matrix->m[1][2], sizeof(float));
    memcpy(push + 11, &matrix->m[2][2], sizeof(float));
    memcpy(push + 12, &matrix->m[3][2], sizeof(float));
    memcpy(push + 13, &matrix->m[0][3], sizeof(float));
    memcpy(push + 14, &matrix->m[1][3], sizeof(float));
    memcpy(push + 15, &matrix->m[2][3], sizeof(float));
    memcpy(push + 16, &matrix->m[3][3], sizeof(float));
    return;
}
}

namespace D3D
{
/* Deliberate, short polling delay. The target performs these counter accesses
 * and the reference BusyLoop documents avoidance of excessive HwGet polling. */
void WINAPI BusyLoop(
    void)
{
    volatile DWORD count = 400;
    do { } while (--count != 0);
    return;
}
void CDevice::InitializePushBuffer(
    void)
{
    m_PushSegmentSize = m_PushBufferSize / 16;
    m_Pusher.m_pPut = m_pPushBase;
    m_Pusher.m_pThreshold = m_pPushBase + m_PushSegmentSize / sizeof(DWORD) - 129;
    m_CpuTime = 5;
    *m_pGpuTime = 3;
    m_LastRunPushBufferTime = 3;
    return;
}
void CDevice::HwPut(
    DWORD *put)
{
    m_pControlDma->Put = (DWORD)put & 0x03ffffffUL;
    return;
}
DWORD *CDevice::GpuGet(
    void)
{
    DWORD *get = HwGet();
    if (get < m_pPushBase || get >= m_pPushLimit)
    {
        get = (DWORD *)((REG_RD32(m_NvBase, 0x324c) & ~1UL) | 0x80000000UL);
    }
    return get;
}
void WINAPI FlushWCCache(
    void)
{
    volatile DWORD *base = (volatile DWORD *)g_pDevice->m_Miniport.m_RegisterBase;
    /* One-instruction, target-evidenced cache fence. The intrinsic trial lets
     * VC7 move an MMIO read before SFENCE. No raw instruction bytes are used. */
    __asm sfence;
    base[0x100410 / sizeof(DWORD)] |= 0x10000;
    while ((base[0x100410 / sizeof(DWORD)] >> 16) & 1) { }
    return;
}
void CDevice::KickOff(
    void)
{
    if (!D3D__NullHardware)
    {
        DWORD *put = (m_StateFlags & 4) ? m_pPushBufferRecordSavedPut : m_Pusher.m_pPut;
        FlushWCCache();
        HwPut(put);
        m_pKickOff = put;
    }
    else
    {
        m_Pusher.m_pPut = HwGet();
        m_pKickOff = m_Pusher.m_pPut;
        *m_pGpuTime = m_CpuTime - 2;
        m_Miniport.m_VBlankFlipCount = m_PresentCount;
    }
    return;
}
DWORD WINAPI SetFence(
    DWORD flags)
{
    CDevice *device = g_pDevice;
    DWORD *push = device->StartPush();
    FenceEncoding *encoding = (FenceEncoding *)push;
    DWORD time = device->m_CpuTime;
    encoding->m_SemaphoreCommand = 0x41d70;
    encoding->m_Time = time;
    encoding->m_SetColorClearCommand1 = 0x41d90;
    encoding->m_SetColorClearArgument1 = 0;
    encoding->m_SetColorClearCommand2 = 0x41d90;
    encoding->m_SetColorClearArgument2 = 0;
    device->EndPush(push + sizeof(FenceEncoding) / sizeof(DWORD));
    DWORD index = (time >> 1) & 63;
    DWORD distance = device->m_PusherPutRunSize;
    device->m_PusherFence[index].Time = time;
    device->m_PusherFence[index].pEncoding = encoding;
    device->m_PusherFence[index].Distance = distance;
    if (flags & 1)
    {
        index = (device->m_PusherLastSegment + 1) & 15;
        device->m_PusherLastSegment = index;
        device->m_PusherSegment[index].Time = time;
        device->m_PusherSegment[index].pEncoding = encoding;
        device->m_PusherSegment[index].Distance = distance;
    }
    if (!(flags & 2)) device->KickOff();
    device->m_CpuTime += 2;
    return time;
}
Fence *WINAPI FindFence(
    DWORD time)
{
    CDevice *device = g_pDevice;
    DWORD age = device->Age(time);
    if (age <= 128)
    {
        return &device->m_PusherFence[(time >> 1) & 63];
    }
    DWORD index = device->m_PusherLastSegment;
    if (device->Age(device->m_PusherSegment[index].Time) > age)
    {
        return &device->m_PusherFence[(device->m_CpuTime >> 1) & 63];
    }
    do
    {
        index = (index + 1) & 15;
    } while (device->Age(device->m_PusherSegment[index].Time) > age);
    return &device->m_PusherSegment[index];
}
DWORD __fastcall ComputeGap(
    CDevice *device,
    Fence *fence)
{
    DWORD get = (DWORD)device->GpuGet();
    DWORD put = (DWORD)device->m_Pusher.m_pPut;
    if (get <= put) get += device->m_PusherLastSize;
    DWORD fenceAddress = (DWORD)fence->pEncoding;
    if (fenceAddress <= put) fenceAddress += device->m_PusherLastSize;
    LONG gap = (LONG)(fenceAddress - get);
    if (gap < 0) return 0;
    return gap + device->m_PusherPutRunSize - device->m_Miniport.m_PusherGetRunSize;
}
void WINAPI KickOffAndWaitForIdle(
    void)
{
    BlockOnTime(g_pDevice->m_CpuTime, FALSE);
    return;
}
void WINAPI BlockOnResource(
    D3DResource *resource)
{
    if (g_pDevice)
    {
        D3DResource *parent = PixelJar::GetParent(resource);
        if (parent)
        {
            if (IsResourceSetInDevice(resource))
            {
                KickOffAndWaitForIdle();
                return;
            }
            resource = parent;
        }
        DWORD time = resource->Lock;
        if (IsResourceSetInDevice(resource)) KickOffAndWaitForIdle();
        else BlockOnTime(time, FALSE);
        resource->Lock = 0;
    }
    return;
}
}
using namespace D3D;
BOOL WINAPI D3DDevice_IsFencePending(
    DWORD fence)
{
    return g_pDevice->IsTimePending(fence);
}
void WINAPI D3DDevice_InsertCallback(
    D3DCALLBACKTYPE type,
    D3DCALLBACK callback,
    DWORD context)
{
    CDevice *device = g_pDevice;
    DWORD *push = device->StartPush();
    Push2(push, 0x1d8c, (DWORD)callback, context);
    if (type == D3DCALLBACK_READ)
    {
        Push1(push + 3, 0x100, 0x314);
        device->EndPush(push + 5);
    }
    else
    {
        Push1(push + 3, 0x110, 0);
        Push1(push + 5, 0x100, 0x318);
        device->EndPush(push + 7);
    }
    return;
}
extern "C" DWORD *WINAPI XMETAL_StartPush(
    _XMETAL_PushBuffer *buffer)
{
    DWORD *put = buffer->m_pPut;
    if (put >= buffer->m_pThreshold)
    {
        put = ((CDevice *)buffer)->MakeSpace();
    }
    return put;
}
extern "C" DWORD *WINAPI XMETAL_StartPushCount(
    _XMETAL_PushBuffer *buffer,
    DWORD count)
{
    DWORD *put = buffer->m_pPut;
    if (put + count >= buffer->m_pThreshold + 128)
    {
        put = ((CDevice *)buffer)->MakeSpace();
    }
    return put;
}

namespace D3D
{
/* The January wait protocol patches a live GPU fence only while its gap is
 * at least 32 KiB. If the GPU wins the race, restore harmless commands and
 * spin on the fence time. No target code is executed by the build tools. */
void WINAPI BlockOnTime(
    DWORD time,
    BOOL makeSpace)
{
    CDevice *device = g_pDevice;
    if (time == 0 || !device->IsTimePending(time))
    {
        return;
    }
    if (time == device->m_CpuTime)
    {
        SetFence(0);
    }
    DWORD spinTime = time;
    Fence *fence = FindFence(time);
    DWORD gap = ComputeGap(device, fence);
    if (gap >= 32768)
    {
        FenceEncoding *encoding = fence->pEncoding;
        KeClearEvent(&device->m_Miniport.m_BusyBlockEvent);
        if (!makeSpace)
        {
            encoding->m_WaitForIdleCommand = 0x40110;
            encoding->m_WaitForIdleArgument = 0;
        }
        encoding->m_NoOperationCommand = 0x40100;
        encoding->m_FenceCommand = 0x310;
        FlushWCCache();
        DWORD newGap = ComputeGap(device, fence);
        if (newGap < 32768)
        {
            encoding->m_WaitForIdleCommand = 0x40100;
            encoding->m_WaitForIdleArgument = 0;
            encoding->m_NoOperationCommand = 0x40100;
            encoding->m_FenceCommand = 0;
            FlushWCCache();
            spinTime = fence->Time;
        }
        else
        {
            while (KeWaitForSingleObject(&device->m_Miniport.m_BusyBlockEvent,
                UserRequest, UserMode, FALSE, NULL) != 0)
            {
            }
            return;
        }
    }
    while (device->IsTimePending(spinTime))
    {
    }
    return;
}
}

namespace D3D
{
/* Addresses used to compare prospective segment limits are integer byte
 * offsets, not pointers formed outside the allocated circular push buffer. */
DWORD *CDevice::MakeSpace(
    void)
{
    DWORD *put = m_Pusher.m_pPut;
    if (m_StateFlags & 4)
    {
        DWORD *base = (DWORD *)m_pPushBufferRecordResource->Data;
        m_PushBufferRecordWrapSize += (DWORD)((BYTE *)put - (BYTE *)base);
        m_Pusher.m_pPut = (DWORD *)m_pPushBufferRecordResource->Data;
        return m_Pusher.m_pPut;
    }
    ULONG_PTR limit = (ULONG_PTR)put + m_PushSegmentSize;
    if (limit + m_PushSegmentSize / 2 >= (ULONG_PTR)m_pPushLimit)
    {
        if ((ULONG_PTR)put + m_PushSegmentSize / 2 <= (ULONG_PTR)m_pPushLimit)
        {
            limit = (ULONG_PTR)m_pPushLimit;
        }
        else
        {
            m_PusherLastSize = (DWORD)((BYTE *)put - (BYTE *)m_pPushBase);
            *m_Pusher.m_pPut = ((DWORD)m_pPushBase & 0x0fffffffUL) | 1;
            while (HwGet() == m_pPushBase)
            {
            }
            put = m_pPushBase;
            limit = (ULONG_PTR)put + m_PushSegmentSize;
            m_Pusher.m_pPut = put;
        }
    }
    DWORD gpuTime = GpuTime();
    DWORD *get = GpuGet();
    if (get > put && (ULONG_PTR)get <= limit)
    {
        DWORD index = m_PusherLastSegment;
        ULONG_PTR midpoint = (ULONG_PTR)put + m_PushBufferSize / 2;
        DWORD fenceTime = m_PusherSegment[index].Time;
        ULONG_PTR fence = (ULONG_PTR)m_PusherSegment[index].pEncoding;
        BOOL blocked = FALSE;
        while (Age(fenceTime) < Age(gpuTime))
        {
            if (fence < (ULONG_PTR)put) fence += m_PusherLastSize;
            if (fence < midpoint)
            {
                BlockOnTime(fenceTime, TRUE);
                blocked = TRUE;
                break;
            }
            index = (index - 1) & 15;
            if (index == m_PusherLastSegment) break;
            fence = (ULONG_PTR)m_PusherSegment[index].pEncoding;
            fenceTime = m_PusherSegment[index].Time;
        }
        if (!blocked)
        {
            do
            {
                BusyLoop();
                get = GpuGet();
            } while (get > put && (ULONG_PTR)get <= limit);
        }
        GpuGet();
    }
    m_Pusher.m_pThreshold = (DWORD *)(limit - 129 * sizeof(DWORD));
    if (m_StateFlags & 0x800)
    {
        m_StateFlags |= 0x1000;
        KickOff();
    }
    else
    {
        SetFence(1);
        KickOff();
    }
    return m_Pusher.m_pPut;
}
}
