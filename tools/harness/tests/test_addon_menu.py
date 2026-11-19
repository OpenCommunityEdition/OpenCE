"""Filtered loadout rows and their gameplay slot lookup use the same ordering."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import build, function, read, run


def generated():
    menus = read("port/linux/game/menu_functions.c")
    addons = read("port/linux/game/addons.c")
    code = "\n".join(function(menus, name) for name in ("addon_of_kind", "addons_of_kind"))
    code += "\n" + function(addons, "addon_slot")
    code += "\n" + function(addons, "addons_weapon_definition")
    return (("under_test.inc", code),)


@pytest.mark.parametrize("case", ["hidden_first", "hidden_middle", "all_visible"])
def test_slots(case):
    status, output = run(build("addon_menu", generated()), case)
    assert status == 0, output
