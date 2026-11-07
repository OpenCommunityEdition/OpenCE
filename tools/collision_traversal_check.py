"""Restore the previous recursive continuations for same-input comparison.

The geometry, guards and leaf tests still come from the production source.
Only the three tail continuations are changed back to their prior call form.
"""
from harness import function

NAME = 'collision_bsp_test_vector_recursive'


def recursive_reference(text):
    body = function(text, NAME)
    replacements = [
        ('while (!(node_index & LONG_MIN))', 'if (!(node_index & LONG_MIN))'),
        ('''depth += data->cache->nodes[cache_slot].skip_count;
				node_index = data->cache->nodes[cache_slot].skip_node;
				continue;''', '''return collision_bsp_test_vector_recursive(data,
                    data->cache->nodes[cache_slot].skip_node, t0, t1,
                    depth + data->cache->nodes[cache_slot].skip_count);'''),
        ('''node_index = node->children[front];
			t0 = t;''', '''if (collision_bsp_test_vector_recursive(data, node->children[front], t, t1, depth + 1))
                return TRUE;'''),
        ('node_index = node->children[reaches_front];', '''if (collision_bsp_test_vector_recursive(data, node->children[reaches_front], t0, t1, depth + 1))
                return TRUE;'''),
        ('\t\tdepth++;', ''),
        ('\t/* port: the final leaf has the same incoming state as the recursive path. */', '\telse'),
    ]
    for before, after in replacements:
        assert body.count(before) == 1, before
        body = body.replace(before, after)
    return body


def instrument(text):
    body = function(text, NAME)
    reference = recursive_reference(text).replace(NAME, 'lighting_recursive_reference')
    actual = body.replace(NAME, 'lighting_iterative_actual')
    wrapper = body[:body.index('{')] + '''{
    return lighting_trace_recursive ? lighting_recursive_reference(data, node_index, t0, t1, depth) :
        lighting_iterative_actual(data, node_index, t0, t1, depth);
}
'''
    return 'extern int lighting_trace_recursive; /* port: comparison build only */\n' + text.replace(body, reference+'\n'+actual+'\n'+wrapper)
