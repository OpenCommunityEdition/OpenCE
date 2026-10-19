"""Generated team-wipe choices, help and row bounds in Co-op Options."""
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path

import port_settings

NAME = 'main_menu.multiplayer_type_select.server_settings.xml'
BASE = 'main_menu/multiplayer_type_select/server_settings/coop_options/'


class CoopWipeMenuTest(unittest.TestCase):
    def test_generated_screen(self):
        text = '\n'.join(port_settings.multiplayer_files()[NAME])
        root = ET.fromstring(text)
        widgets = {widget.get('name'): widget for widget in root.findall('widget')}
        spinner = widgets[BASE + 'coop_team_wipe_spinner']
        strings = root.find(f"strings[@name='{spinner.get('string_list')}']")
        self.assertEqual([item.get('text') for item in strings], ['AUTO', 'REVERT CHECKPOINT', 'RESPAWN ONLY'])
        menu = widgets[BASE + 'coop_options_menu']
        rows = [child for child in menu.findall('child') if '/op_coop_' in child.get('widget', '')]
        self.assertEqual(len(rows), 8)
        self.assertTrue(any(row.get('widget') == BASE + 'op_coop_team_wipe' for row in rows))
        help_top = int(widgets[BASE + 'coop_options_help'].get('top'))
        for row in rows:
            self.assertLess(int(row.get('y')) + int(widgets[row.get('widget')].get('height')), help_top)
        committed = Path(__file__).resolve().parents[1] / 'port/assets/menus/ce' / NAME
        self.assertEqual(committed.read_text(), text)


if __name__ == '__main__':
    unittest.main()
