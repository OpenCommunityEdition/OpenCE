"""Compare and time complete ray batches against the original query in one pose."""
from pathlib import Path

def instrument_collision(text):
    assert text.count('&batch.cache);') == 1
    return ('extern int lighting_trace_uncached; /* test build only */\n'+
            text.replace('&batch.cache);', 'lighting_trace_uncached ? NULL : &batch.cache);'))


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
    reference=original.replace('trace_depths(', 'trace_depths_reference(',1)
    reference=reference.replace('plasma_collision_begin(flags, from, ignore);','(void)0;')
    reference=reference.replace('plasma_collision_end();','(void)0;')
    reference=reference.replace('real_vector3d ray;', 'real_vector3d ray;\n\t\t\t\tstruct collision_result hit;')
    reference=reference.replace('plasma_collision_depth(flags, from, &ray, ignore)',
        '(collision_test_vector(flags, from, &ray, ignore, &hit) ? hit.t : 2.0f)')
    wrapper=(Path(__file__).parent/'fixtures/lighting_trace_check.inc').read_text()
    return text[:start]+original.replace('trace_depths(', 'trace_depths_actual(',1)+'\n'+reference+'\n'+wrapper+text[end:]
