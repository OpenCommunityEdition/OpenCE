/* Reconstructed from the supplied January 2002 D3D memory object. */
#include <xtl.h>
#pragma code_seg("D3D")
extern "C" void *WINAPI D3D_AllocNoncontiguousMemory(
    DWORD size)
{
    return LocalAlloc(LMEM_ZEROINIT, size);
}
extern "C" HLOCAL WINAPI D3D_FreeNoncontiguousMemory(
    void *memory)
{
    return LocalFree(memory);
}
void *WINAPI D3D_AllocContiguousMemory(
    DWORD size,
    DWORD alignment)
{
    return XPhysicalAlloc(size, MAXULONG_PTR, alignment, PAGE_READWRITE | PAGE_WRITECOMBINE);
}
void WINAPI D3D_FreeContiguousMemory(
    void *memory)
{
    if (memory)
    {
        XPhysicalFree(memory);
    }
    return;
}

/* Rectangle-copy algorithm adapted from RXDK-Libs, 2026 Team Resurgent,
 * GPL-3.0-or-later. January's routine accepts bytes-per-pixel, not a
 * hardware format code. See libs/d3d8/PROVENANCE.md. */
#include "push_internal.h"
#include "memory_internal.h"
namespace D3D
{
void WINAPI GPUCopyVideoRectangle(
    DWORD bytesPerPixel,
    DWORD sourceOffset,
    DWORD sourcePitch,
    DWORD destinationOffset,
    DWORD destinationPitch,
    DWORD count,
    RECT *rectangles,
    POINT *points)
{
    CDevice *device = g_pDevice;
    DWORD *push = device->StartPush();
    DWORD format;
    switch (bytesPerPixel)
    {
    case 1: format = 1; break;
    case 2: format = 4; break;
    case 4:
    default: format = 10; break;
    }
    PushCount(push, 0x8300, 4);
    push[1] = format;
    push[2] = (sourcePitch & 0xffff) | (destinationPitch << 16);
    push[3] = sourceOffset;
    push[4] = destinationOffset;
    device->EndPush(push + 5);
    while (count--)
    {
        push = device->StartPush();
        DWORD sourcePoint = ((DWORD)rectangles->left & 0xffff) | ((DWORD)rectangles->top << 16);
        DWORD destinationPoint = ((DWORD)points->x & 0xffff) | ((DWORD)points->y << 16);
        /* Disjoint 16-bit fields; XOR is the target's combining operation. */
        DWORD size = (((DWORD)rectangles->right - (DWORD)rectangles->left) & 0xffff)
            ^ (((DWORD)rectangles->bottom - (DWORD)rectangles->top) << 16);
        PushCount(push, 0x6300, 3);
        push[1] = sourcePoint;
        push[2] = destinationPoint;
        push[3] = size;
        device->EndPush(push + 4);
        rectangles++;
        points++;
    }
    return;
}
}

#include "pusher_state.h"
#include "notification_internal.h"
namespace D3D
{
void WINAPI GPUCopyMemory(
    DWORD sourceOffset,
    DWORD destinationOffset,
    DWORD size)
{
    CDevice *device = g_pDevice;
    volatile NvNotification *notifiers = device->m_pMemCopyNotifiers;
    unsigned __int64 notified = ReadNotificationTime(&notifiers[0]);
    if (size)
    {
        do
        {
            DWORD width, height, remaining;
            if (size <= 8192)
            {
                width = size;
                height = 1;
                remaining = 0;
            }
            else
            {
                width = 8192;
                height = size / 8192;
                if (height <= 2047)
                {
                    remaining = size % 8192;
                }
                else
                {
                    height = 2047;
                    remaining = size - width * height;
                }
            }
            DWORD *push = device->StartPush();
            if (!remaining)
            {
                Push1(push, 0x4104, 0);
                push += 2;
            }
            PushCount(push, 0x4184, 2);
            push[1] = 5;
            push[2] = 4;
            PushCount(push + 3, 0x430c, 8);
            push[4] = sourceOffset;
            push[5] = destinationOffset;
            push[6] = width;
            push[7] = width;
            push[8] = width;
            push[9] = height;
            push[10] = 0x101;
            push[11] = 0;
            device->EndPush(push + 12);
            sourceOffset += size - remaining;
            destinationOffset += size - remaining;
            size = remaining;
        } while (size);
        device->KickOff();
        while (notified == ReadNotificationTime(&notifiers[0]))
        {
            BusyLoop();
        }
        notified = ReadNotificationTime(&notifiers[0]);
        while (ReadNotificationTime(&notifiers[1]) < notified)
        {
            BusyLoop();
        }
    }
    return;
}
}
