/* MCC scenario vehicles retain their own multiplayer placement masks. These
 * rules are active only during an MCC game and never modify a game variant. */
#include "cseries.h"
#include "game/game_engine.h"
#include "mcc_cache.h"
#include "mcc_objects.h"
#include <string.h>

static unsigned short mcc_vehicle_default_mask(void)
{
    if (!mcc_cache_tags_loaded() || !game_engine_running() ||
        game_engine_get_variant()->universal_variant.vehicle_set == 1)
        return 0;
    switch (game_engine_get_variant()->game_engine_index) {
    case game_engine_slayer: return 0x01;
    case game_engine_ctf: return 0x02;
    case game_engine_king: return 0x04;
    case game_engine_oddball: return 0x08;
    default: return 0;
    }
}

boolean mcc_vehicle_placement_rules(void)
{
    return mcc_vehicle_default_mask() != 0;
}

boolean mcc_vehicle_placement_allowed(struct scenario_object_datum const *placement)
{
    unsigned short mask = mcc_vehicle_default_mask(), flags;
    if (!mask) return TRUE;
    /* The independent MCC tag schema checks each 0x78-byte vehicle placement
     * before this entry point; its spawn flags are the word at 0x5A. */
    memcpy(&flags, (unsigned char const *)placement + 0x5A, sizeof(flags));
    return (flags & mask) != 0;
}
