/* The real platform macros and sound preferences select the intended pools. */
#include "harness.h"
#include "under_test.inc"
int main(void)
{
    short *v = default_sound_preferences.virtual_channel_counts;
    short *a = default_sound_preferences.actual_channel_counts;
    printf("{\"virtual\":[%d,%d,%d,%d],\"actual\":[%d,%d,%d,%d]}\n",
        v[0], v[1], v[2], v[3], a[0], a[1], a[2], a[3]);
    return 0;
}
