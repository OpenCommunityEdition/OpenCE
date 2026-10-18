/* Same scheduler-yield semantics as the shared Xbox kernel boundary. */
#include <sched.h>
int SwitchToThread(void)
{
    sched_yield();
    return 1;
}
