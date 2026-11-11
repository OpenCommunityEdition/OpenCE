"""bipeds.c's cabin test and recovery for a rider in an enclosed vehicle (vehicle_riding.c's cabin rules), taken from
the sources and compiled against made-up enclosed collision models (boxes in a vehicle's body frame).

    python -m pytest -q tools/harness/tests/test_vehicle_cabin.py
"""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
sys.path.insert(0, str(Path(__file__).resolve().parent))
from harness import CHECK_FAILED, biped_riding_sources, build, enum_with, function, mutated, read, run  # noqa: E402
from test_vehicle_riding import generated as riding_generated  # noqa: E402

BIPEDS = "port/linux/game/vehicle_riding_bipeds.c"
CASES = ["cabin-interior", "cabin-crossing", "tilt-both-gates",
         "cabin-recovery",
         "cabin-deferred-touch", "cabin-overlap-retry", "cabin-failure-log", "cabin-failure-alternate"]
CASES += ["protected-own-hit", "riding-on-reached", "riding-off-reached"]
CASES += ["window-jump-sample-moving", "window-no-early-return", "window-final-return"]
CASES += ["window-moving-deck-rear", "window-moving-deck-front-left", "window-moving-deck-rear-left",
          "window-stationary-deck"]
BIPED_FUNCTIONS = ["biped_riding_cabin_floor", "biped_riding_cabin_rays", "biped_riding_cabin_crosses",
                   "biped_riding_cabin_sample", "biped_riding_cabin_tick",
                   "biped_riding_over_deck", "biped_riding_cabin_protected", "biped_riding_window_over_deck",
                   "biped_riding_recast_over_deck", "biped_riding_cabin_valid", "biped_riding_cabin_settle",
                   "biped_riding_contacts"]

