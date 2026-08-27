"""Write the 4 HS_EVALUATE_RETURN_BOOLEAN_VOLATILE call sites out as explicit
functions with every argument bound to a named local, and find the shape that
keeps all four byte-exact without `volatile`.

Binding only the real argument works for the VOID family but not this one (four
placements measured inert). Binding *every* argument is what worked for the VOID
family in the original probe, so try that here.

Callee prototypes (hs.c:3590-3648, devices.h:93):
  hs_objects_can_see_object(long object_list_index, long object_index, real degrees)
  hs_objects_can_see_flag(long object_list_index, word cutscene_flag_index, real degrees)
  device_set_desired_position(long device_index, real position)
  device_group_set_desired_value(short group_index, real desired_value)

Local types mirror the union accessor, which matches the parameter type in every
case, so the reads and the conversions are unchanged.
"""
import subprocess, re, shutil

SRC = 'source/hs/hs.c'
BAK = 'scratch/hs_boolwrite_backup.c'
shutil.copy(SRC, BAK)
orig = open(SRC, encoding='latin-1').read()

FNS = ['_code_000ad8a0', '_code_000ad8f0', '_code_000aee50', '_code_000aef60']

# name, callee, [(local_type, local_name, accessor), ...]
SITES = [
 ('code_000ad8a0', 'hs_objects_can_see_object',
  [('long', 'object_list_index', 'long_value'),
   ('long', 'object_index', 'long_value'),
   ('real', 'degrees', 'real_value')]),
 ('code_000ad8f0', 'hs_objects_can_see_flag',
  [('long', 'object_list_index', 'long_value'),
   ('word', 'cutscene_flag_index', 'unsigned_short_value'),
   ('real', 'degrees', 'real_value')]),
 ('code_000aee50', 'device_set_desired_position',
  [('long', 'device_index', 'long_value'),
   ('real', 'position', 'real_value')]),
 ('code_000aef60', 'device_group_set_desired_value',
  [('short', 'group_index', 'short_value'),
   ('real', 'desired_value', 'real_value')]),
]


def emit(name, callee, args, style):
    binds_fwd = list(enumerate(args))
    binds = binds_fwd if 'rev' not in style else list(reversed(binds_fwd))
    call = f'{callee}(' + ', '.join(a[1] for a in args) + ')'
    if style.startswith('decl-init'):
        block = ''.join(f'\t\t{t} {n} = arguments[{i}].{acc};\n' for i, (t, n, acc) in binds)
        top = ''
    else:  # top declarations, assignments in the if
        top = ''.join(f'\t{t} {n};\n' for t, n, acc in args)
        block = ''.join(f'\t\t{n} = arguments[{i}].{acc};\n' for i, (t, n, acc) in binds)
    return (f'void {name}(\n\tshort function_index,\n\tlong thread_index,\n\tboolean initialize)\n{{\n'
            f'\tunion hs_evaluation_argument const *arguments;\n'
            f'\tunion hs_boolean_result result;\n'
            + top +
            f'\n\tresult.value = 0;\n'
            f'\targuments = (union hs_evaluation_argument const *)hs_macro_function_evaluate(function_index, thread_index, initialize);\n'
            f'\tif (arguments)\n\t{{\n'
            + block + '\n'
            f'\t\tresult.boolean = {call};\n'
            f'\t\ths_return(thread_index, result.value);\n'
            f'\t}}\n\n\treturn;\n}}\n')


def gate():
    r = subprocess.run(['python', 'tools/campaign/gate.py', 'source/hs/hs'],
                       capture_output=True, text=True)
    if 'COMPILE FAILED' in r.stdout:
        return None, r.stdout[-220:].replace('\n', ' ')
    m = re.search(r'== exact (\d+)', r.stdout)
    bad = [f for f in FNS if any(f in l and 'residual' in l for l in r.stdout.splitlines())]
    return (int(m.group(1)) if m else None), (f'still residual: {bad}' if bad else 'ALL FOUR EXACT')


pat = re.compile(r'HS_EVALUATE_RETURN_BOOLEAN_VOLATILE\(\n\t(\w+),\n\t[^,\n]+,\n.*?\)\n', re.S)
try:
    for style in ('decl-init-fwd', 'decl-init-rev', 'top-decl-fwd', 'top-decl-rev'):
        s = orig
        for name, callee, args in SITES:
            m = re.search(r'HS_EVALUATE_RETURN_BOOLEAN_VOLATILE\(\n\t' + name + r',\n\t[^,\n]+,\n.*?\)\n',
                          s, re.S)
            assert m, name
            s = s.replace(m.group(0), emit(name, callee, args, style), 1)
        open(SRC, 'w', encoding='latin-1', newline='\n').write(s)
        got, note = gate()
        print(f'{style:16s} exact={got}  {note}', flush=True)
finally:
    shutil.copy(BAK, SRC)
    print('source restored')
