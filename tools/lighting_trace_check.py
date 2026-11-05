"""Compare and time complete ray batches against the original query in one pose."""
from pathlib import Path

def instrument_collision(text):
    marker = '&batch.cache);' if '&batch.cache);' in text else '&cache))'
    assert text.count(marker) == 1
    return ('extern int lighting_trace_uncached; /* test build only */\n'+
            text.replace(marker, 'lighting_trace_uncached ? NULL : '+marker))


def instrument(text):
    marker='static void trace_depths('
    assert text.count(marker)==1
    start=text.index(marker)
    opening=text.index('{',start)
    depth=1
    end=opening+1
    while depth:
        depth += (text[end]=='{')-(text[end]=='}')
        end+=1
    original=text[start:end]
    call = 'plasma_collision_depths(flags, from, vectors, count, ignore, hits);'
    assert original.count(call) == 1
    scalar=original.replace('trace_depths(', 'trace_depths_scalar(',1).replace(call, '''
    plasma_collision_begin(flags, from, ignore);
    for (axis = 0; axis < count; axis++) hits[axis] = plasma_collision_depth(flags, from, &vectors[axis], ignore);
    plasma_collision_end();''')
    reference=original.replace('trace_depths(', 'trace_depths_reference(',1)
    reference=reference.replace(call, '''for (axis = 0; axis < count; axis++)
    {
        struct collision_result hit;
        hits[axis] = collision_test_vector(flags, from, &vectors[axis], ignore, &hit) ? hit.t : 2.0f;
    }''')
    wrapper=(Path(__file__).parent/'fixtures/lighting_trace_check.inc').read_text()
    return text[:start]+original.replace('trace_depths(', 'trace_depths_actual(',1)+'\n'+scalar+'\n'+reference+'\n'+wrapper+text[end:]
