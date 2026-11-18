"""Effects never take music's or scripted dialogue's channel (OpenH1 issue 1125); the pools' sizes: test_sound_channel_pools."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, enum_with, function, mutated, read, run
from harness.audio import class_table, manager_types

CASES = ["music-kept", "speech-kept", "music-takes-effects-channel", "effects-as-before",
         "combat-chatter", "chatter-can-be-preempted", "scripted-classes", "protected-priorities"]
NEGATIVE_CONTROLS = {
    # the class priorities alone, as before the fix: a weapon (4) takes music's (2) channel
    "priorities-alone": (("if (challenger_kept != sound_class_is_music_or_scripted_dialogue(", "if (FALSE && ("), "music-kept"),
}


def generated(fault=None):
    manager = read("source/sound/sound_manager.c")
    code = "\n".join(function(manager, name) for name in
                     ["sound_valid_for_channel", "sound_class_is_music_or_scripted_dialogue", "sound_preempts_sound",
                      "sound_find_best_channel"])
    if fault:
        code = mutated(code, *fault)
    types = manager_types() + enum_with(manager[manager.index("/* ---------- constants */"):],
                                        "_sound_channel_3d_bit") + "\n"
    return (("types.inc", types), ("classes.inc", class_table()), ("under_test.inc", code))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("sound_channel_steal", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("sound_channel_steal", generated(fault)), case)
    assert status == CHECK_FAILED, f"{control} escaped {case}: {output}"