# a fault in bipeds.c ("bipeds") or vehicle_riding.c ("riding"), and the case that must catch it
NEGATIVE_CONTROLS = {
    "fixture-decision-stale": (("riding", "define", "FIXTURE_NO_DECIDE"), "riding-on-reached"),
    "fixture-decision-stale-off": (("riding", "define", "FIXTURE_NO_DECIDE_OFF"), "riding-off-reached"),
    "touching-miss-is-door": (("riding", "else if (!check->touching)", "else"), "cabin-overlap-retry"),
    "failed-recovery-drops-anchor": (("riding", "if (settle == _vehicle_riding_cabin_settle_failed)\n\t\treturn;",
                                       "if (settle == _vehicle_riding_cabin_settle_failed)\n\t\t{ cabin->have_anchor = FALSE; return; }"),
                                      "cabin-overlap-retry"),
    "failure-log-every-settle": (("riding", "!cabin->failure_logged || (unsigned long)tick - (unsigned long)cabin->failure_log_tick >= RIDING_CABIN_LOG_TICKS",
                                   "TRUE"), "cabin-failure-log"),
    "failure-ends-same-tick": (("riding", " && cabin->failure_seen_tick != tick)", ")"), "cabin-failure-alternate"),
    "failure-end-unlogged": (("bipeds", "if (report < 0)", "if (FALSE)"), "cabin-failure-log"),
    "collision-recast-keeps-45-gate": (("bipeds", " ||\n\t\tbiped_riding_cabin_protected(biped_index, biped, definition, riding, vehicle);",
                                        ";"), "tilt-both-gates"),
    "recast-resamples-enclosure": (("bipeds", "\treturn cabin && cabin->unit_index == biped_index",
                                    "\tif (cabin)\n\t{\n\t\tstruct vehicle_riding_cabin_rays rays;\n\n"
                                    "\t\tbiped_riding_cabin_rays(vehicle->object_index, &vehicle->body_up, &vehicle->origin, .6f, .9f, &rays);\n"
                                    "\t}\n\treturn cabin && cabin->unit_index == biped_index"), "tilt-both-gates"),
    "biped-tick-unprotected": (("bipeds", "return biped_riding_over_deck(definition, vehicle, &origin, height) || exception;",
                                "return biped_riding_over_deck(definition, vehicle, &origin, height);"), "tilt-both-gates"),
    "exception-at-45-only": (("riding", "real limit = minimum_normal_k > RIDING_UPRIGHT_K ? minimum_normal_k : RIDING_UPRIGHT_K;",
                              "real limit = .6f;"), "tilt-both-gates"),
    "protected-other-unit": (("bipeds", "return cabin && cabin->unit_index == biped_index && ", "return cabin && "),
                             "tilt-both-gates"),
    "single-ray-interior": (("bipeds", "\trays->hit = vehicle_riding_cabin_hit(rays, jump_height, height);\n",
                             "\trays->hit = rays->inside || (rays->ceiling && rays->ceiling_distance <= jump_height);\n"),
                            "cabin-interior"),
    "other-vehicle-floor": (("bipeds", "collision_model_test_vector(&instance, FLAG(_collision_test_front_facing_surfaces_bit), &floor_start,",
                             "fake_world_vector(FLAG(_collision_test_front_facing_surfaces_bit), &floor_start,"),
                            "cabin-interior"),
    "inside-bypasses-floor": (("bipeds", "\trays->hit = vehicle_riding_cabin_hit(rays, jump_height, height);\n",
                               "\trays->hit = rays->inside || vehicle_riding_cabin_hit(rays, jump_height, height);\n"),
                              "cabin-interior"),
    # (the floor ray's front faces and its normal test guard each other: against any consistent model a front face
    # met along -axis faces along the axis, so neither alone can be seen to fail; the case's slab check pins the pair)
    "floor-any-face-unchecked": (("bipeds", "FLAG(_collision_test_front_facing_surfaces_bit), &floor_start,\n"
                                  "\t\t&ray, &result) && biped_riding_cabin_floor(&instance, &result, axis))",
                                  "FLAG(_collision_test_front_facing_surfaces_bit) | FLAG(_collision_test_back_facing_surfaces_bit), &floor_start,\n"
                                  "\t\t&ray, &result))"),
                                 "cabin-interior"),
    "rays-world-up": (("bipeds", "\tvehicle_riding_cabin_ends(base, height, axis, &ceiling_start, &floor_start);\n",
                       "\tvehicle_riding_cabin_ends(base, height, &world_up, &ceiling_start, &floor_start);\n"
                       "\taxis = &world_up;\n"), "cabin-interior"),
    "crossing-counts-endpoints": (("riding", "return hit && t > 0.f && t < 1.f;", "return hit && t >= 0.f && t <= 1.f;"),
                                  "cabin-crossing"),
    "crossing-front-only": (("bipeds", "FLAG(_collision_test_front_facing_surfaces_bit) | FLAG(_collision_test_back_facing_surfaces_bit), from,",
                             "FLAG(_collision_test_front_facing_surfaces_bit), from,"), "cabin-crossing"),
    "accept-outside-free": (("riding", "return interior && free && !crosses;", "return free && !crosses;"), "cabin-recovery"),
    "candidate-segment-unchecked": (("riding", "return interior && free && !crosses;", "return interior && free;"),
                                    "cabin-recovery"),
    "fix-offset-under-belly-rejected": (("riding", "return interior && free && !crosses;", "return free;"), "cabin-recovery"),
    "return-through-B": (("bipeds", "free = distance3d(&reached, candidate) < 0.001f &&\n\t\t", "free = "),
                         "cabin-recovery"),
    "candidate-in-vehicle": (("bipeds", "!biped_riding_touches(vehicle->object_index, candidate, global_zero_vector3d, height, width)",
                              "TRUE"), "cabin-recovery"),
    "failed-recovery-crushes": (("bipeds", "\t\t*base = *entry;\n",
                                 "\t\t*base = *entry;\n\t\tphysics_biped_vehicle_collision_damage(vehicle->object_index, biped_index, NULL);\n"),
                                "cabin-recovery"),
    "failure-not-restored": (("bipeds", "\t\t*base = *entry;\n", ""), "cabin-recovery"),
    "failure-unlogged": (("bipeds", "if (report > 0)", "if (FALSE)"), "cabin-recovery"),
    "deferred-to-ordinary": (("bipeds", "vehicle.object_index == cabin_vehicle || (cabin && contacts[index].deferred)",
                              "vehicle.object_index == cabin_vehicle"), "cabin-deferred-touch"),
    "everything-to-cabin": (("bipeds", "vehicle.object_index == cabin_vehicle || (cabin && contacts[index].deferred)",
                             "TRUE"), "cabin-deferred-touch"),
    "after-carry-seeds-anchor": (("bipeds", "vehicle_riding_cabin_settled(cabin, settle, final && (settle ==",
                                 "vehicle_riding_cabin_settled(cabin, settle, (settle =="), "cabin-recovery"),
    "window-gate-stale-origin": (("bipeds", "&origin, height) || exception;", "base, height) || exception;"),
                                 "window-moving-deck-rear"),
    "recast-gate-stale-origin": (("bipeds", "return biped_riding_over_deck(definition, vehicle, &origin, height) ||\n\t\tbiped_riding_cabin_protected",
                                  "return biped_riding_over_deck(definition, vehicle, base, height) ||\n\t\tbiped_riding_cabin_protected"),
                                 "window-moving-deck-rear"),
    "window-gate-either-origin": (("bipeds", "return biped_riding_over_deck(definition, vehicle, &origin, height) || exception;",
                                   "return biped_riding_over_deck(definition, vehicle, &origin, height) || biped_riding_over_deck(definition, vehicle, base, height) || exception;"),
                                  "window-moving-deck-front-left"),
    "window-origin-twice": (("riding", "\torigin->x = base->x + moved_base.x - corrected_base.x;\n",
                             "\torigin->x = base->x + 2.f*(moved_base.x - corrected_base.x);\n"), "window-moving-deck-rear-left"),
    "protected-reads-verdict": (("bipeds", "vehicle_riding_cabin_encloses(cabin, riding, vehicle_riding_player_air(player)),",
                                 "vehicle_riding_cabin_verdict(cabin, riding, vehicle_riding_player_air(player), TRUE) == _vehicle_riding_enclosure_enclosed,"),
                                "protected-own-hit"),
    "jump-sample-at-base": (("bipeds", "sample == _vehicle_riding_cabin_sample_jump && !vehicle_riding_is_riding(riding))", "FALSE)"),
                            "window-jump-sample-moving"),
    "window-settles-early": (("bipeds", "\t\t\tif (carried)\n\t\t\t{\n\t\t\t\tbiped_riding_cabin_settle", "\t\t\tif (TRUE)\n\t\t\t{\n\t\t\t\tbiped_riding_cabin_settle"),
                             "window-no-early-return"),
    "window-watched-early": (("prepare", "riding_step_valid && riding_carried && !riding_cabin_settled", "riding_step_valid && !riding_cabin_settled"),
                             "window-no-early-return"),
    "window-never-settles": (("finish", "if (riding_step_valid && vehicle_riding_cabin_watched(riding_cabin, &riding, biped_index))\n\t{\n\t\treal_point3d entry = physics.new_position;",
                              "if (FALSE && riding_step_valid && vehicle_riding_cabin_watched(riding_cabin, &riding, biped_index))\n\t{\n\t\treal_point3d entry = physics.new_position;"),
                             "window-final-return"),
    "window-origin-velocity": (("riding", "\torigin->x = base->x + moved_base.x - corrected_base.x;\n",
                                "\torigin->x = base->x + moved_base.x - corrected_base.x + vehicle->translational_velocity.i;\n"),
                               "window-stationary-deck"),
}


