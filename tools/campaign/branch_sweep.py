"""For each named unit, compile every branch's version of its .c file from
scratch and report any version that gates strictly better than the current one.

Four objects were advanced this session by finding work already done in another
lane; this automates the search.

usage: python tools/campaign/branch_sweep.py source/memory/data_encoding [more units...]
       python tools/campaign/branch_sweep.py --near     (all units 1-3 fns from complete)
"""
import sys, subprocess, os, json, re

GATE = ['python', 'tools/campaign/gate.py']


def unique_blobs(path):
    brs = subprocess.run(['git', 'for-each-ref', '--format=%(refname:short)', 'refs/heads'],
                         capture_output=True, text=True).stdout.split()
    blobs = {}
    for b in brs:
        r = subprocess.run(['git', 'rev-parse', f'{b}:{path}'], capture_output=True, text=True)
        if r.returncode == 0:
            blobs.setdefault(r.stdout.strip(), []).append(b)
    return blobs


def gate(unit, source=None):
    command = GATE + [unit]
    if source:
        command += ['--source', source]
    r = subprocess.run(command, capture_output=True, text=True)
    if 'COMPILE FAILED' in r.stdout:
        return None, 'compile-fail'
    m = re.search(r'== exact (\d+)\s+residual (\d+)\s+unwritten (\d+)', r.stdout)
    if not m:
        return None, 'no-verdict'
    ex, res, unw = (int(x) for x in m.groups())
    # total absolute size delta over non-exact functions, as a tiebreaker
    delta = 0
    for line in r.stdout.splitlines():
        mm = re.search(r'size (\d+)!=(\d+)', line)
        if mm:
            delta += abs(int(mm.group(1)) - int(mm.group(2)))
    # unwritten is worse than residual: a residual function is written, an
    # unwritten one is missing. Rank exact first, then fewest unwritten.
    return (ex, -unw, -res, -delta), f'exact {ex} residual {res} unwritten {unw} sizedelta {delta}'


def sweep(unit):
    path = unit + '.c'
    if not os.path.exists(path):
        print(f'{unit}: no source file')
        return
    cur = subprocess.run(['git', 'rev-parse', f'HEAD:{path}'], capture_output=True, text=True).stdout.strip()
    blobs = unique_blobs(path)
    base_score, base_desc = gate(unit)
    print(f'\n=== {unit}   ({len(blobs)} unique blobs)   current: {base_desc}', flush=True)
    best = (base_score, 'CURRENT', base_desc)
    scratch_source = f'scratch/_branch_sweep_{os.getpid()}.c'
    try:
        for blob, brs in blobs.items():
            if blob == cur:
                continue
            content = subprocess.run(['git', 'cat-file', 'blob', blob], capture_output=True).stdout
            open(scratch_source, 'wb').write(content)
            score, desc = gate(unit, scratch_source)
            tag = brs[0] + (f' (+{len(brs)-1})' if len(brs) > 1 else '')
            better = score is not None and base_score is not None and score > base_score
            print(f'    {blob[:8]} {tag:52s} {desc}{"   <== BETTER" if better else ""}', flush=True)
            if score is not None and (best[0] is None or score > best[0]):
                best = (score, tag, desc)
    finally:
        if os.path.exists(scratch_source):
            os.remove(scratch_source)
        # The sweep never edits the production source or reads build/base.
        # gate.py compiles each branch blob to its own scratch object.
    if best[1] != 'CURRENT':
        print(f'    >>> BEST: {best[1]}  {best[2]}', flush=True)


units = sys.argv[1:]
if len(units) == 2 and units[0] == '--from-file':
    # one unit per line: several unit names contain a space ("saved games/"),
    # so they cannot be passed safely as shell arguments
    units = [l.strip() for l in open(units[1]) if l.strip()]
if units == ['--near']:
    out = subprocess.run(['python', 'tools/campaign/board.py'], capture_output=True, text=True).stdout
    units = [l.split()[0] for l in out.splitlines() if re.search(r'remaining +\d+ B / [123] fn', l)]
    print(f'sweeping {len(units)} near-complete units')
for u in units:
    sweep(u)
