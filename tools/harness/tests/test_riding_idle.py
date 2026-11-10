"""The biped hooks with riding off (or no player): the fast path leaves the world exactly as the full path does.

The tick's prepare and finish (bipeds.c's biped_update_moving, native physics between them stubbed) and
biped_riding_seated are taken from the sources and compiled twice, once as the game builds them and once with the fast
path switched off by text; each case compares everything the tick may write (tests/riding_idle.c).

    python -m pytest -q tools/harness/tests/test_riding_idle.py
"""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
sys.path.insert(0, str(Path(__file__).resolve().parent))
from harness import CHECK_FAILED, biped_riding_sources, build, enum_with, function, mutated, read, run, structure  # noqa: E402
from test_vehicle_riding import generated as riding_generated  # noqa: E402

BIPEDS = "port/linux/game/vehicle_riding_bipeds.c"
RIDING = "port/linux/game/vehicle_riding.c"
HEADER = "port/linux/game/vehicle_riding_bipeds.h"

CASES = ["off-clean-player", "off-ai-biped", "off-seated", "off-after-ride", "off-stale-state-only",
         "off-stale-pad-only", "off-stale-mark-only", "off-stale-contact-only", "off-stale-cabin-only",
         "off-stale-air-only", "off-stale-dyaw-only", "off-negative-zero-fields", "off-negative-zero-tables",
         "off-negative-zero-dyaw", "off-negative-zero-seated", "off-work-skipped", "deferred-reads-fields", "collision-reads-fields",
         "falling-reads-fields", "riding-on-reached", "riding-off-reached"]

CLEARED_STRUCTURES = {
    "riding_mark_cleared": [("vehicle_riding_mark", "mark->"), ("vehicle_riding_correction", "mark->correction.")],
    "riding_air_cleared": [("vehicle_riding_air", "air->")],
    "riding_cabin_cleared": [("vehicle_riding_cabin", "cabin->")],
}

# a fault in the fast path of the prepare ("fast"), its finish gate ("finish"), the biped functions ("bipeds"),
# biped_riding_seated ("seated") or vehicle_riding.c ("riding"), and the case that must catch it
NEGATIVE_CONTROLS = {
    "fixture-decision-stale": (("riding", "define", "FIXTURE_NO_DECIDE"), "riding-on-reached"),
    "fixture-decision-stale-off": (("riding", "define", "FIXTURE_NO_DECIDE_OFF"), "riding-off-reached"),
    "off-misses-air-frame": (("fast", "\t\tphysics.air_frame_velocity = *global_zero_vector3d;\n", ""), "off-clean-player"),
    "off-misses-velocity": (("fast", "\t\tphysics.velocity = biped->object.translational_velocity;\n",
                             "\t\tphysics.velocity = *global_zero_vector3d;\n"), "off-clean-player"),
    "idle-ignores-state": (("bipeds", "biped->biped.riding_ticks == 0 && ", ""), "off-stale-state-only"),
    "idle-ignores-pad": (("bipeds", " &&\n\t\tbiped->biped.riding_pad == 0;", ";"), "off-stale-pad-only"),
    "idle-ignores-mark": (("riding", "riding_mark_cleared(&riding_marks[player_index]) &&\n\t\t", ""),
                          "off-stale-mark-only"),
    "idle-ignores-contacts": (("riding", "riding_contact_counts[player_index] == 0 &&\n\t\t", ""),
                              "off-stale-contact-only"),
    "idle-ignores-cabin": (("riding", " &&\n\t\tcabin->failure_logged == cleared.failure_logged && "
                            "cabin->failure_log_tick == cleared.failure_log_tick &&\n\t\t"
                            "cabin->failure_seen_tick == cleared.failure_seen_tick;", ";"), "off-stale-cabin-only"),
    "idle-ignores-air": (("riding", "riding_air_cleared(&riding_airs[player_index]) &&\n\t\t", ""), "off-stale-air-only"),
    "idle-ignores-dyaw": (("bipeds", " &&\n\t\tvehicle_riding_real_same(vehicle_riding_kept_dyaw(local_player_index), 0.f);", ";"),
                          "off-stale-dyaw-only"),
    "seated-ignores-mark": (("seated", " && vehicle_riding_player_idle(biped_riding_player(biped)) &&\n\t\t"
                             "vehicle_riding_real_same(vehicle_riding_kept_dyaw(local_player_index), 0.f))",
                             " &&\n\t\tvehicle_riding_real_same(vehicle_riding_kept_dyaw(local_player_index), 0.f))"), "off-seated"),
    # (the gate becomes a check of its own, so the case stops at a finish entered on the fast path, before any local
    # the fast path left unset is read)
    "finish-not-gated": (("finish", "\tif (riding_idle)\n\t\tbreak;\n",
                          "\tCHECK(!riding_idle, \"finish entered on the fast path\");\n"), "off-work-skipped"),
    "ai-misses-support-velocity": (("fast", "\t\tphysics.support_velocity = *global_zero_vector3d;\n", ""), "off-ai-biped"),
    # (the whole clear-state test gone: the tick that must still clear everything takes the fast path)
    "after-ride-ignores-state": (("bipeds", " && biped_riding_fields_clear(biped) &&\n\t\tvehicle_riding_player_idle(biped_riding_player(biped)) &&\n\t\t"
                                  "vehicle_riding_real_same(vehicle_riding_kept_dyaw(local_player_index), 0.f);", ";"),
                                 "off-after-ride"),
    "negative-zero-field-equal": (("bipeds", "return vehicle_riding_real_same(a, 0.f) && vehicle_riding_real_same(b, 0.f) && "
                                   "vehicle_riding_real_same(c, 0.f);", "return a == 0.f && b == 0.f && c == 0.f;"),
                                  "off-negative-zero-fields"),
    "negative-zero-table-equal": (("riding", "return left.bits == right.bits;", "return left.value == right.value;"), "off-negative-zero-tables"),
    "negative-zero-dyaw-equal": (("bipeds", "vehicle_riding_real_same(vehicle_riding_kept_dyaw(local_player_index), 0.f);",
                                  "vehicle_riding_kept_dyaw(local_player_index) == 0.f;"), "off-negative-zero-dyaw"),
    "negative-zero-seated-equal": (("seated", "vehicle_riding_real_same(vehicle_riding_kept_dyaw(local_player_index), 0.f))",
                                    "vehicle_riding_kept_dyaw(local_player_index) == 0.f)"), "off-negative-zero-seated"),
    "deferred-ignores-applies": (("bipeds", "riding_ticks > 0 && biped_riding_applies(biped)", "riding_ticks > 0 && TRUE"),
                                 "deferred-reads-fields"),
    "collision-gate-ignores-applies": (("bipeds", "riding_ticks <= 0 || !biped_riding_applies(biped))",
                                        "riding_ticks <= 0)"), "collision-reads-fields"),
    "falling-gate-ignores-falling": (("bipeds", " && !biped->biped.riding_falling)", ")"), "falling-reads-fields"),
}

