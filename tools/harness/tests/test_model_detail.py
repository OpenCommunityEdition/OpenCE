"""display.model_detail's level for each permutation: the most detailed it has, and a model's inside only where the
Xbox drew it, its size on the 480-line screen at its cutoff (port/linux/game/model_detail.c, from eltanschauung's #282)."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run

CASES = ["interior", "reached", "permutations", "rules"]
CONTROLS = {
    "shown-interior": (("rules==_model_detail_rules_inside ||", "TRUE ||"), "interior"),
    "no-fallback": (("permutation->geometry_indices[level]!=NONE &&", "TRUE &&"), "permutations"),
    "below-cutoff": (("return stock_pixels>=model->detail_cutoff_pixels[level];",
                      "return stock_pixels>model->detail_cutoff_pixels[level];"), "reached"),
    "any-size": (("return stock_pixels>=model->detail_cutoff_pixels[level];", "return TRUE;"), "reached"),
    "any-model": (("\t}\n\treturn FALSE;\n}", "\t}\n\treturn TRUE;\n}"), "reached"),
    "debug-level-ignored": (("|| rasterizer_debug_options.debug_model_lod!=NONE", ""), "rules"),
}


def generated(control=None):
    source = read("port/linux/game/model_detail.c")
    code = enum_with(read("port/linux/game/model_detail.h"), "_model_detail_rules_stock")
    code += "\n" + enum_with(source, "MODEL_DETAIL_INTERIOR_CUTOFF_PIXELS")
    for name in ["model_detail_inside_reached", "model_detail_rules", "model_detail_geometry_index"]:
        code += "\n" + function(source, name)
    if control:
        code = mutated(code, *CONTROLS[control][0])
    return (("under_test.inc", code),)


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("model_detail", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", CONTROLS)
def test_negative_control(control):
    status, output = run(build("model_detail", generated(control)), CONTROLS[control][1])
    assert status == CHECK_FAILED, f"{control} escaped its check: {output}"
