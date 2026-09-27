"""Transitive consumer census from an alternate root: CL /Zs /showIncludes for every build unit (production cflags,
cwd = <root>), then list the units that include each requested header (path suffix match, case-insensitive).

    python consumers.py <root> <header-suffix>... [--jobs N] [--json OUT]
e.g.  python consumers.py roots/vTU game/game_engine.h interface/hud.h
"""
import argparse
import concurrent.futures as cf
import json
import os
import subprocess
import sys

WT = r'C:\halo-worktrees\claude-compiler-application-20260925'
sys.path.insert(0, os.path.join(WT, 'research', 'compiler_application_20260925', 'workers', 'W1', 'tools'))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from flags import unit_flags  # noqa: E402
from sweep import all_units, CL  # noqa: E402


def includes(root, unit, ext, tmpbase):
    flags = [t for t in unit_flags(unit, os.path.join(WT, 'build.ninja')) if t != '/c']
    src = (unit + ext).replace('/', chr(92))
    tmp = os.path.join(tmpbase, unit.replace('/', '__'))
    os.makedirs(tmp, exist_ok=True)
    env = dict(os.environ, TMP=tmp, TEMP=tmp)
    r = subprocess.run([CL, '/nologo', '/Zs', '/showIncludes'] + flags + [src], capture_output=True, text=True,
                       cwd=root, encoding='latin-1', env=env)
    inc = set()
    for line in (r.stdout + r.stderr).splitlines():
        if 'Note: including file:' in line:
            inc.add(os.path.normcase(os.path.normpath(line.split('Note: including file:', 1)[1].strip())))
    return unit, inc


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('root')
    ap.add_argument('headers', nargs='+')
    ap.add_argument('--jobs', type=int, default=8)
    ap.add_argument('--json')
    a = ap.parse_args()
    root = os.path.abspath(a.root)
    tmpbase = os.path.join(WT, 'scratch', 'campaign', 'workers', 'A1', '_tmp_consumers')
    units = all_units()
    res = {}
    with cf.ThreadPoolExecutor(max_workers=a.jobs) as ex:
        for unit, inc in ex.map(lambda ue: includes(root, ue[0], ue[1], tmpbase), units):
            res[unit] = inc
    out = {}
    for h in a.headers:
        suf = os.path.normcase(os.path.normpath(h))
        users = sorted(u for u, inc in res.items() if any(p.endswith(os.sep + suf) or p == suf for p in inc))
        out[h] = users
        print('== %s: %d consumer TUs' % (h, len(users)))
        for u in users:
            print('   ', u)
    if a.json:
        json.dump(out, open(a.json, 'w'), indent=1)


if __name__ == '__main__':
    main()
