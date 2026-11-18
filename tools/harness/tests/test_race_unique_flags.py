"""The real race_flags_make_unique and new_rally_flag (game_engine_race.c): a race track's flag whose number another
has is given a free number no later flag shares, so a lap has every flag of the map and flag rally can choose each;
the original marked the flag's own number rather than the one it gave."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, enum_with, function, mutated, read, run, structure  # noqa: E402

CASES = ["duplicate-then-next", "three-of-one", "unique-unchanged", "rally"]

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # the original's: the flag's own number marked again, not the one given
    "own-number-marked": (("SET_FLAG(used_flags, free_index, TRUE);", "SET_FLAG(used_flags, flag->team_index, TRUE);"),
                          "duplicate-then-next"),
    "own-number-rally": (("SET_FLAG(used_flags, free_index, TRUE);", "SET_FLAG(used_flags, flag->team_index, TRUE);"),
                         "rally"),
    # a number of its own not marked used
    "unique-unmarked": (("else\n\t\t\t{\n\t\t\t\tSET_FLAG(used_flags, flag->team_index, TRUE);", "else\n\t\t\t{\n\t\t\t\t;"),
                        "three-of-one"),
}


def generated(fault=None):
    source = read("source/game/game_engine_race.c")
    definitions = read("source/scenario/scenario_definitions.h")
    config = "\n".join([
        "enum { MAXIMUM_RACE_FLAGS = %d, LONG_BITS = %d };" % (
            constant(source, "MAXIMUM_RACE_FLAGS"), constant(read("source/cseries/cseries.h"), "LONG_BITS")),
        "#define NUMBEROF_NUMBERS(a) ((long)(sizeof(a) / sizeof((a)[0])))",
        enum_with(definitions, "_netgame_flag_race_track,"),
        structure(definitions, "scenario_netgame_flag"),
    ]) + "\n"
    code = "\n".join(function(source, name) for name in ("count_bits_32", "race_flags_make_unique", "new_rally_flag"))
    if fault:
        code = mutated(code, *fault)
    return (("config.inc", config), ("under_test.inc", code + "\n"))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("race_unique_flags", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("race_unique_flags", generated(fault)), case)
    assert status == CHECK_FAILED, f"the race flags with a fault ({control}) passed '{case}': the test cannot see it"
