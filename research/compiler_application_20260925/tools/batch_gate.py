"""Batch integration gate (lead only). Run from the worktree root AFTER applying a batch's patches:

    python -B scratch/campaign/batch_gate.py <label>

Steps:
  1. full `ninja` (log scratch/campaign/batch_<label>_ninja.log);
  2. Halo progress numbers vs the frozen base;
  3. stable_verdicts snapshot + diff vs scratch/campaign/base_stable.json (must have ZERO losses);
  4. admission audit and fake_match_scan counts vs base;
  5. compiler-warning counts vs base (new warning lines listed);
  6. pytest (tools/, scratchpad basetemp);
  7. git diff --check.
Writes scratch/campaign/batch_<label>.json and prints a compact verdict. Grants no credit by itself.
"""
import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
BASETEMP = ('C:/Users/isabe/AppData/Local/Temp/claude/C--Users-isabe-Documents-Codex-2026-07-13-i-w-work-halo-exact/'
            '5dc46298-4671-4883-a3be-0caccffac3a1/scratchpad/pytest-batch')
# Measurement base: published canonical a8854940 (owner, 2026-09-26). Its runtime sections equal lane batch 3
# (keyed-proven), so base_stable_a8854940.json = batch_batch3_Q9_stable.json. Frozen cc608036 figures kept for history.
BASE_CC608036 = dict(objects=(388, 468), code=(1584418, 1770166), functions=(7454, 7574), data=2587011, parks=77)
BASE_A8854940 = dict(objects=(388, 468), code=(1588626, 1770166), functions=(7457, 7574), data=2587011, parks=77)
# Measurement base since 2026-09-26: canonical d890c2db (= a8854940 + main_update_time + vehicle_collision). Its exact
# set equals lane snapshot batch_L4_prototypes (L1-L4 are zero-credit), so base_stable_d890c2db.json = that snapshot.
BASE = dict(objects=(388, 468), code=(1591226, 1770166), functions=(7459, 7574), data=2587011, parks=76)


def run(cmd, log=None, **kw):
    r = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, **kw)
    if log:
        Path(log).write_text(r.stdout + r.stderr, encoding='utf-8')
    return r


def progress(text):
    m = re.search(r'halobetacache: [\d.]+% matched, [\d.]+% linked \((\d+) / (\d+) files\)\s+Code: (\d+) / (\d+) bytes '
                  r'\((\d+) / (\d+) functions\)\s+Data: (\d+) / (\d+) bytes', text)
    parks = re.search(r'Validated parked compiler ties: (\d+)', text)
    if not m:
        return None
    g = list(map(int, m.groups()))
    return dict(objects=(g[0], g[1]), code=(g[2], g[3]), functions=(g[4], g[5]), data=g[6],
                parks=int(parks.group(1)) if parks else None)


def warnings(text):
    # multiset of (file, code, message) with LINE NUMBERS stripped, so a pure line shift is not a new warning
    from collections import Counter
    return Counter(re.sub(r'\(\d+\)', '(N)', l.strip()) for l in text.splitlines() if re.search(r'warning C\d{4}', l))


def main():
    label = sys.argv[1]
    out = {}
    nlog = ROOT / ('scratch/campaign/batch_%s_ninja.log' % label)
    r = run(['ninja'], log=nlog)
    text = nlog.read_text(encoding='utf-8')
    out['ninja_rc'] = r.returncode
    out['progress'] = progress(text)
    base_warn = warnings((ROOT / 'scratch/ninja_base.log').read_text(encoding='utf-8'))
    now_warn = warnings(text)
    out['new_warnings'] = sorted((now_warn - base_warn).elements())
    snap = ROOT / ('scratch/campaign/batch_%s_stable.json' % label)
    run([sys.executable, '-B', '-m', 'tools.campaign.stable_verdicts', 'snapshot', str(snap)])
    d = run([sys.executable, '-B', '-m', 'tools.campaign.stable_verdicts', 'diff',
             str(ROOT / 'scratch/campaign/base_stable_d890c2db.json'), str(snap)])
    out['stable_diff_rc'] = d.returncode
    out['stable_diff'] = d.stdout[-4000:]
    a = run([sys.executable, '-B', 'tools/audit_object_admission.py',
             '--output', 'scratch/campaign/batch_%s_admission.json' % label])
    adm = json.loads((ROOT / ('scratch/campaign/batch_%s_admission.json' % label)).read_text())
    out['admission'] = {k: (len(v) if isinstance(v, (list, dict)) else v) for k, v in adm.items()}
    f = run([sys.executable, '-B', 'tools/fake_match_scan.py'])
    out['fake_scan_tail'] = f.stdout.strip().splitlines()[-1:] if f.stdout else []
    p = run([sys.executable, '-m', 'pytest', 'tools/', '-q', '-p', 'no:cacheprovider', '--basetemp=' + BASETEMP])
    out['pytest'] = (p.stdout.strip().splitlines() or [''])[-1]
    c = run(['git', 'diff', '--check'])
    out['diff_check_rc'] = c.returncode
    out['diff_check'] = c.stdout[-2000:]
    (ROOT / ('scratch/campaign/batch_%s.json' % label)).write_text(json.dumps(out, indent=1))
    pr = out['progress'] or {}
    print('ninja rc', out['ninja_rc'])
    if pr:
        print('objects %s (base %s)  code %s (base %s, +%d)  functions %s (base %s, +%d)  data %s  parks %s' % (
            pr['objects'], BASE['objects'], pr['code'], BASE['code'], pr['code'][0] - BASE['code'][0],
            pr['functions'], BASE['functions'], pr['functions'][0] - BASE['functions'][0], pr['data'], pr['parks']))
    print('stable diff rc', out['stable_diff_rc'], '|', out['stable_diff'].strip().splitlines()[-3:])
    print('admission', out['admission'], '| fake scan', out['fake_scan_tail'], '| pytest', out['pytest'])
    print('new warnings', len(out['new_warnings']), out['new_warnings'][:5], '| diff --check rc', out['diff_check_rc'])


if __name__ == '__main__':
    main()
