"""The real intro, rows, seats and exits of network co-op's cryo bay (port/linux/game/coop_cryo.c)."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run, structure  # noqa: E402

CASES = ["hoods", "shots", "rows", "exit-spot", "exits", "seating", "seating-crowded", "seating-filled"]
FUNCTIONS = ("cryo_tube_frame", "cryo_tube_of", "cryo_intro_tick", "cryo_exits_update", "cryo_arm",
             "coop_cryo_exit_seat_end")

# a fault, where it is ("config" or a function), and the case that must catch it
NEGATIVE_CONTROLS = {
    "hood-in-the-white": (("CRYO_NORTH_OPEN = CRYO_WHITE_IN_TICKS,", "CRYO_NORTH_OPEN = CRYO_WHITE_IN_TICKS - 6,"),
                          "config", "hoods"),
    "pan-too-soon": (("CRYO_PAN_WAIT_TICKS = CRYO_HOOD_LIFT_TICKS + 72,", "CRYO_PAN_WAIT_TICKS = CRYO_HOOD_LIFT_TICKS + 52,"),
                     "config", "hoods"),
    "rows-face-out": (("forward->j = north ? -1.0f : 1.0f;", "forward->j = north ? 1.0f : -1.0f;"),
                      "cryo_tube_frame", "rows"),
    "stands-short": (("#define CRYO_EXIT_DISTANCE 0.849f", "#define CRYO_EXIT_DISTANCE 0.749f"), "config",
                     "exit-spot"),
    "climbs-out-late": (("random_range(0, CRYO_EXIT_JITTER_TICKS + 1)", "random_range(0, CRYO_EXIT_JITTER_TICKS + 30)"),
                        "cryo_exits_update", "exits"),
    "seated-twice": (("waiting[pick] = waiting[--waiting_count];", "--waiting_count;"), "cryo_arm", "seating"),
    "scripts-seat-given-away": (("(seat->unit_index = cryo_occupant(seat->vehicle_index)) != NONE",
                                 "(seat->unit_index = NONE) != NONE"), "cryo_arm", "seating-filled"),
}


def generated(fault=None, where=None):
    source = read("port/linux/game/coop_cryo.c")
    config = enum_with(source, "CRYO_ROWS") + "\n" + enum_with(source, "_cryo_camera_north_start") + "\n"
    for name in ("CRYO_ROW_Y", "CRYO_TUBE_Z", "CRYO_EXIT_DISTANCE"):
        match = re.search(r"^#define " + name + r" .*$", source, re.M)
        assert match, f"{name} not found in coop_cryo.c"
        config += match.group(0) + "\n"
    match = re.search(r"^static real const cryo_tube_x\[CRYO_ROW_TUBES\] =\n\{.*?^\};\n", source, re.M | re.S)
    assert match, "cryo_tube_x not found in coop_cryo.c"
    config += match.group(0)
    config += structure(source, "cryo_seat") + "\n"
    match = re.search(r"^static struct\n\{.*?^\} coop_cryo;\n", source, re.M | re.S)
    assert match, "the coop_cryo globals not found in coop_cryo.c"
    config += match.group(0)
    if fault and where == "config":
        config = mutated(config, *fault)
    text = ""
    for name in FUNCTIONS:
        code = function(source, name)
        if fault and name == where:
            code = mutated(code, *fault)
        text += code + "\n"
    return (("config.inc", config), ("under_test.inc", text))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("coop_cryo", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, where, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("coop_cryo", generated(fault, where)), case)
    assert status == CHECK_FAILED, f"coop_cryo.c with a fault ({control}) passed '{case}': the test cannot see it"
