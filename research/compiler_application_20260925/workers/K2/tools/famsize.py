"""K2: per-region byte delta (ours - January) from sdiff, grouped into residual families by January offset."""
import sys, re, difflib
sys.path.insert(0, __import__('os').path.dirname(__import__('os').path.abspath(__file__)))
from sdiff import load, norm
J = load(sys.argv[1]); O = load(sys.argv[2])
def size(L, i):
    return (L[i+1][0] if i+1 < len(L) else L[i][0]+1) - L[i][0]
a = [norm(x, 2) for x in J]; b = [norm(x, 2) for x in O]
sm = difflib.SequenceMatcher(None, a, b, autojunk=False)
FAM = [(0x0, 0x6ff, 'ray-loop operand order (0x46b, 0x630)'), (0xf00, 0xfa0, 'cross-product/magnitude group 0xf52..0xf89'),
       (0x1000, 0x1300, 'marker fst/fstp (0x1046, 0x11eb, 0x11ff, 0x12c8)'), (0x1dd0, 0x1de0, 'align filler 0x1ddd'),
       (0x1ee0, 0x1f40, 'prop interpolation 0x1ee7..0x1f33'), (0x2700, 0x2800, '0x2772 group'),
       (0x2ad0, 0x2b80, 't-copy flying #1'), (0x2c00, 0x2c40, '0x2c1a group'), (0x2cd0, 0x2d60, 'gun cross-jump'),
       (0x39a0, 0x39c0, 'fisub 0x39a6'), (0x4900, 0x49b0, 't-copy flying #2'), (0x4cb0, 0x4d90, 'push order 0x4cb7..0x4d7a'),
       (0x4f30, 0x5040, 'vehicle-avoidance t-copy + push order'), (0x5570, 0x5700, 'vision cone / filler 0x557a..0x56de'),
       (0x59e0, 0x5a10, 'push order 0x59e5')]
res = {}
for tag, i1, i2, j1, j2 in sm.get_opcodes():
    if tag == 'equal' or i1 >= len(J):
        continue
    off = J[i1][0]
    if off >= 0x5ffd:
        continue
    jb = sum(size(J, i) for i in range(i1, i2)); ob = sum(size(O, j) for j in range(j1, j2))
    if tag == 'replace' and all(J[i][1] == 'jmp' for i in range(i1, i2)) and (i2 - i1) == 1 and 'jmptable' in J[i1][3]:
        continue
    fam = next((n for lo, hi, n in FAM if lo <= off < hi), 'other@%x' % off)
    r = res.setdefault(fam, [0, 0, 0]); r[0] += 1; r[1] += (i2 - i1); r[2] += ob - jb
for fam, (n, ni, db) in sorted(res.items(), key=lambda kv: -abs(kv[1][2])):
    print('%-50s regions %2d  J-insns %3d  bytes(ours-J) %+d' % (fam, n, ni, db))
print('TOTAL bytes(ours-J) %+d' % sum(v[2] for v in res.values()))
