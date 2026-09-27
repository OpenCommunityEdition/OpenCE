"""Compile a list of units from two alternate roots with W1's revgate and compare.
    python sweep.py <rootA> <rootB> <units.txt> <outdir> [--jobs N]
Per unit: revgate counts (vs January build/split) for A and B, objeq --norm-labels A vs B, and control objeq
build/base vs A. Writes <outdir>/sweep.tsv."""
import concurrent.futures, os, subprocess, sys
WT = r'C:\halo-worktrees\claude-compiler-application-20260925'
REV = os.path.join(WT, r'research\compiler_application_20260925\workers\W1\tools\revgate.py')
OBJEQ = os.path.join(WT, r'research\compiler_application_20260925\workers\A4\tools\objeq.py')
a_root, b_root, units_file, out = sys.argv[1:5]
jobs = int(sys.argv[sys.argv.index('--jobs') + 1]) if '--jobs' in sys.argv else 6
units = [u.strip() for u in open(units_file) if u.strip()]
os.makedirs(out, exist_ok=True)
def one(u):
    b = u.replace('/', '__').replace(' ', '_')
    res = {'unit': u}
    for tag, root in (('A', a_root), ('B', b_root)):
        obj = os.path.join(out, b + '_' + tag + '.obj')
        r = subprocess.run([sys.executable, '-B', REV, root, u, '--out', obj, '--quiet'], capture_output=True, text=True, cwd=WT)
        lines = [l for l in (r.stdout + r.stderr).splitlines() if l.strip()]
        res[tag] = lines[-1] if lines else 'NO OUTPUT'
        res[tag + '_res'] = [l for l in lines if l.startswith('residual') or l.startswith('UNWRITTEN')]
        res[tag + '_obj'] = obj if os.path.exists(obj) and 'COMPILE FAILED' not in r.stdout else None
        if 'COMPILE FAILED' in r.stdout:
            res[tag] = 'COMPILE FAILED: ' + ' | '.join(l for l in lines if 'error' in l)[:400]
    if res['A_obj'] and res['B_obj']:
        r = subprocess.run([sys.executable, '-B', OBJEQ, res['A_obj'], res['B_obj'], '--norm-labels'], capture_output=True, text=True)
        res['eq'] = r.stdout.strip().replace('\n', ' || ')
        base = os.path.join(WT, 'build', 'base', u + '.obj')
        if os.path.exists(base):
            r = subprocess.run([sys.executable, '-B', OBJEQ, base, res['A_obj'], '--norm-labels', '--quiet'], capture_output=True, text=True)
            res['ctl'] = r.stdout.strip()
        else:
            res['ctl'] = 'no build/base object'
    else:
        res['eq'] = 'n/a'
        res['ctl'] = 'n/a'
    return res
with concurrent.futures.ThreadPoolExecutor(jobs) as ex:
    results = list(ex.map(one, units))
with open(os.path.join(out, 'sweep.tsv'), 'w') as f:
    for r in results:
        f.write('\t'.join([r['unit'], r['A'], r['B'], r['eq'], r['ctl'], ';'.join(r['A_res']), ';'.join(r['B_res'])]) + '\n')
diff = 0
for r in results:
    same_counts = r['A'] == r['B']
    ident = r['eq'].startswith('IDENTICAL')
    ctl = r['ctl'].startswith('IDENTICAL')
    flag = '' if (same_counts and ident and ctl) else '  <<<'
    if flag:
        diff += 1
    print('%-62s A[%s] B[%s] %s ctl=%s%s' % (r['unit'][:62], r['A'].replace('== ', ''), r['B'].replace('== ', ''), r['eq'][:90], 'ok' if ctl else r['ctl'][:40], flag))
    if r['A_res'] != r['B_res']:
        print('      residual set changed: A=%s B=%s' % (r['A_res'], r['B_res']))
print('units %d, flagged %d' % (len(results), diff))
