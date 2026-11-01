"""The retained route/vantage allocators, including partial and repeated disposal."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import build, function, read, run


def test_disposal():
    source = function(read("port/linux/game/bots_routes.c"), "bots_routes_free")
    source += function(read("port/linux/game/bots_vantage.c"), "bots_vantage_free")
    status, output = run(build("bots_lifecycle", (("under_test.inc", source),)), "dispose")
    assert status == 0, output
