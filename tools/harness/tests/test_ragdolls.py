"""Deaths for the ragdolls (ragdolls.c): a hit on a biped remembered for its tick (its direction, the speed of the
damage's instantaneous acceleration as the damage is scaled, its point); a biped with a body that dies queues its node
matrices, its velocity and the hit that killed it, once and no more than there is room for; nothing with
game.ragdolls off, for other objects or for bipeds without a body. The game calls in from object_damage_aftermath and
from unit_died, for a death that is not feigned."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run, structure  # noqa: E402

CASES = ["death", "killing hit", "newest hit", "stale hit", "bad numbers", "off", "not biped", "no body", "once",
         "capacity"]

FUNCTIONS = ["ragdolls_enabled", "ragdolls_object_is_biped", "ragdolls_damage", "ragdolls_hit_get",
             "ragdolls_unit_died", "ragdolls_death_count", "ragdolls_death_get", "ragdolls_deaths_clear",
             "ragdolls_dispose_from_old_map"]

# a fault in the deaths and the case that must catch it
NEGATIVE_CONTROLS = {
    "always-on": (("if (!ragdolls_enabled() || unit_index == NONE", "if (unit_index == NONE"), "off"),
    "any-object": (("unit_index == NONE || !ragdolls_object_is_biped(unit_index))", "unit_index == NONE)"),
                   "not biped"),
    "hits-never-stale": (("hit->object_index == object_index && hit->time == time", "hit->object_index == object_index"),
                         "stale hit"),
    "twice": (("if (ragdolls.deaths[index].object_index == unit_index)\n\t\t\treturn;", ""), "once"),
    "no-capacity": (("if (!body || ragdolls.death_count >= RAGDOLL_DEATHS_MAX)", "if (!body)"), "capacity"),
    "unscaled": (("(zero_scale + (1.0f - zero_scale) * PIN(damage->scale, 0.0f, 1.0f))", "(zero_scale * 0.0f + 1.0f)"), "killing hit"),
    "unnormalised": (("scale_vector3d(&hit->direction, 1.0f / length, &hit->direction);", ""), "killing hit"),
    "no-nan-check": (("if (length > _real_epsilon && hit->speed == hit->speed)", "if (length > _real_epsilon)"),
                     "bad numbers"),
    "oldest-first": (("(ragdolls.hit_count - index) % RAGDOLL_HITS_MAX", "(index - 1) % RAGDOLL_HITS_MAX"), "newest hit"),
}


def generated(fault=None):
    ragdolls = read("port/linux/game/ragdolls.c")
    header = read("port/linux/game/ragdolls.h")
    bodies = read("port/linux/game/ragdoll_bodies.h")
    state = ragdolls[ragdolls.index("static struct\n{"):ragdolls.index("} ragdolls;") + len("} ragdolls;")]
    config = "\n".join([
        enum_with(bodies, "RAGDOLL_BODY_PARTS_MAX"), structure(bodies, "ragdoll_part"), structure(bodies, "ragdoll_body"),
        enum_with(header, "RAGDOLL_DEATHS_MAX"), structure(header, "ragdoll_hit"), structure(header, "ragdoll_death"),
        "static boolean physics_jolt_ragdolls_enabled(void);",
        "static struct ragdoll_body const *ragdoll_body_get(long definition_index);",
        state]) + "\n"
    code = "\n".join(function(ragdolls, name) for name in FUNCTIONS)
    if fault:
        code = mutated(code, *fault)
    return (("config.inc", config), ("under_test.inc", code + "\n"))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("ragdolls", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("name", sorted(NEGATIVE_CONTROLS))
def test_negative_control(name):
    fault, case = NEGATIVE_CONTROLS[name]
    status, output = run(build("ragdolls", generated(fault)), case)
    assert status == CHECK_FAILED, output


def test_the_game_calls_in():
    """the hooks, each marked `port:`, declared for the game's sources"""
    died = function(read("source/units/units.c"), "unit_died")
    not_feigned = died[died.index("if (!feigned)\n\t{\n\t\tobject_set_garbage"):]
    not_feigned = not_feigned[:not_feigned.index("\n\t}\n")]
    assert re.search(r"/\* port:[^*]*\*/\n\t\tragdolls_unit_died\(unit_index\);", not_feigned)
    aftermath = function(read("source/objects/damage.c"), "object_damage_aftermath")
    assert re.search(r"/\* port:[^*]*\*/\n\tragdolls_damage\(object_index, damage\);", aftermath)
    fixups = read("port/linux/include/halo_linux_source_fixups.h")
    assert "void ragdolls_damage(long object_index, struct damage_data const *damage);" in fixups
    assert "void ragdolls_unit_died(long unit_index);" in fixups


def test_deaths_go_with_the_map():
    jolt = function(read("port/linux/game/physics_jolt.c"), "physics_jolt_dispose_from_old_map")
    assert "ragdolls_dispose_from_old_map();" in jolt
