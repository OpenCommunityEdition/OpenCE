/* Xbox handle wrappers over the NT kernel object primitives. */

#include <excpt.h>
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>

__declspec(dllimport) LONG __stdcall NtClose(
    HANDLE handle);

__declspec(dllimport) LONG __stdcall NtDuplicateObject(
    HANDLE source_handle,
    LPHANDLE target_handle,
    DWORD options);

DWORD __stdcall XapiSetLastNTError(
    LONG status);

BOOL __stdcall CloseHandle(
    HANDLE object)
{
    LONG status = NtClose(object);
    if (status >= 0)
    {
        return TRUE;
    }
    XapiSetLastNTError(status);
    return FALSE;
}

BOOL __stdcall DuplicateHandle(
    HANDLE source_process,
    HANDLE source_handle,
    HANDLE target_process,
    LPHANDLE target_handle,
    DWORD desired_access,
    BOOL inherit_handle,
    DWORD options)
{
    LONG status = NtDuplicateObject(source_handle, target_handle, options);
    if (status >= 0)
    {
        return TRUE;
    }
    XapiSetLastNTError(status);
    return FALSE;
}
