"""Compare and time complete ray batches against the original query in one pose."""
from pathlib import Path

def instrument_collision(text):
    marker = '&batch.cache[octant]);' if '&batch.cache[octant]);' in text else '&cache))'
    assert text.count(marker) == 1
    replacement = marker.replace('[octant]', '[lighting_trace_unprepared ? 0 : octant]')
    text = text.replace(marker, 'lighting_trace_uncached ? NULL : '+replacement)
    call = 'trace_prepare(batch.prefixes[octant].node, batch.prefixes[octant].depth, octant, &work, &target, &skipped);'
    if '[octant]' in marker:
        assert text.count(call) == 1
        text = text.replace(call, 'if (!lighting_trace_unprepared) '+call)
    return 'extern int lighting_trace_uncached, lighting_trace_unprepared; /* test build only */\n'+text


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
    reference=original.replace('trace_depths(', 'trace_depths_reference(',1)
    reference=reference.replace(call, '''for (axis = 0; axis < count; axis++)
    {
        struct collision_result hit;
        hits[axis] = collision_test_vector(flags, from, &vectors[axis], ignore, &hit) ? hit.t : 2.0f;
    }''')
    wrapper=(Path(__file__).parent/'fixtures/lighting_trace_check.inc').read_text()
    return text[:start]+original.replace('trace_depths(', 'trace_depths_actual(',1)+'\n'+reference+'\n'+wrapper+text[end:]
