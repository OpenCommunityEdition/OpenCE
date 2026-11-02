"""Objective priorities and optional vehicle input, using the real feature functions."""
import sys
from pathlib import Path
import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import build, function, read, run, structure


@pytest.mark.parametrize("case", ["carrier", "standoff", "recovery", "powerups"])
def test_objectives(case):
    source = read("port/linux/game/enhanced_bots.c")
    start = source.index("static struct\n{\n\treal head, feet")
    code = source[start:source.index("};", start) + 2] + "\n"
    code += "\n".join(function(source, name) for name in (
        "bots_distance3d", "bots_ctf_urgent_goal", "enhanced_bots_powerup_score", "enhanced_bots_retreat"))
    status, output = run(build("bot_decisions", (("under_test.inc", code), ("bots_combat.h", read("port/linux/game/bots_combat.h")))), case)
    assert status == 0, output


@pytest.mark.parametrize("case", ["seat", "driver", "disable", "gunner", "stuck", "ghost", "claim", "boarding", "turret", "flip"])
def test_vehicles(case):
    source = read("port/linux/game/bots_vehicles.c")
    code = structure(source, "bots_vehicle_definition") + "\n"
    # The declaration includes the vehicles array after the structure body.
    code += "static " + structure(source.replace("static struct bots_vehicle_state", "struct bots_vehicle_state"), "bots_vehicle_state") + "\n"
    code += "\n".join(function(source, name) for name in (
        "distance", "angle", "definition", "drivable", "bots_vehicles_reset_player", "bots_vehicles_reset",
        "bots_vehicle_parent", "bots_vehicle_navigation", "bots_vehicle_weapon", "turret_seat", "seat_takeover", "seat_available",
        "friendly_driver", "vehicle_free", "boarding", "turret_action", "bots_vehicle_action"))
    status, output = run(build("bot_vehicles", (("under_test.inc", code),)), case)
    assert status == 0, output
