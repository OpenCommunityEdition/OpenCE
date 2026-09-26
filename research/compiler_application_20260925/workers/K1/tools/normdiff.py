"""Instruction diff with ebp slot offsets, jump targets and reloc names normalised; prints real hunks."""
import sys, re, difflib
def load(p):
    out = []
    for ln in open(p):
        m = re.match(r'\s*([0-9a-f]+)\s+(\S+)\s*(.*?)\s*(;.*)?$', ln.rstrip('\n'))
        if not m: continue
        addr, mn, ops, ann = m.groups()
        o = re.sub(r'\[ebp ([-+]) (0x[0-9a-f]+|\d+)\]', '[ebp#]', ops)
        o = re.sub(r'\[ebp \+ (\w+) ([-+]) (0x[0-9a-f]+|\d+)\]', r'[ebp+\1#]', o)
        if mn.startswith('j') or mn == 'call':
            o = '@'
        out.append((int(addr, 16), mn + ' ' + o + ((' ' + ann.split('->')[-1].strip()) if ann else '')))
    return out
a = load(sys.argv[1]); b = load(sys.argv[2])
sm = difflib.SequenceMatcher(None, [x[1] for x in a], [x[1] for x in b], autojunk=False)
n = 0
for tag, i1, i2, j1, j2 in sm.get_opcodes():
    if tag == 'equal': continue
    n += 1
    print('--- %s T[%d:%d] O[%d:%d] @T %x @O %x' % (tag, i1, i2, j1, j2, a[i1][0] if i1 < len(a) else -1, b[j1][0] if j1 < len(b) else -1))
    for k in range(i1, min(i2, i1 + 12)): print('   T %5x  %s' % a[k])
    for k in range(j1, min(j2, j1 + 12)): print('   O %5x  %s' % b[k])
print('real hunks', n, file=sys.stderr)
