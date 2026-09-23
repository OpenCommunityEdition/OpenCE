#ifndef D3D8_KERNEL_CLOCK_H
#define D3D8_KERNEL_CLOCK_H
#include "kernel_records.h"
extern "C" __declspec(dllimport) void WINAPI KeQuerySystemTime(
    LARGE_INTEGER *time);
extern "C" __declspec(dllimport) void WINAPI RtlTimeToTimeFields(
    LARGE_INTEGER *time,
    _TIME_FIELDS *fields);
#endif
