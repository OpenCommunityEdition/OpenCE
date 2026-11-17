/* The real platform macros and sound preferences select the intended pools. */
#include "harness.h"
#include "under_test.inc"
int main(void)
{
    int type, actual = 0, virtual = 0;
    for (type = 0; type < 4; type++)
    {
        CHECK(default_sound_preferences.actual_channel_counts[type] == expected_actual[type], "actual pool %d", type);
        CHECK(default_sound_preferences.virtual_channel_counts[type] == expected_virtual[type], "virtual pool %d", type);
        actual += expected_actual[type];
        virtual += expected_virtual[type];
    }
    CHECK(actual <= 256 && virtual <= 256, "channel arrays overflow");
    return 0;
}
