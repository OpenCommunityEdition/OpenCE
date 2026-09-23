#ifndef HALO_D3D8_NOTIFICATION_INTERNAL_H
#define HALO_D3D8_NOTIFICATION_INTERNAL_H
#include "hardware_internal.h"
namespace D3D
{
/* Read the GPU-owned words in low/high order without aliasing a DWORD array
 * as an unrelated 64-bit object. This is not an atomic snapshot. */
__inline unsigned __int64 ReadNotificationTime(
    volatile NvNotification *notification)
{
    DWORD low = notification->timeStamp.nanoseconds[0];
    DWORD high = notification->timeStamp.nanoseconds[1];
    return low | ((unsigned __int64)high << 32);
}
}
#endif
