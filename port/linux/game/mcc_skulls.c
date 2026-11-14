/* MCC skull status for MCC maps. Only the host runs a map's scripts, so its
 * skull status globals are the truth; a client knows the skulls the campaign
 * snapshot carries (the replicated ones) and treats the rest as off, since
 * their effects are the host's to apply. */
#include "cseries.h"
#include "game/game.h"
#include "cutscene/cinematics.h"
#include "game/game_allegiance.h"
#include "game/players.h"
#include "items/weapons.h"
#include "items/weapon_definitions.h"
#include "objects/objects.h"
#include "units/units.h"
#include "items/equipment_definitions.h"
#include "items/projectiles.h"
#include "math/real_math.h"
#include "tag_files/tag_groups.h"
#include "game/cheats.h"
#include "scenario/scenario.h"
#include "game/game_globals.h"
#include "mcc_grenades.h"
#include "mcc_cache.h"
#include "mcc_skulls.h"

/* The status globals' MCC names, by enum mcc_skull. */
static char const *const mcc_skull_names[NUMBER_OF_MCC_SKULLS] = {
    "debug_ice_cream_flavor_status_fog",
    "debug_ice_cream_flavor_status_blind",
    "debug_ice_cream_flavor_status_eye_patch",
    "debug_ice_cream_flavor_status_recession",
    "debug_ice_cream_flavor_status_boom",
    "debug_ice_cream_flavor_status_i_would_have_been_your_daddy",
    "debug_ice_cream_flavor_status_sputnik",
    "debug_ice_cream_flavor_status_feather",
    "debug_ice_cream_flavor_status_foreign",
    "debug_ice_cream_flavor_status_malfunction",
    "debug_ice_cream_flavor_status_iron",
    "debug_ice_cream_flavor_status_famine",
    "debug_ice_cream_flavor_status_mythic",
    "debug_ice_cream_flavor_status_assassins",
    "debug_ice_cream_flavor_status_black_eye",
    "debug_ice_cream_flavor_status_catch",
    "debug_ice_cream_flavor_status_anger",
    "debug_ice_cream_flavor_status_thunderstorm",
    "debug_ice_cream_flavor_status_thats_just_wrong",
    "debug_ice_cream_flavor_status_ghost",
    "debug_ice_cream_flavor_status_pinata",
    "debug_ice_cream_flavor_status_grunt_funeral",
    "debug_ice_cream_flavor_status_grunt_birthday_party",
    "debug_ice_cream_flavor_status_streaking",
    "debug_ice_cream_flavor_status_styx",
    "debug_ice_cream_flavor_status_bonded_pair",
    "debug_ice_cream_flavor_status_masterblaster",
    "debug_ice_cream_flavor_force_coop"
};
static short mcc_skull_globals[NUMBER_OF_MCC_SKULLS];
static unsigned long mcc_skulls_received;
/* Malfunction's element (NONE: none), one for all of this machine's players. */
static short mcc_malfunction_element = NONE;
static boolean mcc_malfunction_on;

void mcc_skulls_reset(void)
{
    short skull;

    for (skull = 0; skull < NUMBER_OF_MCC_SKULLS; skull++)
        mcc_skull_globals[skull] = NONE;
    mcc_skulls_received = 0;
    mcc_malfunction_element = NONE;
    mcc_malfunction_on = FALSE;
}

short mcc_skull_find(char const *name)
{
    short skull;

    for (skull = 0; name && skull < NUMBER_OF_MCC_SKULLS; skull++)
    {
        if (!csstrcasecmp(name, mcc_skull_names[skull]))
            return skull;
    }
    return NONE;
}

void mcc_skulls_bind(short skull, short scenario_global)
{
    if (skull >= 0 && skull < NUMBER_OF_MCC_SKULLS)
        mcc_skull_globals[skull] = scenario_global;
}

boolean mcc_skull_active(short skull)
{
    if (skull < 0 || skull >= NUMBER_OF_MCC_SKULLS || !mcc_cache_tags_loaded())
        return FALSE;
    if (game_connection() == _game_connection_network_client)
        return skull < NUMBER_OF_MCC_REPLICATED_SKULLS && (mcc_skulls_received & (1u << skull)) != 0;
    return mcc_skull_globals[skull] != NONE &&
        (mcc_script_scenario_global_value(mcc_skull_globals[skull]) & 0xFF) != 0;
}

unsigned long mcc_skulls_replicated(void)
{
    unsigned long mask = 0;
    short skull;

    for (skull = 0; skull < NUMBER_OF_MCC_REPLICATED_SKULLS; skull++)
    {
        if (mcc_skull_active(skull))
            mask |= 1u << skull;
    }
    return mask;
}

