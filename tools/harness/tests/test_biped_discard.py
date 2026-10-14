"""A biped fallen out of the map, erased by whoever owns it: a client leaves the host's to the host."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run

CASES = ['client', 'client-own', 'host', 'above']


def generated(fault=None):
    config = enum_with(read('source/objects/objects.h'), '_object_outside_of_map_bit') + '\n'
    code = '\n'.join([
        'static long objects_client_has[MAXIMUM_TRACKED_OBJECTS];',
        'static boolean objects_client_deleting;',
        function(read('port/linux/game/network_objects.c'), 'network_objects_may_delete'),
        function(read('source/units/bipeds.c'), 'biped_check_discard'),
    ])
    if fault:
        code = mutated(code, *fault)
    return (('config.inc', config), ('under_test.inc', code))


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('biped_discard', generated()), case)
    assert status == 0, output


def test_negative_control():
    fault = (' &&\n\t\tnetwork_objects_may_delete(biped_index))', ')')
    status, output = run(build('biped_discard', generated(fault)), 'client')
    assert status == CHECK_FAILED, output
