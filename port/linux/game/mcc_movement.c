/* MCC weapon penalties operate on local forward/sideways input, not on
 * world velocity. Knockback, gravity and vehicle physics stay unchanged. */
#include "mcc_movement.h"
#include "mcc_cache.h"
#include "units/units.h"
#include "items/weapons.h"
#include "items/weapon_definitions.h"

void mcc_movement_apply(long unit_index, real_vector3d *desired)
{
    struct unit_datum *unit;
    struct weapon_datum *weapon;
    struct _weapon_definition const *definition;
    long weapon_index;
    boolean zoomed;
    real forward, sideways;
    if (!mcc_cache_tags_loaded() || !desired || !(unit=unit_try_and_get(unit_index)) ||
        unit->unit.player_index==NONE || unit->object.parent_object_index!=NONE ||
        unit->unit.current_weapon_index<0 || unit->unit.current_weapon_index>=MAXIMUM_WEAPONS_PER_UNIT) return;
    weapon_index=unit->unit.weapon_object_indices[unit->unit.current_weapon_index];
    if (!(weapon=weapon_try_and_get(weapon_index))) return;
    definition=&weapon_definition_get(weapon->definition_index)->weapon;
    zoomed=unit->unit.current_zoom_level!=NONE;
    switch (definition->movement_penalty_mode) {
    case 0: break;
    case 1: if (!zoomed) return;break;
    /* Preserve MCC's documented mode-2 quirk: an unzoomed reload removes
     * the penalty, while an unzoomed idle weapon still applies it. */
    case 2:
        if (!zoomed && (weapon->weapon.magazines[0].state==_magazine_reloading ||
            weapon->weapon.magazines[1].state==_magazine_reloading)) return;
        break;
    default: return;
    }
    forward=definition->forward_movement_penalty;
    sideways=definition->sideways_movement_penalty;
    /* Fractions up to two reverse input without exceeding stock speed.
     * Malformed/nonfinite values must never expand the movement envelope. */
    if (forward>=0.0f && forward<=2.0f) desired->i*=1.0f-forward;
    if (sideways>=0.0f && sideways<=2.0f) desired->j*=1.0f-sideways;
}
