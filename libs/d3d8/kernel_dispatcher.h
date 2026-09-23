#ifndef D3D8_KERNEL_DISPATCHER_H
#define D3D8_KERNEL_DISPATCHER_H
#include "kernel_records.h"
/* The wait/mode enumerations are taken from the January PDB, not a later SDK. */
enum _KWAIT_REASON
{
    Executive = 0,
    FreePage = 1,
    PageIn = 2,
    PoolAllocation = 3,
    DelayExecution = 4,
    Suspended = 5,
    UserRequest = 6,
    WrExecutive = 7,
    WrFreePage = 8,
    WrPageIn = 9,
    WrPoolAllocation = 10,
    WrDelayExecution = 11,
    WrSuspended = 12,
    WrUserRequest = 13,
    WrEventPair = 14,
    WrQueue = 15,
    WrLpcReceive = 16,
    WrLpcReply = 17,
    WrVirtualMemory = 18,
    WrPageOut = 19,
    WrRendezvous = 20,
    WrFsCacheIn = 21,
    WrFsCacheOut = 22,
    Spare4 = 23,
    Spare5 = 24,
    Spare6 = 25,
    WrKernel = 26,
    MaximumWaitReason = 27
};
enum _MODE
{
    KernelMode = 0,
    UserMode = 1,
    MaximumMode = 2
};
typedef char WaitReasonSize[sizeof(_KWAIT_REASON) == 4 ? 1 : -1];
typedef char ProcessorModeEnumSize[sizeof(_MODE) == 4 ? 1 : -1];
/* Xbox NT KPROCESSOR_MODE is a signed byte; wait reason is a 32-bit enum.
 * The kernel call owns event synchronization; clearing only resets its signal. */
extern "C" __declspec(dllimport) LONG WINAPI KeWaitForSingleObject(
    void *object,
    _KWAIT_REASON reason,
    CHAR mode,
    BYTE alertable,
    LARGE_INTEGER *timeout);
#define KeClearEvent(event) ((event)->Header.SignalState = 0)
#endif
