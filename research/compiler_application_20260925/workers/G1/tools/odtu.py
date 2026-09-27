"""G1 read-only helper: list the first-party /Od functions of one translation unit in address order next to the
definition order of OUR source, so a function without asserts can be located by position.

usage: python -B odtu.py <unit> "<__FILE__ tail, e.g. effects\\player_effects.c>" [--margin 0x2000]
/Od debug builds keep one object's functions contiguous and in definition order; functions that reference the
__FILE__ string anchor the range. Output: /Od entries (Ghidra table) inside [first anchor - margin, last anchor +
margin] with size and an 'F' mark for __FILE__ referencers; then our function definition order.
"""
import importlib.util
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve()
OD = HERE.parents[2] / 'W1' / 'tools' / 'odbuild.py'
spec = importlib.util.spec_from_file_location('odbuild', OD)
ob = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ob)
TSV = Path(ob.EXE).with_suffix('.exe.functions.tsv')


def main():
    unit, ftail = sys.argv[1], sys.argv[2]
    margin = int(sys.argv[sys.argv.index('--margin') + 1], 16) if '--margin' in sys.argv else 0x1000
    anchors = set()
    for sva in ob.find_strings(ftail):
        for r in ob.refs_to(sva):
            anchors.add(r)
    funcs = []
    for line in open(TSV, encoding='utf-8').read().splitlines()[1:]:
        entry, name, src, ranges = line.split('\t')
        rng = re.findall(r'([0-9a-f]{8}), ([0-9a-f]{8})', ranges)
        lo = min(int(a, 16) for a, b in rng)
        hi = max(int(b, 16) for a, b in rng)
        funcs.append((int(entry, 16), lo, hi))
    funcs.sort()
    owners = set()
    for a in anchors:
        for e, lo, hi in funcs:
            if lo <= a <= hi:
                owners.add(e)
    if not owners:
        sys.exit('no anchors')
    lo_a, hi_a = min(owners) - margin, max(owners) + margin
    for e, lo, hi in funcs:
        if lo_a <= e <= hi_a and not (hi - lo < 8):
            print('  od 0x%06x  %5d B  %s' % (e, hi - lo + 1, 'F' if e in owners else ' '))
    src = open(unit + '.c', encoding='latin-1').read()
    defs = re.findall(r'^(?:static\s+)?(?:[a-z_][\w ]*?[\s*])([a-z_]\w*)\(\s*$', src, re.M)
    print('our definition order (%d):' % len(defs))
    print('  ' + ' '.join(defs))


if __name__ == '__main__':
    main()
