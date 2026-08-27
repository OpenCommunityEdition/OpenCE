"""HS_EVALUATE_RETURN_BOOLEAN_VOLATILE -> real-argument binding.

The VOID macro converted cleanly; this one lost all 4 users on the first attempt.
It differs by an extra `union hs_boolean_result result; result.value = 0;` before
the evaluate call, so where the bound local sits relative to `result` matters.

Edits the file directly and restores - the JSON --edits round-trip mangles macro
line-continuations, which is what defeated the first attempt at this.
"""
import subprocess, re, shutil, sys

SRC = 'source/hs/hs.c'
BAK = 'scratch/hs_bool_backup.c'
shutil.copy(SRC, BAK)
orig = open(SRC, encoding='latin-1').read()
old = re.search(r'#define HS_EVALUATE_RETURN_BOOLEAN_VOLATILE.*?\n\}\n', orig, re.S).group(0)
FNS = ['_code_000ad8a0', '_code_000ad8f0', '_code_000aee50', '_code_000aef60']

B = '\\\n'   # backslash + newline, the macro continuation


def macro(decls, pre_expr):
    return (
        '#define HS_EVALUATE_RETURN_BOOLEAN_WITH_REAL(evaluator, arguments_type, real_index, expression) ' + B +
        'void evaluator( ' + B + '\tshort function_index, ' + B + '\tlong thread_index, ' + B +
        '\tboolean initialize) ' + B + '{ ' + B + decls +
        '\targuments = (arguments_type const *)hs_macro_function_evaluate(function_index, thread_index, initialize); ' + B +
        '\tif (arguments) ' + B + '\t{ ' + B + pre_expr +
        '\t\tresult.boolean = expression; ' + B +
        '\t\ths_return(thread_index, result.value); ' + B + '\t} ' + B + '\treturn; ' + B + '}\n')


D_BASE = ('\targuments_type const *arguments; ' + B + '\tunion hs_boolean_result result; ' + B +
          '\tresult.value = 0; ' + B)
D_REAL_AFTER = ('\targuments_type const *arguments; ' + B + '\tunion hs_boolean_result result; ' + B +
                '\treal real_argument; ' + B + '\tresult.value = 0; ' + B)
D_REAL_FIRST = ('\targuments_type const *arguments; ' + B + '\treal real_argument; ' + B +
                '\tunion hs_boolean_result result; ' + B + '\tresult.value = 0; ' + B)

BIND_DECL = '\t\treal real_argument = arguments[real_index].real_value; ' + B
BIND_ASSIGN = '\t\treal_argument = arguments[real_index].real_value; ' + B
BIND_BLOCK = ('\t\treal real_argument; ' + B + '\t\treal_argument = arguments[real_index].real_value; ' + B)

V = {
    'decl-init-in-if':        macro(D_BASE, BIND_DECL),
    'real-after-result':      macro(D_REAL_AFTER, BIND_ASSIGN),
    'real-before-result':     macro(D_REAL_FIRST, BIND_ASSIGN),
    'block-decl-then-assign': macro(D_BASE, BIND_BLOCK),
}


def gate():
    r = subprocess.run(['python', 'tools/campaign/gate.py', 'source/hs/hs'],
                       capture_output=True, text=True)
    if 'COMPILE FAILED' in r.stdout:
        return None, r.stdout[-200:].replace('\n', ' ')
    m = re.search(r'== exact (\d+)', r.stdout)
    bad = [f for f in FNS if any(f in l and 'residual' in l for l in r.stdout.splitlines())]
    return (int(m.group(1)) if m else None), f'boolean-family residual: {bad}'


try:
    for name, new_macro in V.items():
        s = orig.replace(old, new_macro, 1)
        # rewrite the 4 invocations
        pat = re.compile(r'HS_EVALUATE_RETURN_BOOLEAN_VOLATILE\(\n\t(\w+),\n\t([^,\n]+),\n(.*?)\)\n', re.S)
        def repl(m):
            nm, at, ex = m.group(1), m.group(2), m.group(3)
            idx = re.findall(r'arguments\[(\d+)\]\.real_value', ex)[0]
            return (f'HS_EVALUATE_RETURN_BOOLEAN_WITH_REAL(\n\t{nm},\n\t{at},\n\t{idx},\n'
                    + ex.replace(f'arguments[{idx}].real_value', 'real_argument') + ')\n')
        s = pat.sub(repl, s)
        open(SRC, 'w', encoding='latin-1', newline='\n').write(s)
        got, note = gate()
        print(f'{name:24s} exact={got}  {note}', flush=True)
finally:
    shutil.copy(BAK, SRC)
    print('source restored')
