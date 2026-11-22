"""A weapon readied from the inventory, where it lay hidden where it was put away, or picked up from the ground, is
drawn in the hand as the hand is drawn between the ticks, not slid there from where it was (a quick swap while
moving sent it across the player with Smooth Motion); an object hidden, moved and shown is drawn where it is shown;
a weapon held throughout blends as before."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run, structure
from harness.riding import engine

CASES = ['held', 'readied', 'picked up', 'shown', 'stays hidden']
CONTROLS = [
    # (a snapshot taken hidden blended from: a shown object slid from where it was hidden)
    ('!(record->hidden && !hidden) &&', 'TRUE &&', 'shown'),
    # (any snapshot taken hidden not blended from: a projectile drawn by its effects stepped a tick at a time)
    ('!(record->hidden && !hidden) &&', '!record->hidden &&', 'stays hidden'),
    # (an item's change of parent blended across: a weapon slid from the ground into the hand)
    ('record->parent_object_index == object->object.parent_object_index', 'TRUE', 'picked up'),
    # (a readied weapon drawn as its tick has it, a tick's move ahead of the blended hand)
    ('return record->follows_parent ? render_interpolation_followed_nodes(record, object_index) : NULL;',
     'return NULL;', 'readied'),
    # (drawn on the hand as the tick has it, not as it is drawn)
    ('matrix4x3_multiply(&parent_drawn[record->parent_node_index], &inverse, &carry);',
     'matrix4x3_multiply(&parent_simulated[record->parent_node_index], &inverse, &carry);', 'readied'),
]
RENDER = 'port/linux/game/render_interpolation.c'


def generated(fault=None):
    render = read(RENDER)
    config = '\n'.join(['typedef unsigned char byte;', 'typedef unsigned short word;',
                        'typedef struct { real i, j; } real_vector2d;', engine()]) + '\n'
    code = '\n'.join(
        [re.search(r'^#define ' + name + r' .*$', render, re.M).group() for name in
         ['MAXIMUM_INTERPOLATED_OBJECTS', 'MAXIMUM_INTERPOLATED_NODES', 'OBJECT_SNAP_DISTANCE', 'NODE_SNAP_DISTANCE',
          'CORRECTION_DECAY', 'CORRECTION_NEGLIGIBLE', 'CORRECTION_MAXIMUM']] +
        [structure(render, name) for name in
         ['interpolation_quaternion', 'interpolation_rotation', 'interpolated_object', 'interpolated_camera']] +
        [re.search(r'^static ' + name + r';$', render, re.M).group() for name in
         [r'struct interpolated_object \*interpolated_objects',
          r'struct interpolated_camera interpolated_cameras\[MAXIMUM_LOCAL_PLAYERS\]',
          'long interpolation_tick', 'long interpolation_frame', 'boolean interpolation_rendering']] +
        ['static real interpolation_fraction = 1.0f;'] +
        [function(render, name) for name in
         ['lerp', 'point_lerp', 'vector_length', 'basis_is_rotation', 'quaternion_from_basis', 'basis_from_quaternion',
          'rotation_from_matrix', 'matrix_blend_rotations', 'correction_significant', 'correction_advance',
          'correction_drawn', 'distance_squared', 'render_interpolation_tick',
          'render_interpolation_followed_nodes', 'render_interpolation_object_node_matrices']])
    if fault:
        code = mutated(code, *fault)
    return (('config.inc', config), ('under_test.inc', code))


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('weapon_swap_interpolation', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('before,after,case', CONTROLS)
def test_negative_control(before, after, case):
    status, output = run(build('weapon_swap_interpolation', generated((before, after))), case)
    assert status == CHECK_FAILED, output
