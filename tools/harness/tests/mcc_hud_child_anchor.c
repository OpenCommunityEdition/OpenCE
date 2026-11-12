#include "harness.h"

struct hud_absolute_placement_definition { short corner, pad; };
static int mcc;
#define mcc_cache_tags_loaded() mcc
#include "under_test.inc"

int main(int argc, char **argv)
{
    struct hud_absolute_placement_definition parent = {0, 123}, storage;
    struct hud_absolute_placement_definition const *result;
    unsigned short element[4] = {0, 0, 0, 5};
    int family, child;
    (void)argc;
    (void)argv;
    /* Xbox -> MCC -> CE -> MCC -> Xbox: no retained child placement. */
    for (family = 0; family < 5; ++family) {
        mcc = family == 1 || family == 3;
        for (child = 0; child <= 10; ++child) {
            element[3] = (unsigned short)child;
            result = mcc_hud_child_placement(&parent, element, &storage);
            if (mcc && child >= 1 && child <= 9) {
                CHECK(result == &storage, "anchor selection failed");
                CHECK(result->corner == child - 1, "anchor selection failed");
            } else {
                CHECK(result == &parent, "anchor selection failed");
                CHECK(result->corner == 0, "anchor selection failed");
            }
            CHECK(result->pad == 123, "anchor selection failed");
            CHECK(parent.corner == 0 && parent.pad == 123, "anchor selection failed");
        }
        element[3] = 65535;
        CHECK(mcc_hud_child_placement(&parent, element, &storage) == &parent, "anchor selection failed");
    }
    return 0;
}
