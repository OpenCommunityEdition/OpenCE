#ifndef D3D8_KERNEL_DEBUG_H
#define D3D8_KERNEL_DEBUG_H
#include <xtl.h>
extern "C" ULONG __cdecl DbgPrint(
    const char *format,
    ...);
#endif
