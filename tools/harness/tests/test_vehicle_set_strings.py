"""The vehicle options' strings (port/assets/menus/ce/strings.xml) and the code that indexes them (menu_functions.c)."""
import re
import sys
import xml.etree.ElementTree as ElementTree
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
import port_settings
from harness import read

VEHICLE_EDIT = "main_menu/settings_select/multiplayer_setup/vehicle_options_edit"


def strings(name):
    """a list's strings: (text, its index in the PC version's list or None)"""
    menus = ElementTree.fromstring(read("port/assets/menus/ce/strings.xml").encode("latin-1"))
    for strings in menus.iter("strings"):
        if strings.get("name") == name:
            return [(string.get("text"), string.get("map")) for string in strings.iter("string")]
    raise LookupError(name)


def preset(name):
    return int(re.search(rf"^#define {name} (\d+)$", read("port/linux/game/menu_functions.c"), re.M).group(1))


def test_vehicle_sets_are_the_codes():
    sets = [text for text, _ in strings(f"{VEHICLE_EDIT}/var_vehicle_set")]
    assert len(sets) == preset("VEHICLE_PRESET_CUSTOM") + 1
    assert sets[preset("VEHICLE_PRESET_PC")] == "PC"
    assert sets[preset("VEHICLE_PRESET_CUSTOM")] == "CUSTOM"


def test_each_value_has_its_help():
    """gametype_option_help: the rows' values' helps in turn (respawn, side,
    set, then the six vehicles' counts)"""
    rows = [f"{VEHICLE_EDIT}/var_vehicles_respawn", f"{VEHICLE_EDIT}/var_team", f"{VEHICLE_EDIT}/var_vehicle_set"]
    values = sum(len(strings(row)) for row in rows) + 6 * len(strings(f"{VEHICLE_EDIT}/var_vehicle_options"))
    helps = [text for text, _ in strings(f"{VEHICLE_EDIT}/cap_vehicle_options")]
    assert len(helps) == values
    assert "Halo PC" in helps[7 + 2 + preset("VEHICLE_PRESET_PC")]
    assert helps[7 + 2 + preset("VEHICLE_PRESET_CUSTOM")].startswith("Customize")


def test_strings_xml_has_port_settings_inserts():
    """what tools/ce_menus.py writes from port_settings.STRING_INSERTS, so
    that making strings.xml again keeps them"""
    for name in (f"{VEHICLE_EDIT}/var_vehicle_set", f"{VEHICLE_EDIT}/cap_vehicle_options"):
        shown = strings(name)
        made = [text for text, original in shown if original is not None]
        for at, texts in port_settings.STRING_INSERTS[name]:
            made = made[:at] + texts + made[at:]
        assert made == [text for text, _ in shown], name
