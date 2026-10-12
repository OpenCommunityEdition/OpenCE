"""Production pickup counts, safe placement and checkpoint flags over a fake campaign."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, ROOT, build, mutated, read, run

CASES = ['counts', 'eligibility', 'health-kits', 'floor-and-walls', 'occupied-and-full', 'checkpoint-and-bsp', 'copies']


def generated(fault=None):
    source = re.sub(r'^#include.*\n', '', read('port/linux/game/coop_scaling.h') + '\n' +
                    read('port/linux/game/coop_pickups.c'), flags=re.M)
    if fault:
        source = mutated(source, *fault)
    return (('under_test.inc', source),)


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('coop_pickups', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('fault,case', [
    (('(COOP_SCALING_MAXIMUM_GROWTH - 1) * 100', '100000'), 'counts'),
    (('source->item.flags &= ~PICKUPS_COUNT_MASK;', '(void)source;'), 'checkpoint-and-bsp'),
    (('collision_test_sphere(center, radius + 0.02f, source_index)', 'FALSE'), 'floor-and-walls'),
    (('!TEST_FLAG(item->object.flags, _object_outside_of_map_bit)', 'TRUE'), 'checkpoint-and-bsp'),
    ((' || type == _equipment_powerup_health', ''), 'health-kits'),
])
def test_negative_control(fault, case):
    status, output = run(build('coop_pickups', generated(fault)), case)
    assert status == CHECK_FAILED, output


def test_menu():
    sys.path.insert(0, str(ROOT / 'tools'))
    import port_settings
    rows = port_settings.COOP_SETUP_SCREENS[0][3]
    names = [r[0] for r in rows]
    assert names[names.index('coop_extra_enemies') + 1] == 'coop_extra_pickups'
    help_text = lambda text: text.replace('\\n', ' ')
    assert help_text(rows[names.index('coop_extra_pickups')][3][0]) == (
        "Pickups grow with the players: by the value of 'Per Player' for each player past the first.")
    assert names[names.index('coop_extra_pickups') + 1] == 'coop_enemies_multiplier'
    assert 'coop_player_step' not in names
    for name in ['coop_extra_enemies', 'coop_extra_pickups']:
        assert rows[names.index(name)][2] == ['PER PLAYER', 'STATIC MULTIPLIER', 'NONE']
    assert '"network.coop_pickups", _config_string' in read('port/linux/src/port_config.c')
    assert 'coop_pickups_register(result, scenario_object, palette);' in read('source/objects/objects.c')
    assert 'coop_pickups_update();' in read('port/linux/game/network_coop.c')
