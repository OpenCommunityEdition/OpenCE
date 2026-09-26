"""H1: index the first-party /Od build (as data): for every /Od function (prologue 55 8B EC), the string literals it
references and every absolute .data/.bss address it touches with the access width. Cached to JSON.
    python odindex.py            -> builds scratch/campaign/workers/H1/od_index.json
"""
import json, os, struct, capstone
import odpe

OUT = r"C:\halo-worktrees\claude-compiler-application-20260925\scratch\campaign\workers\H1\od_index.json"
DATA_LO = [s for s in odpe.SECS if s[0] == '.data'][0][1]
DATA_HI = DATA_LO + [s for s in odpe.SECS if s[0] == '.data'][0][2]
RD = [s for s in odpe.SECS if s[0] == '.rdata'][0]


def cstr(va):
    if not (RD[1] <= va < RD[1] + RD[3]):
        return None
    off = RD[4] + va - RD[1]
    e = odpe.B.find(b'\0', off, off + 400)
    if e <= off:
        return None
    s = odpe.B[off:e]
    if len(s) >= 2 and all(32 <= c < 127 or c in (9, 10) for c in s):
        return s.decode('latin-1')
    return None


def build():
    sva, t = odpe.text()
    starts = [sva + i for i in range(1, len(t) - 3) if t[i:i + 3] == b'\x55\x8b\xec' and t[i - 1] in (0xcc, 0xc3, 0xc2, 0x90)]
    starts.append(sva + len(t))
    md = odpe.MD
    idx = {}
    for a, b in zip(starts, starts[1:]):
        off = a - sva
        strs, data = set(), []
        for ins in md.disasm(t[off:off + (b - a)], a):
            for op in ins.operands:
                v = None
                w = None
                if op.type == capstone.x86.X86_OP_MEM and op.mem.disp:
                    v = op.mem.disp & 0xffffffff
                    w = op.size if op.mem.index == 0 and op.mem.base == 0 else -op.size
                elif op.type == capstone.x86.X86_OP_IMM:
                    v = op.imm & 0xffffffff
                    w = 0
                if v is None:
                    continue
                if DATA_LO <= v < DATA_HI:
                    data.append((v, w, ins.mnemonic))
                else:
                    s = cstr(v)
                    if s:
                        strs.add(s)
        idx[a] = {'end': b, 'strs': sorted(strs), 'data': data}
    json.dump({str(k): v for k, v in idx.items()}, open(OUT, 'w'))
    return idx


def load():
    if not os.path.exists(OUT):
        build()
    return {int(k): v for k, v in json.load(open(OUT)).items()}


if __name__ == '__main__':
    idx = build()
    print(len(idx), 'functions indexed')
