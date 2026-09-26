"""A2 admission battery for one final candidate (read-only; writes only the report file).

    python -B battery.py <label> <unit> <candidate.obj> <candidate.c> <split_root> [--provider-root DIR]
                         [--shadow DIR] [--drop /F1,/F2]

Runs, in order, and captures every output verbatim:
  1. strict rows of every January function (rowstat.py = gate.py row logic) vs <split_root>
  2. object_audit (object_audit_split.py, SPLIT_ROOT=<split_root>)
  3. pdb_storage (pdb_storage_cand.py vs <split_root>)
  4. surplus identity (surplus_cand.py)
  5. selected-provider link, both orders (provider_link_v.py; PROVIDER_ROOT defaults to build/base)
  6. protoscan (scratch/tools/protoscan.py) on <candidate.c>
  7. /W3 /Zs census of <candidate.c> with the unit's flags (minus --drop), compiled inside --shadow if given
     (so a patched header tree is used) else inside the worktree
  8. tools/fake_match_scan.py on <candidate.c>
Report -> research/compiler_application_20260925/workers/A2/battery/<label>.txt
"""
import collections
import os
import re
import shutil
import subprocess
import sys

ROOT = r'C:\halo-worktrees\claude-compiler-application-20260925'
A2 = os.path.join(ROOT, 'research', 'compiler_application_20260925', 'workers', 'A2')
T = os.path.join(A2, 'tools')
PY = sys.executable


def run(cmd, env=None, cwd=ROOT):
    e = dict(os.environ)
    if env:
        e.update(env)
    r = subprocess.run(cmd, cwd=cwd, env=e, capture_output=True, text=True, encoding='latin-1')
    return (r.stdout + r.stderr).rstrip() + '\n[exit %d]\n' % r.returncode


def main():
    args = [a for a in sys.argv[1:]]
    label, unit, obj, csrc, split_root = args[:5]
    opts = args[5:]
    prov = shadow = None
    drop = []
    for i, a in enumerate(opts):
        if a == '--provider-root':
            prov = opts[i + 1]
        if a == '--shadow':
            shadow = opts[i + 1]
        if a == '--drop':
            drop = opts[i + 1].split(',')
    out = []
    out.append('# battery %s  unit %s\n# candidate obj %s\n# candidate src %s\n# split root %s\n# provider root %s\n# shadow %s  drop %s\n'
               % (label, unit, obj, csrc, split_root, prov or 'build/base', shadow or '-', drop))
    out.append('## 1 strict rows (gate.py row logic) vs split root\n')
    # rowstat reads build/split; emulate split root by temporary argument: use lab object_audit for sections, and
    # a direct row check here
    sys.path.insert(0, ROOT)
    from tools import coff_compare as cc
    t = cc.load(open(os.path.join(split_root, unit + '.obj'), 'rb').read())
    o = cc.load(open(obj, 'rb').read())

    def fn_syms(x):
        secs = x['sections']
        d = {}
        for s in x['symbols']:
            if (s['type'] == 0x20 and s['section'] > 0 and s['storage'] in (2, 3)
                    and s['value'] == 0 and secs[s['section'] - 1]['name'] == '.text'):
                d.setdefault(s['name'], s)
        return d
    ts, os_ = fn_syms(t), fn_syms(o)
    ex = 0
    for n in sorted(ts):
        if n not in os_:
            st = 'UNWRITTEN'
        else:
            st = 'EXACT' if cc.section_infos_equal(cc.section_info(t, n), cc.section_info(o, n)) else 'residual'
        ex += st == 'EXACT'
        out.append('%-9s %5d  %s\n' % (st, cc.section_info(t, n)['size'], n))
    out.append('== exact %d of %d\n' % (ex, len(ts)))
    out.append('\n## 2 object_audit\n')
    out.append(run([PY, '-B', os.path.join(T, 'object_audit_split.py'), unit, obj], env={'SPLIT_ROOT': split_root}))
    out.append('\n## 3 pdb_storage\n')
    out.append(run([PY, '-B', os.path.join(T, 'pdb_storage_cand.py'), unit, obj, split_root]))
    out.append('\n## 4 surplus identity\n')
    out.append(run([PY, '-B', os.path.join(T, 'surplus_cand.py'), unit, obj]))
    out.append('\n## 5 selected-provider link (both orders)\n')
    out.append(run([PY, '-B', os.path.join(T, 'provider_link_v.py'), unit, obj],
                   env={'PROVIDER_ROOT': prov} if prov else None))
    out.append('\n## 6 protoscan\n')
    out.append(run([PY, '-B', os.path.join(ROOT, 'scratch', 'tools', 'protoscan.py'), csrc]))
    out.append('\n## 7 /W3 /Zs census\n')
    sys.path.insert(0, T)
    import sweep
    for u, src, toks in sweep.units():
        if u == unit:
            toks = [x for x in toks if x not in drop and x != '/c']
            cwd = shadow or ROOT
            dst = os.path.join(cwd, src)
            tmp = None
            if shadow:
                tmp = dst + '.battery_orig'
                shutil.copyfile(dst, tmp)
                shutil.copyfile(csrc, dst)
                target = src
            else:
                target = os.path.join(A2, '_w3_' + os.path.basename(csrc))
                shutil.copyfile(csrc, target)
                toks = toks + ['/I' + os.path.dirname(os.path.join(ROOT, unit))]
            try:
                r = subprocess.run([sweep.CL, '/nologo', '/c', '/W3', '/Zs'] + toks + [target], cwd=cwd,
                                   capture_output=True, text=True, encoding='latin-1')
            finally:
                if tmp:
                    shutil.copyfile(tmp, dst)
                    os.remove(tmp)
                else:
                    os.remove(target)
            text = r.stdout + r.stderr
            c = collections.Counter(re.findall(r'warning (C\d+)', text))
            out.append('rc %d %s\n' % (r.returncode, dict(sorted(c.items()))))
            for l in text.splitlines():
                if 'warning' in l and ('C4013' in l or unit.split('/')[-1] in l.replace('\\', '/') or '_w3_' in l):
                    out.append('   ' + l.strip()[-170:] + '\n')
    out.append('\n## 8 fake_match_scan\n')
    out.append(run([PY, '-B', os.path.join(ROOT, 'tools', 'fake_match_scan.py'), csrc]))
    os.makedirs(os.path.join(A2, 'battery'), exist_ok=True)
    rep = os.path.join(A2, 'battery', label + '.txt')
    open(rep, 'w', encoding='utf-8', newline='\n').write(''.join(out))
    print(''.join(out))


if __name__ == '__main__':
    main()
