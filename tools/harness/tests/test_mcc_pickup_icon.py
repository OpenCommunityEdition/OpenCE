"""Gameplay icon placement follows native MCC's common bitmap scale."""
import pytest
from tools.harness import build, function, mutated, read, run, structure


def code():
    source = read("port/linux/game/mcc_hud_draw.c")
    return "\n".join(function(source, name) for name in
                     ("mcc_hud_bitmap_scale", "mcc_hud_icon_draw"))


def execute(source, case):
    definition = structure(read("source/interface/hud_definitions.h"), "icon_hud_element_definition")
    return run(build("mcc_pickup_icon", (("definition.inc", definition),
                                        ("under_test.inc", source))), case)


@pytest.mark.parametrize("case", ["plain", "half", "highres", "split", "signed",
                                  "clip", "absolute", "menu", "legacy", "transition", "button"])
def test_icon_layout(case):
    status, output = execute(code(), case)
    assert status == 0, output


@pytest.mark.parametrize("fault,case", [("scale", "half"), ("round", "signed"), ("advance", "absolute")])
def test_negative_control(fault, case):
    source = code()
    if fault == "scale":
        source = mutated(source, "offset_scale=menu ? viewport_scale : pixel_scale;", "offset_scale=viewport_scale;")
    elif fault == "round":
        source = mutated(source, "point.x=cursor->x0+(short)(icon->offset.x*offset_scale);",
                         "point.x=(short)(cursor->x0+icon->offset.x*offset_scale);")
    else:
        source = mutated(source, "advance=icon->width_offset*offset_scale;", "advance=icon->width_offset*viewport_scale;")
    status, output = execute(source, case)
    assert status == 1 and "check failed" in output, output
