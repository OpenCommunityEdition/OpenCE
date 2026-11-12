"""Player arrows and names must agree on MCC bitmap units across map changes."""
import pytest
from tools.harness import build, function, mutated, read, run


def code():
    hud = read("source/interface/hud.c")
    return "\n".join([
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
    if fault == "arrow":
        source = mutated(source,
            "mcc_hud_bitmap_scale(interface_get_tag_index(_interface_bitmap_multiplayer_hud)), /* port: MCC bitmap units */",
            "1.0f,")
    else:
        source = mutated(source,
            "y -= (short)(bitmap->registration_point.y *\n\t\t\t\tmcc_hud_bitmap_scale(interface_get_tag_index(_interface_bitmap_multiplayer_hud)));",
            "y -= bitmap->registration_point.y;")
    status, output = execute(source, "half")
    assert status == 1 and "check failed" in output, output
