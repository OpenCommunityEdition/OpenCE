#ifndef D3D8_KERNEL_MEMORY_H
#define D3D8_KERNEL_MEMORY_H
#include <xtl.h>
/* Original Xbox kernel import: signature and import linkage are evidenced
 * by the supplied PDB and January resource/memory relocations. */
extern "C" __declspec(dllimport) void WINAPI MmFreeContiguousMemory(
    void *baseAddress);
extern "C" __declspec(dllimport) void *WINAPI MmAllocateContiguousMemoryEx(
    DWORD numberOfBytes,
    DWORD lowestAddress,
    DWORD highestAddress,
    DWORD alignment,
    DWORD protection);
extern "C" __declspec(dllimport) DWORD WINAPI MmQueryAllocationSize(
    void *baseAddress);
extern "C" __declspec(dllimport) void WINAPI MmPersistContiguousMemory(
    void *baseAddress,
    DWORD bytes,
    BYTE persist);
#endif
