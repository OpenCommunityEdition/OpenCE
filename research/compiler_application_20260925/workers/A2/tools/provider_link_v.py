#!/usr/bin/env python3
"""A2 copy (only changes: ROOT pinned; PROVIDER_ROOT env replaces build/base for provider objects). Selected-provider duplicate-definition check for surplus symbols.

    python scratch/tools/provider_link.py <unit> [candidate.obj] [--symbols=_a,_b] [--baseline=base.obj]

--baseline restricts the check to symbols the candidate defines that the
baseline object does not (i.e. NEW surplus introduced by the candidate).
A symbol FAILS only if an LNK2005 names that symbol.

<unit> is e.g. source/units/vehicles. The candidate object defaults to
build/base/<unit>.obj (so build the candidate first, or pass gate.py --out).

For every EXTERNAL symbol the candidate DEFINES that January's split object for
the same unit does not define (surplus COMDAT helpers, pooled literals, SDK
tables), find January's selected provider (the split object that defines it)
and link the candidate with OUR base build of that provider using VC7 Link.Exe,
in both input orders. PASS means neither order reports LNK2005 or LNK1169.
Unresolved externals (LNK2001/LNK2019/LNK1120) are expected and ignored.

This is bounded duplicate/coalescing evidence between current objects, not a
successful whole-program link.
"""
import glob
import os
import re
import subprocess
import sys
import tempfile

ROOT = 'C:/halo-worktrees/claude-compiler-application-20260925'
sys.path.insert(0, os.path.join(ROOT, 'tools'))
import coff_compare as cc  # noqa: E402

LINK = os.path.join(ROOT, 'xbox', 'bin', 'vc7', 'Link.Exe')
SEP = chr(92)


def defined(path):
    o = cc.load(open(path, 'rb').read())
    out = {}
    for s in o['symbols']:
        if s['section'] > 0 and s['storage'] == 2:
            out[s['name']] = s
    return out


_split_index = None


def split_definers(name):
    global _split_index
    if _split_index is None:
        _split_index = {}
        for p in glob.glob(os.path.join(ROOT, 'build', 'split', '**', '*.obj'), recursive=True):
            try:
                for n in defined(p):
                    _split_index.setdefault(n, []).append(p)
            except Exception:
                continue
    return _split_index.get(name, [])


def link(objs):
    with tempfile.TemporaryDirectory() as td:
        out = os.path.join(td, 'probe.exe')
        cmd = [LINK, '/NOLOGO', '/MACHINE:X86', '/SUBSYSTEM:CONSOLE', '/NODEFAULTLIB',
               '/ENTRY:probe_entry', '/OUT:' + out] + objs
        r = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT)
    text = r.stdout + r.stderr
    dup = [l.strip() for l in text.splitlines() if 'LNK2005' in l or 'LNK1169' in l]
    return dup, text


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    unit = args[0].replace(SEP, '/')
    cand = args[1] if len(args) > 1 else os.path.join(ROOT, 'build', 'base', unit + '.obj')
    only = None
    for a in sys.argv[1:]:
        if a.startswith('--symbols'):
            only = a.split('=', 1)[1].split(',')
    jan = os.path.join(ROOT, 'build', 'split', unit + '.obj')
    cdef = defined(cand)
    jdef = defined(jan) if os.path.exists(jan) else {}
    surplus = sorted(n for n in cdef if n not in jdef and not n.startswith('$'))
    for a in sys.argv[1:]:
        if a.startswith('--baseline='):
            bdef = defined(a.split('=', 1)[1])
            surplus = [n for n in surplus if n not in bdef]
    if only:
        surplus = [n for n in surplus if n in only] + [n for n in only if n not in surplus]
    if not surplus:
        print('no surplus external definitions: PASS (nothing to link)')
        return 0
    bad = 0
    for name in surplus:
        provs = split_definers(name)
        if not provs:
            print('%-44s NO JANUARY PROVIDER (surplus with no selected copy)  FAIL' % name)
            bad += 1
            continue
        for sp in provs:
            rel = os.path.relpath(sp, os.path.join(ROOT, 'build', 'split')).replace(SEP, '/')
            ours = os.path.join(os.environ.get('PROVIDER_ROOT', os.path.join(ROOT, 'build', 'base')), rel)
            if os.path.normcase(os.path.abspath(ours)) == os.path.normcase(os.path.abspath(cand)):
                print('%-44s provider is this unit itself' % name)
                continue
            if not os.path.exists(ours):
                print('%-44s provider %s has no base object  FAIL' % (name, rel))
                bad += 1
                continue
            d1, _ = link([cand, ours])
            d2, _ = link([ours, cand])
            key = ' %s already defined' % name
            d1 = [l for l in d1 if key in l]
            d2 = [l for l in d2 if key in l]
            verdict = 'PASS' if not d1 and not d2 else 'FAIL'
            if verdict == 'FAIL':
                bad += 1
            print('%-44s provider %-48s %s' % (name, rel[:-4], verdict))
            for l in (d1 + d2)[:4]:
                print('      ' + l[:200])
    print('SELECTED-PROVIDER LINK: %s' % ('PASS' if not bad else 'FAIL (%d)' % bad))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