void mcc_skulls_receive(unsigned long mask)
{
    mcc_skulls_received = mask & MCC_SKULLS_REPLICATED_MASK;
}

short mcc_skulls_ammunition_scale(boolean player_weapon)
{
    return player_weapon && mcc_skull_active(_mcc_skull_recession) ? 2 : 1;
}

/* A battery weapon, by weapon_set_current_amount's rule: no magazine, or a
 * trigger whose rounds age it. */
static boolean mcc_skulls_weapon_uses_age(struct weapon_definition const *definition)
{
    long trigger, count = definition->weapon.triggers.count;

    if (definition->weapon.magazines.count == 0)
        return TRUE;
    if (count > (long)NUMBEROF(((struct weapon_datum *)0)->weapon.triggers))
        count = (long)NUMBEROF(((struct weapon_datum *)0)->weapon.triggers);
    for (trigger = 0; trigger < count; trigger++)
    {
        if (((struct weapon_trigger_definition const *)definition->weapon.triggers.address)[trigger].age_generated_per_round > 0.0f)
            return TRUE;
    }
    return FALSE;
}

/* As MCC does it: a magazine's rounds by integer division, and a battery's
 * use halfway to empty. The host's alone: a client's skull is off. */
void mcc_skulls_famine(long unit_index)
{
    struct unit_datum *unit;
    short slot, magazine;

    if (!mcc_skull_active(_mcc_skull_famine) || !(unit = unit_try_and_get(unit_index)))
        return;
    for (slot = 0; slot < MAXIMUM_WEAPONS_PER_UNIT; slot++)
    {
        long weapon_index = unit->unit.weapon_object_indices[slot];
        struct weapon_datum *weapon = weapon_index != NONE ? weapon_try_and_get(weapon_index) : NULL;
        struct weapon_definition const *definition;

        if (!weapon)
            continue;
        definition = weapon_definition_get(weapon->definition_index);
        for (magazine = 0; magazine < (short)NUMBEROF(weapon->weapon.magazines) &&
            magazine < definition->weapon.magazines.count; magazine++)
        {
            weapon->weapon.magazines[magazine].rounds_total /= 2;
            weapon->weapon.magazines[magazine].rounds_loaded /= 2;
        }
        if (mcc_skulls_weapon_uses_age(definition))
            weapon->weapon.age = PIN((weapon->weapon.age + 1.0f) * 0.5f, 0.0f, 1.0f);
    }
}

static boolean mcc_skulls_enemy(long object_index, long object_mask)
{
    struct object_datum *object = object_try_and_get(object_index);

    return object && TEST_FLAG(object_mask, object->object.type) &&
        game_team_is_enemy(_game_team_player, object->object.owner_team_index);
}

/* As MCC does it when a biped is created: Mythic doubles a Covenant or
 * Flood biped's body vitality and raises a Sentinel's shields to its body;
 * Styx triples a Hunter's body vitality. */
void mcc_skulls_vitality(long object_index, real *maximum_body_vitality, real *maximum_shield_vitality)
{
    struct object_datum *object = object_try_and_get(object_index);
    short team;

    if (!object || object->object.type != _object_type_biped)
        return;
    team = object->object.owner_team_index;
    if (mcc_skull_active(_mcc_skull_mythic))
    {
        if (team == _game_team_covenant || team == _game_team_flood)
            *maximum_body_vitality += *maximum_body_vitality;
        else if (team == _game_team_sentinel && *maximum_body_vitality > *maximum_shield_vitality)
            *maximum_shield_vitality = *maximum_body_vitality;
    }
    if (mcc_skull_active(_mcc_skull_styx) &&
        !csstrcasecmp(tag_get_name(object->definition_index), "objects\\characters\\hunter\\hunter"))
        *maximum_body_vitality *= 3.0f;
}

/* As MCC does it, before the blow lands: one plasma grenade (the globals'
 * second type), even from a biped that carries none. The host deals melee
 * damage, a client player's included. */
void mcc_skulls_pinata(long player_index, short attacker_team, long victim_index, boolean melee)
{
    struct object_datum *victim;

    if (!melee || player_index == NONE || !mcc_skull_active(_mcc_skull_pinata))
        return;
    victim = object_try_and_get(victim_index);
    if (victim && victim->object.type == _object_type_biped &&
        !TEST_FLAG(victim->object.damage_flags, _object_dead_bit) &&
        game_team_is_enemy(attacker_team, victim->object.owner_team_index))
        unit_port_drop_one_grenade(victim_index, 1);
}

real mcc_skulls_skip_fraction(real skip_fraction)
{
    return mcc_skull_active(_mcc_skull_iwhbyd) && skip_fraction >= 0.5f ? 1.0f - skip_fraction : skip_fraction;
}

