"""H1: per-function facts from a January split object: string literals referenced, and every .bss/.data owner
symbol touched (+addend, width). Read-only."""
import struct, sys, capstone
sys.path.insert(0, r"C:\halo-worktrees\claude-compiler-application-20260925\tools\campaign")
import coffio

md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
md.detail = True


def facts(path):
    o = coffio.load(path)
    syms = {sy['index']: sy for sy in o.symbols}
    secname = {s['index']: s['name'] for s in o.sections}
    def lit(sy):
        s = o.sections[sy['sec'] - 1]
        d = s['data']
        e = d.find(b'\0', sy['value'])
        if e < 0:
            return None
        t = d[sy['value']:e]
        try:
            return t.decode('latin-1')
        except Exception:
            return None
    owners = {}
    for sy in o.symbols:
        if sy['sec'] > 0 and sy['value'] == 0 and sy['storage'] in (2, 3) and not sy['name'].startswith(('.', '$')):
            if secname[sy['sec']].startswith('.text'):
                owners.setdefault(sy['sec'], sy['name'])
    # data owner symbols (per data section, sorted by value)
    dsyms = {}
    for sy in o.symbols:
        if sy['sec'] > 0 and secname[sy['sec']].startswith(('.bss', '.data')) and not sy['name'].startswith(('.', '$')):
            dsyms.setdefault(sy['sec'], []).append((sy['value'], sy['name']))
    for k in dsyms:
        dsyms[k].sort()
    def owner_of(sec, off):
        best = None
        for v, n in dsyms.get(sec, []):
            if v <= off:
                best = (n, off - v)
        return best
    out = {}
    for s in o.sections:
        if not s['name'].startswith('.text') or s['index'] not in owners:
            continue
        code = s['data']
        insns = list(md.disasm(code, 0))
        strs, data = set(), []
        for va, symidx, typ in s['relocs']:
            sy = syms[symidx]
            if sy['sec'] <= 0:
                continue
            sn = secname[sy['sec']]
            addend = struct.unpack_from('<l', code, va)[0]
            if sy['name'].startswith('??_C@'):
                t = lit(sy)
                if t:
                    strs.add(t)
            elif sn.startswith(('.bss', '.data')):
                ins = next((i for i in insns if i.address <= va < i.address + i.size), None)
                w = 0
                if ins is not None:
                    for op in ins.operands:
                        if op.type == capstone.x86.X86_OP_MEM and (op.mem.disp & 0xffffffff) == (addend & 0xffffffff):
                            w = op.size if op.mem.index == 0 and op.mem.base == 0 else -op.size
                own = owner_of(sy['sec'], sy['value'] + addend)
                data.append((sn, sy['value'] + addend, own, w, ins.mnemonic if ins else '?'))
        out[owners[s['index']]] = {'strs': sorted(strs), 'data': data}
    return o, out, dsyms, secname
