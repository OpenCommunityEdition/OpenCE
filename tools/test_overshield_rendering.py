"""The retained body effect keeps its authored brightness and rise."""
import re
import shutil
import subprocess
import sys
from pathlib import Path

import pytest
from harness import constant, function, structure

ROOT = Path(__file__).resolve().parents[1]


def test_overshield_body_rendering(tmp_path):
    compiler = shutil.which("clang") or shutil.which("cc")
    if not compiler:
        pytest.skip("a C compiler is required")
    text = (ROOT / "port/linux/game/overshield_effect.c").read_text()
    code = r'''
#include <string.h>
typedef float real;
typedef int boolean;
typedef struct {real red, green, blue;} real_rgb_color;
#define FALSE 0
#define TRUE 1
#define NONE (-1)
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define CHECK(c) do {if (!(c)) return __LINE__;} while (0)
static int near(real a, real b) {real d = a - b; return d > -.00001f && d < .00001f;}
'''
    code += f"\n#define OVERSHIELD_EFFECT_MAXIMUM {constant(text, 'OVERSHIELD_EFFECT_MAXIMUM')}\n"
    code += "\n".join(re.findall(r"^#define OVERSHIELD_EFFECT_(?:ENERGY_SPEED|EDGE) .*", text, re.M))
    code += "\n" + structure(text, "overshield_effect")
    code += "\nstatic struct overshield_effect overshield_effects[OVERSHIELD_EFFECT_MAXIMUM];\n"
    code += re.search(r"static real_rgb_color const overshield_effect_rgb = .*?;", text).group()
    code += "\n" + function(text, "overshield_effect_draw")
    code += "\n" + function(text, "overshield_effect_color")
    code += r'''
int main(void) {
    struct overshield_effect *effect = &overshield_effects[0];
    real own[4] = {0}, speed, softness, height;
    real_rgb_color perpendicular = {.1f, .8f, .2f}, parallel = {.3f, .2f, .1f};
    real_rgb_color original_perpendicular = perpendicular, original_parallel = parallel;
    effect->unit_index = 7;
    effect->bottom = -1.f; effect->top = 1.f; effect->progress = .5f;
    CHECK(!overshield_effect_color(NULL, &perpendicular, &parallel));
    CHECK(!overshield_effect_color(own, &perpendicular, &parallel));
    CHECK(!memcmp(&perpendicular, &original_perpendicular, sizeof(perpendicular)));
    CHECK(!memcmp(&parallel, &original_parallel, sizeof(parallel)));
    CHECK(overshield_effect_draw(effect->body_values, &speed, &softness, &height));
    CHECK(near(speed, .3f) && near(softness, 1.f/.24f) && near(height, .12f));
    CHECK(overshield_effect_color(effect->body_values, &perpendicular, &parallel));
    CHECK(near(perpendicular.red, .8f) && near(perpendicular.green, .306f/.992f*.8f));
    CHECK(near(perpendicular.blue, .455f/.992f*.8f));
    CHECK(near(parallel.red, .3f) && near(parallel.green, .306f/.992f*.3f));
    CHECK(near(parallel.blue, .455f/.992f*.3f));
    effect->progress = 1.f;
    CHECK(overshield_effect_draw(effect->body_values, &speed, &softness, &height));
    CHECK(softness == 0 && height == 0);
    effect->unit_index = NONE;
    CHECK(!overshield_effect_draw(effect->body_values, &speed, &softness, &height));
    return 0;
}
'''
    source = tmp_path / "overshield.c"
    source.write_text(code)
    executable = tmp_path / ("overshield.exe" if sys.platform == "win32" else "overshield")
    flags = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld"] if sys.platform == "win32" else []
    subprocess.run([compiler, "-std=gnu89", *flags, str(source), "-o", str(executable)], check=True)
    subprocess.run([str(executable)], check=True, timeout=10)
