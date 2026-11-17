"""A sound's wall test, which ends at its first hit, is blocked exactly when collision_test_vector is."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run, structure

ENUMS = [('source/physics/collisions.h', '_collision_test_structure_bit'),
         ('source/physics/collisions.h', '_collision_result_object'),
         ('source/game/game_globals.h', '_material_water')]


def generated(fault=None):
    enums = '\n'.join(enum_with(read(header), member) for header, member in ENUMS)
    collisions = read('source/physics/collisions.c')
    structures = '\n'.join([
        structure(read('source/physics/collisions.h'), 'collision_result'),
        structure(read('source/physics/collision_bsp.h'), 'collision_bsp_test_vector_result'),
        structure(read('source/structures/structure_bsp_definitions.h'), 'structure_fog_plane'),
        'struct fog_definition { real plane_distance; };',
        structure(collisions, 'collision_usage_times'),
    ])
    # (the size checks after each are the game's, not this world's)
    point_macro = re.search(r'^#define COLLISION_POINT_FROM_LINE3D.*?while \(0\)$', collisions, re.S | re.M).group(0)
    code = '\n\n'.join([
        point_macro,
        function(collisions, 'collision_port_test_vector_blocked'),
        function(collisions, 'collision_test_vector'),
        function(collisions, 'collision_port_test_vector'),
    ])
    prototype = re.search(r'^static boolean collision_port_test_vector\(.*?\);$', collisions, re.S | re.M).group(0)
    if fault:
        code = mutated(code, *fault)
    return (('enums.inc', enums + '\n'), ('structures.inc', structures + '\n'),
            ('under_test.inc', prototype + '\n\n' + code + '\n'))


def test_equivalent():
    status, output = run(build('vector_blocked', generated()), 'equivalent')
    assert status == 0, output


@pytest.mark.parametrize('fault', [
    # water never tested by the first-hit test
    ('\t\t\tcollision->location.cluster_index != NONE &&\n\t\t\t!(hit && any_hit))',
     '\t\t\tcollision->location.cluster_index != NONE &&\n\t\t\t!any_hit)'),
    # objects never tested by the first-hit test
    ('object_index != NONE && !(hit && any_hit);', 'object_index != NONE && !any_hit;'),
])
def test_negative_control(fault):
    status, output = run(build('vector_blocked', generated(fault)), 'equivalent')
    assert status == CHECK_FAILED, output