/* Feather triples the push of damage whose owner holds a weapon; Sputnik
 * triples every push. */
real mcc_skulls_impulse_scale(long owner_object_index)
{
    real scale = 1.0f;
    struct unit_datum *owner;

    if (mcc_skull_active(_mcc_skull_feather) && owner_object_index != NONE &&
        (owner = unit_try_and_get(owner_object_index)) != NULL &&
        unit_inventory_get_weapon(owner_object_index, owner->unit.current_weapon_index) != NONE)
        scale = 3.0f;
    if (mcc_skull_active(_mcc_skull_sputnik))
        scale *= 3.0f;
    return scale;
}

real mcc_skulls_explosion_radius_scale(void)
{
    return mcc_skull_active(_mcc_skull_boom) ? 2.0f : 1.0f;
}

boolean mcc_skulls_shield_recharge_blocked(long object_index)
{
    struct unit_datum *unit;

    if (!mcc_skull_active(_mcc_skull_black_eye))
        return FALSE;
    unit = unit_try_and_get(object_index);
    return unit && unit->unit.player_index != NONE;
}

/* MCC fills the shields when a player's melee damages an enemy, without
 * lowering an overshield. The host deals the damage (clients report it). */
void mcc_skulls_melee_struck(long player_index, long victim_object_index, boolean melee, boolean damaged)
{
    struct player_datum *player;
    struct object_datum *object;

    if (!melee || !damaged || player_index == NONE || !mcc_skull_active(_mcc_skull_black_eye) ||
        !mcc_skulls_enemy(victim_object_index, _object_mask_unit))
        return;
    player = player_try_and_get(player_index);
    object = player && player->unit_index != NONE ? object_try_and_get(player->unit_index) : NULL;
    if (object && object->object.shield_vitality < 1.0f && object->object.maximum_shield_vitality > 0.0f)
        object->object.shield_vitality = 1.0f;
}

boolean mcc_skulls_iron(void)
{
    return mcc_skull_active(_mcc_skull_iron);
}

boolean mcc_skulls_iron_restarts(void)
{
    struct data_iterator iterator;
    long count = 0;

    if (!mcc_skulls_iron())
        return FALSE;
    data_iterator_new(&iterator, player_data);
    while (data_iterator_next(&iterator))
        count++;
    return count == 1;
}

real mcc_skulls_rate_of_fire_scale(short team_index)
{
    return mcc_skull_active(_mcc_skull_anger) && game_team_is_enemy(_game_team_player, team_index) ? 2.0f : 1.0f;
}

boolean mcc_skulls_catch(void)
{
    return mcc_skull_active(_mcc_skull_catch);
}

static boolean mcc_skulls_player_unit(long unit_index)
{
    struct object_datum *unit = object_try_and_get(unit_index);

    return unit && unit->object.owner_player_index != NONE;
}

/* MCC tells Covenant weapons by their type: the needler, plasma pistol and
 * plasma rifle (types 2-4). Weapon cycling, swapping and pickup all ask. */
boolean mcc_skulls_foreign_refuses_weapon(long unit_index, long weapon_index)
{
    struct weapon_datum *weapon;
    short type;

    if (!mcc_skull_active(_mcc_skull_foreign) || !mcc_skulls_player_unit(unit_index) ||
        !(weapon = weapon_try_and_get(weapon_index)))
        return FALSE;
    type = weapon_definition_get(weapon->definition_index)->weapon.weapon_type;
    return type == _weapon_type_needler || type == _weapon_type_plasma_pistol || type == _weapon_type_plasma_rifle;
}

boolean mcc_skulls_foreign_refuses_grenade(long unit_index, long equipment_index)
{
    struct object_datum *equipment;

    if (!mcc_skull_active(_mcc_skull_foreign) || !mcc_skulls_player_unit(unit_index) ||
        !(equipment = object_try_and_get(equipment_index)))
        return FALSE;
    return equipment_definition_get(equipment->definition_index)->equipment.grenade_type == 1;
}

/* MCC's test: the tag's name, exactly. */
boolean mcc_skulls_funeral_grunt(long object_index)
{
    struct object_datum *object;
    char const *name;

    if (!mcc_skull_active(_mcc_skull_grunt_funeral) || !(object = object_try_and_get(object_index)))
        return FALSE;
    name = tag_get_name(object->definition_index);
    return name && (!csstrcmp(name, "characters\\grunt\\grunt") || !csstrcmp(name, "characters\\grunt\\grunt specops"));
}

/* As MCC does it, before the body is depleted: the globals' plasma grenade
 * projectile, ownerless, a centimetre up the Grunt's up vector, detonated
 * at once. */
