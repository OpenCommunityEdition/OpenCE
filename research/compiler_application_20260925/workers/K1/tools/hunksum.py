"""Summarise normalised hunks between two dumpfn listings: per hunk T/O insn counts and byte spans."""
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
end_a = int(sys.argv[3], 16) if len(sys.argv) > 3 else None
end_b = int(sys.argv[4], 16) if len(sys.argv) > 4 else None
if end_a: a = [x for x in a if x[0] < end_a]
if end_b: b = [x for x in b if x[0] < end_b]
sm = difflib.SequenceMatcher(None, [x[1] for x in a], [x[1] for x in b], autojunk=False)
n = 0; tot = 0
def span(lst, i1, i2):
    if i1 >= i2: return 0
    nxt = lst[i2][0] if i2 < len(lst) else lst[-1][0] + 1
    return nxt - lst[i1][0]
for tag, i1, i2, j1, j2 in sm.get_opcodes():
    if tag == 'equal': continue
    n += 1
    ta = span(a, i1, i2); tb = span(b, j1, j2); tot += tb - ta
    print('%-7s T %5x n=%3d b=%4d | O %5x n=%3d b=%4d | dB %+4d cum %+5d' % (tag, a[i1][0] if i1 < len(a) else -1, i2-i1, ta, b[j1][0] if j1 < len(b) else -1, j2-j1, tb, tb-ta, tot))
print('hunks', n, 'net bytes', tot)
