"""The real mixer of dsound_sdl.c on known signals (mixer.c): its resampler against the same low pass worked out in
double precision, sines through it as the ear hears them, voices out of earshot, mono voices and the linear
resampler, cut fades and synchronous packet cancellation.

    python tools/harness/tests/test_mixer.py metrics   the figures
    python tools/harness/tests/test_mixer.py bench [voices] [out of earshot] [indoor]   a game's worth of voices, timed
    python tools/harness/tests/test_mixer.py render > out.raw   noise through the mixer, to compare with another's
(MIXER_SOURCE=path: of another dsound_sdl.c; MIXER_SCALAR=1: its resampler adding tap by tap)
"""

import functools
import os
import subprocess
import sys
import tempfile
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, function, mutated, run  # noqa: E402
from harness.audio import mixer_source  # noqa: E402

CASES = ['reference',
 'impulse',
 'sines',
 'aliases',
 'silent-voice',
 'mono',
 'linear',
 'doppler-limits',
 'doppler-stereo',
 'cut',
 'cuts',
 'cut-overlap',
 'flush',
 'cut-lifecycle',
 'hrtf-restart']

# a fault in the mixer, and the case that must catch it
NEGATIVE_CONTROLS = {
 'hrtf-history-not-reset': (('memset(stream->hrtf_frames, 0, sizeof(stream->hrtf_frames));', ''), 'hrtf-restart'),
 'taps-off-by-one': (('(stream->center + 1 - RESAMPLER_ZERO_CROSSINGS) % RESAMPLER_HISTORY',
                      '(stream->center + 2 - RESAMPLER_ZERO_CROSSINGS) % RESAMPLER_HISTORY'),
                     'reference'),
 'phase-not-blended': (('for (phase = 0; phase <= RESAMPLER_TABLE_STEPS; phase++)',
                        'for (phase = 0; phase <= RESAMPLER_TABLE_STEPS; phase += 2)'),
                       'impulse'),
 'no-stretch': (('#define RESAMPLER_MAXIMUM_STRETCH 4', '#define RESAMPLER_MAXIMUM_STRETCH 1'), 'aliases'),
 'cut-dead': (('mix_voice(stream, output, send, CUT_FADE_FRAMES);', 'return;'), 'cut'),
 'cut-not-faded': (('float fade = 0.5f + 0.5f * (float)cos(3.14159265358979 * (double)(frame + 1) / CUT_FADE_FRAMES);',
                    'float fade = 1.0f;'),
                   'cut'),
 'cut-past-the-earlier': (('(frame < left ? stream->cut_send[stream->cut_played + frame] : 0.0f)',
                           '(frame < left ? 1.0f : 0.0f) * stream->cut_send[(stream->cut_played + frame) % '
                           'CUT_FADE_FRAMES]'),
                          'cuts'),
 'cut-discard-earlier': (('left = CUT_FADE_FRAMES - stream->cut_played', 'left = 0'), 'cut-overlap'),
 'flush-live-voice': (('stream->paused || stream->flushing', 'stream->paused'), 'flush'),
 'linear-nearest': (('left_frames[0] + (left_frames[1] - left_frames[0]) * fraction', 'left_frames[0]'),
                    'linear'),
 'doppler-stereo-always-on': (('stereo_doppler_enabled && !stream->has_3d', '!stream->has_3d'),
                              'doppler-stereo'),
 'frequency-upper-unbounded': (('if (target_step * OUTPUT_RATE > VOICE_FREQUENCY_MAXIMUM)', 'if (0)'),
                               'doppler-limits'),
 'frequency-lower-unbounded': (('if (target_step * OUTPUT_RATE < VOICE_FREQUENCY_MINIMUM)', 'if (0)'),
                               'doppler-limits')}


def generated(source=None, fault=None, scalar=False):
    """the mixer: dsound_sdl.c from its constants to the end of the mixing (before the output), its resampler adding
    tap by tap if scalar"""
    text = mixer_source(source)
    code = text[text.index("#define OUTPUT_RATE"):text.index("/* ---------- output")]
    code += "\n" + text[text.index("#define STREAM_SETTER"):text.index("\n\n", text.index("#define STREAM_SETTER"))] + "\n"
    code += "\n" + "\n".join(function(text, name) for name in (
        "packet_release", "stream_complete_head", "stream_from_interface",
        "stream_flush", "stream_release", "stream_process", "IDirectSoundStream_Pause"))
    code += "\n#ifdef UNDER_TEST_I3DL2_SOURCE\n" + function(text, "IDirectSoundStream_SetI3DL2Source") + "\n#endif\n"
    if fault:
        code = mutated(code, *fault)
    return (("under_test.inc", ("#define RESAMPLER_SCALAR\n" if scalar else "") + code),)


# The flush regression uses a real mixer thread on both native platforms.
_mixer_builds = tempfile.TemporaryDirectory(prefix="halo-mixer-")


@functools.lru_cache(maxsize=None)
def build_mixer(generated_code, fixture="mixer"):
    root = Path(__file__).resolve().parents[3]
    work = Path(_mixer_builds.name) / str(abs(hash(generated_code)))
    work.mkdir(parents=True, exist_ok=True)
    for name, text in generated_code:
        (work / name).write_text(text)
    executable = work / (fixture + ".exe")
    command = [os.environ.get("CC", "clang"), "-m32", "-std=gnu99", "-O2", "-Wall", "-Werror",
               "-Wno-unused-function", "-Wno-unused-variable",
               "-I", str(root / "tools/harness/include"), "-I", str(work),
               str(root / "tools/harness/tests" / (fixture + ".c")), "-o", str(executable)]
    if sys.platform == "win32":
        command += ["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS",
                    "-I", str(root / "port/windows/include/posix"),
                    str(root / "port/windows/src/win32_posix.c"), "-lwinmm"]
    else:
        command += ["-pthread", "-lm"]
    result = subprocess.run(command, capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(result.stderr)
    return executable


def rendered(scalar):
    """mixer.c's render: voices at each rate, then a scene's whole mix, as floats"""
    import array

    result = subprocess.run([str(build_mixer(generated(scalar=scalar))), "render"], capture_output=True,
                            timeout=120, check=True)
    return array.array("f", result.stdout)


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build_mixer(generated()), case)
    assert status == 0, output


def test_vectors_add_as_taps_do():
    """the resampler four taps at a time is the resampler tap by tap but for the order of its additions: the same
    sound to float's rounding, through the whole mix (reverb and limiter)"""
    vector, scalar = rendered(False), rendered(True)
    assert len(vector) == len(scalar) > 0
    worst = max(abs(a - b) for a, b in zip(vector, scalar))
    assert 0 < worst < 1e-5, f"the vector resampler differs by {worst} (none: no vectors)"


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build_mixer(generated(fault=fault)), case)
    assert status == CHECK_FAILED, f"the mixer with a fault ({control}) passed '{case}': the test cannot see it"


if __name__ == "__main__":
    # (MIXER_SOURCE: another dsound_sdl.c, to compare; MIXER_SCALAR=1: its resampler tap by tap)
    other = os.environ.get("MIXER_SOURCE")
    executable = build_mixer(generated(Path(other).read_text(encoding="latin-1") if other else None,
                                      scalar=bool(os.environ.get("MIXER_SCALAR"))))
    sys.exit(subprocess.run([str(executable), *(sys.argv[1:] or ["metrics"])]).returncode)
