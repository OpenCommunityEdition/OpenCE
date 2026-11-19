"""Native input binding-list aliases, without linking SDL."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import build, function, read, run


def test_default_and_aliases():
    code = function(read("port/linux/game/mutator_scripts.c"), "script_input_held")
    status, output = run(build("mutator_input", (("under_test.inc", code),)), "aliases")
    assert status == 0, output
