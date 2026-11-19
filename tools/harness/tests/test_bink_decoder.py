"""The real bink_decoder.c (port/linux/src), which plays the Xbox's movies, on Bink data written bit by bit: a file's
header, video frames of filled and skipped blocks (in the planes' swapped order) and a damaged one, and audio
blocks of a level, a sine and their crossfade, which pin the inverse transform's direction and scale."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, mutated, read, run  # noqa: E402

CASES = ["header", "video fill", "video skip", "video damaged", "audio dc", "audio tone", "audio overlap"]
NEGATIVE_CONTROLS = {
    # revision k's unknown field not skipped
    "k-field": (("\tif (header->revision == 'k')\n\t\toffset += 4;\n", ""), "header"),
    # the chroma planes in the order they are stored
    "not-swapped": (("video->swap_planes = video->revision >= 'h';", "video->swap_planes = 0;"), "video fill"),
    # a fill block left as it was
    "no-fill": (("\t\t\t\tblock_fill(destination, value_get(video, SOURCE_COLORS), stride, 8);",
                 "\t\t\t\tvalue_get(video, SOURCE_COLORS);"), "video fill"),
    # a skipped block not taken from the last frame
    "no-skip": (("\t\t\tcase SKIP_BLOCK:\n\t\t\t\tpixels_put(destination, previous, stride);",
                 "\t\t\tcase SKIP_BLOCK:"), "video skip"),
    # a damaged frame shown
    "damaged-shown": (("\t\tif (plane_decode(video, &reader, index, plane != 0) < 0)\n\t\t\treturn -1;",
                       "\t\tif (plane_decode(video, &reader, index, plane != 0) < 0)\n\t\t\tbreak;"), "video damaged"),
    # twice the level
    "scale": (("audio->root = (float)(2.0 /", "audio->root = (float)(4.0 /"), "audio dc"),
    # the transform's sines the other way
    "sine-sign": (("\t\tim[audio->reversed[k]] = -coefficients[2 * k + 1];",
                   "\t\tim[audio->reversed[k]] = coefficients[2 * k + 1];"), "audio tone"),
    # no crossfade between blocks
    "no-crossfade": (("\tif (!audio->first)\n\t{\n\t\tint count", "\tif (0)\n\t{\n\t\tint count"), "audio overlap"),
}


def generated(fault=None):
    code = read("port/linux/src/bink_decoder.c").replace("\r\n", "\n")
    code = code.replace('#include "../../third_party/ffmpeg-bink/binkdata.h"', '#include "binkdata.inc"')
    if fault:
        code = mutated(code, *fault)
    return (("bink_decoder.h", read("port/linux/src/bink_decoder.h")),
            ("binkdata.inc", read("port/third_party/ffmpeg-bink/binkdata.h")),
            ("under_test.inc", code))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("bink_decoder", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("bink_decoder", generated(fault)), case)
    assert status == CHECK_FAILED, output
