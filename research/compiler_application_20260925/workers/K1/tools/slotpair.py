"""Map January ebp slots to ours through the normalised alignment (equal + equal-length replace blocks).
usage: slotpair.py jan.txt ours.txt [--all]   prints T slot -> {O slot: count}; flags inconsistent maps."""
import sys, re, difflib, collections
def load(p):
    out = []
    for ln in open(p):
        m = re.match(r'\s*([0-9a-f]+)\s+(\S+)\s*(.*?)\s*(;.*)?$', ln.rstrip('\n'))
        if not m: continue
        addr, mn, ops, ann = m.groups()
        o = re.sub(r'\[ebp ([-+]) (0x[0-9a-f]+|\d+)\]', '[ebp#]', ops)
        o = re.sub(r'\[ebp \+ (\w+) ([-+]) (0x[0-9a-f]+|\d+)\]', r'[ebp+\1#]', o)
        o = re.sub(r'\[ebp \+ (\w+\*\d) ([-+]) (0x[0-9a-f]+|\d+)\]', r'[ebp+\1#]', o)
        if mn.startswith('j') or mn == 'call':
            o = '@'
        out.append((int(addr, 16), mn + ' ' + o + ((' ' + ann.split('->')[-1].strip()) if ann else ''), ops))
    return out
pat = re.compile(r'\[ebp (?:\+ \w+(?:\*\d)? )?([-+]) (0x[0-9a-f]+|\d+)\]')
def sl(s):
    return [(-1 if m.group(1) == '-' else 1) * int(m.group(2), 0) for m in pat.finditer(s)]
a = load(sys.argv[1]); b = load(sys.argv[2])
sm = difflib.SequenceMatcher(None, [x[1] for x in a], [x[1] for x in b], autojunk=False)
m = collections.defaultdict(collections.Counter)
for tag, i1, i2, j1, j2 in sm.get_opcodes():
    if tag in ('equal', 'replace') and i2 - i1 == j2 - j1:
        for k in range(i2 - i1):
            s1, s2 = sl(a[i1 + k][2]), sl(b[j1 + k][2])
            if len(s1) == len(s2):
                for x, y in zip(s1, s2):
                    m[x][y] += 1
show_all = '--all' in sys.argv
for k in sorted(m):
    d = m[k]
    if show_all or len(d) > 1 or list(d)[0] != k:
        print('%7s -> %s' % (('-%x' % -k) if k < 0 else ('+%x' % k), ', '.join('%s:%d' % ((('-%x' % -y) if y < 0 else ('+%x' % y)), n) for y, n in d.most_common())))
