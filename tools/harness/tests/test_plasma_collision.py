"""Exercise batch ownership, exact eligibility and safe fallback using real code."""
import re
import sys
import pytest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[2]))
from harness import build,read,run,CHECK_FAILED,function,structure

def code():
    return ('enum { COLLISION_VECTOR_CACHE_NODES = 512 };\n' +
            structure(read('source/physics/collision_bsp.h'),'collision_bsp_vector_cache') + '\n' +
            function(read('source/math/real_math.c'), 'fast_vector_intersects_sphere') + '\n' +
            re.sub(r'^#include.*$','',read('port/linux/game/plasma_collision.c'),flags=re.M))

def test_depth_batch():
    status,output=run(build('plasma_collision',(('under_test.inc',code()),)),'batch')
    assert status==0,output

def test_child_gating_regression():
    broken=code().replace('object->eligible && trace_sphere(object, vector)', 'object->eligible')
    status,output=run(build('plasma_collision',(('under_test.inc',broken),)),'batch')
    assert status==CHECK_FAILED,output


def test_shared_vector_entry_preserves_stock_contract():
    source=read('source/physics/collision_bsp.c')
    functions='\n'.join(function(source,name) for name in (
        'collision_bsp_test_vector_at_node','collision_bsp_test_vector','collision_bsp_test_vector_from_node'))
    context=structure(source,'collision_bsp_test_vector_context')
    status,output=run(build('collision_vector_entry',(('context.inc',context),('under_test.inc',functions))),'entry')
    assert status==0,output


@pytest.mark.parametrize('fault', [None, 'budget', 'bounds', 'eviction'])
def test_vector_cache_preserves_traversal_results(fault):
    source=read('source/physics/collision_bsp.c')
    traversal=function(source,'collision_bsp_test_vector_recursive')
    prepare='\n'.join(function(read('port/linux/game/plasma_collision.c'),name) for name in ('trace_side','trace_prepare'))
    if fault == 'budget':
        traversal=traversal.replace('data->nodes_left -= data->cache->nodes[cache_slot].skip_count - 1;', '')
    if fault == 'bounds':
        prepare=prepare.replace('low > margin', '(low-low+distance) > margin')
    if fault == 'eviction':
        traversal=traversal.replace('data->cache->nodes[cache_slot].skip_count = 0;', '')
    includes=(('context.inc',structure(source,'collision_bsp_test_vector_context')),
              ('cache.inc',structure(read('source/physics/collision_bsp.h'),'collision_bsp_vector_cache')),
              ('prepare.inc',prepare), ('under_test.inc',traversal))
    status,output=run(build('collision_vector_cache',includes),'cache')
    assert status==(CHECK_FAILED if fault else 0),output
