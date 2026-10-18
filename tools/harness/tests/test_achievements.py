"""The rules of achievements.c, fed the events the game's hooks give it: wins and difficulty, the checkpoint's put-back
counts, latched deaths, strict time, disqualification, the profile's counts and file."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, mutated, read, run  # noqa: E402

CASES = ["birth", "off", "taint", "difficulty", "health-packs", "revert", "deaths-latched", "time-strict",
         "maw-timer", "resumed", "campaign", "coop", "counts", "lifetime", "weapons", "damage", "stealth", "beach",
         "kills", "riding", "day", "file"]

# a fault in achievements.c and the case that must catch it
NEGATIVE_CONTROLS = {
    "revert-keeps-packs": (("memcpy(ach.run, ach.saved_run, sizeof(ach.run));", ""), "revert"),
    "deaths-restored": (("ach.player[local_player].deaths++;", "ach.player[local_player].deaths = 0;"),
                        "deaths-latched"),
    "time-rewound": (("	ach.shared = ach.saved_shared;\n}", "	ach.shared = ach.saved_shared;\n	ach.level_ticks = 0;\n}"),
                     "time-strict"),
    "taint-ignored": (("		ach.tainted = 1;", "		ach.tainted = 0;"), "taint"),
    "heroic-is-normal": (("_ach_level_a10, _ach_heroic, 1, 0 },\n	{ \"Shielded\"",
                          "_ach_level_a10, _ach_normal, 1, 0 },\n	{ \"Shielded\""), "difficulty"),
    "popcorn-49": (("infection_forms_shotgunned >= 50", "infection_forms_shotgunned >= 49"), "counts"),
    "fold-lost": (("		profile->headshots += (ach_u32)run->headshots;", "		profile->headshots += 0;"),
                  "lifetime"),
    "resumed-ignored": (("	if (definition->whole_level && ach.history_incomplete)\n		return 0;", ""), "resumed"),
    "file-no-crc": (("|| ach_get(bytes, 200, 4) != ach_crc32(bytes, 200))", ")"), "file"),
}


def generated(fault=None):
    text = read("port/linux/game/achievements.c")
    if fault:
        text = mutated(text, *fault)
    return (("under_test.inc", "#define ACHIEVEMENTS_RULES_ONLY\n" + text),)


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("achievements", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("name", NEGATIVE_CONTROLS)
def test_negative_control(name):
    fault, case = NEGATIVE_CONTROLS[name]
    status, output = run(build("achievements", generated(fault)), case)
    assert status == CHECK_FAILED, f"{name} was not caught by {case}: {output}"
