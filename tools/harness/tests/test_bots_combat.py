"""Aim and Infection policy from the feature module, with engine inputs supplied."""
import sys
from pathlib import Path
import pytest
sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import read, function, structure, build, run


@pytest.mark.parametrize("case", ["precision", "explosive", "swat", "infection", "alone", "skill-mapping", "aim-precedence"])
def test_combat(case):
    source = read("port/linux/game/enhanced_bots.c")
    start = source.index("static struct\n{\n\treal head, feet")
    code = source[start:source.index("};", start) + 2] + "\n"
    code += "\n".join(function(source, name) for name in (
        "bots_random", "bots_random_real", "bots_distance2d", "bots_distance3d", "enhanced_bots_aim", "enhanced_bots_retreat", "enhanced_bots_powerup_score"))
    code += "\n" + function(read("port/linux/game/bots_combat.c"), "bots_combat_aim_head")
    code += "\n" + function(read("port/linux/game/bots.c"), "bots_level_aim_point")
    code += "\n" + read("port/linux/game/bots_infection.inl")
    status, output = run(build("bots_combat", (("under_test.inc", code), ("bots_combat.h", read("port/linux/game/bots_combat.h")))), case)
    assert status == 0, output
