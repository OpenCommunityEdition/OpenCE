"""A body lying limp on a client, which the host moves: its pose goes with it, and it lies limp again where it comes
to rest."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run

CASES = ['pushed', 'landed', 'moved', 'blended', 'living']
CONTROLS = [
    ('if (!at_rest)\n\t\tdistributed_body_unfix_pose(object_index);', 'if (!at_rest)\n\t\t;', 'pushed'),
    ('SET_FLAG(biped->biped.flags, _biped_airborne_bit, FALSE);', ';', 'landed'),
    ('position->z;\n\tdistributed_body_unfix_pose(object_index);', 'position->z;', 'moved'),
    ('blended.z;\n\t\tdistributed_body_unfix_pose(object_index);', 'blended.z;', 'blended'),
    ('return biped && TEST_FLAG(biped->object.damage_flags, _object_dead_bit) ? biped : NULL;', 'return biped;', 'living'),
]


def real_define(source, name):
    return '#define %s %sf\n' % (name, re.search(r'#define\s+' + name + r'\s+([\d.]+)f', source).group(1))


def generated(fault=None):
    network = read('port/linux/game/network_objects.c')
    bipeds = read('source/units/bipeds.c')
    config = '\n'.join([
        enum_with(read('source/objects/object_types.h'), '_object_type_biped'),
        enum_with(read('source/objects/objects.h'), '_object_at_rest_bit'),
        enum_with(read('source/objects/objects.h'), '_object_dead_bit'),
        enum_with(read('source/units/bipeds.h'), '_biped_airborne_bit'),
        enum_with(read('source/units/biped_definitions.h'), '_biped_uses_limp_body_physics_bit'),
    ]) + '\n'
    config += real_define(network, 'REMOTE_OBJECT_TOLERANCE')
    config += real_define(read('port/linux/game/network_distributed.h'), 'REMOTE_BLEND_DISTANCE')
    code = '\n'.join([
        function(bipeds, 'biped_start_limp_body_physics'),
        function(bipeds, 'biped_stop_limp_body_physics'),
        function(network, 'distributed_body_get'),
        function(network, 'distributed_body_unfix_pose'),
        function(network, 'distributed_body_set_at_rest'),
        function(network, 'distributed_object_move'),
        function(network, 'network_objects_reconcile'),
    ])
    if fault:
        code = mutated(code, *fault)
    return (('config.inc', config), ('under_test.inc', code))


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('limp_body_moves', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('before,after,case', CONTROLS)
def test_negative_control(before, after, case):
    status, output = run(build('limp_body_moves', generated((before, after))), case)
    assert status == CHECK_FAILED, output


def test_host_word_on_rest():
    # Each object state takes the host's at rest for a body too, after its own flag, before its correction.
    states = function(read('port/linux/game/network_objects.c'), 'network_objects_handle_states')
    flag = states.index('SET_FLAG(object->object.flags, _object_at_rest_bit, at_rest);')
    body = states.index('distributed_body_set_at_rest(state->object_index, at_rest);')
    assert flag < body < states.index('network_objects_reconcile(state->object_index', body)
