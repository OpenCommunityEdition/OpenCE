"""Production display-only policy plus real parser/config restart tests."""
from pathlib import Path
import re
import sys
import xml.etree.ElementTree as ET
import test_console_options as fixture

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools import port_settings

menu_path = ROOT/'port/assets/menus/ce/main_menu.settings_select.player_setup.player_profile_edit.video_settings.xml'
assert menu_path.read_text() == '\n'.join(port_settings.settings_files()[menu_path.name])
menu = ET.parse(menu_path).getroot()
spinner = next(w for w in menu.findall('widget') if w.get('setting') == 'display.viewmodel_visible')
assert spinner.get('strings') == 'ON|OFF' and spinner.get('values') == 'true|false'
assert [e.get('event') for e in spinner.findall('on')] == ['created']
assert any(w.get('name').endswith('/settings_next_page') for w in menu.findall('widget'))
print('PASS: paginated Video Setup Viewmodels ON/OFF uses the persistent display preference and normal pending-edit lifecycle')

helper = (ROOT/'port/linux/game/viewmodel_visibility.c').read_text()
fixture.PRELUDE += re.sub(r'^#include.*\n', '', helper, flags=re.M)
fixture.TESTS = fixture.TESTS.replace('if(argc>1){', 'if(argc>1){CHECK(!viewmodel_is_visible());', 1)
fixture.TESTS = fixture.TESTS.replace(' char oversized[500];', r'''
 CHECK(viewmodel_is_visible());CHECK(execute("viewmodel_vis 0"));CHECK(!viewmodel_is_visible());
 for(int local=0;local<4;local++)for(int owner=0;owner<4;owner++)for(int attached=0;attached<2;attached++){
  int first_person=local==owner&&attached;
  CHECK(viewmodel_draws_geometry(first_person)==!first_person);
 }
 CHECK(!execute("viewmodel_vis 2"));CHECK(!execute("viewmodel_vis -1"));CHECK(!execute("viewmodel_vis yes"));CHECK(!viewmodel_is_visible());
 CHECK(execute("viewmodel_vis 1"));CHECK(viewmodel_is_visible());CHECK(viewmodel_draws_geometry(1));
 CHECK(execute("viewmodel_vis off"));CHECK(!viewmodel_is_visible());
 CHECK(execute("viewmodel_vis default"));CHECK(viewmodel_is_visible());
 CHECK(execute("viewmodel_vis 0"));CHECK(!viewmodel_is_visible());
 puts("PASS: production visibility policy, four local/owner combinations, live/default/invalid values and independent world geometry");
 char oversized[500];''')

weapons = (ROOT/'source/interface/first_person_weapons.c').read_text()
start = weapons.index('void first_person_weapon_draw(')
assert 'if (!viewmodel_is_visible())' in weapons[start:start+130]
assert weapons.count('viewmodel_is_visible()') == 1  # draw only; lifecycle/markers untouched
particles = (ROOT/'source/render/render_particles.c').read_text()
assert 'viewmodel_draws_geometry(owned_by_local_player &&' in particles
flares = (ROOT/'source/rasterizer/rasterizer_lights.c').read_text()
assert 'viewmodel_draws_geometry(parameters->compressed_window_index & _lens_flare_first_person_weapon_flag)' in flares

if __name__ == '__main__':
    fixture.main()