void mcc_skulls_funeral_explosion(long object_index)
{
    struct game_globals *globals;
    struct game_globals_grenade *grenade;
    struct object_datum *object;
    struct object_placement_data data;
    long projectile_index;

    if (!mcc_skulls_funeral_grunt(object_index))
        return;
    globals = scenario_get_game_globals();
    if (globals->grenades.count < 2)
        return;
    grenade = TAG_BLOCK_GET_ELEMENT(&globals->grenades, 1, struct game_globals_grenade);
    if (grenade->projectile.index == NONE)
        return;
    object = object_get(object_index);
    object_placement_data_new(&data, grenade->projectile.index, NONE);
    data.flags |= FLAG(_new_object_never_automatically_delete_bit);
    data.position = object->object.position;
    data.forward = object->object.forward;
    data.up = object->object.up;
    data.position.z += data.up.k * 0.01f;
    projectile_index = object_new(&data);
    if (projectile_index != NONE)
    {
        projectile_port_detonate(projectile_index);
        object_delete(projectile_index);
    }
}

/* MCC draws from this machine's own random numbers; with Fog the motion
 * sensor (5) is gone already, so it is not chosen. */
static void mcc_skulls_malfunction_choose(void)
{
    mcc_malfunction_element = seed_random_range(get_global_local_random_seed_address(), 0,
        mcc_skull_active(_mcc_skull_fog) ? 5 : 6);
}

boolean mcc_skulls_malfunction(short element)
{
    boolean on = mcc_skull_active(_mcc_skull_malfunction);

    if (on != mcc_malfunction_on)
    {
        mcc_malfunction_on = on;
        if (on)
            mcc_skulls_malfunction_choose();
        else
            mcc_malfunction_element = NONE;
    }
    return on && element == mcc_malfunction_element;
}

void mcc_skulls_malfunction_died(void)
{
    mcc_skulls_malfunction(NONE); /* (the skull turned on or off) */
    if (mcc_malfunction_on)
        mcc_skulls_malfunction_choose();
}

/* 2 is the units' "loud" volume (silent, medium, loud, shout, quiet). MCC
 * replaces the unit's own, so a shouting unit is heard as loud too. */
short mcc_skulls_unit_sound_volume(short constant_sound)
{
    return mcc_skull_active(_mcc_skull_thats_just_wrong) && !cinematic_in_progress() ? 2 : constant_sound;
}

/* MCC cloaks every enemy unit an actor controls, every tick. The camouflage
 * reaches clients with each actor's state (network_actors.c). Bandanna is
 * the infinite ammunition cheat (mcc_scripts.c); with it MCC keeps at least
 * one grenade of each type in every player's unit. */
void mcc_skulls_update(void)
{
    struct object_iterator iterator;
    boolean assassins, bandanna;

    if (!mcc_cache_tags_loaded() || game_connection() == _game_connection_network_client)
        return;
    assassins = mcc_skull_active(_mcc_skull_assassins);
    bandanna = cheat.infinite_ammo;
    if (!assassins && !bandanna)
        return;
    object_iterator_new(&iterator, _object_mask_unit, 0);
    while (object_iterator_next(&iterator))
    {
        struct unit_datum *unit = unit_try_and_get(iterator.index);

        if (!unit || TEST_FLAG(unit->object.damage_flags, _object_dead_bit))
            continue;
        if (bandanna && unit->unit.player_index != NONE)
        {
            short type, types = (short)MIN(scenario_get_game_globals()->grenades.count, NUMBER_OF_UNIT_GRENADE_TYPES);

            for (type = 0; type < types; type++)
            {
                if (unit->unit.grenade_counts[type] < 1)
                    unit->unit.grenade_counts[type] = 1;
            }
            /* (but no plasma grenade, with Foreign) */
            if (types > 1 && mcc_skull_active(_mcc_skull_foreign))
                unit->unit.grenade_counts[1] = 0;
            if (mcc_grenades_active())
            {
                for (type = NUMBER_OF_UNIT_GRENADE_TYPES; type < mcc_grenades_type_count(); type++)
                {
                    if (mcc_grenades_get(iterator.index, type) < 1)
                        mcc_grenades_set(iterator.index, type, 1);
                }
            }
        }
        if (assassins && (unit->unit.actor_index != NONE || unit->unit.swarm_actor_index != NONE) &&
            unit->unit.player_index == NONE &&
            game_team_is_enemy(_game_team_player, unit->object.owner_team_index) &&
            !TEST_FLAG(unit->unit.flags, _unit_active_camouflaged_bit))
        {
            SET_FLAG(unit->unit.flags, _unit_active_camouflaged_bit, TRUE);
            unit->unit.active_camouflage = 1.0f;
        }
    }
}
