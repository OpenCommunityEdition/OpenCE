"""A model's vector test that passes over the nodes it does not come near finds what testing every node finds."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run, structure

CASES = ['random', 'grazing', 'short', 'invalidated']
SPHERE_TEST = ('if (instance->matrices[node_index].scale > 0.f &&\n'
               '\t\t\t\t\t\tcollision_bsp_bounds_cache_sphere(')


def generated(fault=None):
    models = read('source/physics/collision_models.c')
    cache = read('port/linux/game/object_bounds_cache.c')
    matrices = read('source/math/matrix_math.c')
    bsp_spheres = cache[cache.index('#define COLLISION_BSP_BOUNDS_COUNT'):]
    tested = function(models, 'collision_model_test_vector')
    if fault and fault[0] in tested:
        tested, fault = mutated(tested, *fault), None
    without = mutated(tested.replace('collision_model_test_vector(', 'collision_model_test_vector_without_spheres(', 1),
                      SPHERE_TEST, SPHERE_TEST.replace('instance->matrices[node_index].scale > 0.f', 'FALSE'))
    code = '\n\n'.join([
        'static long object_bounds_epoch = 1;',
        function(cache, 'object_bounds_cache_invalidate'),
        bsp_spheres,
        function(matrices, 'matrix4x3_inverse'),
        function(matrices, 'matrix4x3_transform_point'),
        function(matrices, 'matrix4x3_transform_vector'),
        'static LARGE_INTEGER collision_model_test_vector_time;',
        function(models, 'collision_model_port_vector_near_sphere'),
        tested,
        without,
    ])
    if fault:
        code = mutated(code, *fault)
    structures = '\n'.join([
        structure(read('source/math/real_math.h'), 'real_matrix4x3') + '\ntypedef struct real_matrix4x3 real_matrix4x3;',
        re.sub(r'const real_plane3d \*plane;', 'real_plane3d const *plane;',
               structure(read('source/physics/collision_bsp.h'), 'collision_bsp_test_vector_result')),
        structure(read('source/physics/collision_bsp.h'), 'collision_model_test_vector_result'),
    ])
    return (('structures.inc', structures + '\n'), ('under_test.inc', code + '\n'))


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('collision_model_spheres', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('fault,case', [
    # a scaled node's sphere not scaled with it
    (('sphere_radius * instance->matrices[node_index].scale))', 'sphere_radius))'), 'random'),
    # the sphere drawn tighter than the bsp's vertices
    (('1.001f * (real)sqrt(largest) + 0.01f', '0.97f * (real)sqrt(largest)'), 'grazing'),
    # a bsp's sphere kept across an invalidation
    (('if (entry->epoch == object_bounds_epoch && entry->bsp == bsp)', 'if (entry->bsp == bsp)'), 'invalidated'),
])
def test_negative_control(fault, case):
    status, output = run(build('collision_model_spheres', generated(fault)), case)
    assert status == CHECK_FAILED, output
