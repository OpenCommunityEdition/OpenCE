"""The real rasterizer_active_camouflage_draw (rasterizer_xbox_active_camouflage.c) over a device that keeps each draw's
states and viewport and a pixel's depth, stencil and color: a cloaked first-person weapon and arms, fully cloaked or
fading, first set the depth under them to the far plane where none of their groups has been drawn (marking the
stencil), which on the Xbox was the world's and cut them, and are then drawn as the Xbox drew them; the pixel shows the
nearest part in front of the world whatever order the parts are drawn in, and another unit's cloak is drawn as before;
the screen the cloaked first-person weapon distorts is copied once, before any of its transparent geometry is drawn."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run  # noqa: E402

CASES = ["cloaked-first-person", "fading-first-person", "parts-in-order", "parts-out-of-order", "another-unit",
         "first-person-copy", "split-screen"]

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # drawn against the world's depth, as on the Xbox (before fix-camo-first-person-depth)
    "as-the-xbox": (("\t\t\trasterizer_active_camouflage_draw_depth(group, model, TRUE);\n", ""), "cloaked-first-person"),
    # the clearing draw tested against the world's depth, so cut where it is
    "clear-tested": (("clear ? D3DCMP_ALWAYS : D3DCMP_LESSEQUAL", "D3DCMP_LESSEQUAL"), "cloaked-first-person"),
    # the depth under the group set to the near plane
    "cleared-to-near": (("rasterizer_active_camouflage_window_depth(1.0f);",
                         "rasterizer_active_camouflage_window_depth(0.0f);"), "fading-first-person"),
    # cleared under every group, the parts drawn before too: a part behind drawn over them (the first fix's fault)
    "every-part-cleared": (("D3DRS_STENCILFUNC, D3DCMP_GREATER", "D3DRS_STENCILFUNC, D3DCMP_ALWAYS"),
                           "parts-in-order"),
    # the stencil left tested for the Xbox's draws
    "stencil-left-tested": (("\t\tIDirect3DDevice8_SetRenderState(global_d3d_device, D3DRS_STENCILENABLE, FALSE);\n",
                             ""), "cloaked-first-person"),
    # the stencil mode not set to none first, so the rasterizer's cache keeps the one before
    "stencil-mode-stale": (("\t\t\trasterizer_set_stencil_mode(RASTERIZER_STENCIL_MODE_NONE);\n", ""),
                           "cloaked-first-person"),
    # the depth range left pinned to the far plane after the clearing draw
    "not-restored": (("\t\trasterizer_active_camouflage_window_depth(0.0f);\n", ""), "cloaked-first-person"),
    # the screen not copied before the first-person weapon (its cloak then refracts its muzzle flash onto it)
    "no-first-person-copy": (("\t\t\trasterizer_active_camouflage_cache_primary_render_target();\n", ""),
                             "first-person-copy"),
    # copied for any first-person weapon, cloaked or not
    "copied-uncloaked": (("\t\t\tgroup->effect.type == _render_model_effect_type_active_camouflage &&\n", ""),
                         "first-person-copy"),
    # the whole screen's viewport, not the window's
    "whole-screen": (("viewport.X = global_window_parameters.camera.viewport_bounds.x0;", "viewport.X = 0;"),
                     "split-screen"),
}

ENUMERATIONS = ("_rasterizer_target_render_primary", "_rasterizer_geometry_first_person_bit", "_shader_type_model",
                "_shader_model_two_sided_bit", "_render_model_effect_type_active_camouflage",
                "_active_camouflage_tint_edge_density_bit", "_rasterizer_vertex_shader_active_camouflage",
                "RASTERIZER_STENCIL_MODE_NONE", "ACTIVE_CAMOUFLAGE_SCREEN_WIDTH")
FUNCTIONS = ("rasterizer_active_camouflage_window_depth", "rasterizer_active_camouflage_draw_depth",
             "rasterizer_active_camouflage_cache_for_first_person", "rasterizer_active_camouflage_draw")


def generated(fault=None):
    source = read("source/rasterizer/xbox/rasterizer_xbox_active_camouflage.c")
    config = "\n".join(enum_with(source, name) for name in ENUMERATIONS) + "\n"
    code = "\n".join(function(source, name) for name in FUNCTIONS) + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("config.inc", config), ("under_test.inc", code))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("camo_first_person_depth", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("camo_first_person_depth", generated(fault)), case)
    assert status == CHECK_FAILED, f"the cloak with a fault ({control}) passed '{case}': the test cannot see it"
