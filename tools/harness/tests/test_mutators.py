"""The mutators' game side (mutators.c, network_mutators.c, mutator_big_head.c) and their hooks, without assets."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, mutated, read, run


def stripped(path):
    """a source without its #includes (the test's fakes stand in)"""
    return re.sub(r'^#include [<"][^>"\n]+[>"]\n', '', read(path), flags=re.M)


def message_ids():
    return re.search(r'enum\s*\{.*?_distributed_message_player_prediction.*?\};',
                     read('port/linux/game/network_distributed.h'), re.S).group(0)


def generated(fault=None):
    game = stripped('port/linux/game/mutators.c')
    network = stripped('port/linux/game/network_mutators.c')
    big_head = stripped('port/linux/game/mutator_big_head.c')
    if fault == 'range':
        game = mutated(game, 'return value >= mutator_parameter_definitions[parameter].minimum &&', 'return TRUE ||')
    if fault == 'authority':
        network = mutated(network, 'if (machine_index != NONE ||', 'if (FALSE ||')
    if fault == 'client':
        game = mutated(game, 'if (game_connection() == _game_connection_network_client ||', 'if (')
    if fault == 'sphere':
        big_head = mutated(big_head, 'return MAX(bounding_radius, big_head_radius(object_index, 1.f));',
                           'return bounding_radius;')
    return (('mutators_header.inc', read('port/linux/game/mutators.h')), ('mutators.inc', game),
            ('network_mutators.inc', network), ('big_head.inc', big_head), ('message_ids.inc', message_ids()))


@pytest.mark.parametrize('case', ['keys', 'local', 'host', 'wire', 'big_head'])
def test_case(case):
    status, output = run(build('mutators', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('fault,case', [('range', 'keys'), ('authority', 'wire'), ('client', 'local'),
                                        ('sphere', 'big_head')])
def test_negative(fault, case):
    status, output = run(build('mutators', generated(fault)), case)
    assert status == CHECK_FAILED, output


def test_hooks():
    """the engine's hooks, each marked, and the collision test that scales with the node's matrix"""
    bipeds = read('source/units/bipeds.c')
    hook = bipeds[bipeds.index('void biped_preprocess_node_orientations('):]
    hook = hook[:hook.index('\n}\n')]
    assert 'mutator_big_head_node_orientations(biped_index, node_orientations);' in hook
    assert 'port: the big head mutator' in hook
    objects = read('source/objects/objects.c')
    grown = objects.index('mutator_big_head_bounding_radius(object_index,')
    assert grown < objects.index('object_bounds_cache_update(object_index, &object->object.bounding_sphere_center')
    assert 'port: grown to hold a head' in objects[grown - 120:grown]
    header = read('source/objects/objects.h')
    render = header[header.index('void object_get_render_bounding_sphere('):]
    assert 'mutator_big_head_render_radius(object_index, *radius);' in render[:render.index('\n}\n')]
    # (what makes the head's hits follow its scale: collision_models.c tests a
    # node's geometry through its matrix, the radius scaled by its inverse)
    collision = read('source/physics/collision_models.c')
    assert collision.count('scaled_radius = inverted_matrix.scale * radius;') == 2
    distributed = read('port/linux/game/network_distributed.c')
    ready = distributed[distributed.index('case _distributed_message_client_ready:'):]
    assert ready.index('network_mutators_send(machine_index);') < ready.index('network_objects_client_asked(')


def test_message_id():
    import network_format
    messages = {name: int(value) for name, value in network_format.items().items()
                if name.startswith('enum _distributed_message_')}
    assert len(set(messages.values())) == len(messages), messages
    assert messages['enum _distributed_message_mutators'] == 112


def test_menu():
    """the Mods screen's MUTATORS, as the generator writes it, runs a function the menus have (else the game
    refuses the PC menus and plays the Xbox's)"""
    import xml.etree.ElementTree as ET
    import port_settings
    name = port_settings.MODS.replace('/', '.') + '.xml'
    lines = port_settings.multiplayer_files()[name]
    assert '\n'.join(lines) == read('port/assets/menus/ce/' + name), 'the screen is not the generator\'s'
    root = ET.fromstring('\n'.join(lines))
    runs = {handler.attrib['run'] for handler in root.iter('on') if 'run' in handler.attrib}
    assert 'port mods switch' in runs
    tags = read('port/linux/game/menu_tags.c')
    table = tags[tags.index('"port mods init"'):]
    table = table[:table.index('};')]
    for run_name in (name for name in runs if name.startswith('port mods')):
        assert f'"{run_name}"' in table, run_name
    assert '"port mods switch"' in read('port/linux/game/menu_functions.c')
