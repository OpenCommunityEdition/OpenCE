# Adapted from OpenH1/OpenH1 6750db5a6c9716857bd1e63d9ec188a20c49f1d3.
# Copyright OpenH1 contributors; AGPL-3.0-or-later.
"""The HRTF of dsound_sdl.c against the Xbox's (hrtf.c): the time between the ears, their levels apart in each
octave and what a sound behind loses, over a grid of directions, against the Xbox's own filters; and each ear's
level in each octave and the time between the ears against recordings of an Xbox's output, every 30 degrees round
at 40, 20 and 0 degrees below and above, and on 5.1 its back pair's at 135 degrees, 40 below and above.

    python tools/harness/tests/test_hrtf.py metrics   our figures, beside the Xbox's
(MIXER_SOURCE=path: of another dsound_sdl.c, to compare)

The Xbox's figures in hrtf.c were measured on the Xbox's own filters, which this repository does not hold: the
XDK DirectSound linked into the game's default.xbe, once the game calls DirectSoundUseFullHRTF, works out each
voice's azimuth and elevation (45 degrees times the ratio of the lesser side over the greater in each octant),
rounds them to its grid (6 degrees of elevation; 3, 6 or 12 of azimuth as the elevation rises), and looks up a
pair of 31-tap filters with a whole-frame delay between the ears, which it writes to the MCPX's voice processor
(NV1BA0_PIO_SET_HRIR). Each tap is a byte: a sign, a 3-bit exponent and a 4-bit mantissa (the magnitude the
mantissa below 16, else 16 plus it, doubled each exponent past the first). To measure them again from one's own
default.xbe: find DirectSoundUseFullHRTF (it stores its lookup's address for the 3D calculator to call), run that
lookup as it is (x86, stdcall: azimuth, elevation, the fold behind (off: DirectSound sets it only for Dolby
Surround and Dolby Digital), and two pointers it sets to the left and
right ears' filters) for each direction of hrtf.c's grid with the fold off, make each ear's response at 48 kHz
from its 31 taps decoded, after its delay (the filter's last byte: the left ear's when the sound is to the
right), 512 frames each, and give them to "measure" (raw floats, in the grid's order) for its line of
xbox_figures.
"""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, run  # noqa: E402
from harness.tests.test_mixer import generated, build_mixer  # noqa: E402

CASES = ["invalid-angles", "delays", "levels", "rear", "recorded", "recorded-behind"]

# a fault in the HRTF, and the case that must catch it
NEGATIVE_CONTROLS = {
    "nan-elevation": (("if (elevation != elevation) elevation = 0.0f;", ""), "invalid-angles"),
    # the pole below the mean of the 40-degree rows, as it was before the tenth run recorded it (a sound straight
    # below 20 to 30 dB too bright above 4.5 kHz)
    "pole-below-guessed": (("{ { -0.3f, -0.3f, 2.0f, -32.3f, -40.1f, -22.4f, -19.1f, -43.1f, -31.9f },\n"
                            "\t  { -0.3f, -0.3f, 2.0f, -32.3f, -40.1f, -22.4f, -19.1f, -43.1f, -31.9f }, 0.0f },",
                            "{ { 0.5f, 0.1f, 3.4f, -0.7f, -1.7f, -8.1f, -1.4f, 1.1f, -5.8f },\n"
                            "\t  { 0.5f, 0.1f, 3.4f, -0.7f, -1.7f, -8.1f, -1.4f, 1.1f, -5.8f }, 0.0f },"), "recorded"),
    # the time between the ears 15% short (a sphere's, at 40 degrees up or down)
    "delays-short": (("floorf(strength * direction.delay + 0.5f)", "floorf(0.85f * strength * direction.delay + 0.5f)"),
                     "delays"),
    # the ears' delays to a fraction of a frame, not whole ones as the Xbox's
    "fractional-delays": (("floorf(strength * direction.delay + 0.5f)",
                           "(floorf(strength * direction.delay + 0.25f) + 0.25f)"), "delays"),
    # both ears at the far ear's levels: no level between them
    "ears-alike": (("const float *decibels = ear ^ mirrored ? direction.right : direction.left;",
                    "const float *decibels = mirrored ? direction.right : direction.left;"), "levels"),
    # behind heard as the direction mirrored ahead (the stereo Xbox played its table all round)
    "rear-as-front": (("azimuth = atan2f(fabsf(right), ahead)", "azimuth = atan2f(fabsf(right), fabsf(ahead))"),
                      "rear"),
    # no elevation: above and below as level
    "no-elevation": (("elevation = atan2f(up,", "elevation = 0.0f * atan2f(up,"), "recorded"),
    # above and below swapped
    "upside-down": (("elevation = atan2f(up,", "elevation = atan2f(-up,"), "recorded"),
    # the bands an octave up
    "bands-moved": (("{ 500, 1000, 2000, 3000, 4500, 6700, 10000, 14000 }",
                     "{ 1000, 2000, 3000, 4500, 6700, 10000, 14000, 20000 }"), "recorded"),
    # on 5.1, behind as the direction mirrored ahead and nothing more (the far ear 3 to 4 dB dull at 8 kHz)
    "behind-as-mirrored": (("if (behind && elevation != 0.0f)", "if (0 && behind && elevation != 0.0f)"),
                           "recorded-behind"),
}


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build_mixer(generated(), "hrtf"), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build_mixer(generated(fault=fault), "hrtf"), case)
    assert status == CHECK_FAILED, f"the HRTF with a fault ({control}) passed '{case}': the test cannot see it"


if __name__ == "__main__":
    import os
    import subprocess

    other = os.environ.get("MIXER_SOURCE")
    executable = build_mixer(generated(Path(other).read_text(encoding="latin-1") if other else None), "hrtf")
    sys.exit(subprocess.run([str(executable), *(sys.argv[1:] or ["metrics"])]).returncode)
