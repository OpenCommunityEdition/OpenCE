"""The high-res HUD's 4x upscale of a map's sheet on the CPU (port/linux/src/hud_hires.c, upscale_pixels, used
where the GPU's pass cannot be made): its functions cut from the source and compiled on their own, against an
exact reference that computes each output texel the plain way, 16 taps with the Catmull-Rom weights from their
formula, in 64-bit integers (the GPU's shader sums the same in 32). Needs a C compiler (clang, or CC), not the
game.

    python -m pytest -q tools/test_hud_upscale.py
"""

import os
import shutil
import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
SOURCE = ROOT / "port/linux/src/hud_hires.c"

FIRST_LINE = "/* Catmull-Rom interpolation of the map's original texels."
LAST_FUNCTION = "static unsigned char *upscale_pixels("

MAIN = r"""
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned char *test_upscale_pixels(const unsigned char *bgra, unsigned long width, unsigned long height,
	unsigned long maximum);

/* the weights from the formula, for t in eighths, in 1/1024 (exact: t^3 / 2 is a multiple of 1/1024) */
static void reference_weights(int eighths, long long *weights)
{
	long long t = eighths * 128, square = t * t / 1024, cube = square * t / 1024;

	weights[0] = (-t * 512 + square * 1024 - cube * 512) / 1024;
	weights[1] = (1024LL * 1024 - square * 2560 + cube * 1536) / 1024;
	weights[2] = (t * 512 + square * 2048 - cube * 1536) / 1024;
	weights[3] = (-square * 512 + cube * 512) / 1024;
}

static long clamp(long index, long count)
{
	return index < 0 ? 0 : index >= count ? count - 1 : index;
}

/* each output texel: source texels around (x + .5) / 4 - .5, rounded half up, swapped to RGBA */
static unsigned char *reference(const unsigned char *bgra, long width, long height)
{
	unsigned char *out = malloc(width * height * 64);
	long x, y, row, column;
	int channel;

	for (y = 0; y < height * 4; y++)
		for (x = 0; x < width * 4; x++)
		{
			/* the source position in eighths: (8x + 4) / 4 - 4 = 2x - 3 */
			long sy = 2 * y - 3, sx = 2 * x - 3;
			long iy = sy >= 0 ? sy / 8 : -((-sy + 7) / 8), ix = sx >= 0 ? sx / 8 : -((-sx + 7) / 8);
			long long wy[4], wx[4];

			reference_weights((int)(sy - iy * 8), wy);
			reference_weights((int)(sx - ix * 8), wx);
			for (channel = 0; channel < 4; channel++)
			{
				int source_channel = channel == 0 ? 2 : channel == 2 ? 0 : channel;
				long long value = 0;

				for (row = 0; row < 4; row++)
					for (column = 0; column < 4; column++)
						value += wy[row] * wx[column] * bgra[(clamp(iy + row - 1, height) * width +
							clamp(ix + column - 1, width)) * 4 + source_channel];
				value = value <= 0 ? 0 : (value + (1 << 19)) >> 20;
				out[(y * width * 4 + x) * 4 + channel] = (unsigned char)(value > 255 ? 255 : value);
			}
		}
	return out;
}

int main(int argc, char **argv)
{
	long width = atol(argv[1]), height = atol(argv[2]), maximum = atol(argv[3]), i, bytes = width * height * 4;
	const char *pattern = argv[4];
	unsigned char *bgra = malloc(bytes), *expected, *got;
	unsigned long differ = 0, worst = 0;

	srand(7);
	for (i = 0; i < bytes; i++)
	{
		if (!strcmp(pattern, "random"))
			bgra[i] = (unsigned char)(i % 7 == 0 ? 0 : i % 11 == 0 ? 255 : rand() & 255);
		else if (!strcmp(pattern, "edges"))
			/* (only 0 and 255: the overshoot each side of every edge is clamped) */
			bgra[i] = (unsigned char)(rand() & 1 ? 255 : 0);
		else
			/* one texel lit, in the middle */
			bgra[i] = (unsigned char)(i / 4 == (height / 2) * width + width / 2 ? 200 : 0);
	}
	/* (256: the HUD sheets', as hud_hires_upscale_texture's; 512 its atlases') */
	got = test_upscale_pixels(bgra, width, height, maximum ? maximum : 256);
	if (!got)
	{
		printf("none\n");
		return 0;
	}
	expected = reference(bgra, width, height);
	for (i = 0; i < bytes * 16; i++)
	{
		unsigned long d = got[i] > expected[i] ? got[i] - expected[i] : expected[i] - got[i];

		differ += d != 0;
		if (d > worst)
			worst = d;
	}
	printf("texels %ld differ %lu worst %lu\n", bytes * 16, differ, worst);
	return 0;
}
"""


