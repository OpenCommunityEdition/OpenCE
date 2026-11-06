"""The real nv2a_pixel_shader_to_glsl (port/linux/src/nv2a_psh.c): the active camouflage's distortion gets its texture
coordinates 1 and 2, places on the screen, interpolated linearly across it (noperspective) on desktop OpenGL, so that a
triangle reaching toward the camera does not read the copy of the screen far off its edge; every other shader, and all
on OpenGL ES, keep the perspective."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, enum_with, function, mutated, read, run, structure  # noqa: E402

CASES = ["cloak", "others"]
VARIANTS = ["desktop", "gles"]

D3D_NAMES = ("D3DRS_PS_MAX", "D3DRS_PSALPHAINPUTS0", "D3DRS_PSALPHAOUTPUTS0", "D3DRS_PSCOMBINERCOUNT",
             "D3DRS_PSCOMPAREMODE", "D3DRS_PSDOTMAPPING", "D3DRS_PSFINALCOMBINERINPUTSABCD",
             "D3DRS_PSFINALCOMBINERINPUTSEFG", "D3DRS_PSINPUTTEXTURE", "D3DRS_PSRGBINPUTS0", "D3DRS_PSRGBOUTPUTS0",
             "D3DCMP_NEVER", "D3DCMP_LESS", "D3DCMP_EQUAL", "D3DCMP_LESSEQUAL", "D3DCMP_GREATER", "D3DCMP_NOTEQUAL",
             "D3DCMP_GREATEREQUAL", "D3DFOG_EXP", "D3DFOG_EXP2", "D3DFOG_LINEAR", "D3DTSS_COLORSIGN",
             "D3DTSS_MIPMAPLODBIAS")

# a fault in the code, and the case and variant that must catch it
NEGATIVE_CONTROLS = {
    # the coordinates interpolated with the perspective, as before fix-camo-screen-coordinates
    "with-the-perspective": (('screen_place_coordinates(key) ? "noperspective " : "",\n'
                              '\t\tscreen_place_coordinates(key) ? "noperspective " : "");',
                              '"", "");'), "cloak", "desktop"),
    # only coordinate 1 linear
    "one-coordinate": (('\t\tscreen_place_coordinates(key) ? "noperspective " : "");', '\t\t"");'), "cloak", "desktop"),
    # every shader with a dot product stage
    "every-dot-product": (("stage_mode(key, 1) == _mode_dot_product && stage_mode(key, 2) == _mode_dot_st",
                           "stage_mode(key, 1) == _mode_dot_product"), "others", "desktop"),
    # noperspective on OpenGL ES, which has none
    "on-es-too": (("#ifdef HALO_GLES\n\t(void)key;", "#if 0\n\t(void)key;"), "cloak", "gles"),
}


def generated(variant, fault=None):
    xdk = read("port/include/xdk/xdk_pdb.h")
    xgpu = read("port/linux/src/xgpu.h")
    start = xgpu.index("#ifdef HALO_GLES\n/* ES samplers")
    end = xgpu.index("#define XGPU_MODEL_LIGHT_COUNT")
    config = "\n".join(
        (["#define HALO_GLES"] if variant == "gles" else []) +
        ["enum { %s };" % ", ".join("%s = %d" % (name, constant(xdk, name)) for name in D3D_NAMES),
         enum_with(xgpu, "_xgpu_sampler_none"),
         structure(xgpu, "xgpu_text"),
         structure(xgpu, "nv2a_pixel_shader_key"),
         xgpu[start:xgpu.index("\n", end)],
         function(read("port/linux/src/xgpu_text.c"), "xgpu_text_append")]) + "\n"
    code = "\n".join(line for line in read("port/linux/src/nv2a_psh.c").split("\n")
                     if not re.match(r"\s*#include", line)) + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("config.inc", config), ("under_test.inc", code))


@pytest.mark.parametrize("variant", VARIANTS)
@pytest.mark.parametrize("case", CASES)
def test_case(case, variant):
    status, output = run(build("shader_screen_place", generated(variant)), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case, variant = NEGATIVE_CONTROLS[control]
    status, output = run(build("shader_screen_place", generated(variant, fault)), case)
    assert status == CHECK_FAILED, f"the translator with a fault ({control}) passed '{case}': the test cannot see it"
