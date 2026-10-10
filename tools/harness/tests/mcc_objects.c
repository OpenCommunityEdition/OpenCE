#include "cseries.h"
#include "game/game_engine.h"
#include "mcc_objects.h"
#include <stdio.h>
#include <string.h>
#undef memcpy
#undef memset

static boolean loaded, running;
static struct game_variant variant;
boolean mcc_cache_tags_loaded(void) { return loaded; }
boolean game_engine_running(void) { return running; }
struct game_variant *game_engine_get_variant(void) { return &variant; }
void *csmemcpy(void *to, void const *from, unsigned long size) { return memcpy(to, from, size); }

#define CHECK(value) do { if (!(value)) { fprintf(stderr, "failed at %d\n", __LINE__); return 1; } } while (0)
int main(void) {
    unsigned char bytes[0x78] = {0};
    struct scenario_object_datum *placement = (void *)bytes;
    unsigned short flags;
    int engine, enabled;
    running = TRUE;
    variant.game_engine_index = game_engine_slayer;
    CHECK(!mcc_vehicle_placement_rules());
    CHECK(mcc_vehicle_placement_allowed(placement));
    loaded = TRUE;
    for (engine = game_engine_ctf; engine <= game_engine_king; ++engine) {
        static unsigned short masks[] = {0, 2, 1, 8, 4};
        variant.game_engine_index = engine;
        for (enabled = 0; enabled < 16; ++enabled) {
            flags = (unsigned short)enabled;
            memcpy(bytes + 0x5A, &flags, 2);
            CHECK(mcc_vehicle_placement_rules());
            CHECK(mcc_vehicle_placement_allowed(placement) == ((enabled & masks[engine]) != 0));
        }
    }
    variant.game_engine_index = game_engine_race;
    CHECK(!mcc_vehicle_placement_rules() && mcc_vehicle_placement_allowed(placement));
    variant.game_engine_index = game_engine_slayer;
    variant.universal_variant.vehicle_set = 1;
    CHECK(!mcc_vehicle_placement_rules());
    variant.universal_variant.vehicle_set = 0;
    running = FALSE;
    CHECK(!mcc_vehicle_placement_rules() && mcc_vehicle_placement_allowed(placement));
    return 0;
}
