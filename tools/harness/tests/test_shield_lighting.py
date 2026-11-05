"""Real shield lighting in a synthetic world: colour, ownership and timing."""
from pathlib import Path
import re
import shutil
import sys
import pytest
import subprocess
import xml.etree.ElementTree as ET
sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import enum_with


ROOT = Path(__file__).resolve().parents[3]

def test_render_owned_shield_lighting(tmp_path):
    source = (ROOT / 'port/linux/game/shield_glow.c').read_text()
    source = re.sub(r'^#include.*$', '', source, flags=re.M)
    policy = enum_with((ROOT / 'port/linux/game/network_shield_effects.h').read_text(), '_shield_effects_stock')
    (tmp_path / 'shield_under_test.inc').write_text(policy + '\n' + source)
    fixture = tmp_path / 'shield.c'
    fixture.write_text((ROOT / 'tools/fixtures/shield_glow_test.c').read_text())
    compiler = shutil.which('clang')
    if not compiler:
        pytest.skip('clang required')
    executable = tmp_path / ('shield.exe' if sys.platform == 'win32' else 'shield')
    flags = ['--target=i686-pc-windows-msvc', '-fuse-ld=lld'] if sys.platform == 'win32' else ['-lm']
    subprocess.run([compiler, '-std=gnu89', '-Wall', '-Wextra', '-Werror', str(fixture), *flags, '-o', str(executable)], check=True)
    result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, result.stdout + result.stderr


def test_menu_ownership_and_generated_layout():
    import port_settings
    spec = port_settings.SCREENS['video_settings/dynamic_lighting']
    rows = spec['rows']
    assert len(rows) == 10
    assert [row[1] for row in rows[2:7]] == [
        'lighting.weapon_glow', 'display.plasma_prong_lighting', 'lighting.shield_emitters',
        'display.vehicle_lighting', 'display.world_lighting']
    assert rows[-2][1] == 'display.shield_glow'
    assert rows[-1][1] == 'display.shield_glow_intensity'
    extra = {row[1] for row in port_settings.SCREENS['video_settings/extra_effects']['rows']}
    assert 'display.overshield_effect' in extra
    assert not {'display.shield_glow', 'display.shield_glow_intensity'} & extra
    assert 73 + (len(rows)-1)*spec['spacing'] + 28 < spec['help_top']
    assert spec['help_top'] + spec['help_height'] <= 414
    for name, lines in port_settings.settings_files().items():
        assert (ROOT/'port/assets/menus/ce'/name).read_text() == '\n'.join(lines)
        if name.endswith(('dynamic_lighting.xml', 'controller_remapping.xml')):
            widgets = ET.fromstring('\n'.join(lines)).iter('widget')
            help_widget = next(w for w in widgets if w.get('name', '').endswith('/help'))
            if name.endswith('dynamic_lighting.xml'):
                assert help_widget.get('height') == str(spec['help_height'])
                assert help_widget.get('string_list', '').endswith('/help_strings')
            else:
                assert help_widget.get('height') == str(port_settings.SCREENS['controller_remapping']['help_height'])
                assert 'string_list' not in help_widget.attrib