def refusal_tick(bipeds):
    """The tick's snapshot/begin and post-physics record/acquire wiring, with native physics supplied by the case."""
    moving = function(bipeds, "biped_update_moving")

    def between(start, end):
        assert moving.count(start) == 1, start
        assert moving.count(end) == 1, end
        return moving[moving.index(start):moving.index(end)]

    declarations = between("\tstruct vehicle_riding_state riding;", "\n\tbiped = biped_get(biped_index);")
    snapshot = between("\tbiped_riding_get(biped, &riding);", "\tif (riding_carried)\n")
    begin = between("\triding_cabin = vehicle_riding_player_cabin(riding_player);",
                    "\t/* (in the window, once before its physics:")
    start = moving.index("\tif (TEST_FLAG(biped->unit.flags, _unit_suspended_bit))\n\t{\n\t\tphysics.new_position")
    end = moving.index("\n\t{\n\t\treal_point3d new_biped_position", start)
    return ("static boolean biped_cabin_refusal_tick(long biped_index, struct biped_datum *biped, "
            "struct biped_definition const *definition, struct biped_physics *entry)\n{\n"
            "\tstruct biped_physics physics = *entry;\n" + declarations + "\n" + snapshot
            + "\triding_player = biped_riding_player(biped);\n"
            + "\triding_air = vehicle_riding_player_air(riding_player);\n" + begin + moving[start:end]
            + "\n\t(void)riding_exception;\n\t*entry = physics;\n\treturn riding_step_valid;\n}\n")


