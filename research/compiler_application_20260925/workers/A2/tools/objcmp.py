"""A2 lab tool (read-only): compare two COFF objects the way the admission battery cares about.

    python -B objcmp.py <a.obj> <b.obj>

1. every named section (first defining symbol at value 0, as keyed_diff.py) compared with
   coff_compare.section_infos_equal: CHANGED / ADDED / REMOVED;
2. every symbol with storage 2/3 and section > 0 (non-'.'/'$'): storage class and value;
3. every UNDEF / COMMON external (section 0): value (COMMON size) differences, additions, removals.
Prints IDENTICAL when nothing differs (.debug$* sections are ignored).
"""
import sys
from pathlib import Path

ROOT = Path(r'C:\halo-worktrees\claude-compiler-application-20260925')
sys.path.insert(0, str(ROOT))
from tools import coff_compare as cc  # noqa: E402


def load(p):
    return cc.load(Path(p).read_bytes())


def named_sections(o):
    out = {}
    for s in o['symbols']:
        n = s.get('name')
        if not n or n.startswith('.') or n.startswith('$') or s.get('section', 0) <= 0 or s.get('value', 0):
            continue
        sec = o['sections'][s['section'] - 1]
        if sec['name'].startswith('.debug'):
            continue
        if n in out:
            continue
        try:
            out[n] = cc.section_info(o, n)
        except Exception:
            continue
    return out


def defined(o):
    out = {}
    for s in o['symbols']:
        n = s['name']
        if n.startswith('.') or n.startswith('$') or s['section'] <= 0 or s['storage'] not in (2, 3):
            continue
        sec = o['sections'][s['section'] - 1]
        if sec['name'].startswith('.debug'):
            continue
        out[n] = (s['storage'], sec['name'], s['value'])
    return out


def undef(o):
    out = {}
    for s in o['symbols']:
        if s['section'] == 0 and s['storage'] == 2:
            out[s['name']] = s['value']
    return out


def main():
    A, B = load(sys.argv[1]), load(sys.argv[2])
    diffs = 0
    SA, SB = named_sections(A), named_sections(B)
    for n in sorted(SA.keys() & SB.keys()):
        if not cc.section_infos_equal(SA[n], SB[n]):
            print('CHANGED  %-50s %s -> %s' % (n, SA[n].get('size'), SB[n].get('size')))
            diffs += 1
    for n in sorted(SB.keys() - SA.keys()):
        print('ADDED    %-50s %s' % (n, SB[n].get('size')))
        diffs += 1
    for n in sorted(SA.keys() - SB.keys()):
        print('REMOVED  %-50s %s' % (n, SA[n].get('size')))
        diffs += 1
    DA, DB = defined(A), defined(B)
    for n in sorted(DA.keys() | DB.keys()):
        if DA.get(n) != DB.get(n):
            print('SYMBOL   %-50s %s -> %s' % (n, DA.get(n), DB.get(n)))
            diffs += 1
    UA, UB = undef(A), undef(B)
    for n in sorted(UA.keys() | UB.keys()):
        if UA.get(n) != UB.get(n):
            print('EXTERN   %-50s %s -> %s' % (n, UA.get(n, '-'), UB.get(n, '-')))
            diffs += 1
    print('IDENTICAL' if not diffs else 'DIFFERENT (%d)' % diffs)
    return 1 if diffs else 0


if __name__ == '__main__':
    sys.exit(main())
