/* MCC player input/profile extensions. Native action and profile layouts
 * remain unchanged; extra counts belong to the MCC inventory sidecar. */
#include "cseries.h"
#include "math/real_math.h"
#include "game/players.h"
#include "game/game.h"
#include "scenario/scenario.h"
#include "mcc_cache.h"
#include "units/units.h"
#include "scenario/scenario_definitions.h"
#include "mcc_grenades.h"
#include "mcc_player.h"

boolean mcc_player_choice_supported(short choice)
{
    return mcc_grenades_active() &&
        (choice == NONE || (choice >= 0 && choice < mcc_grenades_type_count()));
}

boolean mcc_player_action_valid(struct player_action const *action)
{
    if (!action || !mcc_player_choice_supported(action->desired_grenade_index)) return FALSE;
    if (!valid_real(action->desired_facing.yaw) || !valid_real(action->desired_facing.pitch) ||
        !valid_real(action->throttle.i) || !valid_real(action->throttle.j) || !valid_real(action->primary_trigger))
        return FALSE;
    if (action->desired_weapon_index != NONE &&
        (action->desired_weapon_index < 0 || action->desired_weapon_index >= MAXIMUM_WEAPONS_PER_UNIT)) return FALSE;
    return action->desired_zoom_level >= NONE;
}

void mcc_player_profile_grenades(long unit, struct scenario_starting_profile const *profile, boolean reset)
{
    short slot;
    unsigned char const *counts;
    if (!mcc_grenades_active() || !profile) return;
    counts = (unsigned char const *)profile + offsetof(struct scenario_starting_profile, grenade_counts);
    if (reset) mcc_grenades_clear(unit);
    /* MCC uses the two bytes following the original pair for slots 2/3. */
    for (slot = 2; slot < mcc_grenades_type_count(); ++slot)
        if (counts[slot])
            mcc_grenades_set(unit, slot, (short)MIN(127, mcc_grenades_get(unit, slot) + counts[slot]));
}

void mcc_player_postspawn_grenades(long unit, boolean disabled)
{
    if (mcc_grenades_active() && disabled) mcc_grenades_clear(unit);
}

/* MCC reset profiles replace the held weapon too. The Xbox inventory clear
 * retains it; Meincrap's selector would keep its readied dummy
 * weapon and repeatedly select new profiles. Delete through the normal object
 * service so first-person references and replicated deletions are handled. */
void mcc_player_add_equipment(long unit_index, short profile_index, boolean reset)
{
    struct unit_datum *unit;
    struct scenario *scenario;
    short slot;
    if (!mcc_cache_tags_loaded() || game_connection() == _game_connection_network_client ||
        unit_index == NONE || !(unit = unit_try_and_get(unit_index)) || unit->unit.player_index == NONE)
        return;
    scenario = global_scenario_get();
    if (!scenario || profile_index < 0 || profile_index >= scenario->starting_profiles.count) return;
    slot = unit->unit.current_weapon_index;
    if (reset && slot >= 0 && slot < MAXIMUM_WEAPONS_PER_UNIT &&
        unit->unit.weapon_object_indices[slot] != NONE)
        object_delete(unit->unit.weapon_object_indices[slot]);
    /* MCC scripts name each recipient themselves, including local_players;
     * do not expand one player's profile to the legacy co-op followers. */
    player_add_equipment(unit_index, profile_index, reset);
}
