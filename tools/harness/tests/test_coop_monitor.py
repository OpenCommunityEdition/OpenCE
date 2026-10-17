"""The real encounter placement keeps scripted Monitors out of Extra Enemies."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run


def generated(fault=None):
    source = read('source/ai/encounters.c')
    code = enum_with(read('source/ai/actor_types.h'), '_actor_monitor') + '\n'
    code += function(source, 'encounter_place_actor') + '\n' + function(source, 'encounter_create')
    if fault == 'squad':
        code = mutated(code, 'actor_type == _actor_monitor ? 0 :', 'FALSE ? 0 :')
    if fault == 'variant':
        code = mutated(code, 'actor_definition_get(placed_variant->actor_reference.index)->type == _actor_monitor', 'FALSE')
    return (('under_test.inc', code),)


@pytest.mark.parametrize('case', ['monitor', 'combat', 'mixed', 'override', 'upgrade', 'original', 'inactive'])
def test_case(case):
    status, output = run(build('coop_monitor', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('fault,case', [('squad', 'monitor'), ('variant', 'override'), ('variant', 'upgrade')])
def test_negative_control(fault, case):
    status, output = run(build('coop_monitor', generated(fault)), case)
    assert status == CHECK_FAILED, output
