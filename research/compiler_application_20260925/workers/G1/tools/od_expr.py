"""G1 read-only evidence tool: symbolic expression readout of the first-party /Od build (halo_cache_symbols.exe).

usage: python -B od_expr.py <va_hex> [--all]

Symbolically executes the SSE/x87 floating-point code of one /Od function (data only; never runs the binary) and
prints every floating-point STORE / ARGUMENT / COMPARE as a fully parenthesised expression tree in /Od evaluation
order. /Od does not reassociate, so the tree IS the source association (a*b/c*d prints ((((a*b)/c)*d)); a*(b/c)
prints (a*(b/c))). Operand ORDER is shown as /Od evaluated it (left operand first).
Leaves: vN = [ebp-N] local, pN = [ebp+N] parameter, gADDR = absolute, K(value) = float constant, *{r}.+off = field
through a tracked register. --all also prints integer stores and pushes for context.
"""
import importlib.util
import re
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve()
OD = HERE.parents[2] / 'W1' / 'tools' / 'odbuild.py'
spec = importlib.util.spec_from_file_location('odbuild', OD)
ob = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ob)

RELOC = None      # optional {instruction address -> relocation symbol name} (our COFF objects)
CUR = [0]
MEMRE = re.compile(r'(?:(byte|word|dword|qword|xmmword) ptr )?\[([^\]]+)\]')


def fconst(addr, width):
    o = ob.va2off(addr)
    if o is None:
        return None
    if width == 'qword':
        return struct.unpack_from('<d', ob.DATA, o)[0]
    return struct.unpack_from('<f', ob.DATA, o)[0]


class St:
    def __init__(self):
        self.x = {}
        self.g = {}
        self.f = []
        self.out = []


def gpname(st, r):
    return st.g.get(r, r)


def mem(st, op, is_float=True):
    m = MEMRE.search(op)
    if not m:
        return None
    width, inner = m.group(1) or 'dword', m.group(2).replace(' ', '')
    if RELOC is not None and CUR[0] in RELOC and re.fullmatch(r'(0x[0-9a-f]+|0)', inner):
        name = RELOC[CUR[0]]
        if name.startswith('__real@'):
            hx = name[7:]
            v = struct.unpack('<f', bytes.fromhex(hx)[::-1])[0] if len(hx) == 8 else struct.unpack('<d', bytes.fromhex(hx)[::-1])[0]
            return 'K(%r)' % (round(v, 7) if len(hx) == 8 else v)
        return 'G(%s)' % name
    if re.fullmatch(r'0x[0-9a-f]+', inner):
        a = int(inner, 16)
        if is_float and 0x93c000 <= a < 0x93c000 + 0xb2e00:      # .rdata only: a literal constant
            v = fconst(a, width)
            s = ob.cstring(a)
            if v is not None and not s and abs(v) < 1e12 and (v == 0 or abs(v) > 1e-12):
                return 'K(%r)' % (round(v, 7) if width != 'qword' else v)
        return 'g%x' % a
    mm = re.fullmatch(r'ebp([-+])0x([0-9a-f]+)', inner)
    if mm:
        return ('v%x' if mm.group(1) == '-' else 'p%x') % int(mm.group(2), 16)
    if inner == 'ebp':
        return 'v0'
    mm = re.fullmatch(r'esp(?:\+0x([0-9a-f]+))?', inner)
    if mm:
        return 'ARG@esp+%s' % (mm.group(1) or '0')
    mm = re.fullmatch(r'([a-z]{3})(?:([-+])0x([0-9a-f]+)|([-+])(\d+))?', inner)
    if mm:
        base = gpname(st, mm.group(1))
        off = mm.group(3) or mm.group(5) or '0'
        sign = mm.group(2) or mm.group(4) or '+'
        return '{%s}%s%s' % (base, sign, off if mm.group(3) is None else '0x' + off)
    return '[%s]' % ','.join(gpname(st, t) if t in ('eax', 'ebx', 'ecx', 'edx', 'esi', 'edi') else t
                              for t in re.split(r'([-+*])', inner))


def xsrc(st, op):
    op = op.strip()
    if op.startswith('xmm'):
        return st.x.get(op, op)
    return mem(st, op)


OPS = {'add': '+', 'sub': '-', 'mul': '*', 'div': '/'}


def run(va, show_all):
    start = va
    end = ob.func_end(start)
    o = ob.va2off(start)
    return symexec(ob.MD.disasm(bytes(ob.DATA[o:o + (end - start)]), start), start, show_all)


