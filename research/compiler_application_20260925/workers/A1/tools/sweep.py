"""Consumer sweep from an ALTERNATE source root (read-only w.r.t. the worktree).

Compiles each unit's .c/.cpp from <root> with the unit's production build.ninja cflags (the /Isource/* paths are
re-rooted into <root>, exactly as W1's revgate.py does; cwd = <root>), then compares EVERY section-defining symbol
of the new object against build/base/<unit>.obj with tools/coff_compare.section_infos_equal (the keyed_diff.py
judgement). Changed sections are also classified against January (build/split/<unit>.obj): EXACT / residual /
absent. A unit is SAME when no section changed, appeared or vanished.

    python sweep.py <root> <outdir> (--all | --units-file FILE | UNIT ...) [--jobs N] [--json OUT]
"""
import argparse
import concurrent.futures as cf
import json
import os
import re
import subprocess
import sys

WT = r'C:\halo-worktrees\claude-compiler-application-20260925'
sys.path.insert(0, WT)
sys.path.insert(0, os.path.join(WT, 'research', 'compiler_application_20260925', 'workers', 'W1', 'tools'))
from tools import coff_compare as cc  # noqa: E402
from flags import unit_flags  # noqa: E402

CL = os.path.join(WT, 'xbox', 'bin', 'vc7', 'CL.Exe')


def all_units():
    bn = open(os.path.join(WT, 'build.ninja')).read().replace('$\n', ' ')
    units = []
    for m in re.finditer(r'^build build\\base\\(source\\.+?)\.obj: cl\s+(\S.*?)$', bn, flags=re.M):
        unit = m.group(1).replace('$ ', ' ').replace('\\', '/')
        src = m.group(2).strip().replace('$ ', ' ')
        units.append((unit, os.path.splitext(src)[1]))
    return units


def sections(path):
    obj = cc.load(open(path, 'rb').read())
    out = {}
    for s in obj['symbols']:
        n = s.get('name')
        if not n or n.startswith('.') or n.startswith('$') or s.get('section', 0) <= 0 or s.get('value', 0):
            continue
        if n in out:
            continue
        try:
            out[n] = cc.section_info(obj, n)
        except Exception:
            continue
    return out


def compile_unit(root, outdir, unit, ext):
    """Production-faithful: cwd = <root> (which must hold an `xbox` junction to the worktree's xbox), the unit's
    cflags verbatim (relative /I paths), the source passed as the relative backslash path ninja uses."""
    flags = unit_flags(unit, os.path.join(WT, 'build.ninja'))
    src = (unit + ext).replace('/', chr(92))
    obj = os.path.join(os.path.abspath(outdir), unit.replace('/', '__') + '.obj')
    if os.path.exists(obj):
        os.remove(obj)
    cmd = [CL, '/nologo', '/c'] + flags + ['/Fo' + obj, src]
    # private TMP per unit: parallel CL runs otherwise collide on %TEMP% intermediates (C1083 / C1001 seen)
    tmp = os.path.join(os.path.abspath(outdir), '_tmp', unit.replace('/', '__'))
    os.makedirs(tmp, exist_ok=True)
    env = dict(os.environ, TMP=tmp, TEMP=tmp)
    for attempt in range(3):
        r = subprocess.run(cmd, capture_output=True, text=True, cwd=root, encoding='latin-1', env=env)
        if not r.returncode and os.path.exists(obj):
            return unit, obj, None
    return unit, None, (r.stdout[-1500:] + r.stderr[-500:])


def compare(unit, obj, against=None):
    base = (os.path.join(against, unit.replace('/', '__') + '.obj') if against
            else os.path.join(WT, 'build', 'base', unit + '.obj'))
    A, B = sections(base), sections(obj)
    jan_path = os.path.join(WT, 'build', 'split', unit + '.obj')
    J = sections(jan_path) if os.path.exists(jan_path) else {}
    changed = sorted(n for n in A.keys() & B.keys() if not cc.section_infos_equal(A[n], B[n]))
    added, removed = sorted(B.keys() - A.keys()), sorted(A.keys() - B.keys())

    def jan(n, info):
        if n not in J:
            return 'absent-in-January'
        return 'EXACT' if cc.section_infos_equal(J[n], info) else 'residual'

    return {
        'unit': unit,
        'sections': len(B),
        'changed': [(n, jan(n, A[n]), jan(n, B[n])) for n in changed],
        'added': [(n, jan(n, B[n])) for n in added],
        'removed': [(n, jan(n, A[n])) for n in removed],
    }


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('root')
    ap.add_argument('outdir')
    ap.add_argument('units', nargs='*')
    ap.add_argument('--all', action='store_true')
    ap.add_argument('--units-file')
    ap.add_argument('--jobs', type=int, default=8)
    ap.add_argument('--json')
    ap.add_argument('--against', help='compare with the objects of another sweep outdir instead of build/base')
    a = ap.parse_args()
    root = os.path.abspath(a.root)
    assert os.path.isdir(os.path.join(root, 'xbox', 'include')), 'root needs an xbox junction'
    os.makedirs(a.outdir, exist_ok=True)
    known = dict(all_units())
    if a.all:
        units = sorted(known)
    else:
        units = list(a.units)
        if a.units_file:
            units += [u.strip() for u in open(a.units_file) if u.strip() and not u.startswith('#')]
    todo = []
    for u in units:
        u = u.replace('\\', '/')
        if u.endswith('.c') or u.endswith('.cpp'):
            u = os.path.splitext(u)[0]
        if u not in known:
            print('NOT-A-BUILD-UNIT', u)
            continue
        todo.append((u, known[u]))
    results = []
    with cf.ThreadPoolExecutor(max_workers=a.jobs) as ex:
        futs = [ex.submit(compile_unit, root, a.outdir, u, e) for u, e in todo]
        for f in cf.as_completed(futs):
            unit, obj, err = f.result()
            if obj is None:
                results.append({'unit': unit, 'error': err})
                continue
            results.append(compare(unit, obj, a.against))
    results.sort(key=lambda r: r['unit'])
    same = diff = err = 0
    for r in results:
        if 'error' in r:
            err += 1
            print('COMPILE-FAILED %s\n%s' % (r['unit'], r['error']))
            continue
        if r['changed'] or r['added'] or r['removed']:
            diff += 1
            print('DIFF  %-60s changed %d added %d removed %d' % (r['unit'], len(r['changed']), len(r['added']),
                                                                  len(r['removed'])))
            for n, jb, ja in r['changed']:
                print('      CHANGED %-50s base:%s -> new:%s' % (n, jb, ja))
            for n, j in r['added']:
                print('      ADDED   %-50s %s' % (n, j))
            for n, j in r['removed']:
                print('      REMOVED %-50s %s' % (n, j))
        else:
            same += 1
            print('SAME  %-60s %d sections' % (r['unit'], r['sections']))
    print('== %d units: %d SAME, %d DIFF, %d compile failures' % (len(results), same, diff, err))
    if a.json:
        json.dump(results, open(a.json, 'w'), indent=1)


if __name__ == '__main__':
    main()
