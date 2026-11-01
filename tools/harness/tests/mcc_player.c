#include "cseries.h"
#include "math/real_math.h"
#include "game/players.h"
#include "game/game.h"
#include "units/units.h"
#include "scenario/scenario_definitions.h"
#include "mcc_player.h"
#include <stdio.h>
#include <string.h>
#undef memset
static boolean active;
static short types = 4, counts[4];
static struct unit_datum equipment_unit;
static struct scenario equipment_scenario;
static short connection = _game_connection_local;
static unsigned equipment_calls, deleted_weapons;
static long deleted_weapon, given_unit;
static short given_profile;
static boolean given_reset, held_at_assignment;
short game_connection(void) { return connection; }
struct scenario *global_scenario_get(void) { return &equipment_scenario; }
void *object_try_and_get_and_verify_type(long index, unsigned long mask) {
    return index == 123 && (mask & _object_mask_biped) ? &equipment_unit : NULL;
}
void object_delete(long weapon) {
    short slot=equipment_unit.unit.current_weapon_index;
    ++deleted_weapons;deleted_weapon=weapon;
    /* The engine's deleted-object notification clears matching inventory. */
    if (slot>=0 && equipment_unit.unit.weapon_object_indices[slot]==weapon) {
        equipment_unit.unit.weapon_object_indices[slot]=NONE;
        equipment_unit.unit.current_weapon_index=NONE;
    }
}
void player_add_equipment(long unit, short profile, boolean reset) {
    ++equipment_calls;given_unit=unit;given_profile=profile;given_reset=reset;
    held_at_assignment=equipment_unit.unit.current_weapon_index!=NONE;
}
static struct { long weapon_index; } cached_weapons[4];
static unsigned forgotten;
boolean mcc_cache_tags_loaded(void) { return active; }
static void first_person_weapon_forget_weapon(short local)
{
    forgotten |= 1U << local;
    cached_weapons[local].weapon_index = NONE;
}
#define first_person_weapon_get(local) (&cached_weapons[local])
#include "mcc_first_person.inl"
boolean mcc_grenades_active(void) { return active; }
short mcc_grenades_type_count(void) { return types; }
void mcc_grenades_clear(long unit) { (void)unit; counts[2] = counts[3] = 0; }
short mcc_grenades_get(long unit, short type) { (void)unit; return counts[type]; }
void mcc_grenades_set(long unit, short type, short count) { (void)unit; counts[type] = count; }
#define CHECK(value) do { if (!(value)) { fprintf(stderr,"failed at %d\n",__LINE__); return 1; } } while (0)
int main(void) {
    struct player_action action = {0};
    struct scenario_starting_profile profile = {0};
    union { unsigned long bits; float value; } invalid;
    short choice;
    equipment_scenario.starting_profiles.count=2;
    equipment_unit.unit.player_index=7;
    equipment_unit.unit.current_weapon_index=1;
    equipment_unit.unit.weapon_object_indices[0]=44;
    equipment_unit.unit.weapon_object_indices[1]=55;
    mcc_player_add_equipment(123,1,TRUE);
    CHECK(!equipment_calls && !deleted_weapons); /* Xbox/CE */
    active=TRUE;
    connection=_game_connection_network_client;
    mcc_player_add_equipment(123,1,TRUE);
    CHECK(!equipment_calls && !deleted_weapons);
    connection=_game_connection_local;
    mcc_player_add_equipment(NONE,1,TRUE);
    mcc_player_add_equipment(999,1,TRUE);
    mcc_player_add_equipment(123,NONE,TRUE);
    mcc_player_add_equipment(123,2,TRUE);
    equipment_unit.unit.player_index=NONE;
    mcc_player_add_equipment(123,1,TRUE);
    CHECK(!equipment_calls && !deleted_weapons);
    equipment_unit.unit.player_index=7;
    mcc_player_add_equipment(123,1,FALSE);
    CHECK(equipment_calls==1 && !deleted_weapons && held_at_assignment && !given_reset);
    mcc_player_add_equipment(123,1,TRUE);
    CHECK(equipment_calls==2 && deleted_weapons==1 && deleted_weapon==55 && !held_at_assignment);
    CHECK(given_unit==123 && given_profile==1 && given_reset);
    CHECK(equipment_unit.unit.weapon_object_indices[0]==44); /* native service clears the rest */
    mcc_player_add_equipment(123,1,TRUE);
    CHECK(equipment_calls==3 && deleted_weapons==1); /* unarmed/repeated reset */
    equipment_unit.unit.current_weapon_index=MAXIMUM_WEAPONS_PER_UNIT;
    mcc_player_add_equipment(123,1,TRUE);
    CHECK(equipment_calls==4 && deleted_weapons==1);
    equipment_unit.unit.current_weapon_index=1;
    equipment_unit.unit.weapon_object_indices[1]=NONE;
    mcc_player_add_equipment(123,1,TRUE);
    CHECK(equipment_calls==5 && deleted_weapons==1);
    connection=_game_connection_network_server;
    equipment_unit.unit.weapon_object_indices[1]=66;
    mcc_player_add_equipment(123,1,TRUE);
    CHECK(equipment_calls==6 && deleted_weapons==2 && deleted_weapon==66 && !held_at_assignment);
    active=FALSE;
    mcc_player_add_equipment(123,1,TRUE);
    CHECK(equipment_calls==6 && deleted_weapons==2); /* unload/legacy transition */
    cached_weapons[0].weapon_index = cached_weapons[2].weapon_index = 123;
    cached_weapons[1].weapon_index = 456;
    cached_weapons[3].weapon_index = NONE;
    mcc_first_person_weapon_deleted(123);
    CHECK(!forgotten && cached_weapons[0].weapon_index == 123);
    active = TRUE;
    mcc_first_person_weapon_deleted(NONE);
    mcc_first_person_weapon_deleted(789);
    CHECK(!forgotten);
    mcc_first_person_weapon_deleted(123);
    CHECK(forgotten == 5 && cached_weapons[0].weapon_index == NONE &&
          cached_weapons[2].weapon_index == NONE && cached_weapons[1].weapon_index == 456);
    mcc_first_person_weapon_deleted(123);
    CHECK(forgotten == 5);
    active = FALSE;
    CHECK(!mcc_player_choice_supported(0));
    mcc_player_profile_grenades(1, &profile, TRUE);
    active = TRUE;
    for (choice = NONE; choice < 4; ++choice) {
        action.desired_grenade_index = choice;
        CHECK(mcc_player_action_valid(&action));
    }
    action.desired_grenade_index = 4;
    CHECK(!mcc_player_action_valid(&action));
    action.desired_grenade_index = -2;
    CHECK(!mcc_player_action_valid(&action));
    action.desired_grenade_index = 3;
    types = 2;
    CHECK(!mcc_player_action_valid(&action));
    types = 4;
    invalid.bits = 0x7FC00000;
    action.throttle.i = invalid.value;
    CHECK(!mcc_player_action_valid(&action));
    action.throttle.i = 0;
    action.desired_weapon_index = 100;
    CHECK(!mcc_player_action_valid(&action));
    action.desired_weapon_index = NONE;
    action.desired_zoom_level = -2;
    CHECK(!mcc_player_action_valid(&action));
    profile.pad[0] = 2; profile.pad[1] = 3;
    counts[0] = 1; counts[1] = 4; counts[2] = 10;
    mcc_player_profile_grenades(1, &profile, TRUE);
    CHECK(counts[0] == 1 && counts[1] == 4 && counts[2] == 2 && counts[3] == 3);
    mcc_player_profile_grenades(1, &profile, FALSE);
    CHECK(counts[2] == 4 && counts[3] == 6);
    counts[2] = 126;
    mcc_player_profile_grenades(1, &profile, FALSE);
    CHECK(counts[2] == 127 && counts[3] == 9);
    active = FALSE;
    mcc_player_profile_grenades(1, &profile, TRUE);
    CHECK(counts[2] == 127 && counts[3] == 9);
    mcc_player_postspawn_grenades(1, TRUE);
    CHECK(counts[2] == 127 && counts[3] == 9);
    active = TRUE;
    mcc_player_postspawn_grenades(1, FALSE);
    CHECK(counts[2] == 127 && counts[3] == 9);
    mcc_player_postspawn_grenades(1, TRUE);
    CHECK(counts[0] == 1 && counts[1] == 4 && !counts[2] && !counts[3]);
    return 0;
}
