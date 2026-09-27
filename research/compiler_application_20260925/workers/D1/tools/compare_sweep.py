"""D1: tabulate a sweep (see sweep.py): control = build/base/<unit>.obj vs <sweep>/<A>/<unit>.obj, and the A -> B
change for every unit (objcmp with .debug$ ignored, keyed per-section diff with January classification, gate rows).

    python -B compare_sweep.py <sweep_dir> <units.txt> <A> <B>
"""
import os
import subprocess
import sys

WT = r'C:\halo-worktrees\claude-compiler-application-20260925'
T = os.path.join(WT, r'research\compiler_application_20260925\workers\D1\tools')
sweep, units_file, A, B = sys.argv[1:5]
units = [u.strip() for u in open(units_file) if u.strip()]


def run(args):
    r = subprocess.run([sys.executable, '-B'] + args, capture_output=True, text=True, cwd=WT, encoding='latin-1')
    return r.stdout.strip().splitlines()


def gate_line(name, flat):
    p = os.path.join(sweep, name, flat + '.gate.txt')
    ls = [ln for ln in open(p, encoding='latin-1').read().splitlines() if ln.startswith('==')]
    return ls[-1] if ls else 'NOGATE'


def rows(name, flat):
    p = os.path.join(sweep, name, flat + '.gate.txt')
    return sorted(ln for ln in open(p, encoding='latin-1').read().splitlines() if ln.startswith(('residual', 'UNWRITTEN')))


tot = {'units': 0, 'control_identical': 0, 'identical': 0, 'changed': 0}
for u in units:
    flat = u.replace('/', '__')
    a = os.path.join(sweep, A, flat + '.obj')
    b = os.path.join(sweep, B, flat + '.obj')
    base = os.path.join(WT, 'build', 'base', u + '.obj')
    ctl = run([os.path.join(T, 'objcmp.py'), base, a, '--quiet'])[-1] if os.path.exists(base) else 'NO build/base'
    ab = run([os.path.join(T, 'objcmp.py'), a, b, '--quiet'])[-1]
    tot['units'] += 1
    tot['control_identical'] += 'IDENTICAL' in ctl
    ga, gb = gate_line(A, flat), gate_line(B, flat)
    line = '%-52s control:%-9s %s->%s:%-9s gate %s' % (u, 'IDENT' if 'IDENTICAL' in ctl else 'DIFF', A, B,
                                                         'IDENT' if 'IDENTICAL' in ab else 'DIFF', gb)
    if ga != gb:
        line += '   (was %s)' % ga
    print(line)
    if 'IDENTICAL' in ab:
        tot['identical'] += 1
    else:
        tot['changed'] += 1
        for ln in run([os.path.join(WT, r'scratch\campaign\keyed_diff.py'), a, b, '--vs-january', u]):
            print('      ' + ln)
        ra, rb = rows(A, flat), rows(B, flat)
        for r in sorted(set(ra) - set(rb)):
            print('      row gone : ' + r)
        for r in sorted(set(rb) - set(ra)):
            print('      row new  : ' + r)
print('TOTAL units %(units)d | control identical %(control_identical)d | A->B identical %(identical)d, changed '
      '%(changed)d' % tot)
