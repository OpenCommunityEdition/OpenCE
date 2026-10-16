/* MCC object compatibility: colors, placement and boat controls.
 * The legacy object paths and game variants remain unchanged. */
#include "cseries.h"
#include "game/game_engine.h"
#include "mcc_cache.h"
#include "mcc_objects.h"
#include "physics/powered_mass_point_datum.h"
#include <string.h>

/* The three-point boat controller assigns only its propeller's friction
 * speed. MCC boats can enable water friction on the other two hydrofoils,
 * too. Their inactive controls must start at zero, not leftover stack data.
 * The caller subsequently supplies all three rotations and lift ratios. */
void mcc_boat_controls_initialize(struct powered_mass_point_datum *state)
{
    if (mcc_cache_tags_loaded()) memset(state, 0, 3 * sizeof(*state));
}

/* Signed object functions can extrapolate an RGB change color. The caller
 * darkens and clamps the final color; the native interpolation helper asserts
 * too early, before that clamp. Keep the same arithmetic for this MCC case. */
boolean mcc_object_color_extrapolate(union real_rgb_color *result, unsigned long flags,
    union real_rgb_color const *lower, union real_rgb_color const *upper, real fraction)
{
    real inverse;
    /* Bit zero selects the native HSV interpolation path. */
    if (!mcc_cache_tags_loaded() || (flags & 1u) ||
        (fraction >= 0.f && fraction <= 1.f)) return FALSE;
    inverse = 1.f - fraction;
    result->red = inverse * lower->red + fraction * upper->red;
    result->green = inverse * lower->green + fraction * upper->green;
    result->blue = inverse * lower->blue + fraction * upper->blue;
    return TRUE;
}

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
