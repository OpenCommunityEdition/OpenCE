#ifndef D3D8_PUSHER_STATE_H
#define D3D8_PUSHER_STATE_H
#include "hardware_internal.h"
namespace D3D
{
/* Original 24-byte fence packet and its patchable command fields. */
struct FenceEncoding
{
    DWORD m_SemaphoreCommand;
    DWORD m_Time;
    union { DWORD m_SetColorClearCommand1; DWORD m_WaitForIdleCommand; };
    union { DWORD m_SetColorClearArgument1; DWORD m_WaitForIdleArgument; };
    union { DWORD m_SetColorClearCommand2; DWORD m_NoOperationCommand; };
    union { DWORD m_SetColorClearArgument2; DWORD m_FenceCommand; };
};
typedef char fence_encoding_size[sizeof(FenceEncoding) == 24 ? 1 : -1];
void WINAPI BusyLoop(
    void);
void WINAPI FlushWCCache(
    void);
Fence *WINAPI FindFence(
    DWORD time);
DWORD __fastcall ComputeGap(
    CDevice *device,
    Fence *fence);
}
#endif
