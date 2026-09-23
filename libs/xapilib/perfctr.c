/* Xbox performance-counter wrappers. */

#include <excpt.h>
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>

__declspec(dllimport) LONGLONG __stdcall KeQueryPerformanceCounter(
    void);

__declspec(dllimport) LONGLONG __stdcall KeQueryPerformanceFrequency(
    void);

BOOL __stdcall QueryPerformanceCounter(
    LARGE_INTEGER *counter)
{
    counter->QuadPart = KeQueryPerformanceCounter();
    return TRUE;
}

BOOL __stdcall QueryPerformanceFrequency(
    LARGE_INTEGER *frequency)
{
    frequency->QuadPart = KeQueryPerformanceFrequency();
    return TRUE;
}
