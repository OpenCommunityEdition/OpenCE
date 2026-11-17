"""The native builds' sound channel pools hold a heavy fight: the desktop's with no channel taken, stock or with a
sound mod's stereo 44 kHz effects; the phones' a stock one, and never the music's or speech's (#1125)."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, enum_with, function, mutated, read, run
from harness.audio import class_table, manager_types

CAPACITY = "port/linux/include/halo_port_capacity.h"
CASES = ["desktop", "phone", "normal-fight-unchanged"]
NEGATIVE_CONTROLS = {
    # the Xbox's hardware voices (9, 46, 9, 9): a heavy fight's effects take channels from each other
    "xbox-pools": ("#define DESKTOP_VIRTUAL_CHANNELS { 24, 96, 32, 80 }",
                   "#define DESKTOP_VIRTUAL_CHANNELS { 9, 46, 9, 9 }", "desktop"),
}


def pools():
    """(phone, desktop): the virtual and actual channel counts of each type, as halo_port_capacity.h has them"""
    text = read(CAPACITY)
    block = text[text.index("#define HALO_PORT_SOUND_VIRTUAL_CHANNELS") - 200:]
    phone_part, desktop_part = block.split("#else", 1)

    def counts(part, name):
        return [int(n) for n in re.search(rf"#define {name} \{{([\d, ]+)\}}", part).group(1).split(",")]

    return [{kind: counts(part, f"HALO_PORT_SOUND_{kind.upper()}_CHANNELS") for kind in ("virtual", "actual")}
            for part in (phone_part, desktop_part)]


def generated(fault=None):
    manager = read("source/sound/sound_manager.c")
    code = "\n".join(function(manager, name) for name in
                     ["sound_valid_for_channel", "sound_class_is_music_or_speech", "sound_preempts_sound",
                      "sound_find_best_channel"])
    phone, desktop = pools()
    flags = re.search(r"^short sound_channel_type_flags\[4\] = .*;$", read("source/sound/sound_preferences.c"),
                      re.M).group(0)
    pools_text = "\n".join([f"static {flags}",
                            f"#define DESKTOP_VIRTUAL_CHANNELS {{ {', '.join(map(str, desktop['virtual']))} }}",
                            f"#define PHONE_VIRTUAL_CHANNELS {{ {', '.join(map(str, phone['virtual']))} }}"])
    if fault:
        pools_text = mutated(pools_text, fault[0], fault[1])
    types = manager_types() + enum_with(manager[manager.index("/* ---------- constants */"):],
                                        "_sound_channel_3d_bit") + "\n"
    return (("types.inc", types), ("classes.inc", class_table()), ("pools.inc", pools_text), ("under_test.inc", code))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("sound_channel_pools", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    *fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("sound_channel_pools", generated(fault)), case)
    assert status == CHECK_FAILED, f"{control} escaped {case}: {output}"


def test_actual_channels_fit():
    """each type has a few more actual channels (the dsound streams) than virtual, as the Xbox's had, and all of them
    fit MAXIMUM_SOUND_CHANNELS"""
    most = constant(read("source/sound/sound_dsound_xbox.c"), "MAXIMUM_SOUND_CHANNELS")
    for pool in pools():
        assert all(actual > virtual for actual, virtual in zip(pool["actual"], pool["virtual"])), pool
        assert sum(pool["actual"]) <= most and sum(pool["virtual"]) <= most, pool
