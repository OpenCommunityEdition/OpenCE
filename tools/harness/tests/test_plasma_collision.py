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


@pytest.mark.parametrize('fault', [None, 'depth', 'far_t'])
def test_iterative_traversal_matches_recursive_state(fault):
    from collision_traversal_check import recursive_reference, NAME
    source=read('source/physics/collision_bsp.c')
    actual=function(source,NAME).replace(NAME,'iterative_actual')
    reference=recursive_reference(source).replace(NAME,'recursive_reference')
    if fault == 'depth': actual=actual.replace('\t\tdepth++;', '')
    if fault == 'far_t': actual=actual.replace('t0 = t;', 't0 = t1;')
    wrapper='''
static boolean collision_bsp_test_vector_recursive(struct collision_bsp_test_vector_context *data,
 long node_index,real t0,real t1,short depth)
{
 struct collision_bsp_test_vector_context old=*data;
 struct collision_bsp_test_vector_result expected=*data->result;
 struct collision_bsp_vector_cache cache;
 boolean fast,original;
 int saved_entries,saved_distances;
 old.result=&expected;
 if(data->cache) { cache=*data->cache;old.cache=&cache; }
 fast=iterative_actual(data,node_index,t0,t1,depth);
 saved_entries=entries;saved_distances=distances;
 original=recursive_reference(&old,node_index,t0,t1,depth);
 CHECK(fast==original&&!memcmp(data->result,&expected,sizeof(expected)),"iterative traversal preserves every result and leaf");
 CHECK(data->nodes_left==old.nodes_left&&data->last_plane_index==old.last_plane_index&&
  data->last_leaf_index==old.last_leaf_index&&data->last_contents==old.last_contents,"iterative traversal preserves all continuation state");
 if(data->cache) CHECK(!memcmp(data->cache,&cache,sizeof(cache)),"iterative traversal preserves cache state");
 entries=saved_entries;distances=saved_distances;
 return fast;
}
'''
    prepare='\n'.join(function(read('port/linux/game/plasma_collision.c'),name) for name in ('trace_side','trace_prepare'))
    includes=(('context.inc',structure(source,'collision_bsp_test_vector_context')),
              ('cache.inc',structure(read('source/physics/collision_bsp.h'),'collision_bsp_vector_cache')),
              ('prepare.inc',prepare),('under_test.inc',actual+'\n'+reference+'\n'+wrapper))
    status,output=run(build('collision_vector_cache',includes),'cache')
    assert status==(CHECK_FAILED if fault else 0),output


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
