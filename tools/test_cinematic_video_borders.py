"""Exercise the production video matte draws with a recording D3D device."""
from pathlib import Path
import re
import shutil
import subprocess
from unittest import SkipTest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "source/rasterizer/xbox/rasterizer_xbox_screen_effect.c"


def production_function(source, name):
    match = re.search(r"static boolean " + name + r"\s*\([^)]*\)\s*\{", source)
    assert match, f"Missing production helper {name}"
    start = match.start()
    opening = source.index("{", match.start())
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


PRELUDE = r"""
#include <assert.h>
#include <math.h>
#include <stdio.h>
typedef int boolean;
typedef float real;
#define TRUE 1
#define FALSE 0
#define D3DRS_SRCBLEND 1
#define D3DBLEND_ZERO 0
#define D3DBLEND_INVSRCALPHA 5
#define D3DPT_TRIANGLEFAN 6
#define _rasterizer_vertex_register_position 0
static void *global_d3d_device;
static int blend = D3DBLEND_INVSRCALPHA, begins, ends, nvertices, bad_texcoords;
static real positions[8][2];
static int fail_at = -1, calls;
static int result(void) { return calls++ == fail_at ? -1 : 0; }
static int IDirect3DDevice8_SetRenderState(void *d, int state, int value) {
    (void)d; assert(state == D3DRS_SRCBLEND); blend = value; return result();
}
static int IDirect3DDevice8_Begin(void *d, int primitive) {
    (void)d; assert(primitive == D3DPT_TRIANGLEFAN);
    assert(blend == D3DBLEND_ZERO); begins++; return result();
}
static int IDirect3DDevice8_SetVertexData2s(void *d, int reg, short x, short y) {
    (void)d; bad_texcoords += reg != 4 || x != 0 || y != 0; return result();
}
static int IDirect3DDevice8_SetVertexData2f(void *d, int reg, real x, real y) {
    (void)d; assert(reg == _rasterizer_vertex_register_position);
    assert(nvertices < 8); positions[nvertices][0] = x; positions[nvertices++][1] = y;
    return result();
}
static int IDirect3DDevice8_End(void *d) { (void)d; ends++; return result(); }
"""

CHECKS = r"""
static void reset(void) {
    blend = D3DBLEND_INVSRCALPHA; begins = ends = nvertices = bad_texcoords = calls = 0;
    fail_at = -1;
}
static void width(short w, int video, short height) {
    reset(); assert(rasterizer_screen_effect_video_borders(video, w, height));
    assert(blend == D3DBLEND_INVSRCALPHA); assert(!bad_texcoords);
    if (!video || w <= 640 || height <= 0) { assert(!calls && !begins && !nvertices); return; }
    assert(begins == 2 && ends == 2 && nvertices == 8);
    real inner = 640.0f / w;
    real pixel = 1.0f / w;
    assert(positions[0][0] == -1 - pixel && positions[1][0] == -inner - pixel);
    assert(positions[2][0] == -inner - pixel && positions[3][0] == -1 - pixel);
    assert(positions[4][0] == inner - pixel && positions[5][0] == 1 - pixel);
    assert(positions[6][0] == 1 - pixel && positions[7][0] == inner - pixel);
    for (int i = 0; i < 8; i++) assert(positions[i][1] == ((i % 4 < 2) ? -1 : 1) + 1.0f / height);
    /* Map the actual clip-space edges through distinct offset viewports and
       render scales. The matte must remain outside the authored 640 lines. */
    for (int origin = 0; origin <= 123; origin += 123)
    for (int scale = 1; scale <= 3; scale++) {
        real left = (origin + (positions[1][0] + pixel + 1) * w / 2) * scale;
        real right = (origin + (positions[4][0] + pixel + 1) * w / 2) * scale;
        real outside_left = (origin + (positions[0][0] + pixel + 1) * w / 2) * scale;
        real outside_right = (origin + (positions[5][0] + pixel + 1) * w / 2) * scale;
        real top = (origin + (1 - (positions[2][1] - 1.0f / height)) * height / 2) * scale;
        real bottom = (origin + (1 - (positions[0][1] - 1.0f / height)) * height / 2) * scale;
        assert(fabsf((right - left) - 640 * scale) < 0.001f);
        assert(fabsf(left - (origin + (w - 640) / 2.0f) * scale) < 0.001f);
        assert(fabsf(outside_left - origin * scale) < 0.001f);
        assert(fabsf(outside_right - (origin + w) * scale) < 0.001f);
        assert(fabsf(top - origin * scale) < 0.001f);
        assert(fabsf(bottom - (origin + height) * scale) < 0.001f);
        for (int x = origin * scale; x < (origin + w) * scale; x++) {
            real center = x + 0.5f;
            int in_matte = center < left || center >= right;
            real expected_left = (origin + (w - 640) / 2.0f) * scale;
            real expected_right = (origin + (w + 640) / 2.0f) * scale;
            if (fabsf(center - expected_left) > 0.001f && fabsf(center - expected_right) > 0.001f)
                assert(in_matte == (center < expected_left || center >= expected_right));
        }
    }
}
int main(void) {
    short widths[] = {0, 320, 639, 640, 641, 852, 853, 854, 1280};
    short heights[] = {0, 360, 480, 960};
    for (int i = 0; i < 9; i++) for (int j = 0; j < 4; j++) {
        width(widths[i], 0, heights[j]); width(widths[i], 1, heights[j]);
    }
    reset(); assert(rasterizer_screen_effect_video_borders(TRUE, 853, 480)); int total = calls;
    for (int i = 0; i < total; i++) {
        reset(); fail_at = i; assert(!rasterizer_screen_effect_video_borders(TRUE, 853, 480));
        assert(blend == D3DBLEND_INVSRCALPHA && ends == 2);
    }
    puts("PASS: video-only opaque side mattes, fractional bounds, scaling and blend restoration");
}
"""


def test_video_matte_draws_preserve_authored_window(tmp_path):
    compiler = shutil.which("clang") or shutil.which("cc")
    if not compiler:
        raise SkipTest("A C compiler is required for the cinematic video probe")
    source = SOURCE.read_text()
    helper = production_function(source, "rasterizer_screen_effect_video_borders")
    path = tmp_path / "video-borders.c"
    binary = tmp_path / "video-borders"
    path.write_text(PRELUDE + helper + CHECKS)
    subprocess.run([compiler, "-std=c11", "-Wall", "-Wextra", "-Werror", "-O1",
                    "-fsanitize=address,undefined", str(path), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
    effect = source[source.index("void _rasterizer_screen_effect("):]
    matte = effect.index("rasterizer_screen_effect_video_borders(parameters->video_on, viewport_width, viewport_height)")
    assert effect.rfind("IDirect3DDevice8_End(global_d3d_device)", 0, matte) >= 0
    assert effect.index("rasterizer_set_target(\n\t\t\tglobal_window_parameters.rasterizer_target", matte) > matte
