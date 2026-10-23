"""A client's inventories from the host whose unit or weapons have not come yet: kept, and applied once they have."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, enum_with, function, mutated, read, run, structure

CASES = ['unit-late', 'weapon-late', 'newest', 'full', 'forget', 'expiry']
CONTROLS = [
    ('else\n\t\tdistributed_client_retry_inventory(inventory);', 'else\n\t\t(void)0;', 'unit-late'),
    ('distributed_client_apply_inventory(&inventory);', '(void)inventory;', 'unit-late'),
    ('if (!distributed_client_weapon_here(inventory->weapon_indices[weapon_slot]))', 'if (FALSE)', 'weapon-late'),
    ('if (objects_client_retried_inventories[index].inventory.unit_index == inventory->unit_index)', 'if (FALSE)',
     'newest'),
    ('.time < objects_client_retried_inventories[oldest].time', '.time > objects_client_retried_inventories[oldest].time',
     'full'),
    ('objects_client_retried_inventory_count < MAXIMUM_CLIENT_RETRIED_INVENTORIES',
     'objects_client_retried_inventory_count < MAXIMUM_CLIENT_RETRIED_INVENTORIES - 1', 'full'),
    ('objects_client_retried_inventories[index] =\n\t\t\t\tobjects_client_retried_inventories[--objects_client_retried_inventory_count];',
     '--objects_client_retried_inventory_count;', 'forget'),
    ('distributed_client_forget_retried_inventory(inventory->unit_index);\n\telse', '(void)0;\n\telse', 'forget'),
    ('- retried->time >= EMPTY_INVENTORY_TICKS', '- retried->time > EMPTY_INVENTORY_TICKS', 'expiry'),
    ('\tretried->inventory = *inventory;\n\tretried->time = game_time_get();',
     '\tif (retried->inventory.unit_index != inventory->unit_index)\n\t\tretried->time = game_time_get();\n'
     '\tretried->inventory = *inventory;', 'expiry'),
]


def declaration(source, first, last):
    """The text from one marker to the end of another, as a static's declaration with its structure."""
    start = source.index(first)
    return source[start:source.index(last, start) + len(last)]


def generated(fault=None):
    network = read('port/linux/game/network_objects.c')
    units = read('source/units/units.h')
    limits = read('port/linux/game/network_distributed.h') + read('port/linux/include/halo_port_limits.h')
    config = '\n'.join([
        '#define TICKS_PER_SECOND %d' % constant(read('source/cseries/cseries.h'), 'TICKS_PER_SECOND'),
        '#define MAXIMUM_OBJECTS_PER_MAP %d' % constant(read('source/objects/objects.h'), 'MAXIMUM_OBJECTS_PER_MAP'),
        '#define MAXIMUM_TRACKED_PLAYERS %d' % constant(limits, 'MAXIMUM_TRACKED_PLAYERS'),
        re.search(r'^#define AGE_SCALE .*$', network, re.M).group(0),
        enum_with(read('source/objects/object_types.h'), '_object_mask_unit'),
        enum_with(read('source/objects/objects.h'), '_object_dead_bit'),
        enum_with(read('source/game/players.h'), 'MAXIMUM_LOCAL_PLAYERS'),
        enum_with(units, 'MAXIMUM_WEAPONS_PER_UNIT ='),
        enum_with(units, 'NUMBER_OF_UNIT_GRENADE_TYPES\n'),
        enum_with(units, '_unit_grenade_throw_idle ='),
        enum_with(network, 'EMPTY_INVENTORY_TICKS ='),
        structure(network, 'distributed_inventory'),
    ]) + '\n'
    code = '\n'.join([
        declaration(network, 'static struct distributed_own_inventory\n',
                    'objects_client_own_inventories[MAXIMUM_LOCAL_PLAYERS];'),
        declaration(network, 'static struct distributed_retried_inventory\n',
                    'static short objects_client_retried_inventory_count;'),
        function(network, 'distributed_client_weapon_here'),
        function(network, 'distributed_client_inventory_whole'),
        function(network, 'distributed_client_retry_inventory'),
        function(network, 'distributed_client_forget_retried_inventory'),
        function(network, 'distributed_client_apply_inventory'),
        function(network, 'distributed_client_apply_retried_inventories'),
    ])
    if fault:
        code = mutated(code, *fault)
    return (('config.inc', config), ('under_test.inc', code))


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('retried_inventories', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('before,after,case', CONTROLS)
def test_negative_control(before, after, case):
    status, output = run(build('retried_inventories', generated((before, after))), case)
    assert status == CHECK_FAILED, output


def test_wired():
    # Each client tick applies the kept ones; a new game keeps none.
    network = read('port/linux/game/network_objects.c')
    assert 'distributed_client_apply_retried_inventories();' in function(network, 'network_objects_client_tick')
    assert 'objects_client_retried_inventory_count = 0;' in function(network, 'network_objects_new_game')
