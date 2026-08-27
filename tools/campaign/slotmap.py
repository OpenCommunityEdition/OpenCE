"""Classify bijection_walk -v STRUCT events: same instruction with a different
frame slot, or a genuinely different instruction. Reads the walk output on stdin.

usage: python tools/bijection_walk.py -v --unit U FN | python scratch/slotmap.py
"""
import sys, re, collections

m = collections.Counter()
other = []
for line in sys.stdin:
    if 'STRUCT' not in line:
        continue
    g = re.search(r'T:(.*?)\s{2,}O:(.*)$', line)
    if not g:
        continue
    a, b = g.group(1).strip(), g.group(2).strip()
    ta = re.findall(r'ebp - (0x[0-9a-f]+|\d+)', a)
    tb = re.findall(r'ebp - (0x[0-9a-f]+|\d+)', b)
    norm = lambda s: re.sub(r'ebp - (?:0x[0-9a-f]+|\d+)', 'ebp-S', s)
    if norm(a) == norm(b) and len(ta) == len(tb) and ta:
        for x, y in zip(ta, tb):
            m[(x, y)] += 1
    else:
        other.append((a, b))

print(f'STRUCT events that are the SAME instruction, different frame slot: {sum(m.values())}')
print(f'STRUCT events that are genuinely different instructions:        {len(other)}')
print(f'\ndistinct slot mappings (target -> ours), {len(m)} of them, by frequency:')
for (t_, o_), c in sorted(m.items(), key=lambda kv: -kv[1]):
    print(f'   [ebp-{t_:>6}] -> [ebp-{o_:>6}]   x{c}')
if other:
    print('\nfirst 12 genuinely-different instructions:')
    for a, b in other[:12]:
        print(f'   T: {a:<44s} O: {b}')
