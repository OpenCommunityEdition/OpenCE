"""The real overshield_material_tags_loaded (overshield_material.c): the overshield's collision material, one named
"force field" whose type the maps left dirt, is made an engineer's force field, as the active camouflage's is, so a
shot at it plays the powerups' energy, not dirt's dust; any other material, powerup or type is left as the map has it."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run, structure  # noqa: E402

CASES = ["maps", "overshield only", "other materials", "not an overshield", "no collision"]

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # the material left as it was
    "not-fixed": (("\tmaterial->material_type = _material_engineer_force_field;\n", ""), "maps"),
    # any material left dirt, whatever its name
    "any-name": (("strncmp(material->name, OVERSHIELD_MATERIAL_NAME, sizeof(material->name)) != 0", "0"),
                 "other materials"),
    # a force field of any type
    "any-type": (("material->material_type != _material_dirt ||", ""), "other materials"),
    # any powerup's
    "any-powerup": (("equipment->equipment.powerup_type != _equipment_powerup_overshield ||", ""),
                    "not an overshield"),
    # the first material only
    "first-only": (("material_index < collision_model->resistance.materials.count", "material_index < 1"),
                   "other materials"),
}


def generated(fault=None):
    source = read("port/linux/game/overshield_material.c").replace("\r\n", "\n")
    materials = enum_with(read("source/game/game_globals.h").replace("\r\n", "\n"), "_material_dirt")
    materials = materials.replace("GAME_GLOBALS_TAG = 'matg',", "")
    code = "\n\n".join([
        materials,
        enum_with(read("source/items/equipment_definitions.h").replace("\r\n", "\n"), "_equipment_powerup_overshield"),
        structure(read("source/objects/damage_resistances.h").replace("\r\n", "\n"), "damage_resistance_material"),
        '#define OVERSHIELD_MATERIAL_NAME "force field"',
        function(source, "overshield_material_fixed"),
        function(source, "overshield_material_tags_loaded"),
    ]) + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("under_test.inc", code),)


def test_tags_loaded_fix_it():
    """An Xbox map's tags, as they load, have the overshield's material fixed, before the mods' tags play over them."""
    loader = function(read("source/cache/cache_files.c").replace("\r\n", "\n"), "scenario_tags_load")
    bounds = loader.index("powerup_render_bounds_tags_loaded();")
    fixed = loader.index("overshield_material_tags_loaded();")
    assert bounds < fixed < loader.index("mods_tags_loaded(stripped_scenario_name);", bounds)
    assert loader.count("overshield_material_tags_loaded();") == 1
    assert '#define OVERSHIELD_MATERIAL_NAME "force field"' in read("port/linux/game/overshield_material.c")


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("overshield_material", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("overshield_material", generated(fault)), case)
    assert status == CHECK_FAILED, f"overshield_material with a fault ({control}) passed '{case}'"
