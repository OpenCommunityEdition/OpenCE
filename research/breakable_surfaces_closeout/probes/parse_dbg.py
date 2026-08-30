"""Parse dbg_result.txt into per-hit records: registers + the [edi] IR node.

For the m6only compile, ebx is the operand's frame displacement, so each
hit can be labelled with the vector member it references:
  a.i=-0x18 a.j=-0x14 a.k=-0x10 | b.i=-0xc b.j=-8 b.k=-4
"""
import re
import sys

PATH = sys.argv[1] if len(sys.argv) > 1 else r"tools/c2dbg32/dbg_result.txt"
NAMES = {0x18: "a.i", 0x14: "a.j", 0x10: "a.k", 0xc: "b.i", 8: "b.j", 4: "b.k"}
OPS = {0x24d: "fld?", 0x24e: "fld", 0x250: "fadd", 0x251: "fsub", 0x252: "fmul?",
       0x253: "fmul?", 0x258: "fstore", 0x249: "op249", 0x261: "op261"}

txt = open(PATH, encoding="latin-1", errors="replace").read()
lines = txt.splitlines()
hits = []
i = 0
while i < len(lines):
    m = re.match(r"HIT bp(\d+) #\w+ eip=(0x[0-9a-f]+) eax=(0x[0-9a-f]+) "
                 r"ebx=(0x[0-9a-f]+) ecx=(0x[0-9a-f]+) edx=(0x[0-9a-f]+)", lines[i])
    if not m:
        i += 1
        continue
    rec = {"bp": int(m.group(1)), "eip": m.group(2), "ebx": int(m.group(4), 16),
           "ecx": int(m.group(5), 16), "edx": int(m.group(6), 16)}
    for j in range(i + 1, min(i + 12, len(lines))):
        m2 = re.search(r"esi=(0x[0-9a-f]+) edi=(0x[0-9a-f]+)", lines[j])
        if m2:
            rec["edi"] = int(m2.group(2), 16)
        m3 = re.match(r"\s*\[edi\]@(0x[0-9a-f]+):((?: 0x[0-9a-f]+)+)", lines[j])
        if m3:
            rec["node_addr"] = int(m3.group(1), 16)
            rec["node"] = [int(w, 16) for w in m3.group(2).split()]
        if lines[j].startswith("HIT "):
            break
    hits.append(rec)
    i += 1

print("hits:", len(hits))
print("%-4s %-8s %-6s %-10s %-8s %-10s %-10s %-10s" %
      ("#", "ebx", "member", "node", "op(+4)", "+8", "+0x18", "+0x24"))
for n, h in enumerate(hits):
    nd = h.get("node", [])
    op = nd[1] if len(nd) > 1 else 0
    f8 = nd[2] if len(nd) > 2 else 0
    f18 = nd[6] if len(nd) > 6 else 0
    f24 = nd[9] if len(nd) > 9 else 0
    print("%-4d %-8s %-6s 0x%08x %-8s 0x%08x 0x%08x 0x%08x" %
          (n + 1, hex(h["ebx"]), NAMES.get(h["ebx"], ""), h.get("node_addr", 0),
           OPS.get(op, hex(op)), f8, f18, f24))
