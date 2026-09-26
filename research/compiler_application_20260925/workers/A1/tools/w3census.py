"""/W3 /Zs warning census, HEAD root vs candidate root, over every build unit (or a list), with production
cflags (cwd = root, relative /I paths, ninja's relative source path; /W* flags stripped then /W3 added, as W2's w3.py).
Warnings are compared as multisets of (file basename, code, message) with line numbers dropped; TUs whose
multisets differ are printed with the added/removed warnings.

    python w3census.py <head_root> <cand_root> [--all | UNIT ...] [--out FILE]
"""
import argparse
import collections
import concurrent.futures as cf
import os
import re
import subprocess
import sys

WT = r'C:\halo-worktrees\claude-compiler-application-20260925'
sys.path.insert(0, os.path.join(WT, 'research', 'compiler_application_20260925', 'workers', 'W1', 'tools'))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from flags import unit_flags  # noqa: E402
from sweep import all_units, CL  # noqa: E402

WARN = re.compile(r'^(.*?)\((\d+)\) : (warning|error) (C\d+): (.*)$')


def census(root, unit, ext):
    flags = [t for t in unit_flags(unit, os.path.join(WT, 'build.ninja')) if not re.match(r'^/W[0-4]$', t)]
    src = (unit + ext).replace('/', chr(92))
    tmp = os.path.join(WT, 'scratch', 'campaign', 'workers', 'A1', '_tmp_w3', os.path.basename(root), unit.replace('/', '__'))
    os.makedirs(tmp, exist_ok=True)
    env = dict(os.environ, TMP=tmp, TEMP=tmp)
    r = subprocess.run([CL] + flags + ['/W3', '/Zs', src], capture_output=True, text=True, cwd=root,
                       encoding='latin-1', env=env)
    c = collections.Counter()
    for line in (r.stdout + r.stderr).splitlines():
        m = WARN.match(line.strip())
        if m:
            c[(os.path.basename(m.group(1)).lower(), m.group(4), m.group(5).strip())] += 1
    return c


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('head_root')
    ap.add_argument('cand_root')
    ap.add_argument('units', nargs='*')
    ap.add_argument('--all', action='store_true')
    ap.add_argument('--out')
    a = ap.parse_args()
    known = dict(all_units())
    units = sorted(known) if a.all else [u for u in a.units]
    lines = []
    ndiff = 0
    total_h = total_c = 0
    with cf.ThreadPoolExecutor(max_workers=8) as ex:
        fh = {u: ex.submit(census, os.path.abspath(a.head_root), u, known[u]) for u in units}
        fc = {u: ex.submit(census, os.path.abspath(a.cand_root), u, known[u]) for u in units}
        for u in units:
            h, c = fh[u].result(), fc[u].result()
            total_h += sum(h.values())
            total_c += sum(c.values())
            if h != c:
                ndiff += 1
                lines.append('DIFF %s' % u)
                for k, v in sorted((c - h).items()):
                    lines.append('   + %dx %s %s: %s' % (v, k[0], k[1], k[2]))
                for k, v in sorted((h - c).items()):
                    lines.append('   - %dx %s %s: %s' % (v, k[0], k[1], k[2]))
    lines.append('== %d units: %d with a different warning multiset; total warnings head %d, candidate %d'
                 % (len(units), ndiff, total_h, total_c))
    text = '\n'.join(lines)
    print(text)
    if a.out:
        open(a.out, 'w').write(text + '\n')


if __name__ == '__main__':
    main()
