"""Host weapon cheats must not create unowned objectives that clients refuse."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import build, enum_with, function, read, run


@pytest.mark.parametrize("case", ["host-list", "host-fallback", "offline-list", "offline-fallback",
                                 "other-objects", "no-player"])
def test_cheat_spawns(case):
    source = read("source/game/cheats.c")
    config = "\n".join([
        enum_with(read("source/game/game.h"), "_game_connection_local"),
        enum_with(read("source/objects/object_types.h"), "_object_type_biped"),
        enum_with(read("source/items/weapon_definitions.h"), "_weapon_vertical_heat_display_bit"),
    ])
    code = "\n".join(function(source, name) for name in
                     ["cheat_objects", "cheat_objects_from_block", "cheat_all_weapons"])
    status, output = run(build("cheat_spawns", (("config.inc", config), ("under_test.inc", code))), case)
    assert status == 0, output
