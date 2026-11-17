"""Effects never take music's or speech's channel, and the stereo channel types are as large as the mono 3D type (#1125)."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, enum_with, function, mutated, read, run
from harness.audio import manager_types

CASES = ["music-kept", "speech-kept", "music-takes-effects-channel", "effects-as-before"]
NEGATIVE_CONTROLS = {
    # the class priorities alone, as before the fix: a weapon (4) takes music's (2) channel
    "priorities-alone": (("if (challenger_kept != sound_class_is_music_or_speech(", "if (FALSE && ("), "music-kept"),
}


def classes():
    source = read("source/sound/sound_classes.c")
    macro = re.search(r"^#define SOUND_CLASS_DEFINITION\(.*?[^\\]\n", source, re.M | re.S).group(0)
    tables = [source[source.index(start):source.index("};", source.index(start)) + 2]
              for start in ["struct sound_class_definition sound_classes[", "char const *sound_class_names["]]
    return "\n".join([f"#define NUMBER_OF_SOUND_CLASSES {constant(source, 'NUMBER_OF_SOUND_CLASSES')}",
                      enum_with(read("source/sound/sound_classes.h"), "_sound_class_music"), macro, *tables])


def generated(fault=None):
    manager = read("source/sound/sound_manager.c")
    code = "\n".join(function(manager, name) for name in
                     ["sound_valid_for_channel", "sound_class_is_music_or_speech", "sound_preempts_sound",
                      "sound_find_best_channel"])
    if fault:
        code = mutated(code, *fault)
    types = manager_types() + enum_with(manager[manager.index("/* ---------- constants */"):],
                                        "_sound_channel_3d_bit") + "\n"
    return (("types.inc", types), ("classes.inc", classes()), ("under_test.inc", code))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("sound_channel_steal", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("sound_channel_steal", generated(fault)), case)
    assert status == CHECK_FAILED, f"{control} escaped {case}: {output}"


def test_stereo_pools_match_mono_3d():
    """sound_channel_type_flags: 2D mono, 3D mono, stereo 22 kHz, stereo 44 kHz; actual counts, then virtual."""
    source = read("source/sound/sound_preferences.c")
    body = source[source.index("default_sound_preferences ="):]
    actual, virtual = ([int(n) for n in group.split(",")] for group in re.findall(r"\{([\d ,]+)\}", body)[:2])
    for counts in (actual, virtual):
        assert counts[2] >= counts[1] and counts[3] >= counts[1], counts
