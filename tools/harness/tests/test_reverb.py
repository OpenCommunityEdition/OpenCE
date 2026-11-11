# Adapted from OpenH1/OpenH1 6750db5a6c9716857bd1e63d9ec188a20c49f1d3.
# Copyright OpenH1 contributors; AGPL-3.0-or-later.
"""The reverb of dsound_sdl.c against the Xbox's (reverb.c), for each sound environment of the maps: the parameters
DirectSound downloaded to the DSP for it, and the impulse response of each of the reverb's four channels (the
reflections' and the late reverberation's energy, when the reflections start, the decay times), with how it follows
the game from one environment to another.

    python tools/harness/tests/test_reverb.py metrics   the figures of each environment, beside the Xbox's
(MIXER_SOURCE=path: of another dsound_sdl.c, to compare)

The Xbox's figures in reverb.c were measured on the Xbox's own code, which this repository does not hold: the I3DL2
reverb of the effects image the game downloads (sound_dsound_xbox.c), run on a DSP56300 interpreter, with the
parameters the XDK DirectSound linked into the game's default.xbe works out from the listener properties, run as it
is (x86) on the properties of each environment. To measure them again from one's own default.xbe: decrypt the
image's code with DirectSound's own loader (CDirectSound::DownloadEffectsImage), load the reverb's code and state at
the DSP's addresses, give it the parameter words (state words 70 to 135), and let it settle (it fades each tap in
over 16 frames, one tap after another, some 320 frames) before the impulse; then "measure" takes each response, as
raw floats of the four channels (front left and right, back left and right) normalized to a unit impulse with the
room level out, and prints its line of xbox_figures.
"""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, run  # noqa: E402
from harness.tests.test_mixer import generated, build_mixer  # noqa: E402

CASES = ["ring-wrap", "parameters", "response", "levels", "transition", "change", "delays"]

# a fault in the reverb, and the case that must catch it
NEGATIVE_CONTROLS = {
    "negative-ring-index": (("position - delay % size + size", "position - delay + size"), "ring-wrap"),
    # the decay HF ratio over 1 let through (a cave's high frequencies decaying slower), as it was
    "hf-ratio-over-1": (("if (properties->flDecayHFRatio < 1.0f)", "if (properties->flDecayHFRatio != 1.0f)"),
                        "parameters"),
    # the send not halved: the reverb 6 dB too wet, on every channel
    "send-whole": (("properties->lRoomHF ? 0.5f : -0.5f", "properties->lRoomHF ? 1.0f : -1.0f"), "response"),
    # the reverberation the right way up at a Room HF of 0, where the DSP's 24-bit word turns it over
    "polarity-kept": (("properties->lRoomHF ? 0.5f : -0.5f", "0.5f"), "response"),
    # the reflections' combs at -0.2 (their -0.4's word read as -0.2)
    "comb-halved": (("{ -0.4f, 0.4f, -0.4f, -0.4f }", "{ -0.2f, 0.4f, -0.2f, -0.2f }"), "response"),
    # the reverb at the output's rate, each frame its own (no 24 kHz core): every delay half as long
    "full-rate": (("		reverb.odd = !reverb.odd;\n", ""), "response"),
    # the reflections' comb left out
    "no-comb": (("reverb_comb_gains[index] * reverb_read", "0.0f * reverb_read"), "response"),
    # the lines' lengths in whole milliseconds at 48 kHz, as the port's own reverb had
    "lines-48k": (("{ 1392, 1684, 2044, 2476 }", "{ 2784, 3368, 4088, 4952 }"), "parameters"),
    # a delay longer than the input's line not wrapping round it (the mountains' reflections a quarter second later)
    "no-wrap": (("delay = delay < 16 ? 16 : (delay - 16) % size + 16;", "delay = delay < 16 ? 16 : delay;\n"
                 "\tif (delay > size) delay = size;"), "response"),
    # the four channels on stereo each side's pair summed whole, not the back at -3 dB as the Xbox's downmix
    "stereo-back-whole": (("(held[0] + 0.70710678f * held[2]) * level", "(held[0] + held[2]) * level"), "levels"),
    # a new environment faded to over 85 ms, as it was (a sound started with it echoing as the room before)
    "fade-slow": (("#define REVERB_FADE_FRAMES 96", "#define REVERB_FADE_FRAMES 4096"), "change"),
    # a new environment's delays at once, as they were (a click just after the change echoing at the new room's)
    "delays-at-once": (("#define REVERB_DELAY_HOLD_FRAMES 4800", "#define REVERB_DELAY_HOLD_FRAMES 0"), "delays"),
    # the hold started again by each change (the delays never moving while the game steps from room to room)
    "hold-restarted": (("	held = reverb.delay_hold || reverb.delay_fade;", "	held = FALSE;"), "delays"),
}


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build_mixer(generated(), "reverb"), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build_mixer(generated(fault=fault), "reverb"), case)
    assert status == CHECK_FAILED, f"the reverb with a fault ({control}) passed '{case}': the test cannot see it"


if __name__ == "__main__":
    import os
    import subprocess

    other = os.environ.get("MIXER_SOURCE")
    executable = build_mixer(generated(Path(other).read_text(encoding="latin-1") if other else None), "reverb")
    sys.exit(subprocess.run([str(executable), *(sys.argv[1:] or ["metrics"])]).returncode)
