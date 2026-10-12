"""Production vehicle budgets, lifecycle, host authority and checkpoint restore."""
import re
import sys
from pathlib import Path
import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import build, read, run, mutated, CHECK_FAILED

def generated(fault=None):
    source = re.sub(r'^#include.*\n', '', read('port/linux/game/coop_vehicles.c'), flags=re.M)
    if fault:
        source = mutated(source, *fault)
    return (('under_test.inc', source),)

@pytest.mark.parametrize('case', ['counts', 'authority', 'driving', 'delivery', 'chain', 'blocked', 'checkpoint', 'recreated'])
def test_case(case):
    status, output = run(build('coop_vehicles', generated()), case)
    assert status == 0, output

@pytest.mark.parametrize('fault,case', [
    (('#define VEHICLE_SCALING_DIVISOR 2', '#define VEHICLE_SCALING_DIVISOR 1'), 'counts'),
    (('game_connection() == _game_connection_network_server', 'TRUE'), 'authority'),
    (('#define VEHICLE_CLEARANCE 1.15f', '#define VEHICLE_CLEARANCE 1.0f'), 'driving'),
    (('(home->radius + vehicle->object.bounding_sphere_radius)', '(home->radius)'), 'driving'),
    (('home->copies >= budget', '((void)budget, FALSE)'), 'chain'),
    (('!home->entered ||', 'FALSE ||'), 'driving'),
    (('vehicle->object.parent_object_index != NONE', 'FALSE'), 'delivery'),
    (('coop_vehicle_homes = saved_vehicles;', '(void)saved_vehicles;'), 'checkpoint'),
    (('|| !vehicles_home_clear(home)', '|| FALSE'), 'blocked'),
    (('!collision_model_test_sphere(&instance, &center, radius)', 'FALSE'), 'blocked'),
])
def test_negative_control(fault, case):
    status, output = run(build('coop_vehicles', generated(fault)), case)
    assert status == CHECK_FAILED, output

def test_menu():
    import port_settings
    rows = port_settings.COOP_SETUP_SCREENS[0][3]
    names = [row[0] for row in rows]
    assert names[names.index('coop_extra_enemies') + 1] == 'coop_extra_vehicles'
    assert names[names.index('coop_extra_vehicles') + 1] == 'coop_enemies_multiplier'
    row = rows[names.index('coop_extra_vehicles')]
    assert row[2] == ['PER PLAYER', 'STATIC MULTIPLIER', 'NONE']
    assert row[3][0].replace('\\n', ' ') == (
        "Vehicles respawn after being driven a short distance based on the number of players: "
        "by half the value of 'Per Player' for each player past the first.")
    assert '"network.coop_vehicles", _config_string' in read('port/linux/src/port_config.c')
    assert 'coop_vehicles_register(result, scenario_object);' in read('source/objects/objects.c')
