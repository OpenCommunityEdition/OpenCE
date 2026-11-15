"""The real sergeant_johnson_map, sergeant_johnson_leader_variant_in and sergeant_johnson_scenario_variant_in
(sergeant_johnson.c): a marine squad's leader is Sergeant Johnson or Sergeant Lehto at random, so Johnson led marines
where a10's and a50's story does not have him. a10's and a50's leaders are never Johnson; a50's opening has him as its
last marine out of the Pelican, and the gravity lift cinematic does not. The game calls the decisions where it places
a squad's marines and a scenario's objects."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run  # noqa: E402

CASES = ["maps", "leaders", "other maps", "a50 objects"]

FUNCTIONS = ["sergeant_johnson_map", "sergeant_johnson_leader_variant_in", "sergeant_johnson_scenario_variant_in"]

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # a10's Johnson kept
    "a10-kept": (("(map == _sergeant_johnson_map_a10 || map == _sergeant_johnson_map_a50) &&",
                  "map == _sergeant_johnson_map_a50 &&"), "leaders"),
    # a50's leaders left as the game has them
    "a50-kept": (("(map == _sergeant_johnson_map_a10 || map == _sergeant_johnson_map_a50) &&",
                  "map == _sergeant_johnson_map_a10 &&"), "leaders"),
    # every leader Lehto, any map
    "any-map": (("if ((map == _sergeant_johnson_map_a10 || map == _sergeant_johnson_map_a50) &&\n\t\tvariant",
                 "if ((1) &&\n\t\tvariant"), "other maps"),
    # any of a50's marines Johnson
    "any-marine": (('&&\n\t\t!csstrcmp(object_name, "ini_marine_7"))', ")"), "a50 objects"),
    # any biped of the name
    "any-definition": (('!csstrcmp(definition_name, "characters\\\\marine_armored\\\\marine_armored") &&',
                        "1 &&"), "a50 objects"),
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
    """The leaders and the scenario's objects go through sergeant_johnson.c."""
    encounters = function(read("source/ai/encounters.c").replace("\r\n", "\n"), "encounter_create")
    leader = encounters.index("initial_variant = sergeant_johnson_leader_variant(initial_variant);")
    assert leader > encounters.index("case _unique_leader_type_sergeant_lehto:")
    assert leader < encounters.index("encounter_place_actor(encounter_index, squad_index, initial_variant")
    placed = function(read("source/objects/objects.c").replace("\r\n", "\n"), "object_new_from_scenario")
    variant = placed.index("placement_data.variant_number = sergeant_johnson_scenario_variant(")
    assert placed.index("placement_data.variant_number = scenario_object->variant_number;") < variant
    assert variant < placed.index("result = object_new(&placement_data);")


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("sergeant_johnson", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("sergeant_johnson", generated(fault)), case)
    assert status == CHECK_FAILED, output
