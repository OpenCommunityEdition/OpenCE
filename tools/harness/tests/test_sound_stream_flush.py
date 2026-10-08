"""Cancellation drains SDL packets without refilling the original game channel."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run
from harness.audio import stream_types

CASES = ["mixed-head", "unfinished", "all-finished", "paused", "repeat", "ordinary",
         "wrapped", "no-callback", "empty"]
NEGATIVE_CONTROLS = {
    "successful-cancellation": (("stream_complete_head(stream, XMEDIAPACKET_STATUS_FLUSHED,",
        "stream_complete_head(stream, head->finished ? XMEDIAPACKET_STATUS_SUCCESS : XMEDIAPACKET_STATUS_FLUSHED,"), "mixed-head"),
    "does-not-unlock-cache": (("sound_cache_sound_hardware_unlock(packet_context);", ""), "mixed-head"),
    "does-not-reset-resampler": (("resampler_reset(stream);", ""), "repeat"),
}


def generated(fault=None):
    backend = read("port/linux/src/dsound_sdl.c")
    game = read("source/sound/sound_dsound_xbox.c")
    code = function(game, "dsound_channel_callback") + "\n" + function(game, "channel_stop") + "\n"
    code += "\n".join(function(backend, name) for name in ["packet_release", "stream_complete_head",
        "streams_complete_finished", "stream_from_interface", "stream_flush"])
    if fault:
        code = mutated(code, *fault)
    return (("types.inc", stream_types()), ("under_test.inc", code))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("sound_stream_flush", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("sound_stream_flush", generated(fault)), case)
    assert status == CHECK_FAILED, f"{control} escaped {case}: {output}"
