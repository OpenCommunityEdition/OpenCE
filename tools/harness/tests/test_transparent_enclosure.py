"""Static enclosure recognition and actual native model-part linking, without GL."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, enum_with, function, mutated, read, run, structure
from harness.render_types import transparent_types

CASES = ["closed", "open", "outside", "reversed", "invalid", "strips", "uncompressed",
         "materials", "authored-links", "part-zero", "glass-zero", "opaque-part", "skinned",
         "additional-transparent", "frame-cost", "map-reload", "render-links"]
CONTROLS = {
    "revived-part-zero-gate": (("part->previous_part_index>=0", "part->previous_part_index>0"), "render-links"),
    "lost-enclosure-link": (("parts[energy].next_part_index = (char)glass;", "parts[energy].next_part_index = NONE;"), "part-zero"),
    "wrong-target-zero": (("if (glass == 0)", "if (FALSE)"), "glass-zero"),
    "overwrites-authored-links": (("parts[i].previous_part_index != NONE || parts[i].next_part_index != NONE ||", "FALSE ||"), "authored-links"),
    "accepts-open-mesh": (("reverse != 1 || forward != 1", "reverse < 0 || forward < 0"), "open"),
    "accepts-other-blend": (("core->framebuffer_blend_function != _framebuffer_blend_function_add", "FALSE"), "materials"),
}


def generated(control=None):
    models = read("source/models/models.c")
    raster = read("source/rasterizer/xbox/rasterizer_xbox_transparent_geometry.c")
    types = f"#define TAG_STRING_LENGTH {constant(read('source/tag_files/tag_files.h'), 'TAG_STRING_LENGTH')}\n#define UNSIGNED_SHORT_MAX {constant(read('source/cseries/cseries.h'), 'UNSIGNED_SHORT_MAX')}\n" + transparent_types()
    for path, names in [
        ("source/models/model_definitions.h", ["model", "model_region", "model_region_permutation"]),
        ("source/models/models.c", ["model_shader_reference", "model_geometry", "model_geometry_part", "shader_model_definition", "render_sort_filth"]),
        ("source/render/render.h", ["render_skinning"]),
        ("source/cache/cache_files.h", ["tag_iterator"]),
    ]:
        types += "\n" + "\n".join(structure(read(path), name) for name in names)
    for member in ["_render_model_immediate_bit", "_render_model_pass_solid", "_model_geometry_part_stripped_bit", "_shader_model_detail_after_reflection_bit"]:
        types += "\n" + enum_with(models, member)
    types += "\n" + enum_with(read("source/models/model_definitions.h"), "MODELS_GROUP_TAG")
    code = read("source/rasterizer/rasterizer_transparent_enclosure.h")
    code += "\n" + function(raster, "rasterizer_transparent_geometry_is_enclosure")
    for name in ["model_geometry_fix_transparent_part_links", "models_fix_transparent_part_links", "render_model_parts"]:
        code += "\n" + function(models, name)
    if control:
        fault, _ = CONTROLS[control]
        code = mutated(code, *fault)
    return (("types.inc", types), ("under_test.inc", code))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("transparent_enclosure", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", CONTROLS)
def test_negative_control(control):
    status, output = run(build("transparent_enclosure", generated(control)), CONTROLS[control][1])
    assert status == CHECK_FAILED, f"{control} escaped its check: {output}"
