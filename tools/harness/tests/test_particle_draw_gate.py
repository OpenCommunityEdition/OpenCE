"""A particle is drawn once a tick's update has passed, as at 30 Hz: the plasma bolts are never drawn between ticks."""

import os
import re
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, enum_with, function, mutated, read, run  # noqa: E402

SOURCE = "source/effects/particles.c"
CASES = ("bolt", "bolt-jitter", "tick-paced", "double-speed", "frames", "once-a-tick", "created-after-update")
PAST_A_TICK = "return age > tick_dt * (1.0f + PARTICLE_DRAW_TICK_TOLERANCE);"


def generated(fault=None):
    source = read(SOURCE)
    code = "\n".join(function(source, name) for name in (
        "particle_next_sequence", "particle_next_frame", "particle_update_frame_time", "particle_age_drawn",
        "particle_drawn"))
    if fault:
        code = mutated(code, *fault)
    tolerance = re.search(r"#define PARTICLE_DRAW_TICK_TOLERANCE (\S+)", source).group(1)
    config = "\n".join((
        enum_with(source, "_particle_definition_animation_stops_at_rest_bit"),
        enum_with(read("source/effects/particles.h"), "_particle_datum_animates_backwards_bit"),
        enum_with(source, "_particle_state_next_sequence_initial"),
        f"#define PARTICLE_DRAW_TICK_TOLERANCE {tolerance}",
    ))
    return (("config.inc", config + "\n"), ("under_test.inc", code + "\n"))


def build_native(generated_files):
    """Only floats and the particle's own fields; macOS hosts lack a 32-bit C runtime."""
    work = Path(tempfile.mkdtemp(prefix="particle-draw-gate-"))
    for name, contents in generated_files:
        (work / name).write_text(contents)
    executable = work / "particle_draw_gate"
    root = Path(__file__).resolve().parents[3]
    command = [os.environ.get("CC", "clang"), "-std=gnu99", "-O2", "-Wall", "-Werror", "-ffp-contract=off",
               "-Wno-unused-function", "-Wno-unused-variable", "-I", str(root / "tools/harness/include"),
               "-I", str(work), str(Path(__file__).with_name("particle_draw_gate.c")), "-o", str(executable), "-lm"]
    result = subprocess.run(command, capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return executable


class TestParticleDrawGate(unittest.TestCase):
    def test_gate(self):
        executable = build_native(generated())
        for case in CASES:
            with self.subTest(case=case):
                status, output = run(executable, case)
                self.assertEqual(status, 0, output)

    def test_drawn_at_a_tick_detected(self):
        """Drawn at a tick rather than past it: a frames' sum of exactly a tick draws the bolt."""
        fault = (PAST_A_TICK, "return age >= tick_dt * (1.0f - PARTICLE_DRAW_TICK_TOLERANCE);")
        status, output = run(build_native(generated(fault)), "bolt")
        self.assertEqual(status, CHECK_FAILED, output)

    def test_once_a_tick_by_age_detected(self):
        """A flash that animates once a tick, held back by its age: the tick it is drawn for can pass unseen."""
        fault = ("_particle_definition_animate_once_per_frame_bit) &&", "_particle_definition_animate_once_per_frame_bit) && 0 &&")
        status, output = run(build_native(generated(fault)), "once-a-tick")
        self.assertEqual(status, CHECK_FAILED, output)

    def test_ungated_detected(self):
        status, output = run(build_native(generated((PAST_A_TICK, "return TRUE;"))), "bolt")
        self.assertEqual(status, CHECK_FAILED, output)