def symexec(insns, start, show_all, reloc_at=None):
    st = St()
    for ins in insns:
        mn, ops = ins.mnemonic, ins.op_str
        if reloc_at is not None:
            CUR[0] = reloc_at(ins)
        parts = [p.strip() for p in re.split(r',(?![^\[]*\])', ops)] if ops else []
        a = ins.address
        emit = lambda s: st.out.append('%08x +%04x  %s' % (a, a - start, s))
        try:
            # ---------------- SSE ----------------
            if mn in ('movss', 'movsd', 'movaps', 'movups', 'movapd', 'movq', 'movd') and len(parts) == 2:
                d, s = parts
                if d.startswith('xmm'):
                    if s.startswith('xmm'):
                        st.x[d] = st.x.get(s, s)
                    elif s.startswith('e'):
                        st.x[d] = 'bits(%s)' % gpname(st, s)
                    else:
                        st.x[d] = mem(st, s)
                else:
                    tgt = mem(st, d) if '[' in d else d
                    val = st.x.get(s, s)
                    emit('%s := %s' % (tgt, val))
                    if not d.startswith('['):
                        st.g[d] = 'bits(%s)' % val
                continue
            m = re.fullmatch(r'(add|sub|mul|div)(ss|sd|ps)', mn)
            if m:
                d, s = parts
                st.x[d] = '(%s %s %s)' % (st.x.get(d, d), OPS[m.group(1)], xsrc(st, s))
                continue
            if mn in ('sqrtss', 'sqrtsd'):
                st.x[parts[0]] = 'sqrt(%s)' % xsrc(st, parts[1])
                continue
            if mn in ('maxss', 'minss'):
                st.x[parts[0]] = '%s(%s, %s)' % (mn[:3], st.x.get(parts[0]), xsrc(st, parts[1]))
                continue
            if mn in ('cvtsi2ss', 'cvtsi2sd'):
                s = parts[1]
                st.x[parts[0]] = '(real)%s' % (gpname(st, s) if not s.startswith(('dword', 'byte', 'word')) else mem(st, s, False))
                continue
            if mn in ('cvtss2sd', 'cvtsd2ss'):
                st.x[parts[0]] = ('(double)%s' if mn == 'cvtss2sd' else '(float)%s') % xsrc(st, parts[1])
                continue
            if mn in ('cvttss2si', 'cvtss2si', 'cvttsd2si'):
                st.g[parts[0]] = '(long)%s' % xsrc(st, parts[1])
                continue
            if mn in ('xorps', 'xorpd'):
                d, s = parts
                if d == s:
                    st.x[d] = '0'
                else:
                    st.x[d] = '-%s' % st.x.get(d, d)
                continue
            if mn in ('andps', 'andpd'):
                st.x[parts[0]] = 'fabs(%s)' % st.x.get(parts[0], parts[0])
                continue
            if mn in ('comiss', 'ucomiss', 'comisd', 'ucomisd'):
                emit('CMP %s ? %s' % (st.x.get(parts[0], parts[0]), xsrc(st, parts[1])))
                continue
            # ---------------- x87 ----------------
            if mn in ('fld', 'fild'):
                s = parts[0]
                if s.startswith('st('):
                    st.f.insert(0, st.f[int(s[3])])
                else:
                    v = mem(st, s, mn == 'fld')
                    st.f.insert(0, v if mn == 'fld' else '(real)' + v)
                continue
            if mn in ('fldz', 'fld1'):
                st.f.insert(0, 'K(0.0)' if mn == 'fldz' else 'K(1.0)')
                continue
            if mn in ('fst', 'fstp', 'fistp', 'fist', 'fisttp'):
                d = parts[0]
                v = st.f[0] if st.f else '?'
                if d.startswith('st('):
                    st.f[int(d[3])] = v
                else:
                    emit('%s := %s%s' % (mem(st, d), '(long)' if 'fi' in mn else '', v))
                if mn.endswith('p') and st.f:
                    st.f.pop(0)
                continue
            m = re.fullmatch(r'f(add|sub|subr|mul|div|divr)(p?)', mn)
            if m or mn in ('fiadd', 'fisub', 'fimul', 'fidiv'):
                if not m:
                    m = re.fullmatch(r'fi(add|sub|subr|mul|div|divr)', mn)
                    src = '(real)' + mem(st, parts[0], False)
                    op = m.group(1)
                    top = st.f[0]
                    st.f[0] = '(%s %s %s)' % ((top, OPS[op[:3]], src) if not op.endswith('r') else (src, OPS[op[:3]], top))
                    continue
                op, pop = m.group(1), m.group(2)
                rev = op.endswith('r')
                sym = OPS[op[:3]]
                if not parts:
                    parts = ['st(1)', 'st(0)']
                    pop = 'p'
                if len(parts) == 1 and parts[0].startswith('st('):
                    parts = [parts[0], 'st(0)']
                if len(parts) == 1:          # f op mem  -> st0 = st0 op mem
                    src = mem(st, parts[0])
                    top = st.f[0]
                    st.f[0] = '(%s %s %s)' % ((src, sym, top) if rev else (top, sym, src))
                    continue
                d, s = parts
                di, si = int(d[3]), int(s[3])
                dv, sv = st.f[di], st.f[si]
                st.f[di] = '(%s %s %s)' % ((sv, sym, dv) if rev else (dv, sym, sv))
                if pop:
                    st.f.pop(0)
                continue
            if mn in ('fchs', 'fabs', 'fsqrt', 'fsin', 'fcos'):
                st.f[0] = {'fchs': '-%s', 'fabs': 'fabs(%s)', 'fsqrt': 'sqrt(%s)', 'fsin': 'sin(%s)',
                           'fcos': 'cos(%s)'}[mn] % st.f[0]
                continue
            if mn == 'fxch':
                i = int(parts[0][3]) if parts else 1
                st.f[0], st.f[i] = st.f[i], st.f[0]
                continue
            if mn.startswith('fcom') or mn.startswith('fucom'):
                rhs = mem(st, parts[0]) if parts and not parts[0].startswith('st') else (st.f[1] if len(st.f) > 1 else '?')
                emit('FCMP %s ? %s' % (st.f[0] if st.f else '?', rhs))
                npop = mn.count('p')
                for _ in range(npop):
                    if st.f:
                        st.f.pop(0)
                continue
            # ---------------- integer / control ----------------
            if mn == 'call':
                if RELOC is not None and CUR[0] in RELOC:
                    t = RELOC[CUR[0]]
                else:
                    t = ob.thunk_target(int(ops, 16)) if ops.startswith('0x') else ops
                emit('CALL %s' % (hex(t) if isinstance(t, int) else t))
                st.f = ['ret(%s)' % (hex(t) if isinstance(t, int) else t)] + st.f[:7]
                st.g['eax'] = 'ret(%s)' % (hex(t) if isinstance(t, int) else t)
                st.x = {}
                continue
            if mn == 'push':
                if show_all:
                    emit('PUSH %s' % (mem(st, ops, False) if '[' in ops else gpname(st, ops)))
                continue
            if mn in ('mov', 'movsx', 'movzx') and len(parts) == 2:
                d, s = parts
                if '[' in d:
                    if show_all:
                        emit('%s := %s' % (mem(st, d, False), gpname(st, s) if not s.startswith('0x') else s))
                    continue
                if '[' in s:
                    v = mem(st, s, False)
                    st.g[d] = '*' + v if not v.startswith('K(') else v
                else:
                    st.g[d] = gpname(st, s) if not s.startswith('0x') else s
                continue
            if mn == 'lea' and len(parts) == 2:
                st.g[parts[0]] = '&' + mem(st, parts[1], False)
                continue
            if mn in ('add', 'sub', 'imul', 'shl', 'sar', 'shr', 'and', 'or', 'xor') and len(parts) >= 2 and '[' not in parts[0]:
                d = parts[0]
                if d in ('esp', 'ebp'):
                    continue
                if mn == 'xor' and parts[1] == d:
                    st.g[d] = '0'
                else:
                    st.g[d] = '(%s %s %s)' % (gpname(st, d), mn, gpname(st, parts[-1]) if not parts[-1].startswith('[') else mem(st, parts[-1], False))
                continue
            if mn in ('jmp',) or mn.startswith('j'):
                if show_all:
                    emit('%s %s' % (mn, ops))
                continue
        except (IndexError, KeyError, ValueError, TypeError) as e:
            emit('?? %s %s (%s)' % (mn, ops, e))
    return st.out


def main():
    va = int(sys.argv[1], 16)
    for line in run(va, '--all' in sys.argv):
        print(line)


if __name__ == '__main__':
    main()
