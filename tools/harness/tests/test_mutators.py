"""The mutators' game side (mutators.c, network_mutators.c, object_pose.c) and their hooks, without assets."""
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
    pose = stripped('port/linux/game/object_pose.c')
    if fault == 'number':
        game = mutated(game, '!(values[index] == values[index] &&', '!(TRUE ||')
    if fault == 'authority':
        network = mutated(network, 'if (machine_index != NONE ||', 'if (FALSE ||')
    if fault == 'client':
        game = mutated(game, 'if (game_connection() == _game_connection_network_client ||', 'if (')
    if fault == 'order':
        network = mutated(network, '(message.offset && (message.offset != network_mutator_tag_edits.received ||',
                          '(FALSE && (message.offset != network_mutator_tag_edits.received ||')
    if fault == 'sphere':
        pose = mutated(pose, 'posed = object_pose_radius(object_index, 1.f);', 'posed = 0.f;')
    if fault == 'children':
        pose = mutated(pose, 'if (VALID_INDEX(node->first_child_node_index, node_count) && depth < node_count)',
                       'if (FALSE)')
    if fault == 'siblings':
        pose = mutated(pose, 'if (index != node_index && VALID_INDEX(node->next_sibling_node_index, node_count)',
                       'if (VALID_INDEX(node->next_sibling_node_index, node_count)')
    if fault == 'twice':
        pose = mutated(pose, 'pose->nodes[index].scale *= scale;', 'pose->nodes[index].scale = scale;')
    header = '\n'.join(line for line in read('port/linux/game/object_pose.h').splitlines()
                       if line.startswith('#define OBJECT_POSE_'))
    return (('mutators_header.inc', read('port/linux/game/mutators.h') + '\n' + header), ('mutators.inc', game),
            ('network_mutators.inc', network), ('object_pose.inc', pose), ('message_ids.inc', message_ids()))


@pytest.mark.parametrize('case', ['keys', 'local', 'host', 'wire', 'scripts', 'tag_edits', 'pose'])
def test_case(case):
    status, output = run(build('mutators', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('fault,case', [('number', 'host'), ('authority', 'wire'), ('client', 'local'),
                                        ('order', 'tag_edits'), ('authority', 'tag_edits'), ('sphere', 'pose'),
                                        ('children', 'pose'), ('siblings', 'pose'), ('twice', 'pose')])
def test_negative(fault, case):
    status, output = run(build('mutators', generated(fault)), case)
    assert status == CHECK_FAILED, output


def test_hooks():
    """the engine's hooks, each marked and none a mutator's own, and the collision test that scales with the node's
    matrix"""
    for path in ('source/units/bipeds.c', 'source/objects/objects.c', 'source/objects/objects.h',
                 'source/objects/object_types.c'):
        assert 'big_head' not in read(path) and 'big head' not in read(path), path
    types = read('source/objects/object_types.c')
    hook = types[types.index('void object_type_preprocess_node_orientations('):]
    hook = hook[:hook.index('\n}\n')]
    # (after every part's own, the biped's among them)
    assert hook.index('object_pose_node_orientations(object_index, node_orientations);') > hook.index(
        'datum_preprocess_node_orientations(object_index, node_orientations);')
    assert 'port: and posed' in hook
    objects = read('source/objects/objects.c')
    grown = objects.index('object_pose_bounding_radius(object_index,')
    assert grown < objects.index('object_bounds_cache_update(object_index, &object->object.bounding_sphere_center')
    assert "port: grown to hold the nodes a mutator's poses moved" in objects[grown - 120:grown]
    header = read('source/objects/objects.h')
    render = header[header.index('void object_get_render_bounding_sphere('):]
    assert 'object_pose_render_radius(object_index, *radius);' in render[:render.index('\n}\n')]
    # (what makes the head's hits follow its scale: collision_models.c tests a
    # node's geometry through its matrix, the radius scaled by its inverse)
    collision = read('source/physics/collision_models.c')
    assert collision.count('scaled_radius = inverted_matrix.scale * radius;') == 2
    # (a unit's death as a mutator's [death] makes it, after the game's own: object_death.c)
    units = read('source/units/units.c')
    died = units[units.index('void unit_died('):]
    died = died[:died.index('\n}\n')]
    assert died.index('object_death(unit_index);') > died.index('unit->unit.time_of_death = game_time_get();')
    assert "port: and as a mutator's [death] has it" in died
    # (an explosion's decal, after its damage: explosion_decals.c)
    effects = read('source/effects/effects.c')
    part = effects[effects.index('case DAMAGE_EFFECT_DEFINITION_TAG:'):]
    part = part[:part.index('break;')]
    assert part.index('explosion_decal(part_definition->reference.index, world_point, world_forward);') > part.index(
        'area_of_effect_cause_damage(&damage, NONE);')
    assert "port: and the decal a mutator's [explosion] leaves" in part
    assert 'do { if (explosion_decal_count > 0) explosion_decal_found(' in read('port/linux/game/explosion_decals.h')
    death = read('port/linux/game/object_death.h')
    assert 'do { if (object_death_count > 0) object_death_found(unit_index); } while (0)' in death
    distributed = read('port/linux/game/network_distributed.c')
    ready = distributed[distributed.index('case _distributed_message_client_ready:'):]
    assert ready.index('network_mutators_send(machine_index);') < ready.index('network_objects_client_asked(')


def test_message_id():
    import network_format
    messages = {name: int(value) for name, value in network_format.items().items()
                if name.startswith('enum _distributed_message_')}
    assert len(set(messages.values())) == len(messages), messages
    assert messages['enum _distributed_message_mutators'] == 112
    assert messages['enum _distributed_message_mutator_tag_edits'] == 113


def test_menu():
    """the Mods screen's MUTATORS and Server Setup's MUTATOR OPTIONS, as the generator writes them, run functions the
    menus have (else the game refuses the PC menus and plays the Xbox's)"""
    import xml.etree.ElementTree as ET
    import port_settings
    tags = read('port/linux/game/menu_tags.c')
    table = tags[tags.index('"port mods init"'):]
    table = table[:table.index('};')]
    functions = read('port/linux/game/menu_functions.c')
    for screen, wanted in ((port_settings.MODS, 'port mods switch'),
                           (f'{port_settings.MT}/server_settings', 'port mutators init')):
        name = screen.replace('/', '.') + '.xml'
        lines = port_settings.multiplayer_files()[name]
        assert '\n'.join(lines) == read('port/assets/menus/ce/' + name), f'{name} is not the generator\'s'
        root = ET.fromstring('\n'.join(lines))
        runs = {handler.attrib['run'] for handler in root.iter('on') if 'run' in handler.attrib}
        assert wanted in runs
        for run_name in (name for name in runs if name.startswith(('port mods', 'port mutators'))):
            assert f'"{run_name}"' in table, run_name
            assert f'"{run_name}"' in functions, run_name
