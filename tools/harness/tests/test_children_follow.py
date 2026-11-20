"""An object the netcode moves has what it carries (a held weapon, a seated unit) put back on it at once."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run

CONTROL = ('object_compute_node_matrices_recursive(child_index);', ';')


def generated(fault=None):
    network = read('port/linux/game/network_objects.c')
    config = enum_with(read('source/objects/objects.h'), '_object_at_rest_bit') + '\n'
    wrapper = function(network, 'network_objects_set_position')
    if fault:
        wrapper = mutated(wrapper, *fault)
    code = '\n'.join([
        function(read('source/objects/objects.c'), 'object_compute_node_matrices_recursive'),
        wrapper,
        function(network, 'distributed_object_move'),
        function(network, 'network_objects_reconcile'),
    ])
    return (('config.inc', config), ('under_test.inc', code))


def test_case():
    status, output = run(build('children_follow', generated()), 'moved')
    assert status == 0, output


def test_negative_control():
    status, output = run(build('children_follow', generated(CONTROL)), 'moved')
    assert status == CHECK_FAILED, output


def test_netcode_moves_through_wrapper():
    # (a bare object_set_position at a netcode move leaves the children where the object was)
    network = read('port/linux/game/network_objects.c')
    assert network.count('object_set_position(') == 1
    for file in ('network_coop', 'vehicle_riding_bipeds', 'network_distributed', 'network_actors'):
        assert read('port/linux/game/%s.c' % file).count('object_set_position(') == 0, file
    wrapper = function(network, 'network_objects_set_position')
    assert wrapper.index('object_set_position(') < wrapper.index('object_compute_node_matrices_recursive(')
