"""Exercise the production HUD interpolator without maps or a graphics driver."""
from pathlib import Path
import shutil
import subprocess
import sys

import pytest

ROOT = Path(__file__).resolve().parents[1]


def test_original_pixels_keep_channels_centres_and_bounds(tmp_path):
    compiler = shutil.which("clang")
    if not compiler:
        pytest.skip("clang required")
    source = (ROOT / "port/linux/src/hud_hires.c").read_text()
    code = source[source.index("static void upscale_weights"):
                  source.index("static unsigned char *png_decode")]
    harness = r'''
#include <assert.h>
#include <stdlib.h>
#include <string.h>
PRODUCTION
int main(void)
{
    unsigned char pixel[4] = {37, 91, 173, 255}, input[7*7*4] = {0};
    unsigned char *output;
    unsigned long x, y, c;
    double mass = 0, centre_x = 0, centre_y = 0;
    assert(!hud_hires_upscale_pixels(NULL, 1, 1));
    assert(!hud_hires_upscale_pixels(pixel, 0, 1));
    assert(!hud_hires_upscale_pixels(pixel, 1, 0));
    assert(!hud_hires_upscale_pixels(pixel, 257, 1));
    assert(!hud_hires_upscale_pixels(pixel, 1, 257));
    output = hud_hires_upscale_pixels(pixel, 1, 1);
    assert(output);
    for (x = 0; x < 16; x++) {
        assert(output[x*4] == 173 && output[x*4+1] == 91);
        assert(output[x*4+2] == 37 && output[x*4+3] == 255);
    }
    free(output);
    /* A sprite's centre must stay at the same normalized texture position. */
    memset(input + (3*7+3)*4, 255, 4);
    output = hud_hires_upscale_pixels(input, 7, 7);
    assert(output);
    for (y = 0; y < 28; y++) for (x = 0; x < 28; x++) {
        unsigned char alpha = output[(y*28+x)*4+3];
        mass += alpha;
        centre_x += (x+.5)*alpha;
        centre_y += (y+.5)*alpha;
        for (c = 0; c < 4; c++) {
            assert(output[(y*28+x)*4+c] == output[(x*28+y)*4+c]);
            assert(output[(y*28+x)*4+c] == output[(y*28+27-x)*4+c]);
        }
    }
    assert(mass > 0 && centre_x/mass == 14 && centre_y/mass == 14);
    free(output);
    memset(input, 0, sizeof(input));
    output = hud_hires_upscale_pixels(input, 7, 7);
    assert(output);
    for (x = 0; x < 28*28*4; x++) assert(!output[x]);
    free(output);
    return 0;
}
'''
    unit = tmp_path / "hud_upscale.c"
    unit.write_text(harness.replace("PRODUCTION", code))
    executable = tmp_path / "hud_upscale.exe"
    target = ["--target=i686-pc-windows-msvc"] if sys.platform == "win32" else []
    subprocess.run([compiler, *target, "-std=gnu89", "-O2", str(unit), "-o", str(executable)],
                   check=True, capture_output=True, text=True)
    subprocess.run([str(executable)], check=True, timeout=10)
