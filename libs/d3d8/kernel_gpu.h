#ifndef D3D8_KERNEL_GPU_H
#define D3D8_KERNEL_GPU_H
#include "device_layout.h"
/* Xbox kernel imports; 32-bit stdcall contracts used by the January driver. */
extern "C" __declspec(dllimport) void *WINAPI MmClaimGpuInstanceMemory(
    DWORD bytes,
    DWORD *paddingBytes);
extern "C" __declspec(dllimport) void WINAPI MmSetAddressProtect(
    void *address,
    DWORD bytes,
    DWORD protection);
extern "C" __declspec(dllimport) void WINAPI KeStallExecutionProcessor(
    DWORD microseconds);
#endif
