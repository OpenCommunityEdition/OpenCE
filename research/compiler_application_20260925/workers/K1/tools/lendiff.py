"""List aligned (normalised-equal) instruction pairs whose encoded LENGTH differs (jcc short/near, disp8/disp32)."""
import sys, re, difflib
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
        out.append([int(addr, 16), mn + ' ' + o, mn + ' ' + ops])
    for i in range(len(out) - 1):
        out[i].append(out[i + 1][0] - out[i][0])
    out[-1].append(0)
    return out
a = load(sys.argv[1]); b = load(sys.argv[2])
end = int(sys.argv[3], 16) if len(sys.argv) > 3 else 1 << 30
sm = difflib.SequenceMatcher(None, [x[1] for x in a], [x[1] for x in b], autojunk=False)
for tag, i1, i2, j1, j2 in sm.get_opcodes():
    if tag != 'equal': continue
    for k in range(i2 - i1):
        x, y = a[i1 + k], b[j1 + k]
        if x[0] >= end: break
        if x[3] != y[3]:
            print('T %5x len %d %-40s | O %5x len %d %s' % (x[0], x[3], x[2][:40], y[0], y[3], y[2][:40]))
