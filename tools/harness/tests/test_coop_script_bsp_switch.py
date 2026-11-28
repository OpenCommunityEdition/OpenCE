"""Co-op: a script's switch_bsp back waits while players stand in the volume another switched it for."""
import sys
from pathlib import Path
import pytest
sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import build, read, run, function, mutated, CHECK_FAILED


def generated(fault=None):
    hs = read('source/hs/hs.c')
    players = read('source/game/players.c')
    hold = function(players, 'players_coop_script_bsp_switch_allowed') + '\n' + \
        function(players, 'players_coop_note_script_bsp_switch')
    switch = function(hs, 'hs_volume_found') + '\n' + function(hs, 'scenario_switch_structure_bsp_evaluate')
    if fault == 'unheld':
        hold = mutated(hold, 'return FALSE;', 'return TRUE;')
    if fault == 'any-script':
        switch = mutated(switch, 'hs_runtime_port_executing_script_type() == _hs_script_continuous', 'TRUE')
    if fault == 'any-tick':
        switch = mutated(switch, ' && hs_volume_found_time == game_time_get()', '')
    return (('hold.inc', hold), ('switch.inc', switch))


@pytest.mark.parametrize('case', ['straddle', 'crossing', 'once', 'unheld', 'earlier'])
def test_script_bsp_switch(case):
    status, output = run(build('coop_script_bsp_switch', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('fault,case', [('unheld', 'straddle'), ('any-script', 'once'), ('any-tick', 'earlier')])
def test_negative(fault, case):
    status, output = run(build('coop_script_bsp_switch', generated(fault)), case)
    assert status == CHECK_FAILED, output
