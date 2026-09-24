"""Integrator batch gate: run the full verification chain and summarise against the frozen baseline.

    python scratch/campaign/batch_gate.py <label>

Steps: guard objdiff version (3.3.1) and CL path, ninja (full; re-patch build.ninja if configure
re-ran), parse progress, stable_verdicts snapshot+diff vs scratch/baseline/stable.json, parks,
admission audit, fake-match scan (lead count vs baseline), pytest, git diff --check.
Writes scratch/campaign/gate_<label>/ with every raw log.
"""
import json
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
os.chdir(ROOT)
label = sys.argv[1]
out = os.path.join('scratch', 'campaign', 'gate_' + label)
os.makedirs(out, exist_ok=True)


def run(name, cmd, timeout=3600):
    r = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout, shell=isinstance(cmd, str))
    open(os.path.join(out, name + '.log'), 'w', encoding='utf-8').write(r.stdout + '\n' + r.stderr)
    return r


def objdiff_version():
    return run('objdiff_version', [os.path.join('build', 'tools', 'objdiff-cli.exe'), '--version']).stdout.strip()


summary = {}
v = objdiff_version()
summary['objdiff'] = v
if '3.3.1' not in v:
    print('ABORT: objdiff is not 3.3.1:', v)
    sys.exit(2)

# ninja; if configure re-ran, re-patch the CL path and rebuild
r = run('ninja', ['ninja'])
if r.returncode != 0 and 'The system cannot find the file specified' in (r.stdout + r.stderr):
    run('ninja_manifest', ['ninja', 'build.ninja'])
    run('patch', [sys.executable, 'scratch/patch_ninja.py'])
    r = run('ninja', ['ninja'])
bn = open('build.ninja').read()
if 'command = xbox/bin/vc7/CL.Exe' in bn:
    run('patch', [sys.executable, 'scratch/patch_ninja.py'])
    r = run('ninja', ['ninja'])
summary['ninja_exit'] = r.returncode
v2 = objdiff_version()
summary['objdiff_after'] = v2
log = r.stdout
m = re.search(r'halobetacache: ([\d.]+)% matched, ([\d.]+)% linked \((\d+) / (\d+) files\)\s+Code: (\d+) / (\d+) bytes \((\d+) / (\d+) functions\)\s+Data: (\d+) / (\d+) bytes', log)
if m:
    summary['halo'] = {'objects': int(m.group(3)), 'of': int(m.group(4)), 'code': int(m.group(5)),
                       'code_total': int(m.group(6)), 'functions': int(m.group(7)), 'data': int(m.group(9))}
m = re.search(r'All: [\d.]+% matched, [\d.]+% linked \((\d+) / (\d+) files\)', log)
if m:
    summary['all_objects'] = int(m.group(1))

run('snapshot', [sys.executable, '-m', 'tools.campaign.stable_verdicts', 'snapshot', os.path.join(out, 'stable.json')])
d = run('stable_diff', [sys.executable, '-m', 'tools.campaign.stable_verdicts', 'diff', 'scratch/baseline/stable.json', os.path.join(out, 'stable.json')])
summary['stable_diff_exit'] = d.returncode
summary['stable_diff_tail'] = d.stdout.strip().splitlines()[-6:]

p = run('parks', [sys.executable, '-B', '-m', 'tools.parked_functions'])
try:
    summary['parks'] = json.loads(p.stdout)['summary']
except Exception:
    summary['parks'] = p.stdout[-400:]
a = run('admission', [sys.executable, 'tools/audit_object_admission.py'])
try:
    aj = json.loads(a.stdout)
    summary['admission'] = aj['summary']
    summary['admission_contradicted'] = [c.get('unit') for c in aj.get('contradicted', [])]
    summary['admission_revoked'] = [c.get('unit') for c in aj.get('revoked', [])]
except Exception:
    summary['admission'] = a.stdout[-400:]
f = run('fake_scan', [sys.executable, 'tools/fake_match_scan.py'])
m = re.search(r'(\d+) review lead', f.stdout)
summary['fake_leads'] = int(m.group(1)) if m else f.stdout[-200:]
t = run('pytest', [sys.executable, '-m', 'pytest', '-q', 'tools', '--basetemp=' + os.path.join(ROOT, 'scratch', 'pytest_tmp')])
summary['pytest'] = t.stdout.strip().splitlines()[-1] if t.stdout.strip() else t.stderr[-200:]
g = run('diff_check', ['git', 'diff', '--check'])
summary['diff_check'] = 'clean' if not g.stdout.strip() else g.stdout[:400]
json.dump(summary, open(os.path.join(out, 'summary.json'), 'w'), indent=1)
print(json.dumps(summary, indent=1))
