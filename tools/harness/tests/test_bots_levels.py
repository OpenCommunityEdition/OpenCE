"""The bots' skill levels: bots.c's table of the six levels (easy, normal, heroic and legendary as they were, each
level better than the one below) and bots.skill as bots.c reads it."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run, structure  # noqa: E402

CASES = ["levels-kept", "levels-ordered", "settings"]

# a fault in bots.c (the table, the setting), and the case that must catch it
NEGATIVE_CONTROLS = {
    "normal-retuned": ("bots.c", ('{ "normal", 0.45f', '{ "normal", 0.49f'), "levels-kept"),
    "legendary-retuned": ("bots.c", ('{ "legendary", 0.18f, 55.0f', '{ "legendary", 0.18f, 50.0f'), "levels-kept"),
    "veteran-above-legendary": ("bots.c", ('{ "veteran", 0.24f', '{ "veteran", 0.16f'), "levels-ordered"),
    "unknown-skill-not-normal": ("bots.c", ("\n\treturn _bots_skill_normal;\n}", "\n\treturn _bots_skill_easy;\n}"),
                                 "settings"),
}

SKILL_FUNCTIONS = ("bots_skill",)


def generated(fault_file=None, fault=None):
    bots = read("port/linux/game/bots.c")
    if fault_file == "bots.c":
        bots = mutated(bots, *fault)
    table = re.search(r"^static struct bots_skill_definition const bots_skills\[NUMBER_OF_BOTS_SKILLS\] =\n\{.*?\n\};",
                      bots, re.M | re.S)
    assert table, "the skill table not found in bots.c"
    skills = enum_with(bots, "_bots_skill_recruit") + "\n" + structure(bots, "bots_skill_definition") + "\n" + \
        table.group(0) + "\n"
    for name in SKILL_FUNCTIONS:
        skills += function(bots, name) + "\n"
    return (("skills.inc", skills),)


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("bots_levels", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault_file, fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("bots_levels", generated(fault_file, fault)), case)
    assert status == CHECK_FAILED, f"the levels with a fault ({control}) passed '{case}': the test cannot see it"
