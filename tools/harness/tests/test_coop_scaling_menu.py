"""All 27 enemy/pickup/vehicle modes, row positions and help indices."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run


def generated(fault=None):
    source = read('port/linux/game/menu_functions.c')
    # (OPENCE OPTIONS' rows name the modes: game_engine.h)
    code = enum_with(read('source/game/game_engine.h'), '_game_mode_none = 0') + '\n' + \
        function(source, 'gametype_option_help')
    if fault:
        code = mutated(code, *fault)
    return (('under_test.inc', code),)


@pytest.mark.parametrize('case', ['layout', 'help', 'unrelated'])
def test_case(case):
    status, output = run(build('coop_scaling_menu', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('fault,case', [
    (('row->vertical_offset = y;', 'row->vertical_offset = y; row->child->vertical_offset = y;'), 'layout'),
    (('if (row->visible && !strncmp(row->name, "op_coop_", 8))', 'if (!strncmp(row->name, "op_coop_", 8))'), 'layout'),
    (('index += spinner->parameters.list.number_of_items;',
      'if (row->visible) index += spinner->parameters.list.number_of_items;'), 'help'),
    (('struct widget_instance *pickups = named(list, "coop_extra_pickups_spinner", 0);',
      'struct widget_instance *pickups = NULL;'), 'layout'),
    (('struct widget_instance *vehicles = named(list, "coop_extra_vehicles_spinner", 0);',
      'struct widget_instance *vehicles = NULL;'), 'layout'),
])
def test_negative_control(fault, case):
    status, output = run(build('coop_scaling_menu', generated(fault)), case)
    assert status == CHECK_FAILED, output
