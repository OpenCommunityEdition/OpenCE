"""Our listing names -> our offsets -> January offsets (through the normalised alignment).
usage: namemap.py jan.txt ours.txt ours.cod FUNCTION"""
import sys, re, difflib, collections
jan, ours, cod, fn = sys.argv[1:5]
lines = open(cod, errors='replace').read().split('\n')
idx = [i for i, l in enumerate(lines) if l.startswith(fn + ' PROC')][0]
names = collections.defaultdict(list)
j = idx - 1
while j > 0 and not lines[j].startswith('_TEXT') and 'PROC' not in lines[j] and 'ENDP' not in lines[j]:
    m = re.match(r'^(\S+) = (-?\d+)$', lines[j].strip())
    if m:
        names[int(m.group(2))].append(m.group(1))
    j -= 1
def load(p):
    out = []
    for ln in open(p):
        m = re.match(r'\s*([0-9a-f]+)\s+(\S+)\s*(.*?)\s*(;.*)?$', ln.rstrip('\n'))
        if not m: continue
        addr, mn, ops, ann = m.groups()
        o = re.sub(r'\[ebp ([-+]) (0x[0-9a-f]+|\d+)\]', '[ebp#]', ops)
        o = re.sub(r'\[ebp \+ (\w+(?:\*\d)?) ([-+]) (0x[0-9a-f]+|\d+)\]', r'[ebp+\1#]', o)
        if mn.startswith('j') or mn == 'call':
            o = '@'
        out.append((int(addr, 16), mn + ' ' + o + ((' ' + ann.split('->')[-1].strip()) if ann else ''), ops))
    return out
pat = re.compile(r'\[ebp (?:\+ \w+(?:\*\d)? )?([-+]) (0x[0-9a-f]+|\d+)\]')
def sl(s):
    return [(-1 if m.group(1) == '-' else 1) * int(m.group(2), 0) for m in pat.finditer(s)]
a = load(jan); b = load(ours)
sm = difflib.SequenceMatcher(None, [x[1] for x in a], [x[1] for x in b], autojunk=False)
m = collections.defaultdict(collections.Counter)
for tag, i1, i2, j1, j2 in sm.get_opcodes():
    if tag in ('equal', 'replace') and i2 - i1 == j2 - j1:
        for k in range(i2 - i1):
            s1, s2 = sl(a[i1 + k][2]), sl(b[j1 + k][2])
            if len(s1) == len(s2):
                for x, y in zip(s1, s2):
                    m[y][x] += 1
def h(v):
    return ('-%x' % -v) if v < 0 else ('+%x' % v)
for off in sorted(set(names) | set(m)):
    if off > 0x40: continue
    print('%7s  jan:%-22s %s' % (h(off), ','.join('%s:%d' % (h(x), n) for x, n in m.get(off, {}).most_common()), ' '.join(names.get(off, []))))