PREPARE_DECLARATIONS = ("\tstruct vehicle_riding_state riding;", "\n\tbiped = biped_get(biped_index);")
WRAP = "\tdo\n\t{\n"
UNWRAP = "\t} while (0);\n"
FIRST_STATEMENT = "\t/* (nothing to clear: the full path would write these same values) */\n"
GATE = "\tif (biped_riding_idle("
FINISH_GATE = "\tif (riding_idle)\n\t\tbreak;\n"


def tick_slice(bipeds):
    """biped_update_moving's expanded prepare and finish, the native physics between them stubbed (nothing moves), as
    functions of the biped and physics: (declarations, prepare, finish)."""
    moving = function(bipeds, "biped_update_moving")
    for anchor in (FIRST_STATEMENT, FINISH_GATE, PREPARE_DECLARATIONS[0], PREPARE_DECLARATIONS[1]):
        assert moving.count(anchor) == 1, anchor
    declarations = moving[moving.index(PREPARE_DECLARATIONS[0]):moving.index(PREPARE_DECLARATIONS[1])]
    first = moving.index(FIRST_STATEMENT)
    start = moving.rindex(WRAP, 0, first)
    prepare = moving[start:moving.index(UNWRAP, first) + len(UNWRAP)]
    gate = moving.index(FINISH_GATE)
    start = moving.rindex(WRAP, 0, gate)
    finish = moving[start:moving.index(UNWRAP, gate) + len(UNWRAP)]
    return declarations, prepare, finish


def tick_function(name, declarations, prepare, finish):
    return ("static boolean " + name + "(long biped_index, struct biped_datum *biped, "
            "struct biped_definition *definition, struct biped_physics *entry)\n{\n"
            "\tstruct biped_physics physics = *entry;\n" + declarations + "\n" + prepare
            + "\tphysics.new_position = physics.position;\n\tphysics.new_velocity = physics.velocity;\n"
            "\tphysics.out_flags = 0;\n" + finish + "\t*entry = physics;\n\treturn riding_idle;\n}\n")


def contacts_head(port):
    """biped_riding_contacts as far as the off path runs it: the contacts taken, then the return for a biped that does not
    ride (what follows needs the collision world, which the fast path never reaches)."""
    contacts = function(port, "biped_riding_contacts")
    gate = "\tif (!applies)\n\t\treturn FALSE;\n"
    assert contacts.count(gate) == 1
    return contacts[:contacts.index(gate) + len(gate)] + "\n\treturn settled;\n}\n"


def collision_head(port):
    """biped_riding_collision as far as its window decision: what follows needs the vehicle's tags and the collision
    world, which a biped riding cannot touch never reaches."""
    collision = function(port, "biped_riding_collision")
    anchor = "\tif (decision == _vehicle_riding_collision_original)\n\t\treturn decision;\n"
    assert collision.count(anchor) == 1
    head = collision[:collision.index(anchor) + len(anchor)] + "\n\treturn decision;\n}\n"
    return head.replace("short biped_riding_collision(", "static short biped_riding_collision_head(", 1)


