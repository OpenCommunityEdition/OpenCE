"""A looping sound's track stops with its looping sound, however close the frames (refresh_sound)."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run
from harness.audio import manager_types

CASES = ["same-millisecond", "next-millisecond", "refreshed", "delayed-impulse"]
NEGATIVE_CONTROLS = {
    # the test of the start time alone: the crash of a carousel network game
    "tracks-only-after-start": (("!datum_try_and_get(looping_sound_data, sound->source_identifier))", "FALSE)"),
                                "same-millisecond"),
}


def generated(fault=None):
    manager = read("source/sound/sound_manager.c")
    code = "\n".join(function(manager, name) for name in ["track_loop_track_sound", "refresh_sound"])
    if fault:
        code = mutated(code, *fault)
    return (("types.inc", manager_types()), ("under_test.inc", code))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("sound_orphan", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("sound_orphan", generated(fault)), case)
    assert status == CHECK_FAILED, f"{control} escaped {case}: {output}"
