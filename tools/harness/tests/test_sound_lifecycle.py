"""Production fades and voice creation at multiple frame rates, including negative controls."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run
from harness.audio import manager_types

CASES = ["fresh-loop", "fresh-impulse", "pending", "restart", "intro-loop",
         "rapid", "other-owner", "other-track", "stop-cue", "fake-impulse", "repeated-stop", "four-tracks"]
NEGATIVE_CONTROLS = {
    "keeps-replaced-voice": (("sound_index != except_sound_index &&", "FALSE &&"), "restart"),
    "cancels-other-owner": (("sound->source_identifier == looping_sound_index &&", "TRUE &&"), "other-owner"),
    "cancels-other-track": (("sound->loop_track_index == track_index)", "TRUE)"), "other-track"),
}


def generated(fault=None, case=None):
    manager = read("source/sound/sound_manager.c")
    names = ["update_potentially_audible_looping_sound", "sound_new_impulse", "sound_calculate_fade",
             "sound_start_fade", "sound_fade_looping_track_components", "sound_refresh_looping"]
    code = []
    for name in names:
        text = function(manager, name)
        if fault and ((case == "fresh-loop" and name == names[0]) or
                      (case == "fresh-impulse" and name == names[1]) or
                      (case not in ("fresh-loop", "fresh-impulse") and name in names[2:])):
            if fault[0] in text:
                text = mutated(text, *fault)
        code.append(text)
    enums = manager_types()
    constants_section = manager[manager.index("/* ---------- constants */"):]
    for member in ["_sound_promotion_dont", "_sound_compression_xbox_adpcm", "_sound_encoding_mono"]:
        enums += enum_with(constants_section, member) + "\n"
    enums += enum_with(read("source/sound/sound_classes.h"), "_sound_class_unit_dialog") + "\n"
    for name in ["sound_fade_exponent", "sound_inaudible_fade_out_time", "speed_of_sound_threshold"]:
        enums += re.search(r"^.*\b" + name + r"\s*=.*;", manager, re.M).group(0) + "\n"
    return (("types.inc", enums), ("under_test.inc", "\n".join(code)))


@pytest.mark.parametrize("hz", [30, 60, 120, 240])
@pytest.mark.parametrize("case", CASES)
def test_case(case, hz):
    status, output = run(build("sound_lifecycle", generated()), f"{case}:{hz}")
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("sound_lifecycle", generated(fault, case)), f"{case}:240")
    assert status == CHECK_FAILED, f"{control} escaped {case}: {output}"
