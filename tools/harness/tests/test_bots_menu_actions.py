"""The actual bot UI and console handlers route typed actions and show feedback."""
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import build, function, read, run


def test_actions_and_host_feedback():
    code = read("port/linux/game/bots_menu.inl")
    code += function(read("port/linux/game/bots_network.c"), "bots_console_command")
    status, output = run(build("bots_menu", (("under_test.inc", code), ("bots.h", ""))), "actions")
    assert status == 0, output
