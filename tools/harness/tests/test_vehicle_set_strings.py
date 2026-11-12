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
    """gametype_option_help: the rows' values' helps in turn (respawn, riding,
    side, set, then the six vehicles' counts)"""
    rows = [f"{VEHICLE_EDIT}/var_vehicles_respawn", f"{VEHICLE_EDIT}/var_team", f"{VEHICLE_EDIT}/var_vehicle_set"]
    extra = ElementTree.fromstring("\n".join(port_settings.multiplayer_files()[VEHICLE_EDIT.replace('/', '.') + '.port.xml']))
    riding = extra.find(f"strings[@name='{VEHICLE_EDIT}/var_vehicle_riding']")
    assert riding is not None
    rows.insert(1, f"{VEHICLE_EDIT}/var_vehicle_riding")
    values = sum(len(riding) if row.endswith('/var_vehicle_riding') else len(strings(row)) for row in rows) + \
        6 * len(strings(f"{VEHICLE_EDIT}/var_vehicle_options"))
    helps = [text for text, _ in strings(f"{VEHICLE_EDIT}/cap_vehicle_options")]
    assert len(helps) == values
    assert "Halo PC" in helps[7 + 2 + 2 + preset("VEHICLE_PRESET_PC")]
    assert helps[7 + 2 + 2 + preset("VEHICLE_PRESET_CUSTOM")].startswith("Customize")
    assert helps[7:9] == ["Players standing on a vehicle are carried by it.\\nOff: as the original game, riders slide off."] * 2


def test_strings_xml_has_port_settings_inserts():
    """what tools/ce_menus.py writes from port_settings.STRING_INSERTS, so
    that making strings.xml again keeps them"""
    for name in (f"{VEHICLE_EDIT}/var_vehicle_set", f"{VEHICLE_EDIT}/cap_vehicle_options",
                 f"{VEHICLE_EDIT}/vehicle_options_labels"):
        shown = strings(name)
        made = [text for text, original in shown if original is not None]
        for at, texts in port_settings.STRING_INSERTS[name]:
            made = made[:at] + texts + made[at:]
        assert made == [text for text, _ in shown], name


def test_vehicle_riding_committed_patch():
    """The committed PC-derived child list agrees with the generator's patch."""
    root = ElementTree.fromstring(read('port/assets/menus/ce/' + VEHICLE_EDIT.replace('/', '.') + '.xml'))
    children = root.find(f"widget[@name='{VEHICLE_EDIT}/vehicle_options_menu']").findall('child')
    names = [child.get('widget') for child in children]
    respawn = names.index(VEHICLE_EDIT + '/op_vehicles_respawn')
    assert names[respawn + 1:respawn + 3] == [VEHICLE_EDIT + '/op_vehicle_riding', VEHICLE_EDIT + '/op_team']
    riding = children[respawn + 1]
    assert riding.attrib == {'widget': VEHICLE_EDIT + '/op_vehicle_riding', 'x': '54', 'y': '100'}
    patch = port_settings.WIDGET_PATCHES[VEHICLE_EDIT + '/vehicle_options_menu']['insert_before']
    assert ElementTree.fromstring(patch[VEHICLE_EDIT + '/op_team'][0]).attrib == riding.attrib
    assert strings(VEHICLE_EDIT + '/vehicle_options_labels')[9] == ('VEHICLE RIDING:', None)
