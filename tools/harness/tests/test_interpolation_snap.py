"""An object drawn between ticks: whether its two snapshots snap or blend, found once a tick, agrees with the
snapshots as they are in every frame, after a tick and after a correction moves them."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run, structure

CASES = ['tick', 'correction']
CONTROLS = [
    # a tick's new snapshots keep the last tick's snap
    ('record->blended_frame = NONE;\n\t\trecord->snap_valid = FALSE;\n', 'record->blended_frame = NONE;\n', 'tick'),
    # a correction's moved snapshots keep the snap found before it
    ('/* (its snapshots moved: found again, as before) */\n\t\trecord->snap_valid = FALSE;\n', '', 'correction'),
]


def define(source, name):
    return re.search(r'^#define ' + name + r' .*', source, re.M).group()


def generated(fault=None):
    source = read('port/linux/game/render_interpolation.c')
    maths = read('source/math/real_math.h')
    matrices = read('source/math/matrix_math.c')
    config = '\n'.join(
        [define(source, name) for name in
         ['MAXIMUM_INTERPOLATED_OBJECTS', 'MAXIMUM_INTERPOLATED_NODES', 'OBJECT_SNAP_DISTANCE', 'NODE_SNAP_DISTANCE',
          'FIRST_PERSON_HALF_TURN_COSINE', 'CORRECTION_DECAY', 'CORRECTION_NEGLIGIBLE', 'CORRECTION_MAXIMUM']] +
        [define(maths, 'REAL_MAX'),
         structure(maths, 'real_matrix4x3'),
         'typedef struct real_matrix4x3 real_matrix4x3;'] +
        [structure(source, name) for name in
         ['interpolation_quaternion', 'interpolation_rotation', 'interpolated_object', 'interpolated_camera']]) + '\n'
    code = '\n'.join(
        [function(matrices, 'matrix4x3_inverse_transform_point')] +
        [function(source, name) for name in
         ['lerp', 'point_lerp', 'vector_length', 'vector_nlerp', 'basis_is_rotation', 'quaternion_from_basis',
          'basis_from_quaternion', 'rotation_from_matrix', 'matrix_blend_rotations', 'correction_significant',
          'correction_advance', 'correction_drawn', 'correction_add', 'distance_squared',
          'render_interpolation_tick', 'render_interpolation_object_node_matrices',
          'render_interpolation_correct_object']])
    if fault:
        code = mutated(code, *fault)
    return (('config.inc', config), ('under_test.inc', code))


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('interpolation_snap', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('before,after,case', CONTROLS)
def test_negative_control(before, after, case):
    status, output = run(build('interpolation_snap', generated((before, after))), case)
    assert status == CHECK_FAILED, output
