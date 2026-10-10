#include "cseries.h"
#include "math/real_math.h"
#include "game/players.h"
#include "scenario/scenario_definitions.h"
#include "mcc_player.h"
#include <stdio.h>
#include <string.h>
#undef memset
static boolean active;
static short types = 4, counts[4];
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
