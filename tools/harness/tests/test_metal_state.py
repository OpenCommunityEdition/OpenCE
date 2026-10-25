"""The Metal device's translations of Direct3D 8 state into the render ring's values (d3d8_metal_device.c): the
pipeline's, depth-stencil state's and samplers' keys, and the primitives Metal lacks drawn through indices."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run  # noqa: E402

CASES = ["compare", "blend", "stencil", "sampler", "cull", "write-mask", "primitives", "primitive-indices"]

FUNCTIONS = [
    "ring_compare",
    "ring_blend_factor",
    "ring_blend_operation",
    "ring_stencil_operation",
    "ring_address",
    "ring_filter",
    "ring_border",
    "ring_cull",
    "write_mask",
    "ring_primitive",
    "primitive_indices",
]

ENUMS = [
    "D3DCMP_NEVER",
    "D3DBLEND_ZERO",
    "D3DBLENDOP_ADD",
    "D3DSTENCILOP_KEEP",
    "D3DTADDRESS_WRAP",
    "D3DTEXF_NONE",
    "D3DCULL_NONE",
    "D3DFRONT_CW",
    "D3DPT_POINTLIST",
]

# a fault in the translations and the case that must catch it
NEGATIVE_CONTROLS = {
    "compare-off-by-one": (
        (
            "(unsigned int)(function - D3DCMP_NEVER) + OPENCE_RENDER_COMPARE_NEVER",
            "(unsigned int)(function - D3DCMP_NEVER) + OPENCE_RENDER_COMPARE_LESS",
        ),
        "compare",
    ),
    "cull-swapped": (
        ("OPENCE_RENDER_CULL_FRONT : OPENCE_RENDER_CULL_BACK", "OPENCE_RENDER_CULL_BACK : OPENCE_RENDER_CULL_FRONT"),
        "cull",
    ),
    "red-as-blue": (
        ("D3DCOLORWRITEENABLE_RED) ? OPENCE_RENDER_WRITE_RED", "D3DCOLORWRITEENABLE_RED) ? OPENCE_RENDER_WRITE_BLUE"),
        "write-mask",
    ),
    "quad-wrong-corner": (("VERTEX(index + 3)", "VERTEX(index + 1)"), "primitive-indices"),
    "signed-reverse-as-add": (
        ("case D3DBLENDOP_REVSUBTRACT: case D3DBLENDOP_REVSUBTRACTSIGNED:", "case D3DBLENDOP_REVSUBTRACT:"),
        "blend",
    ),
}


def generated(fault=None):
    device = read("port/macos/guest/d3d8_metal_device.c")
    xdk = read("port/include/xdk/xdk_pdb.h")
    d3d8 = read("port/include/xdk/xdk_d3d8.h")
    config = "\n".join(enum_with(xdk, member) for member in ENUMS) + "\n"
    config += "".join(
        line + "\n"
        for line in d3d8.splitlines()
        if line.startswith("#define D3DCOLORWRITEENABLE_") and "_ALL" not in line
    )
    config += (
        "#define D3DCOLORWRITEENABLE_ALL (D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | "
        "D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA)\n"
    )
    code = "\n\n".join(function(device, name) for name in FUNCTIONS)
    if fault:
        code = mutated(code, *fault)
    return (
        ("opence_render.h", read("port/macos/include/opence_render.h")),
        ("config.inc", config),
        ("under_test.inc", code + "\n"),
    )


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("metal_state", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("metal_state", generated(fault)), case)
    assert status == CHECK_FAILED, (
        f"the state translations with a fault ({control}) passed '{case}': the test cannot see it"
    )
