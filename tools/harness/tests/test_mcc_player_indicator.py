"""Player arrows and names must agree on MCC bitmap units across map changes."""
import pytest
from tools.harness import build, function, mutated, read, run


def code():
    hud = read("source/interface/hud.c")
    return "\n".join([
        function(read("port/linux/game/custom_edition_hud_scale.h"), "custom_edition_hud_scale"),
        function(read("port/linux/game/custom_edition_hud_draw.c"), "custom_edition_hud_bitmap_scale"),
        function(read("port/linux/game/mcc_hud_draw.c"), "mcc_hud_bitmap_scale"),
        function(hud, "hud_draw_friendly_indicator"),
        function(hud, "hud_draw_player_name"),
    ])


def execute(source, case):
    return run(build("mcc_player_indicator", (("under_test.inc", source),)), case)


@pytest.mark.parametrize("case", ["plain", "half", "highres", "transitions"])
def test_indicator_and_name_units(case):
    status, output = execute(code(), case)
    assert status == 0, output


@pytest.mark.parametrize("fault", ["arrow", "name"])
def test_missing_scaling_is_detected(fault):
    source = code()
    name = "hud_draw_friendly_indicator" if fault == "arrow" else "hud_draw_player_name"
    original = function(source, name)
    changed = mutated(original,
        "mcc_hud_bitmap_scale(interface_get_tag_index(_interface_bitmap_multiplayer_hud))",
        "1.0f")
    status, output = execute(mutated(source, original, changed), "half")
    assert status == 1 and "check failed" in output, output
