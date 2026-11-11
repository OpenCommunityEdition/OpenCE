/* MCC's weapon-authored camera uses the engine's existing following camera.
 * Inventory replication supplies the equipped weapon on both peers. */
#include "cseries.h"
#include "units/units.h"
#include "items/weapons.h"
#include "items/weapon_definitions.h"
#include "mcc_cache.h"
#include "mcc_camera.h"

boolean mcc_camera_weapon_following(long unit_index)
{
    struct unit_datum *unit;
    struct weapon_datum *weapon;
    struct weapon_definition *definition;
    short slot;
    long weapon_index;

    if (!mcc_cache_tags_loaded() || unit_index == NONE) return FALSE;
    unit = unit_try_and_get(unit_index);
    if (!unit || unit->object.parent_object_index != NONE ||
        TEST_FLAG(unit->object.damage_flags, _object_dead_bit) ||
        unit->unit.current_zoom_level != NONE) return FALSE;
    slot = unit->unit.current_weapon_index;
    if (slot < 0 || slot >= MAXIMUM_WEAPONS_PER_UNIT) return FALSE;
    weapon_index = unit->unit.weapon_object_indices[slot];
    if (weapon_index == NONE) return FALSE;
    weapon = weapon_try_and_get(weapon_index);
    if (!weapon) return FALSE;
    definition = weapon_definition_get(weapon->definition_index);
    /* H1A weapon flags bit 17; independently documented by c20's weapon
     * format reference and present on Spasm Playground's Digsite weapons. */
    return (definition->weapon.flags & 0x20000UL) != 0;
}
