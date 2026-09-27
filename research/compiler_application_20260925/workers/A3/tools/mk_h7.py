"""A3: build the H-7 union-free probe from copies/hs_H6 (CRLF preserved). Each replacement asserts its count."""
import os
import re

A = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
src = os.path.join(A, 'copies', 'hs_H6', 'source', 'hs', 'hs_runtime.c')
out = os.path.join(A, 'copies', 'hs_H7', 'source', 'hs', 'hs_runtime.c')
t = open(src, 'rb').read().decode('latin-1')
NL = '\r\n'


def rep(old, new, count):
    global t
    o = old.replace('\n', NL)
    n = new.replace('\n', NL)
    c = t.count(o)
    assert c == count, (old, c, count)
    t = t.replace(o, n)


rep('global->value.boolean', '*(boolean *)&global->value', 2)
rep('global->value.real', '*(real *)&global->value', 2)
rep('global->value.short_integer', '*(short *)&global->value', t.count('global->value.short_integer'))
rep('global->value.long_integer', 'global->value', t.count('global->value.long_integer'))
rep('global->value.string', '*(char const **)&global->value', 2)
rep('&global_datum->value.long_integer', '&global_datum->value', 1)
rep('global_index))->value.long_integer)', 'global_index))->value)', 1)
rep('->\n\t\tvalue.long_integer;\n', '->\n\t\tvalue;\n', 1)
rep('\tunion hs_conversion_result value;\n};', '\tlong value;\n};', 1)
# logical
rep('\tunion hs_conversion_result value_out;\n\n\tmatch_assert', '\tlong result_long;\n\n\tmatch_assert', 1)
rep('\t\tvalue_out.boolean = *result;\n\n\t\ths_return(thread_index, value_out.long_integer);\n',
    '\t\t*(boolean *)&result_long= *result;\n\n\t\ths_return(thread_index, result_long);\n', 1)
# arithmetic
rep('\tunion hs_conversion_result value_out;\n', '\tlong result_long;\n', 1)
rep('\t\tvalue_out.real = *result;\n', '\t\t*(real *)&result_long= *result;\n', 1)
rep('\t\ths_return(thread_index, value_out.long_integer);\n', '\t\ths_return(thread_index, result_long);\n', 1)
# equality
rep('\tlong *arguments;\n\tunion hs_conversion_result result;\n\tshort type;\n',
    '\tlong *arguments;\n\tlong result_long;\n\tshort type;\n', 1)
rep('\t\tresult.boolean = equal;\n\t\ths_return(thread_index, result.long_integer);\n',
    '\t\t*(boolean *)&result_long= equal;\n\t\ths_return(thread_index, result_long);\n', 1)
# inequality
rep('\t\tunion hs_conversion_result result;\n\t\tboolean comparison;\n',
    '\t\tlong result_long;\n\t\tboolean comparison;\n', 1)
rep('\t\tresult.boolean = comparison;\n\t\ths_return(thread_index, result.long_integer);\n',
    '\t\t*(boolean *)&result_long= comparison;\n\t\ths_return(thread_index, result_long);\n', 1)
# if and sleep_until stack slots
rep('union hs_conversion_result *condition = hs_stack_allocate(thread_index, sizeof(long));',
    'long *condition = hs_stack_allocate(thread_index, sizeof(long));', 2)
rep('union hs_conversion_result *ticks = hs_stack_allocate(thread_index, sizeof(long));',
    'long *ticks = hs_stack_allocate(thread_index, sizeof(long));', 1)
rep('\t\tcondition->long_integer = 0;\n', '\t\t*condition = 0;\n', 1)
rep('&condition->long_integer', 'condition', t.count('&condition->long_integer'))
rep('condition->boolean', '*(boolean *)condition', t.count('condition->boolean'))
rep('&ticks->long_integer', 'ticks', 1)
rep('ticks->short_integer', '*(short *)ticks', t.count('ticks->short_integer'))
# union definition
m = re.search(r'union hs_conversion_result\r\n\{\r\n(?:\t[^\r\n]*\r\n)*\};\r\n\r\n', t)
assert m, 'union definition not found'
t = t[:m.start()] + t[m.end():]
assert 'hs_conversion_result' not in t, [l for l in t.split(NL) if 'hs_conversion_result' in l]
assert '.long_integer' not in t and '.short_integer' not in t and 'value_out' not in t
os.makedirs(os.path.dirname(out), exist_ok=True)
open(out, 'wb').write(t.encode('latin-1'))
print('wrote', out)
