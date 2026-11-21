"""The first-person weapon drawn between ticks: its own animations blended however far they move in a tick (a
rocket launcher's melee), another weapon's or skeleton's pose never blended to, a node's half turn snapped, and
the nodes of an overlay that shows a count (the rounds loaded) not blended from one of its frames to the next,
nor a node the animation puts elsewhere in a tick (a round put from the gun into the hand as a reload starts),
with what it holds."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run, structure

CASES = ['melee', 'weapon', 'graph', 'within a tick', 'gap', 'not a number', 'half turn', 'large turn', 'count',
         'count kept', 'count loose', 'jump', 'small', 'by degrees', 'first tick', 'no count']
CONTROLS = [
    # the distance limit the weapon's identity replaced
    ('<= REAL_MAX))', '<= 0.25f * 0.25f))', 'melee'),
    ('first_person->node_count == node_count &&\n\t\t\tfirst_person->weapon_index == weapon_index &&',
     'first_person->node_count == node_count &&', 'weapon'),
    ('first_person->weapon_index == weapon_index &&\n\t\t\tfirst_person->animation_graph_index == animation_graph_index;',
     'first_person->weapon_index == weapon_index;', 'graph'),
    ('if (first_person->tick != interpolation_tick ||\n\t\tfirst_person->weapon_index != weapon_index ||\n'
     '\t\tfirst_person->animation_graph_index != animation_graph_index)',
     'if (first_person->tick != interpolation_tick)', 'within a tick'),
    ('boolean continued = first_person->tick == interpolation_tick - 1 &&', 'boolean continued = ', 'gap'),
    ('< FIRST_PERSON_HALF_TURN_COSINE)', '< 0.0f)', 'half turn'),
    # the count's nodes blended through the gun as the count changes (before fix-first-person-count-snap)
    ('count_changed = node_parents &&', 'count_changed = FALSE &&', 'count'),
    # every node placed as the count changes, not the count's alone
    ('!count_changed || !BIT_VECTOR_TEST_FLAG(first_person->count_nodes, node_index)) &&',
     '!count_changed) &&', 'count'),
    # a node put elsewhere in the tick blended (slid there alone)
    ('\t\t\t!jumped;', '\t\t\tTRUE;', 'jump'),
    # what it holds blended, left to slide
    ('spreading = placing;', 'spreading = FALSE;', 'jump'),
    # any move on the parent a jump, however short
    ('move >= FIRST_PERSON_JUMP_DISTANCE * FIRST_PERSON_JUMP_DISTANCE &&\n\t\t\t', '', 'small'),
    # a long move made by degrees a jump
    (' &&\n\t\t\tmove > FIRST_PERSON_JUMP_RATIO * FIRST_PERSON_JUMP_RATIO * first_person->moves[node_index];', ';',
     'by degrees'),
    # the move before measured from the tick's own poses (none)
    ('first_person->moves[node_index] = first_person_move_on_parent(\n\t\t\t\tfirst_person->previous,',
     'first_person->moves[node_index] = first_person_move_on_parent(\n\t\t\t\tfirst_person->latest,', 'by degrees'),
    # a jump told with no move before it measured
    ('boolean jumped = first_person->has_moves &&\n\t\t\t', 'boolean jumped = ', 'first tick'),
    # the count's nodes placed every tick, its frame unchanged
    ('first_person->count_frame != first_person->previous_count_frame;', 'TRUE;', 'count kept'),
    # the count's nodes snapped to the tick's pose, ahead of the hand
    ('matrix4x3_multiply(&drawn[parent], &local, &drawn[node_index]);',
     'drawn[node_index] = first_person->latest[node_index];', 'count'),
    # their new place on the hand taken a tick early
    ('matrix4x3_inverse(&first_person->previous[parent], &inverse_parent);\n'
     '\t\t\t\tmatrix4x3_multiply(&inverse_parent, &first_person->previous[node_index], &local);',
     'matrix4x3_inverse(&first_person->latest[parent], &inverse_parent);\n'
     '\t\t\t\tmatrix4x3_multiply(&inverse_parent, &first_person->latest[node_index], &local);', 'count'),
    # a count's node without a parent at the tick's pose
    ('\t\t\t\tdrawn[node_index] = first_person->previous[node_index];',
     '\t\t\t\tdrawn[node_index] = first_person->latest[node_index];', 'count loose'),
    # the frame before not kept with the pose before
    ('\t\tfirst_person->previous_count_frame = first_person->count_frame;\n', '', 'count'),
]


def generated(fault=None):
    source = read('port/linux/game/render_interpolation.c')
    maths = read('source/math/real_math.h')
    matrices = read('source/math/matrix_math.c')
    cseries = read('source/cseries/cseries.h')
    config = '\n'.join([
        '#define MAXIMUM_LOCAL_PLAYERS 4',
        'enum { ' + ', '.join(re.search(r'^\t(%s = \d+),' % name, cseries, re.M).group(1)
                             for name in ['LONG_BITS', 'LONG_BITS_BITS']) + ' };',
        re.search(r'^#define BIT_VECTOR_SIZE_IN_LONGS\(.*', cseries, re.M).group(),
        re.search(r'^#define BIT_VECTOR_TEST_FLAG\(.*', cseries, re.M).group(),
        re.search(r'^#define MAXIMUM_INTERPOLATED_NODES .*', source, re.M).group(),
        re.search(r'^#define FIRST_PERSON_HALF_TURN_COSINE .*', source, re.M).group(),
        re.search(r'^#define FIRST_PERSON_JUMP_DISTANCE .*', source, re.M).group(),
        re.search(r'^#define FIRST_PERSON_JUMP_RATIO .*', source, re.M).group(),
        re.search(r'^#define REAL_MAX .*', maths, re.M).group(),
        structure(maths, 'real_matrix4x3'),
        'typedef struct real_matrix4x3 real_matrix4x3;',
    ]) + '\n'
    code = '\n'.join(
        [function(matrices, name) for name in
         ['matrix4x3_inverse', 'matrix4x3_rotation_from_vectors', 'matrix4x3_from_point_and_vectors',
          'matrix4x3_multiply']] +
        [structure(source, name) for name in
         ['interpolation_quaternion', 'interpolation_rotation', 'interpolated_first_person']] +
        ['static struct interpolated_first_person interpolated_first_person[MAXIMUM_LOCAL_PLAYERS];'] +
        [function(source, name) for name in
         ['lerp', 'point_lerp', 'vector_length', 'basis_is_rotation', 'quaternion_from_basis',
          'basis_from_quaternion', 'rotation_from_matrix', 'matrix_blend_rotations', 'distance_squared',
          'first_person_move_on_parent', 'render_interpolation_first_person']])
    if fault:
        code = mutated(code, *fault)
    return (('config.inc', config), ('under_test.inc', code))


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('first_person_interpolation', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('before,after,case', CONTROLS)
def test_negative_control(before, after, case):
    status, output = run(build('first_person_interpolation', generated((before, after))), case)
    assert status == CHECK_FAILED, output
