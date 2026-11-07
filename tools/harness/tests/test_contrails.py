"""Real contrail emission, point-state aging and map lifecycle at varied render rates."""
import os
import re
from pathlib import Path
import subprocess
import sys

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import HARNESS, enum_with, function, read, structure


@pytest.fixture(scope="module")
def executables(tmp_path_factory):
    source = read("source/effects/contrails.c")
    header = read("source/effects/contrails.h")
    definitions = read("source/effects/contrail_definitions.h")
    config = "\n".join(structure(header, n) for n in ("contrail_datum", "contrail_point_datum"))
    config += "\n" + structure(definitions, "contrail_point_state")
    config += "\n" + "\n".join(enum_with(source, n) for n in (
        "_contrail_active_bit", "_contrail_point_new_bit", "_contrail_state_duration_bit",
        "_contrail_scales_point_generation_rate_bit"))
    code = "\n".join(function(source, n) for n in (
        "contrails_reset_render_state", "contrails_initialize_for_new_map", "contrails_dispose_from_old_map",
        "contrail_scale_value", "contrail_scale_random_value", "contrail_compute_new_point_count",
        "contrail_update_points", "contrails_update"))
    clocks = source.split("/* ---------- globals */", 1)[1].split("/* ---------- public code */", 1)[0]
    state = read("source/saved games/game_state.c")
    callbacks = re.search(r"static game_state_after_load_proc after_load_procs\[\] =\s*\{.*?\};", state, re.S).group(0)
    names = re.findall(r"^\s*(\w+)\s*,?\s*$", callbacks, re.M)
    after_load = "typedef void (*game_state_after_load_proc)(void);\n#define NUMBEROF(a) (sizeof(a) / sizeof((a)[0]))\n"
    after_load += "\n".join(f"static void {name}(void) {{ }}" for name in names if name != "contrails_reset_render_state")
    after_load += "\n" + callbacks + "\n" + function(state, "game_state_call_after_load_procs")
    result = {}
    for variant in ("fixed", "frame-aging", "stale-clock", "missing-hook"):
        work = tmp_path_factory.mktemp("contrails-" + variant)
        (work / "config.inc").write_text(config)
        selected = code
        if variant == "frame-aging":
            before = "contrail_update_points(contrail_index, dt, point_dt);"
            assert selected.count(before) == 1
            selected = selected.replace(before, "contrail_update_points(contrail_index, dt, dt);")
        if variant == "stale-clock":
            selected = selected.replace("last_point_game_time = NONE;", "")
        (work / "under_test.inc").write_text(clocks + selected)
        (work / "after_load.inc").write_text(after_load.replace("\tcontrails_reset_render_state,", "") if variant == "missing-hook" else after_load)
        exe = work / ("contrails.exe" if os.name == "nt" else "contrails")
        flags = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld"] if os.name == "nt" else ["-m32"]
        subprocess.run([os.environ.get("CC", "clang"), *flags, "-std=gnu89", "-O2", "-Wall", "-Werror",
                        "-Wno-unused-function", "-Wno-unused-variable", "-Wno-parentheses",
                        "-I", str(HARNESS / "include"), "-I", str(work),
                        str(HARNESS / "tests/contrails.c"), "-o", str(exe)], check=True)
        result[variant] = exe
    return result


@pytest.mark.parametrize("case", ["rates", "irregular", "lifecycle", "new-point", "pause-activation", "detached", "checkpoint", "catch-up", "double-speed"])
def test_contrails(executables, case):
    result = subprocess.run([str(executables["fixed"]), case], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr


@pytest.mark.parametrize("variant,case", [("frame-aging", "rates"), ("frame-aging", "irregular"), ("stale-clock", "lifecycle"), ("missing-hook", "checkpoint")])
def test_negative_control(executables, variant, case):
    result = subprocess.run([str(executables[variant]), case], capture_output=True, text=True)
    assert result.returncode == 1, result.stdout + result.stderr
