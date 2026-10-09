"""The real mixer of dsound_sdl.c on known signals (mixer.c): its resampler against the same low pass worked out in
double precision, sines through it as the ear hears them, voices out of earshot and mono voices.

    python tools/harness/tests/test_mixer.py metrics   the figures
    python tools/harness/tests/test_mixer.py bench [voices] [out of earshot] [indoor]   a game's worth of voices, timed
    python tools/harness/tests/test_mixer.py render > out.raw   noise through the mixer, to compare with another's
(MIXER_SOURCE=path: of another dsound_sdl.c)
"""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, mutated, read, run  # noqa: E402

SOURCE = "port/linux/src/dsound_sdl.c"
CASES = ["reference", "impulse", "sines", "aliases", "silent-voice", "mono"]

# a fault in the mixer, and the case that must catch it
NEGATIVE_CONTROLS = {
    "taps-off-by-one": (("unsigned long first = stream->center + 1 - RESAMPLER_ZERO_CROSSINGS;",
                         "unsigned long first = stream->center + 2 - RESAMPLER_ZERO_CROSSINGS;"), "reference"),
    "phase-not-blended": (("for (phase = 0; phase <= RESAMPLER_TABLE_STEPS; phase++)",
                           "for (phase = 0; phase <= RESAMPLER_TABLE_STEPS; phase += 2)"), "impulse"),
    "no-stretch": (("#define RESAMPLER_MAXIMUM_STRETCH 2", "#define RESAMPLER_MAXIMUM_STRETCH 1"), "aliases"),
}


def generated(source=None, fault=None):
    """the mixer: dsound_sdl.c from its constants to the end of the mixing (before the output)"""
    text = source if source is not None else read(SOURCE)
    code = text[text.index("#define OUTPUT_RATE"):text.index("/* ---------- output")]
    if fault:
        code = mutated(code, *fault)
    return (("under_test.inc", code),)


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("mixer", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("mixer", generated(fault=fault)), case)
    assert status == CHECK_FAILED, f"the mixer with a fault ({control}) passed '{case}': the test cannot see it"


if __name__ == "__main__":
    import os
    import subprocess

    # (MIXER_SOURCE: another dsound_sdl.c, to compare)
    other = os.environ.get("MIXER_SOURCE")
    executable = build("mixer", generated(Path(other).read_text(encoding="latin-1") if other else None))
    sys.exit(subprocess.run([str(executable), *(sys.argv[1:] or ["metrics"])]).returncode)
