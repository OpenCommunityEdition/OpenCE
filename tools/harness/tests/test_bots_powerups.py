"""Contested powerups use distance and a stable tie-break, not duplicate pursuit."""
import sys
from pathlib import Path
import pytest
sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import read, function, build, run


@pytest.mark.parametrize("case", ["closer", "tie", "released"])
def test_claims(case):
    code = function(read("port/linux/game/bots_powerups.inl"), "bots_powerup_claimed")
    status, output = run(build("bots_powerups", (("under_test.inc", code),)), case)
    assert status == 0, output
