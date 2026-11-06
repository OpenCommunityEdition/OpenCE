"""Compare model-major batches with the original model query and matrix maths."""
import re
from tools.harness import build, read, run, function, structure


def test_model_major_batch_matches_original_queries():
    code = structure(read('source/math/real_math.h'), 'real_matrix4x3') + '\ntypedef struct real_matrix4x3 real_matrix4x3;\n'
    code += structure(read('source/physics/collision_models.h'), 'collision_model_instance') + '\n'
    maths = read('source/math/matrix_math.c')
    inverse = function(maths, 'matrix4x3_inverse').replace('matrix4x3_inverse(', 'original_matrix4x3_inverse(', 1)
    code += inverse + '\nstatic void matrix4x3_inverse(const real_matrix4x3 *m,real_matrix4x3 *r) { inversions++;original_matrix4x3_inverse(m,r); }\n'
    code += '\n'.join(function(maths, name) for name in ('matrix4x3_transform_point', 'matrix4x3_transform_vector'))
    original = read('source/physics/collision_models.c')
    code += '\n'+function(original, 'collision_node_region_valid')+'\nstatic LARGE_INTEGER collision_model_test_vector_time;\n'
    code += function(original, 'collision_model_test_vector') + '\n'
    code += re.sub(r'^#include.*$', '', read('port/linux/game/plasma_collision_models.c'), flags=re.M)
    status, output = run(build('plasma_collision_models', (('under_test.inc', code),)), 'batch')
    assert status == 0, output
