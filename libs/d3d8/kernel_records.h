#ifndef D3D8_KERNEL_RECORDS_H
#define D3D8_KERNEL_RECORDS_H
/* January-PDB kernel layouts used by the miniport. No later SDK layout is
 * substituted. Interrupt/DPC internals are opaque: only size/alignment and
 * actual containing-member addresses are needed by this driver. */
#include <xtl.h>
#include <stddef.h>
struct _KINTERRUPT
{
    DWORD Opaque[28];
};
struct _KDPC
{
    DWORD Opaque[7];
};
struct _DISPATCHER_HEADER
{
    BYTE Type;
    BYTE Absolute;
    BYTE Size;
    BYTE Inserted;
    LONG SignalState;
    LIST_ENTRY WaitListHead;
};
struct _KEVENT
{
    _DISPATCHER_HEADER Header;
};
struct _HAL_SHUTDOWN_REGISTRATION
{
    void (WINAPI *NotificationRoutine)(
        _HAL_SHUTDOWN_REGISTRATION *registration);
    LONG Priority;
    LIST_ENTRY ListEntry;
};
struct _TIME_FIELDS
{
    SHORT Year;
    SHORT Month;
    SHORT Day;
    SHORT Hour;
    SHORT Minute;
    SHORT Second;
    SHORT Milliseconds;
    SHORT Weekday;
};
enum _EVENT_TYPE
{
    NotificationEvent,
    SynchronizationEvent
};
enum _KINTERRUPT_MODE
{
    LevelSensitive,
    Latched
};
typedef char kernel_interrupt_size[sizeof(_KINTERRUPT) == 112 ? 1 : -1];
typedef char kernel_dpc_size[sizeof(_KDPC) == 28 ? 1 : -1];
typedef char kernel_event_size[sizeof(_KEVENT) == 16 ? 1 : -1];
typedef char kernel_shutdown_size[sizeof(_HAL_SHUTDOWN_REGISTRATION) == 16 ? 1 : -1];
typedef char kernel_time_fields_size[sizeof(_TIME_FIELDS) == 16 ? 1 : -1];
typedef char kernel_waitlist_offset[offsetof(_DISPATCHER_HEADER, WaitListHead) == 8 ? 1 : -1];
typedef char kernel_event_header_offset[offsetof(_KEVENT, Header) == 0 ? 1 : -1];
typedef char kernel_signal_offset[offsetof(_DISPATCHER_HEADER, SignalState) == 4 ? 1 : -1];
typedef char kernel_shutdown_priority_offset[offsetof(_HAL_SHUTDOWN_REGISTRATION, Priority) == 4 ? 1 : -1];
typedef char kernel_shutdown_list_offset[offsetof(_HAL_SHUTDOWN_REGISTRATION, ListEntry) == 8 ? 1 : -1];
typedef char kernel_time_seconds_offset[offsetof(_TIME_FIELDS, Second) == 10 ? 1 : -1];
#endif
