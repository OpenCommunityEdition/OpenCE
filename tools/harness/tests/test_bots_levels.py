"""The bots' skill levels and their fight by level: bots.c's table of the six levels (easy, normal, heroic and legendary
as they were, each level better than the one below), bots.skill as bots.c reads it, and bots_combat.c's rules
(reaction, aim, firing, blows, grenades, retreat, moves), compiled as they are. A wiring check: bots.c's fight asks
every rule and uses every part of its fight by level (none is left unused)."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run, structure  # noqa: E402

CASES = ["levels-kept", "levels-ordered", "settings", "reaction", "aim", "firing", "melee", "grenades", "retreat",
         "moves"]

# a fault in bots.c (the table, the settings) or bots_combat.c (the rules), and the case that must catch it
NEGATIVE_CONTROLS = {
    "normal-retuned": ("bots.c", ('{ "normal", 0.45f', '{ "normal", 0.49f'), "levels-kept"),
    "legendary-retuned": ("bots.c", ('{ "legendary", 0.18f, 55.0f', '{ "legendary", 0.18f, 50.0f'), "levels-kept"),
    "veteran-above-legendary": ("bots.c", ('{ "veteran", 0.24f', '{ "veteran", 0.16f'), "levels-ordered"),
    "unknown-skill-not-normal": ("bots.c", ("\n\treturn _bots_skill_normal;\n}", "\n\treturn _bots_skill_easy;\n}"),
                                 "settings"),
    "recruit-reacts-quicker": ("bots_combat.c", ("skill == _bots_skill_recruit ? 1.0f : 0.85f", "0.85f"), "reaction"),
    "normal-aim-trails": ("bots_combat.c", ("skill == _bots_skill_easy ? 6 : 0", "skill == _bots_skill_easy ? 6 : 2"),
                          "aim"),
    "recruit-aim-settles": ("bots_combat.c", ("\tif (magnitude < low)\n\t\tvalue += sign * 0.5f * step;\n", ""), "aim"),
    "legendary-floor-at-range": ("bots_combat.c", ("1.6f, 1.2f, 0.0f }", "1.6f, 1.2f, 1.0f }"), "aim"),
    "vertical-view-unlimited": ("bots_combat.c", ("return off <= bots_combat_vertical_half_view(skill);",
                                                  "return off <= 90.0f;"), "aim"),
    "paced-close-up": ("bots_combat.c", ("distance <= 8.0f)", "distance <= 0.0f)"), "firing"),
    "heroic-blows-anywhere": ("bots_combat.c", ("reach = 0.6f;", "reach = 1.1f;"), "melee"),
    "recruit-throws": ("bots_combat.c", ("\tcase _bots_skill_recruit:\n\t\treturn 0;\n"
                                         "\tcase _bots_skill_easy:\n\t\trate",
                                         "\tcase _bots_skill_easy:\n\t\trate"), "grenades"),
    "easy-retreats": ("bots_combat.c", ("return skill >= _bots_skill_normal && skill < NUMBER_OF_BOTS_SKILLS ? 0.1f",
                                        "return skill >= _bots_skill_easy && skill < NUMBER_OF_BOTS_SKILLS ? 0.1f"),
                      "retreat"),
    "no-retreat-as-shields-pop": ("bots_combat.c", ("popped = skill >= _bots_skill_veteran &&", "popped = 0 &&"),
                                  "retreat"),
    "easy-halts": ("bots_combat.c", ("\tcase _bots_skill_recruit:\n\tcase _bots_skill_easy:\n\t\treturn 0;\n"
                                     "\tcase _bots_skill_normal: chance",
                                     "\tcase _bots_skill_normal: chance"), "moves"),
    "crouches-without-a-wait": ("bots_combat.c", ("if (now < state->next || random >= chance / TICKS)",
                                                  "if (random >= chance / TICKS)"), "moves"),
    "low-ground-credited": ("bots_combat.c", ("\tif (height_above <= 0.0f)\n\t\treturn 0.0f;\n", ""), "moves"),
}

SKILL_FUNCTIONS = ("bots_skill",)


def generated(fault_file=None, fault=None):
    bots = read("port/linux/game/bots.c")
    rules = read("port/linux/game/bots_combat.c")
    if fault_file == "bots.c":
        bots = mutated(bots, *fault)
    elif fault_file == "bots_combat.c":
        rules = mutated(rules, *fault)
    table = re.search(r"^static struct bots_skill_definition const bots_skills\[NUMBER_OF_BOTS_SKILLS\] =\n\{.*?\n\};",
                      bots, re.M | re.S)
    assert table, "the skill table not found in bots.c"
    skills = structure(bots, "bots_skill_definition") + "\n" + table.group(0) + "\n"
    for name in SKILL_FUNCTIONS:
        skills += function(bots, name) + "\n"
    return (("bots_combat.h", read("port/linux/game/bots_combat.h")), ("under_test.inc", rules),
            ("skills.inc", skills))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("bots_levels", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault_file, fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("bots_levels", generated(fault_file, fault)), case)
    assert status == CHECK_FAILED, f"the levels with a fault ({control}) passed '{case}': the test cannot see it"


RULE = re.compile(r"^(?:[\w ]+?)\*?\b(bots_combat_\w+)\(", re.M)
LEVEL_PART = re.compile(r"^static [\w ]+?\*?\b(bots_level_\w+|bots_roam_higher)\(", re.M)


def unused(bots, rules_header, rules):
    """the rules (bots_combat.h) and the parts of bots.c's fight by level (bots_level_*) that nothing calls: every
    one is either asked by bots.c or by another rule"""
    found = []
    for name in sorted(set(RULE.findall(rules_header))):
        calls = len(re.findall(r"\b" + name + r"\(", bots)) + len(re.findall(r"\b" + name + r"\(", rules)) - 1
        if calls < 1:
            found.append(name)
    for name in sorted(set(LEVEL_PART.findall(bots))):
        if len(re.findall(r"\b" + name + r"\(", bots)) < 2:
            found.append(name)
    return found


def wiring(bots):
    return unused(bots, read("port/linux/game/bots_combat.h"), read("port/linux/game/bots_combat.c"))


def test_every_level_rule_is_asked():
    assert wiring(read("port/linux/game/bots.c")) == []


def test_wiring_check_sees_a_rule_left_unasked():
    bots = mutated(read("port/linux/game/bots.c"), "\t\tbots_level_prenade(bot, &tick);\n", "")
    assert "bots_level_prenade" in wiring(bots), "the wiring check passed a part of the fight that nothing calls"
