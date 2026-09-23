#ifndef D3D8_KERNEL_INTERRUPT_H
#define D3D8_KERNEL_INTERRUPT_H
#include "device_layout.h"
/* Genuine Xbox kernel interrupt/shutdown interfaces. The target PDB owns record sizes
 * and containing-member offsets; event/shutdown fields are decoded, while the
 * passed-through DPC/interrupt record internals remain opaque. */
extern "C" __declspec(dllimport) void WINAPI HalRegisterShutdownNotification(
    _HAL_SHUTDOWN_REGISTRATION *registration,
    BYTE registerNotification);
extern "C" __declspec(dllimport) BYTE WINAPI KeDisconnectInterrupt(
    _KINTERRUPT *interrupt);
extern "C" __declspec(dllimport) void WINAPI KeInitializeDpc(
    _KDPC *dpc,
    void (WINAPI *routine)(
        _KDPC *dpc,
        void *context,
        void *argument1,
        void *argument2),
    void *context);
extern "C" __declspec(dllimport) BYTE WINAPI KeInsertQueueDpc(
    _KDPC *dpc,
    void *argument1,
    void *argument2);
extern "C" __declspec(dllimport) DWORD WINAPI HalGetInterruptVector(
    DWORD busInterruptLevel,
    BYTE *irql);
extern "C" __declspec(dllimport) void WINAPI KeInitializeInterrupt(
    _KINTERRUPT *interrupt,
    BYTE (WINAPI *routine)(
        _KINTERRUPT *interrupt,
        void *context),
    void *context,
    DWORD vector,
    BYTE irql,
    _KINTERRUPT_MODE mode,
    BYTE shareVector);
extern "C" __declspec(dllimport) BYTE WINAPI KeConnectInterrupt(
    _KINTERRUPT *interrupt);
extern "C" __declspec(dllimport) LONG WINAPI KeSetEvent(
    _KEVENT *event,
    LONG increment,
    BYTE wait);
/* The period NT header exposes this operation as a macro, not a callable
 * routine. All call sites here pass a stable event address and constants.
 * Fields/size are authenticated from the January PDB. */
#define KeInitializeEvent(event, type, state) do { \
    (event)->Header.Type = (BYTE)(type); \
    (event)->Header.Size = sizeof(_KEVENT) / sizeof(DWORD); \
    (event)->Header.SignalState = (state); \
    (event)->Header.WaitListHead.Flink = (event)->Header.WaitListHead.Blink = \
        &(event)->Header.WaitListHead; \
} while (0)
#endif
