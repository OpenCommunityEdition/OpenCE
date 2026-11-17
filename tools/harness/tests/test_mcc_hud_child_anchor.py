"""MCC child anchors must reach every weapon HUD element's draw call."""
import re

import pytest

from tools.harness import build, function, read, run


@pytest.mark.parametrize("draw", [
    "hud_draw_static_element", "hud_draw_meter", "hud_draw_numbers",
    "hud_draw_weapon_overlays",
])
def test_resolved_anchor_reaches_draw(draw):
    # The original regression computed the correct placement but discarded it.
    source = function(read("source/interface/hud_weapon.c"), "render_weapon_hud")
    calls = re.findall(r"\b" + draw + r"\(\s*local_player_index,\s*(\w+)\s*,", source)
    assert calls == ["placement"]


def test_anchor_selection_and_map_transitions():
    source = function(read("port/linux/game/mcc_hud_draw.c"), "mcc_hud_child_placement")
    status, output = run(build("mcc_hud_child_anchor", (("under_test.inc", source),)), "anchors")
    assert status == 0, output
