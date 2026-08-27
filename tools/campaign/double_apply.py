"""Replace every remaining `volatile` in hs.c with a `double` local.

Measured mechanism: January routes these HS `real` script arguments through the
FPU (fld / push / fstp [esp], or fld / fstp [slot]). `volatile` produces that by
forcing a genuine typed read. So does declaring the local `double` and letting the
ordinary implicit conversion happen at the prototyped call - with no cast, no
`volatile`, and no change to what the program means, since float -> double -> float
is exact.

Verified on one member of each family before writing this:
  code_000b23d0 (struct family)  EXACT with `double value1 = arguments->value1;`
                                 and a plain implicit conversion at the call
  the 4 boolean wrappers         EXACT
  code_000b3f50                  size goes 80 -> 96, matching January

16 wrappers use HS_EVALUATE_VOID_FROM_ARGUMENTS with a `*_volatile_*` struct;
4 more are the RETURN_BOOLEAN_VOLATILE family. That is all 20 remaining dependents.
"""
import re, sys

SRC = 'source/hs/hs.c'
s = open(SRC, encoding='latin-1').read()

# ---------- 1. the struct family ----------
structs = {}
for m in re.finditer(r'struct (hs_arguments_\w+)\n\{(.*?)\n\};', s, re.S):
    vols = [l.split()[-1].rstrip(';') for l in m.group(2).split('\n') if 'volatile ' in l]
    if vols:
        structs[m.group(1)] = vols

renames = {}
for name in structs:
    new = name.replace('_volatile', '')
    assert new not in s, f'rename collision: {new}'
    renames[name] = new

# strip volatile from those struct definitions
for name in structs:
    m = re.search(r'struct ' + name + r'\n\{(.*?)\n\};', s, re.S)
    body = m.group(0)
    s = s.replace(body, body.replace('volatile ', ''), 1)

# rewrite the 16 invocations before renaming, so the pattern still matches
count = 0
for name, vols in structs.items():
    while True:
        m = re.search(r'HS_EVALUATE_VOID_FROM_ARGUMENTS\(\n\t(\w+),\n\tstruct ' + name + r',\n(.*?)\)\n',
                      s, re.S)
        if not m:
            break
        fn, expr = m.group(1), m.group(2).strip()
        used = [v for v in vols if f'arguments->{v}' in expr]
        assert used, (fn, vols, expr)
        decls = ''.join(f'\t\tdouble {v} = arguments->{v};\n' for v in used)
        new_expr = expr
        for v in used:
            new_expr = re.sub(r'arguments->' + v + r'\b', v, new_expr)
        s = s.replace(m.group(0),
                      f'''void {fn}(
\tshort function_index,
\tlong thread_index,
\tboolean initialize)
{{
\tstruct {renames[name]} const *arguments;

\targuments = (struct {renames[name]} const *)hs_macro_function_evaluate(function_index, thread_index, initialize);
\tif (arguments)
\t{{
{decls}
\t\t{new_expr};
\t\ths_return(thread_index, 0);
\t}}

\treturn;
}}
''', 1)
        count += 1

# now rename the struct types everywhere
for old, new in renames.items():
    s = re.sub(r'\b' + old + r'\b', new, s)
print(f'converted {count} struct wrappers; renamed {len(renames)} structs')

# ---------- 2. the boolean family ----------
BOOL = [
 ('code_000ad8a0', 'hs_objects_can_see_object',
  ['arguments[0].long_value', 'arguments[1].long_value'], 'degrees', 2),
 ('code_000ad8f0', 'hs_objects_can_see_flag',
  ['arguments[0].long_value', 'arguments[1].unsigned_short_value'], 'degrees', 2),
 ('code_000aee50', 'device_set_desired_position',
  ['arguments[0].long_value'], 'position', 1),
 ('code_000aef60', 'device_group_set_desired_value',
  ['arguments[0].short_value'], 'desired_value', 1),
]
nb = 0
for fn, callee, ints, rname, ridx in BOOL:
    m = re.search(r'HS_EVALUATE_RETURN_BOOLEAN_VOLATILE\(\n\t' + fn + r',\n\t[^,\n]+,\n.*?\)\n', s, re.S)
    assert m, fn
    args = ', '.join(ints + [rname])
    s = s.replace(m.group(0), f'''void {fn}(
\tshort function_index,
\tlong thread_index,
\tboolean initialize)
{{
\tunion hs_evaluation_argument const *arguments;
\tunion hs_boolean_result result;

\tresult.value = 0;
\targuments = (union hs_evaluation_argument const *)hs_macro_function_evaluate(function_index, thread_index, initialize);
\tif (arguments)
\t{{
\t\tdouble {rname} = arguments[{ridx}].real_value;

\t\tresult.boolean = {callee}({args});
\t\ths_return(thread_index, result.value);
\t}}

\treturn;
}}
''', 1)
    nb += 1
print(f'converted {nb} boolean wrappers')

# ---------- 3. drop the now-unused volatile macro ----------
m = re.search(r'#define HS_EVALUATE_RETURN_BOOLEAN_VOLATILE.*?\n\}\n\n', s, re.S)
if m and 'HS_EVALUATE_RETURN_BOOLEAN_VOLATILE(' not in s.replace(m.group(0), ''):
    s = s.replace(m.group(0), '', 1)
    print('removed the now-unused HS_EVALUATE_RETURN_BOOLEAN_VOLATILE macro')

open(SRC, 'w', encoding='latin-1', newline='\n').write(s)
print('volatile qualifiers remaining:', s.count('volatile '))
