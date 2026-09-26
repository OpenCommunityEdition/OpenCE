"""A2 shadow-tree whole-board compile sweep (LAB ONLY; never touches tracked files).

Modelled on the fifty-objects breakable_surfaces_audit2/sweep.py (read before writing this one).

  python -B sweep.py setup <variant>                 copy the worktree's source/ -> SH/<variant>/source
  python -B sweep.py compile <variant> [--jobs N] [--only SUBSTR] [--drop UNIT=/F1,/F2]
        compile every build\\base\\source\\*.obj unit exactly as build.ninja does (same CL, same flag
        tokens, same relative source path), with cwd = SH/<variant> so __FILE__ and relative /I paths
        give production's text; objects -> OUT/<variant>/<unit>.obj
  python -B sweep.py vsbase <variant>                compare every object with build/base
  python -B sweep.py compare <variantA> <variantB>   compare two variants
Comparison = A2 objcmp logic: named non-debug sections by coff_compare.section_infos_equal, defined-symbol
storage/section/value, UNDEF/COMMON externals.
"""
import os
import re
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = r'C:\halo-worktrees\claude-compiler-application-20260925'
HERE = os.path.join(ROOT, 'scratch', 'campaign', 'workers', 'A2')
SH = os.path.join(HERE, 'sh')
OUT = os.path.join(HERE, 'sweep')
CL = os.path.join(ROOT, 'xbox', 'bin', 'vc7', 'CL.Exe')
sys.path.insert(0, os.path.join(ROOT, 'research', 'compiler_application_20260925', 'workers', 'A2', 'tools'))
import objcmp  # noqa: E402


def units():
    bn = open(os.path.join(ROOT, 'build.ninja'), encoding='latin-1').read().replace('\r\n', '\n')
    res = []
    for m in re.finditer(r'^build build\\base\\(source\\.*?)\.obj: cl (?:\$\n\s+)?(.*?)\n  cflags = ', bn, re.M):
        unit = m.group(1).replace('$ ', ' ').replace('\\', '/')
        src = m.group(2).strip().replace('$ ', ' ')
        j = m.end()
        k = bn.index('\nbuild ', j)
        cf = bn[j:k].replace('$\n', ' ')
        cf = re.sub(r'\s+', ' ', cf).strip()
        toks = re.findall(r'/I"[^"]+"|\S+', cf)
        toks = [('/I' + t[3:].rstrip('"')) if t.startswith('/I"') else t for t in toks]
        toks = [('/I' + os.path.join(ROOT, 'xbox', 'include')) if t == '/Ixbox/include' else t for t in toks]
        res.append((unit, src, toks))
    return res


def setup(variant):
    dst = os.path.join(SH, variant, 'source')
    if os.path.exists(dst):
        shutil.rmtree(dst)
    shutil.copytree(os.path.join(ROOT, 'source'), dst)
    print('copied ->', dst)


def compile_one(args):
    unit, src, toks, variant, drop = args
    if unit in drop:
        toks = [t for t in toks if t not in drop[unit]]
    out = os.path.join(OUT, variant, unit + '.obj')
    os.makedirs(os.path.dirname(out), exist_ok=True)
    if os.path.exists(out):
        os.remove(out)
    cmd = [CL, '/nologo', '/c'] + toks + ['/Fo' + out, src]
    r = subprocess.run(cmd, cwd=os.path.join(SH, variant), capture_output=True, text=True, encoding='latin-1')
    ok = r.returncode == 0 and os.path.exists(out)
    return unit, ok, (r.stdout + r.stderr)[-800:]


def compile_all(variant, jobs, only, drop):
    us = units()
    if only:
        us = [u for u in us if only in u[0]]
    fails = []
    with ThreadPoolExecutor(jobs) as ex:
        for unit, ok, msg in ex.map(compile_one, [(u, s, t, variant, drop) for (u, s, t) in us]):
            if not ok:
                fails.append((unit, msg))
    print('compiled', len(us), 'failed', len(fails))
    for u, m in fails:
        print('FAIL', u, m)


def cmp_objs(pa, pb):
    import io
    import contextlib
    buf = io.StringIO()
    old = sys.argv
    sys.argv = ['objcmp', pa, pb]
    try:
        with contextlib.redirect_stdout(buf):
            rc = objcmp.main()
    finally:
        sys.argv = old
    lines = [l for l in buf.getvalue().splitlines() if not l.startswith('IDENTICAL') and not l.startswith('DIFFERENT')]
    return rc, lines


def compare(fa, fb):
    same = 0
    diffs = []
    missing = []
    for unit, _, _ in units():
        pa, pb = fa(unit), fb(unit)
        if not (os.path.exists(pa) and os.path.exists(pb)):
            missing.append(unit)
            continue
        rc, lines = cmp_objs(pa, pb)
        if rc == 0:
            same += 1
        else:
            diffs.append((unit, lines))
    print('identical', same, 'different', len(diffs), 'missing', len(missing))
    for u, d in diffs:
        print('DIFF', u)
        for l in d[:40]:
            print('     ', l)
    for u in missing:
        print('MISSING', u)


if __name__ == '__main__':
    cmd = sys.argv[1]
    if cmd == 'setup':
        setup(sys.argv[2])
    elif cmd == 'compile':
        variant = sys.argv[2]
        jobs, only, drop = 6, None, {}
        if '--jobs' in sys.argv:
            jobs = int(sys.argv[sys.argv.index('--jobs') + 1])
        if '--only' in sys.argv:
            only = sys.argv[sys.argv.index('--only') + 1]
        for i, a in enumerate(sys.argv):
            if a == '--drop':
                u, f = sys.argv[i + 1].split('=', 1)
                drop.setdefault(u, set()).update(f.split(','))
        compile_all(variant, jobs, only, drop)
    elif cmd == 'vsbase':
        va = sys.argv[2]
        compare(lambda u: os.path.join(ROOT, 'build', 'base', u + '.obj'), lambda u: os.path.join(OUT, va, u + '.obj'))
    elif cmd == 'compare':
        va, vb = sys.argv[2], sys.argv[3]
        compare(lambda u: os.path.join(OUT, va, u + '.obj'), lambda u: os.path.join(OUT, vb, u + '.obj'))
    elif cmd == 'count':
        print(len(units()))
