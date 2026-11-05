"""The real active_camouflage_next_part and active_camouflage_transparent_part
(source/rasterizer/xbox/rasterizer_xbox_transparent_geometry.c): the player's own legs, cloaked by the unit that
cloaks the first-person weapon, end their cloak in the world, and the weapon's display is drawn once, with the
weapon, even when its muzzle flash comes between the legs and the weapon; another cloak's display is drawn with its
own cloak, as on the Xbox."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run  # noqa: E402
from harness.render_types import transparent_types  # noqa: E402

CASES = ["fired", "legs-then-weapon", "no-legs", "another-cloak"]

VIEW = ("TEST_FLAG({0}->geometry_flags, _rasterizer_geometry_first_person_bit) ==\n"
        "\t\t\tTEST_FLAG(group->geometry_flags, _rasterizer_geometry_first_person_bit)")

# a fault in the code, and the case that must catch it
NEGATIVE_CONTROLS = {
    # a cloak goes on into another view: the legs and the weapon one cloak, the legs' part drawn with the weapon
    "next-part-any-view": (("_shader_type_model &&\n\t\t" + VIEW.format("next_group") + ";",
                            "_shader_type_model;"), "legs-then-weapon"),
    # a cloak's end draws the transparent parts of either view: the display drawn with the legs too
    "transparent-part-any-view": (("_render_model_effect_type_active_camouflage &&\n\t\t" + VIEW.format("part") + ";",
                                   "_render_model_effect_type_active_camouflage;"), "fired"),
    # a cloak's end draws any object's transparent parts (not the change: the Xbox's own test)
    "transparent-part-any-object": (("part->active_camouflage_transparent_source_object_index == "
                                     "group->source_object_index &&", ""), "another-cloak"),
}


def generated(fault=None):
    raster = read("source/rasterizer/xbox/rasterizer_xbox_transparent_geometry.c")
    types = "\n".join([transparent_types(), enum_with(raster, "_render_model_effect_type_cortana")]) + "\n"
    code = "\n".join(function(raster, name) for name in
                     ("active_camouflage_next_part", "active_camouflage_transparent_part")) + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("types.inc", types), ("under_test.inc", code))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("camo_view_batches", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("camo_view_batches", generated(fault)), case)
    assert status == CHECK_FAILED, f"the code with a fault ({control}) passed '{case}': the test cannot see it"
