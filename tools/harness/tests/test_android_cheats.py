"""Real typed cheat queue and game-side permissions/lifecycle, without Android assets."""
import re
import sys
from pathlib import Path
import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import build, read, run

@pytest.mark.parametrize("case", ["disabled", "offline", "network", "reset", "queue", "cinematic", "all-cheats",
                                 "host", "host-disabled", "host-client", "client-lobby", "host-reset", "host-cinematic"])
def test_cheat_menu(case):
    source = read("port/android/host/host_android_cheats.c") + read("port/linux/game/android_cheats.c")
    source = re.sub(r'^#include.*\n', '', source, flags=re.M)
    status, output = run(build("android_cheats", (("under_test.inc", source),)), case)
    assert status == 0, output
