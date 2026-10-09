"""Actual transparent queue sorting, skinning and plane ordering, without GL."""
import sys
import re
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, function, mutated, read, run, structure
from harness.render_types import transparent_types

CASES = ["same-side", "opposite-side", "reverse-normal", "unrelated", "multiple-models",
         "multiple-planes", "conflict", "intersect", "camera-plane", "invalid-plane",
         "invalid-model", "materials", "rigid-transform", "skinned", "compressed-skinning",
         "linked", "linked-union", "invalid-links", "full-queue", "four-views", "no-glass",
         "external-sort", "fallback-flags", "invalid-order"]
CONTROLS = {
    "same-side-backwards": (("? -1 : 1;", "? 1 : -1;"), "same-side"),
    "camera-ignored": (("(distance > 0) == (side > 0)", "side > 0"), "opposite-side"),
    "model-left-in-wrong-gap": (("(short)PIN(original_gap[root], lower, upper)", "original_gap[root]"), "unrelated"),
    "not-adjacent-to-glass": (("for (phase = -1; phase <= 1; ++phase)", "for (phase = 1; phase >= -1; --phase)"), "unrelated"),
    "only-final-linked-part-bounded": (("steps == 1 ?", "TRUE ?"), "linked-union"),
    "skinning-ignored": (("group->node_matrix_count == 1", "TRUE"), "skinned"),
    "compressed-byte-weight": (("(real)vertex->node_weight * (1.0f / 32767.0f)", "(real)(byte)vertex->node_weight * (1.0f / 255.0f)"), "compressed-skinning"),
    "intersection-accepted": (("high <= epsilon && low < -epsilon", "low < -epsilon"), "intersect"),
    "sort-hook-removed": (("rasterizer_transparent_geometry_order_models(\n\t\ttransparent_geometry_group_sorted_indices, transparent_geometry_group_count);", "(void)0;"), "external-sort"),
}


def generated(control=None):
    raster = read("source/rasterizer/xbox/rasterizer_xbox_transparent_geometry.c")
    core = read("source/rasterizer/rasterizer_transparent_geometry.c")
    types = transparent_types()
    for path, names in [
        ("source/math/real_math.h", ["real_matrix4x3"]),
        ("source/rasterizer/rasterizer_model_types.h", ["model_vertex_compressed", "model_vertex_uncompressed"]),
    ]:
        types += "\n" + "\n".join(structure(read(path), name) for name in names)
    for path, name in [("source/cseries/cseries.h", "UNSIGNED_SHORT_MAX"),
                       ("source/models/model_definitions.h", "MAXIMUM_NODES_PER_MODEL"),
                       ("source/rasterizer/rasterizer.h", "RASTERIZER_MAXIMUM_TRANSPARENT_GEOMETRY_GROUPS")]:
        types += f"\n#define {name} {constant(read(path), name)}\n"
    code = function(read("source/math/matrix_math.c"), "matrix4x3_transform_point")
    for name in ["shader_is_water_decal", "shader_type_is_transparent", "shader_type_is_valid_for_model"]:
        code += "\n" + function(read("source/shaders/shaders.c"), name)
    for name in ["transparent_planar_glass", "transparent_model_world_bounds", "transparent_plane_model_order",
                 "transparent_chain_root", "rasterizer_transparent_geometry_order_models"]:
        code += "\n" + function(raster, name)
    for name in ["rasterizer_sort_internal", "group_sorted_indices_cmpfn", "rasterizer_sort_external"]:
        code += "\n" + function(core, name)
    if control:
        before, after = CONTROLS[control][0]
        if control == "only-final-linked-part-bounded":
            # Both ends of the union deliberately lose the previous parts.
            assert code.count(before) == 2
            code = code.replace(before, after)
        else:
            code = mutated(code, before, after)
    return (("types.inc", types), ("under_test.inc", code))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("transparent_plane_order", generated()), case)
    assert status == 0, output


def test_compressed_weight_declaration():
    # Confirm that the CPU's signed-short weights match the actual draw stream.
    source = read("source/rasterizer/xbox/rasterizer_xbox_vertex_shaders_initialize.c")
    declaration = re.search(r"vertex_shader_declarations\[\]\s*=\s*\{([^}]+)\}", source).group(1)
    tokens = [int(v, 16) for v in re.findall(r"0x[\dA-Fa-f]+", declaration)]
    offset = int(re.search(r"vertex_shader_table\[13\]\.declaration = &vertex_shader_declarations\[(0x[\dA-Fa-f]+)", source).group(1), 16) // 4
    weight = next(v for v in tokens[offset:offset+9] if v != 0xFFFFFFFF and v & 0xFFFF == 6)
    assert (weight >> 16) & 0xFF == constant(read("port/include/xdk/xdk_d3d8.h"), "D3DVSDT_NORMSHORT1")


@pytest.mark.parametrize("control", CONTROLS)
def test_negative_control(control):
    status, output = run(build("transparent_plane_order", generated(control)), CONTROLS[control][1])
    assert status == CHECK_FAILED, f"{control} escaped its check: {output}"
