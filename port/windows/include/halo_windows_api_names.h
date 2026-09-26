/*
HALO_WINDOWS_API_NAMES.H

The platform layer implements these Xbox SDK functions under the same names
as the Windows functions they correspond to (CreateFileA, ReadFile, Sleep,
...). In a Windows program a call from the Windows-facing code
(port/windows/src/win32_*.c) would then reach the Xbox implementation, so
the game and the Xbox-facing platform code see them under other names.

The list is every function defined in port/linux/src that the Windows SDK
also declares.
*/

#ifndef __HALO_WINDOWS_API_NAMES_H
#define __HALO_WINDOWS_API_NAMES_H

#define CloseHandle halo_xbox_CloseHandle
#define CompareFileTime halo_xbox_CompareFileTime
#define CopyFileA halo_xbox_CopyFileA
#define CreateDirectoryA halo_xbox_CreateDirectoryA
#define CreateEventA halo_xbox_CreateEventA
#define CreateFileA halo_xbox_CreateFileA
#define CreateMutexA halo_xbox_CreateMutexA
#define CreateThread halo_xbox_CreateThread
#define DeleteFileA halo_xbox_DeleteFileA
#define FindFirstFileA halo_xbox_FindFirstFileA
#define FindNextFileA halo_xbox_FindNextFileA
#define GetDiskFreeSpaceExA halo_xbox_GetDiskFreeSpaceExA
#define GetExitCodeThread halo_xbox_GetExitCodeThread
#define GetFileAttributesA halo_xbox_GetFileAttributesA
#define GetFileAttributesExA halo_xbox_GetFileAttributesExA
#define GetFileSize halo_xbox_GetFileSize
#define GetFileTime halo_xbox_GetFileTime
#define GetLastError halo_xbox_GetLastError
#define GetSystemTime halo_xbox_GetSystemTime
#define GetTickCount halo_xbox_GetTickCount
#define GlobalAlloc halo_xbox_GlobalAlloc
#define GlobalMemoryStatus halo_xbox_GlobalMemoryStatus
#define GlobalReAlloc halo_xbox_GlobalReAlloc
#define LocalFree halo_xbox_LocalFree
#define LocalSize halo_xbox_LocalSize
#define MoveFileA halo_xbox_MoveFileA
#define OutputDebugStringA halo_xbox_OutputDebugStringA
#define QueryPerformanceCounter halo_xbox_QueryPerformanceCounter
#define QueryPerformanceFrequency halo_xbox_QueryPerformanceFrequency
#define ReadFile halo_xbox_ReadFile
#define ReadFileEx halo_xbox_ReadFileEx
#define ReleaseMutex halo_xbox_ReleaseMutex
#define RemoveDirectoryA halo_xbox_RemoveDirectoryA
#define ResetEvent halo_xbox_ResetEvent
#define ResumeThread halo_xbox_ResumeThread
#define SetEndOfFile halo_xbox_SetEndOfFile
#define SetEvent halo_xbox_SetEvent
#define SetFileAttributesA halo_xbox_SetFileAttributesA
#define SetFilePointer halo_xbox_SetFilePointer
#define SetFileTime halo_xbox_SetFileTime
#define SetLastError halo_xbox_SetLastError
#define SetThreadPriority halo_xbox_SetThreadPriority
#define Sleep halo_xbox_Sleep
#define SleepEx halo_xbox_SleepEx
#define SwitchToThread halo_xbox_SwitchToThread
#define SystemTimeToFileTime halo_xbox_SystemTimeToFileTime
#define VirtualProtect halo_xbox_VirtualProtect
#define WaitForSingleObject halo_xbox_WaitForSingleObject
#define WaitForSingleObjectEx halo_xbox_WaitForSingleObjectEx
#define WriteFile halo_xbox_WriteFile
#define WriteFileEx halo_xbox_WriteFileEx
#define WSACleanup halo_xbox_WSACleanup
#define __WSAFDIsSet halo_xbox___WSAFDIsSet
#define WSAGetLastError halo_xbox_WSAGetLastError
#define WSASetLastError halo_xbox_WSASetLastError
#define WSAStartup halo_xbox_WSAStartup

#endif
