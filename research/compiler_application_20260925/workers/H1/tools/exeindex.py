"""H1: index an optimised Win32 build (as data): function starts = direct CALL targets (+ /Od prologues), each
function = [start, next start). Per function: string literals referenced and absolute .data/.bss accesses with
width (negative width = indexed/based access, 0 = immediate address). Cached to JSON.
    python exeindex.py <exe> <out.json>
"""
import json, os, struct, sys, re, capstone
import pefile_min


def build(exe, out):
    P = pefile_min.PE(exe)
    t = P.sec('.text')
    tva, tsize, traw = t[1], t[3], t[4]
    seg = P.B[traw:traw + tsize]
    d = P.sec('.data')
    dlo, dhi = d[1], d[1] + d[2]
    rd = P.sec('.rdata')
    starts = set()
    for m in re.finditer(b'\xe8', seg):
        i = m.start()
        if i + 5 > len(seg):
            continue
        tgt = tva + i + 5 + struct.unpack_from('<i', seg, i + 1)[0]
        if tva <= tgt < tva + tsize:
            starts.add(tgt)
    for i in range(1, len(seg) - 3):
        if seg[i:i + 3] == b'\x55\x8b\xec' and seg[i - 1] in (0xcc, 0xc3, 0xc2, 0x90):
            starts.add(tva + i)
    starts = sorted(starts) + [tva + tsize]

    def cstr(va):
        if not (rd[1] <= va < rd[1] + rd[3]):
            return None
        off = rd[4] + va - rd[1]
        e = P.B.find(b'\0', off, off + 400)
        if e <= off:
            return None
        s = P.B[off:e]
        if len(s) >= 2 and all(32 <= c < 127 or c in (9, 10) for c in s):
            return s.decode('latin-1')
        return None
    idx = {}
    md = P.md
    for a, b in zip(starts, starts[1:]):
        off = a - tva
        strs, data = set(), []
        for ins in md.disasm(seg[off:off + (b - a)], a):
            for op in ins.operands:
                if op.type == capstone.x86.X86_OP_MEM and op.mem.disp:
                    v = op.mem.disp & 0xffffffff
                    w = op.size if op.mem.index == 0 and op.mem.base == 0 else -op.size
                elif op.type == capstone.x86.X86_OP_IMM:
                    v = op.imm & 0xffffffff
                    w = 0
                else:
                    continue
                if dlo <= v < dhi:
                    data.append((v, w, ins.mnemonic))
                else:
                    s = cstr(v)
                    if s:
                        strs.add(s)
        idx[a] = {'end': b, 'strs': sorted(strs), 'data': data}
    json.dump({str(k): v for k, v in idx.items()}, open(out, 'w'))
    return idx


def load(out):
    return {int(k): v for k, v in json.load(open(out)).items()}


if __name__ == '__main__':
    idx = build(sys.argv[1], sys.argv[2])
    print(len(idx), 'functions indexed')
