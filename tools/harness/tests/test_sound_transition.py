"""Real instance limiting must not update a voice it retires."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run
from harness.audio import manager_types

CASES = ["self-retirement", "other-victim", "no-limit", "definition-limit", "no-channel"]
NEGATIVE_CONTROLS = {
    "continues-after-retirement": (("if (!sound_set_definition_end(channel->sound_index))",
        "if ((sound_set_definition_end(channel->sound_index), FALSE))"), "self-retirement"),
    "reports-retired-voice-alive": (("return FALSE;", "return TRUE;"), "self-retirement"),
    "includes-own-channel": (("channel->sound_index != NONE && channel->sound_index != sound_index",
        "channel->sound_index != NONE"), "no-limit"),
}


def generated(fault=None):
    manager = read("source/sound/sound_manager.c")
    code = "\n".join(function(manager, name) for name in ["sound_channel_summary_build",
        "sound_find_like_channel", "sound_set_definition_end", "update_channel_for_looping_sound"])
    if fault:
        code = mutated(code, *fault)
    return (("types.inc", manager_types()), ("under_test.inc", code))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("sound_transition", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("sound_transition", generated(fault)), case)
    assert status == CHECK_FAILED, f"{control} escaped {case}: {output}"
