"""A player riding a vehicle is drawn gliding with it through the netcode's writes, which the player's carry takes
too: not by those of a vehicle the player is not riding."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run, structure
from harness.riding import engine, without_includes  # noqa: E402

CASES = ['deck', 'other vehicle', 'unmoved camera', 'remote take', 'smooth off', 'remote off']
CONTROLS = [
    # (the vehicle's glide never handed to its riders)
    ('biped_riding_correct_riders(object_index, offset);', ';', 'deck'),
    ('biped->biped.riding_object_index == vehicle_index', 'biped->biped.riding_object_index != NONE', 'other vehicle'),
    ('render_interpolation_camera_unmoved(units[at], offset);', ';', 'unmoved camera'),
    ('distributed_rider_prediction_pending(player, units[at])', 'FALSE', 'remote take'),
    # (the host's rider drawn where the tick will carry it, with smooth motion off)
    ('carried->position.x += offset.i;', ';', 'smooth off'),
    # (a rider's weapon left where the tick has it)
    ('if (!render_interpolation_carry_family(child_index, offset))', 'if (FALSE)', 'remote off'),
]
RENDER = 'port/linux/game/render_interpolation.c'
BIPEDS = 'port/linux/game/vehicle_riding_bipeds.c'
RIDING = 'port/linux/game/vehicle_riding.c'


def generated(fault=None):
    render = read(RENDER)
    bipeds = read(BIPEDS)
    config = '\n'.join([
        'typedef unsigned short word;',
        'typedef struct { real i, j; } real_vector2d;',
        engine(),
        without_includes(read('port/linux/game/vehicle_riding.h')),
    ]) + '\n'
    code = '\n'.join(
        [re.search(r'^#define ' + name + r' .*$', render, re.M).group() for name in
         ['MAXIMUM_INTERPOLATED_OBJECTS', 'MAXIMUM_INTERPOLATED_NODES', 'CARRIED_OBJECTS', 'CARRIED_NODES',
          'OBJECT_SNAP_DISTANCE', 'NODE_SNAP_DISTANCE',
          'CORRECTION_DECAY', 'CORRECTION_NEGLIGIBLE', 'CORRECTION_MAXIMUM',
          'CAMERA_CUT_DISTANCE', 'CAMERA_CUT_COSINE']] +
        [structure(render, name) for name in
         ['interpolation_quaternion', 'interpolation_rotation', 'interpolated_object', 'interpolated_camera',
          'carried_rider', 'carried_object']] +
        [re.search(r'^static ' + name + r';$', render, re.M).group() for name in
         [r'struct interpolated_object \*interpolated_objects',
          r'struct interpolated_camera interpolated_cameras\[MAXIMUM_LOCAL_PLAYERS\]',
          'long interpolation_tick', 'long interpolation_frame', 'boolean interpolation_rendering',
          'boolean interpolation_drawing', r'struct observer_result carried_cameras\[MAXIMUM_LOCAL_PLAYERS\]',
          r'struct carried_rider carried_riders\[HALO_PORT_MAXIMUM_NETWORK_PLAYERS\]',
          r'struct carried_object carried_objects\[CARRIED_OBJECTS\]',
          r'real_matrix4x3 carried_nodes\[CARRIED_NODES\]', 'long carried_frame', 'short carried_object_count',
          'short carried_node_count']] +
        ['static real interpolation_fraction = 1.0f;',
         structure(bipeds, 'biped_riding_write_record'),
         re.search(r'^static struct biped_riding_write_record biped_riding_writes\[.*$', bipeds, re.M).group()] +
        [function(render, name) for name in
         ['lerp', 'point_lerp', 'vector_length', 'vector_nlerp', 'basis_is_rotation', 'quaternion_from_basis',
          'basis_from_quaternion', 'rotation_from_matrix', 'matrix_blend_rotations', 'correction_significant', 'correction_advance',
          'correction_drawn', 'correction_add', 'distance_squared', 'render_interpolation_tick',
          'render_interpolation_frame_begin', 'render_interpolation_frame_end',
          'render_interpolation_carry_family', 'render_interpolation_carried_nodes',
          'render_interpolation_object_node_matrices', 'render_interpolation_object_point',
          'render_interpolation_correct_object',
          'render_interpolation_camera_unmoved', 'observer_blend', 'observer_cut',
          'render_interpolation_blended_camera', 'render_interpolation_carried_camera',
          'render_interpolation_direct_camera', 'render_interpolation_camera']] +
        [function(read(RIDING), name) for name in
         ['vehicle_riding_correction_identity', 'riding_correction_matrix', 'vehicle_riding_correction_compose',
          'vehicle_riding_correction_since', 'vehicle_riding_yaw', 'vehicle_riding_turn_vector',
          'vehicle_riding_applies']] +
        [function(read('port/linux/game/network_distributed.c'), name) for name in
         ['distributed_rider_prediction_pending']] +
        [function(bipeds, name) for name in ['biped_riding_applies', 'biped_riding_write_candidates', 'biped_riding_correct_riders',
                                             'biped_riding_uncarried_write']])
    if fault:
        code = mutated(code, *fault)
    return (('config.inc', config), ('under_test.inc', code))


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('rider_glide', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('before,after,case', CONTROLS)
def test_negative_control(before, after, case):
    status, output = run(build('rider_glide', generated((before, after))), case)
    assert status == CHECK_FAILED, output
