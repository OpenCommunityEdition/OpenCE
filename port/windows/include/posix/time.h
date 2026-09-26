/*
TIME.H

Windows <time.h> plus the POSIX clocks and nanosleep
(port/windows/src/win32_posix.c).
*/

#ifndef __HALO_WINDOWS_TIME_H
#define __HALO_WINDOWS_TIME_H

#include_next <time.h>

typedef int clockid_t;

#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 1

#define TIMER_ABSTIME 1

int clock_gettime(clockid_t clock, struct timespec *time);
int nanosleep(const struct timespec *duration, struct timespec *remaining);
/* CLOCK_MONOTONIC with TIMER_ABSTIME, or relative */
int clock_nanosleep(clockid_t clock, int flags, const struct timespec *time, struct timespec *remaining);

#endif
