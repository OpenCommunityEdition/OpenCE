"""The real sergeant_johnson_map, sergeant_johnson_leader_variant_in and sergeant_johnson_scenario_variant_in
(sergeant_johnson.c): a marine squad's leader is Sergeant Johnson or Sergeant Lehto at random, so Johnson led marines
where a10's and a50's story does not have him. a10's leaders are never Johnson; a50's opening has him as its last
marine out of the Pelican, and the gravity lift cinematic and room have him only if he made it to the lift. The game
calls the decisions where it places a squad's marines and a scenario's objects, before ai_erase_all erases, when a
unit dies and for a new map."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run  # noqa: E402

CASES = ["maps", "a10 leaders", "a50 leaders", "other maps", "a50 objects"]

FUNCTIONS = ["sergeant_johnson_map", "sergeant_johnson_leader_variant_in", "sergeant_johnson_scenario_variant_in"]

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # a10's Johnson kept
    "a10-kept": (("(map == _sergeant_johnson_map_a10 || map == _sergeant_johnson_map_a50) &&",
                  "map == _sergeant_johnson_map_a50 &&"), "a10 leaders"),
    # the room's leader Johnson though he died
    "room-always": (("return alive ? JOHNSON_VARIANT : LEHTO_VARIANT;", "return JOHNSON_VARIANT;"), "a50 leaders"),
    # any marine of the room made a leader
    "no-leader-check": (("\tif (variant != JOHNSON_VARIANT && variant != LEHTO_VARIANT)\n\t\treturn variant;\n", ""),
                        "a50 leaders"),
    # a50's other leaders left as the game has them
    "a50-kept": (("(map == _sergeant_johnson_map_a10 || map == _sergeant_johnson_map_a50) &&",
                  "map == _sergeant_johnson_map_a10 &&"), "a50 leaders"),
    # the lift's first marine Johnson though he died
    "lift-always": (('!csstrcmp(object_name, "lift_marine_1") && alive', '!csstrcmp(object_name, "lift_marine_1")'),
                    "a50 objects"),
    # any biped of the name
    "any-definition": (("||\n\t\tcsstrcmp(definition_name, ", "||\n\t\t0 * csstrcmp(definition_name, "), "a50 objects"),
    # the map's name matched by its start
    "map-prefix": (('!csstrcmp(scenario_name, "levels\\\\a10\\\\a10")',
                    '!strncmp(scenario_name, "levels\\\\a10\\\\a10", 14)'), "maps"),
}


def generated(fault=None):
    source = read("port/linux/game/sergeant_johnson.c").replace("\r\n", "\n")
    header = read("port/linux/game/sergeant_johnson.h").replace("\r\n", "\n")
    defines = "\n".join(re.findall(r"^#define (?:JOHNSON|LEHTO)_VARIANT .*$", source, re.M))
    code = "\n\n".join([enum_with(header, "_sergeant_johnson_map_other"), defines] +
                       [function(source, name) for name in FUNCTIONS]) + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("under_test.inc", code),)


def test_the_game_calls_them():
    """The leaders, the scenario's objects, the erase, the deaths and a new map go through sergeant_johnson.c."""
    encounters = function(read("source/ai/encounters.c").replace("\r\n", "\n"), "encounter_create")
    leader = encounters.index("initial_variant = sergeant_johnson_leader_variant(encounter_definition->name, "
                              "initial_variant);")
    assert leader > encounters.index("case _unique_leader_type_sergeant_lehto:")
    assert leader < encounters.index("encounter_place_actor(encounter_index, squad_index, initial_variant")
    objects = read("source/objects/objects.c").replace("\r\n", "\n")
    placed = function(objects, "object_new_from_scenario")
    variant = placed.index("placement_data.variant_number = sergeant_johnson_scenario_variant(")
    assert placed.index("placement_data.variant_number = scenario_object->variant_number;") < variant
    assert variant < placed.index("result = object_new(&placement_data);")
    assert "sergeant_johnson_reset();" in function(objects, "objects_initialize_for_new_map")
    erase = function(read("source/ai/ai_script.c").replace("\r\n", "\n"), "ai_scripting_erase_all")
    assert erase.index("sergeant_johnson_erasing_all();") < erase.index("ai_erase(NONE, NONE, NONE, FALSE);")
    died = function(read("source/units/units.c").replace("\r\n", "\n"), "unit_died")
    assert died.index("sergeant_johnson_unit_died(unit_index);") < died.index("actor_died(unit->unit.actor_index);")


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("sergeant_johnson", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("sergeant_johnson", generated(fault)), case)
    assert status == CHECK_FAILED, output