def bipeds_text(control_fault=None):
    """The compiled biped functions (a fault, or none, in `bipeds`), the counters through define wrappers, and the
    seated hook twice."""
    port = read(BIPEDS)
    which, before, after = control_fault if control_fault else (None, None, None)
    text = "short biped_riding_player(struct biped_datum const *biped);\n"
    text += "".join(function(port, name) + "\n\n" for name in ("biped_riding_get", "biped_riding_set", "biped_riding_clear"))
    text += ("static long get_calls, set_calls, air_begin_calls, cabin_begin_calls;\n"
             "#define biped_riding_get(...) (get_calls++, biped_riding_get(__VA_ARGS__))\n"
             "#define biped_riding_set(...) (set_calls++, biped_riding_set(__VA_ARGS__))\n"
             "#define vehicle_riding_air_begin(...) (air_begin_calls++, vehicle_riding_air_begin(__VA_ARGS__))\n"
             "#define vehicle_riding_cabin_begin(...) (cabin_begin_calls++, vehicle_riding_cabin_begin(__VA_ARGS__))\n")
    code = "".join(function(port, name) + "\n\n" for name in (
        "biped_riding_applies", "biped_riding_player", "biped_riding_zero3", "biped_riding_fields_clear",
        "biped_riding_idle", "biped_riding_deferred", "biped_riding_falling_velocity"))
    code += contacts_head(port) + "\n" + collision_head(port) + "\n"
    if which == "bipeds":
        code = mutated(code, before, after)
    seated = function(port, "biped_riding_seated")
    if which == "seated":
        seated = mutated(seated, before, after)
    reference = mutated(seated.replace("void biped_riding_seated(", "void reference_seated(", 1), "\tif (biped_riding_fields_clear(",
                        "\tif (FALSE && biped_riding_fields_clear(")
    text += code + seated + "\n\n" + reference + "\n\n"
    declarations, prepare, finish = tick_slice(biped_riding_sources())
    fast_prepare = prepare
    fast_finish = finish
    if which == "fast":
        fast_prepare = mutated(prepare, before, after)
    if which == "finish":
        fast_finish = mutated(finish, before, after)
    reference_prepare = mutated(prepare, GATE, "\tif (FALSE && biped_riding_idle(")
    text += tick_function("idle_tick", declarations, fast_prepare, fast_finish) + "\n"
    text += tick_function("reference_tick", declarations, reference_prepare, finish)
    return text


def generated(control=None):
    which = NEGATIVE_CONTROLS[control][0][0] if control else None
    fault = NEGATIVE_CONTROLS[control][0] if control else None
    riding = dict(riding_generated(fault[1:] if which == "riding" else None))
    header = read(HEADER)
    objects = read("source/objects/objects.h")
    types = enum_with(objects, "_object_dead_bit") + "\n"
    types += enum_with(read("source/units/bipeds.h"), "_biped_airborne_bit") + "\n"
    types += enum_with(read("source/units/units.h"), "_unit_suspended_bit") + "\n"
    types += enum_with(header, "_biped_physics_out_airborne_bit") + "\n"
    types += structure(header, "biped_physics") + "\n"
    return (("enums.inc", riding["enums.inc"]), ("engine.inc", riding["engine.inc"]),
            ("under_test.inc", riding["under_test.inc"]), ("idle_types.inc", types),
            ("bipeds.inc", bipeds_text(fault if which in ("bipeds", "seated", "fast", "finish") else None)))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("riding_idle", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    status, output = run(build("riding_idle", generated(control)), NEGATIVE_CONTROLS[control][1])
    assert status == CHECK_FAILED, f"a fault ({control}) passed '{NEGATIVE_CONTROLS[control][1]}': the test cannot see it: {output}"


def field_names(source, name):
    body = structure(source, name).split("{", 1)[1].rsplit("}", 1)[0]
    return re.findall(r"^\t(?:struct |const )?[\w ]*?[ *](\w+)(?:\[\w+\])?;", body, re.M)


def test_cleared_checks_name_every_field():
    """A field added to the player's tables must join the idle check, or the fast path would skip clearing it."""
    riding = read(RIDING)
    header = read("port/linux/game/vehicle_riding.h")
    for name, structures in CLEARED_STRUCTURES.items():
        body = function(riding, name)
        for struct, prefix in structures:
            fields = field_names(header, struct)
            assert fields, struct
            for field in fields:
                if struct == "vehicle_riding_mark" and field == "correction":
                    continue
                assert re.search(r"(?:real_same\()?" + re.escape(prefix) + field + r"[\w.]*(?: ==|,)", body), f"{name} leaves out {struct}.{field}"


def test_fields_clear_names_every_stored_field():
    """Every riding field biped_riding_set stores is one biped_riding_fields_clear compares."""
    port = read(BIPEDS)
    stored = re.findall(r"biped->biped\.(riding_\w+) = ", function(port, "biped_riding_set"))
    assert len(stored) >= 14, stored
    body = function(port, "biped_riding_fields_clear")
    for field in stored:
        assert re.search(r"biped->biped\." + field + r"\b", body), f"biped_riding_fields_clear leaves out {field}"


if __name__ == "__main__":
    sys.exit(pytest.main(["-q", __file__]))
