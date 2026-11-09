"""The real needler_reload_rounds_loaded (first_person_weapons.c): a needler's reload draws its needles growing toward
those it will hold when the reload ends, as weapon_magazine_finish_reload loads them (weapons.c), not toward the rounds
left alone, which with fewer than 20 left sank the needles during the reload and popped them up as it ended."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run, structure  # noqa: E402

CASES = ["plenty left", "few left", "magazine rules"]

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # toward the rounds left alone (before fix-needler-reload-needles)
    "rest-alone": (("long rounds_loaded = TEST_FLAG(magazine_definition->flags, 0) ? 0 : magazine->rounds_loaded;",
                    "long rounds_loaded = 0;"), "few left"),
    # a wasting magazine's rounds kept
    "waste-kept": (("TEST_FLAG(magazine_definition->flags, 0) ? 0 : magazine->rounds_loaded", "magazine->rounds_loaded"),
                   "magazine rules"),
    # all the rest loaded at once
    "all-at-once": (("MIN(magazine->rounds_total, magazine_definition->rounds_reloaded)", "magazine->rounds_total"),
                    "magazine rules"),
}


def generated(fault=None):
    source = read("source/interface/first_person_weapons.c").replace("\r\n", "\n")
    code = structure(read("source/items/weapons.h").replace("\r\n", "\n"), "weapon_magazine") + "\n"
    code += function(source, "needler_reload_rounds_loaded") + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("under_test.inc", code),)


def test_reload_draws_it():
    """The needler's reload grows its needles toward needler_reload_rounds_loaded's count."""
    source = read("source/interface/first_person_weapons.c").replace("\r\n", "\n")
    assert "predicted_frame_index= needler_reload_rounds_loaded(\n" \
           "\t\t\t\t\t\t\t\t&weapon->weapon.magazines[0],\n" \
           "\t\t\t\t\t\t\t\tmagazine_definition);" in source


def test_as_the_reload_loads():
    """weapon_magazine_finish_reload still loads as needler_reload_rounds_loaded counts."""
    weapons = read("source/items/weapons.c").replace("\r\n", "\n")
    finish = function(weapons, "weapon_magazine_finish_reload")
    assert "if (TEST_FLAG(magazine_definition->flags, 0))\n\t\tmagazine->rounds_loaded = 0;" in finish
    assert "rounds_to_load = magazine_definition->rounds_reloaded>magazine->rounds_total ? " \
           "magazine->rounds_total : magazine_definition->rounds_reloaded;" in finish
    assert "rounds_loaded = magazine->rounds_loaded+rounds_to_load;\n" \
           "\tif (rounds_loaded>magazine_definition->rounds_loaded_maximum)\n" \
           "\t\trounds_loaded = magazine_definition->rounds_loaded_maximum;" in finish


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("needler_reload", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("needler_reload", generated(fault)), case)
    assert status == CHECK_FAILED, f"needler_reload_rounds_loaded with a fault ({control}) passed '{case}'"