def window_settle_tick(bipeds):
    """The tick's snapshot, carry, cabin and window ray as window_tick has them, then the touches the vehicles recorded
    and the watched place's settle (biped_update_moving, up to the physics): the place it leaves the biped in."""
    moving = function(bipeds, "biped_update_moving")

    def between(start, end):
        assert moving.count(start) == 1, start
        assert moving.count(end) == 1, end
        return moving[moving.index(start):moving.index(end)]

    declarations = between("\tstruct vehicle_riding_state riding;", "\n\tbiped = biped_get(biped_index);")
    head = between("\tbiped_riding_get(biped, &riding);", "\tlocal_player_index = ")
    tail = between("\tif (riding_player != NONE)\n\t{\n\t\triding_cabin_settled = biped_riding_contacts",
                   "\tphysics.riding_applies = riding_applies;")
    return ("static void biped_window_settle_tick(long biped_index, struct biped_datum *biped, "
            "struct biped_definition const *definition, struct biped_physics *entry)\n{\n"
            "\tstruct biped_physics physics = *entry;\n" + declarations + "\n" + head + tail
            + "\t(void)riding_exception;\n\t*entry = physics;\n}\n")


def window_tick(bipeds):
    """The tick's snapshot, carry, cabin and window ray (biped_update_moving), up to its turn of the view: the
    over-deck verdict the air frame and the hold read."""
    moving = function(bipeds, "biped_update_moving")

    def between(start, end):
        assert moving.count(start) == 1, start
        assert moving.count(end) == 1, end
        return moving[moving.index(start):moving.index(end)]

    declarations = between("\tstruct vehicle_riding_state riding;", "\n\tbiped = biped_get(biped_index);")
    body = between("\tbiped_riding_get(biped, &riding);", "\tlocal_player_index = ")
    return ("static boolean biped_window_tick(long biped_index, struct biped_datum *biped, "
            "struct biped_definition const *definition, struct biped_physics const *entry)\n{\n"
            "\tstruct biped_physics physics = *entry;\n" + declarations + "\n" + body
            + "\t(void)riding_exception;\n"
            "\treturn riding_step_valid && riding_step.over_deck;\n}\n")


def generated(control=None):
    which, before, after = NEGATIVE_CONTROLS[control][0] if control else (None, None, None)
    riding = dict(riding_generated((before, after) if which == "riding" else None))
    collisions = read("source/physics/collisions.h")
    # (one enum holds the face bits and the bipeds' flags, collisions.h)
    types = enum_with(collisions, "_collision_test_front_facing_surfaces_bit") + "\n"
    bipeds = biped_riding_sources()
    types += enum_with(read("source/units/bipeds.h"), "_biped_airborne_bit") + "\n"
    types += enum_with(read("source/units/units.h"), "_unit_suspended_bit") + "\n"
    types += enum_with(bipeds, "_biped_physics_out_airborne_bit") + "\n"
    code = "".join(function(bipeds, name) + "\n" for name in BIPED_FUNCTIONS)
    tick = refusal_tick(bipeds)
    if which == "bipeds":
        code = mutated(code, before, after)
    settle = window_settle_tick(bipeds)
    if which == "prepare":
        settle = mutated(settle, before, after)
    if which == "finish":
        tick = mutated(tick, before, after)
    return (("enums.inc", riding["enums.inc"]), ("engine.inc", riding["engine.inc"]),
            ("under_test.inc", riding["under_test.inc"]), ("cabin_types.inc", types), ("bipeds_cabin.inc", code + tick + window_tick(bipeds) + settle))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("vehicle_cabin", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    status, output = run(build("vehicle_cabin", generated(control)), NEGATIVE_CONTROLS[control][1])
    assert status == CHECK_FAILED, f"a fault ({control}) passed '{NEGATIVE_CONTROLS[control][1]}': the test cannot see it"


if __name__ == "__main__":
    sys.exit(pytest.main(["-q", __file__]))
