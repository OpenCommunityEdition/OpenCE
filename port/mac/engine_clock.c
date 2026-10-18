/* Native monotonic performance counter in nanoseconds, with the same frequency
   for every caller. Split the conversion to avoid overflowing ticks*numer. */
#include <mach/mach_time.h>
#include <time.h>
#include <errno.h>
#include <sched.h>
#include "engine_xdk_prefix.h"
int QueryPerformanceCounter(union _LARGE_INTEGER *out)
{
    mach_timebase_info_data_t scale;
    uint64_t ticks,seconds,remainder;
    if(!out || mach_timebase_info(&scale)!=KERN_SUCCESS || !scale.denom)return 0;
    ticks=mach_absolute_time();
    seconds=ticks/scale.denom;
    remainder=ticks%scale.denom;
    out->QuadPart=(long long)(seconds*scale.numer+remainder*scale.numer/scale.denom);
    return 1;
}
int QueryPerformanceFrequency(union _LARGE_INTEGER *out)
{
    if(!out)return 0;
    out->QuadPart=1000000000LL;
    return 1;
}

/* Match the engine's unsigned 32-bit millisecond clock, including wraparound. */
unsigned long system_milliseconds(void)
{
    union _LARGE_INTEGER now;
    if(!QueryPerformanceCounter(&now))abort();
    return (unsigned long)((uint64_t)now.QuadPart/1000000ULL);
}

/* The source API is Unix wall-clock seconds, used to seed local randomness. */
unsigned long system_seconds(void)
{
    return (unsigned long)time(NULL);
}

unsigned long GetTickCount(void)
{
    return system_milliseconds();
}
void Sleep(unsigned long milliseconds)
{
    struct timespec delay,remaining;
    if(!milliseconds){sched_yield();return;}
    delay.tv_sec=milliseconds/1000;
    delay.tv_nsec=(milliseconds%1000)*1000000;
    for(;;)
    {
        if(nanosleep(&delay,&remaining)==0)
        {
            /* INFINITE is a wait without a deadline, not 49.7 days. */
            if(milliseconds==0xffffffffUL)continue;
            return;
        }
        if(errno!=EINTR)abort();
        delay=remaining;
    }
}
