"""The real batches and kept maps of the objects' shadows (render_objects.c), frame after frame: without
display.shadow_map_reuse every map is drawn each frame; with it a small shadow's every other frame, a large one's each
frame, every map again when the light turns or the slots were freed, and the kept maps within their byte budget."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, function, mutated, read, run, structure  # noqa: E402

CASES = ["off", "small-every-other-frame", "large-every-frame", "light-turned", "slots-freed", "budget"]

# a fault in the shadows' code and the case that must catch it
NEGATIVE_CONTROLS = {
    # kept maps reused without the setting
    "reused-without-the-setting": (("if (halo_shadow_map_reuse())", "if (TRUE)"), "off"),
    # every entry the cache has, whatever the budget
    "budget-ignored": (("shadow_port_cache_count = halo_shadow_map_slots(SHADOW_PORT_CACHE_BYTES);",
                        "shadow_port_cache_count = SHADOW_PORT_CACHE_SIZE;"), "budget"),
    # the light's direction not compared
    "light-ignored": (("dot_product3d(&data->shadow_matrix.up, &cached->saved.matrix.up) < 0.9995f ||", ""),
                      "light-turned"),
    # the entries kept when the slots were freed
    "freed-slots-kept": (("csmemset(shadow_port_cache, 0, sizeof(shadow_port_cache));", ""), "slots-freed"),
    # small shadows' maps drawn each frame
    "small-each-frame": (("SHADOW_PORT_REDRAW_PIXELS ? 1 : 2;", "SHADOW_PORT_REDRAW_PIXELS ? 1 : 1;"),
                         "small-every-other-frame"),
}


def generated(fault=None):
    source = read("source/render/render_objects.c")
    rasterizer = read("source/rasterizer/rasterizer.h")
    config = (f"#define RASTERIZER_SHADOW_PORT_BATCH {constant(rasterizer, 'RASTERIZER_SHADOW_PORT_BATCH')}\n"
              + structure(rasterizer, "rasterizer_shadow_port_saved") + "\n"
              + structure(source, "object_render_data") + "\n")
    # the batches' and kept maps' state and code, up to render_object_shadows, and it
    start = source.index("/* port: the shadows drawn in batches, each batch's maps one after another")
    end = source.index("void render_object_shadows(\n")
    code = source[start:end] + function(source, "render_object_shadows")
    if fault:
        code = mutated(code, *fault)
    return (("config.inc", config), ("under_test.inc", code + "\n"))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("shadow_map_reuse", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("shadow_map_reuse", generated(fault)), case)
    assert status == CHECK_FAILED, f"the shadows with a fault ({control}) passed '{case}': the test cannot see it"
