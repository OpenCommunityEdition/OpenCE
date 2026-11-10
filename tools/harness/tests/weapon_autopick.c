/* Whether walking over a weapon picks it up (unit_should_autopick_weapon, source/game/players.c): a first
weapon, the flag and the ball, and in campaign a second; in multiplayer a second is the action button's
("Hold X to pick up"), as on the Xbox */
#include "harness.h"
struct weapon_datum { long definition_index; };
struct weapon_definition { struct { unsigned long flags; } weapon; };
#include "under_test_enums.inc"

/* the world: the unit's weapons, what its game allows, the weapon lying there */
static long weapon_count;
static boolean pickup_approved = TRUE;
static boolean multiplayer;
static boolean forced;
static struct weapon_datum weapon = {7};
static struct weapon_definition definition;

static struct weapon_datum *weapon_try_and_get(long weapon_index) { (void)weapon_index; return &weapon; }
static struct weapon_definition *weapon_definition_get(long definition_index)
{
    CHECK(definition_index == weapon.definition_index, "the weapon's own definition");
    return &definition;
}
static long unit_get_weapon_count(long unit_index) { (void)unit_index; return weapon_count; }
static boolean unit_approve_weapon_pickup(long unit_index, long weapon_index)
{
    (void)unit_index; (void)weapon_index;
    return pickup_approved;
}
static boolean game_engine_running(void) { return multiplayer; }
static boolean game_engine_force_autopickup(long unit_index, long weapon_index)
{
    (void)unit_index; (void)weapon_index;
    return forced;
}

#include "under_test.inc"

static boolean walked_over(boolean in_multiplayer, long count)
{
    multiplayer = in_multiplayer;
    weapon_count = count;
    return unit_should_autopick_weapon(1, 2);
}

int main(int argc, char **argv)
{
    char const *case_name = argc > 1 ? argv[1] : "";
    CASE("multiplayer") {
        CHECK(walked_over(TRUE, 0), "a first weapon, in multiplayer");
        CHECK(!walked_over(TRUE, 1), "a second weapon is the action button's, in multiplayer");
        CHECK(!walked_over(TRUE, 2), "a third is a swap, in multiplayer");
    }
    CASE("campaign") {
        CHECK(walked_over(FALSE, 0), "a first weapon, in campaign");
        CHECK(walked_over(FALSE, 1), "a second weapon into the empty slot, in campaign");
        CHECK(!walked_over(FALSE, 2), "a third is a swap, in campaign");
        pickup_approved = FALSE;
        CHECK(!walked_over(FALSE, 1), "a second the unit may not pick up, in campaign");
        pickup_approved = TRUE;
    }
    CASE("not counted") {
        /* a weapon that does not count toward the two (the flag's, the ball's flags) */
        definition.weapon.flags = FLAG(_weapon_doesnt_count_toward_maximum_bit);
        CHECK(walked_over(TRUE, 1) && walked_over(TRUE, 2), "one that does not count, in multiplayer");
        CHECK(walked_over(FALSE, 2), "one that does not count, in campaign");
        pickup_approved = FALSE;
        CHECK(!walked_over(TRUE, 1), "one that does not count, but the unit may not pick up");
        pickup_approved = TRUE;
        definition.weapon.flags = FLAG(_weapon_must_be_readied_bit);
        CHECK(!walked_over(TRUE, 1), "another flag counts");
        definition.weapon.flags = 0;
    }
    CASE("forced") {
        /* capture the flag's flag (game_engine_force_autopickup) */
        forced = TRUE;
        CHECK(walked_over(TRUE, 1) && walked_over(TRUE, 2), "the game engine's forced pickup");
        forced = FALSE;
    }
    return 0;
}