def upscale_source():
    text = SOURCE.read_text(encoding="latin-1")
    start = text.index(FIRST_LINE)
    last = text.index(LAST_FUNCTION)
    end = text.index("\n}\n", last) + 3
    # (the file's static upscale_pixels, reachable from the test)
    return ("#include <stdlib.h>\n" + text[start:end].replace("\r\n", "\n") +
            "\nunsigned char *test_upscale_pixels(const unsigned char *bgra, unsigned long width, "
            "unsigned long height, unsigned long maximum)\n{\n\treturn upscale_pixels(bgra, width, height, maximum);\n}\n")


def build(work, source):
    compiler = os.environ.get("CC", "clang")
    if not shutil.which(compiler):
        pytest.skip(f"no {compiler}")
    (work / "upscale.c").write_text(source)
    (work / "main.c").write_text(MAIN)
    executable = work / "upscale.exe"
    # (the game's float settings: no fused multiply-add)
    subprocess.run([compiler, "-O2", "-ffp-contract=off", "-Wall", "-Werror", "-Wno-deprecated-declarations",
                    str(work / "main.c"), str(work / "upscale.c"), "-o", str(executable)], check=True)
    return executable


@pytest.fixture(scope="module")
def program(tmp_path_factory):
    return build(tmp_path_factory.mktemp("hud_upscale"), upscale_source())


def run(program, width, height, maximum=0, pattern="random"):
    result = subprocess.run([str(program), str(width), str(height), str(maximum), pattern], capture_output=True,
                            text=True, timeout=60, check=True)
    return result.stdout.strip()


def counts(line):
    words = line.split()
    return int(words[1]), int(words[3]), int(words[5])


@pytest.mark.parametrize("width, height, maximum, pattern", [
    (128, 128, 0, "random"),
    (128, 32, 0, "random"),
    (256, 256, 0, "random"),
    (64, 64, 0, "edges"),
    (512, 512, 512, "random"),  # (a kind 4 atlas)
    (1, 1, 0, "random"),
    (3, 2, 0, "random"),
    (5, 7, 0, "point"),
])
def test_matches_the_exact_filter(program, width, height, maximum, pattern):
    texels, differ, worst = counts(run(program, width, height, maximum, pattern))
    # float rounding can put a texel exactly on a half a step the other way: by one step, and rarely
    assert worst <= 1
    assert differ <= texels // 20000


def test_sizes_past_the_maximum_are_not_upscaled(program):
    assert run(program, 257, 16) == "none"
    assert run(program, 16, 257) == "none"
    assert run(program, 513, 16, 512) == "none"
    assert run(program, 512, 16, 512).startswith("texels")


@pytest.mark.parametrize("old, new", [
    ("UPSCALE_WEIGHT(399)", "UPSCALE_WEIGHT(400)"),  # a weight a step off
    ("UPSCALE_PHASE_1(e, d, c, b)", "UPSCALE_PHASE_1(b, c, d, e)"),  # a phase not mirrored
    ("upscale_clamp((long)row - 2, height)", "upscale_clamp((long)row - 1, height)"),  # the rows off by one
    ("padded[x * 4 + 0] = texel[2];", "padded[x * 4 + 0] = texel[0];"),  # red and blue not swapped
])
def test_negative_controls(tmp_path, old, new):
    source = upscale_source()
    assert source.count(old) >= 1
    program = build(tmp_path, source.replace(old, new))
    _, differ, worst = counts(run(program, 64, 64))
    assert worst > 1 or differ > 64 * 64 * 64 // 20000
