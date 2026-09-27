"""D1 read-only PDB reader (MSF7 + TPI + DBI module streams); prints type facts as data.

    python -B pdbtypes.py <pdb> struct <name>          all members of a (non-forward) LF_STRUCTURE, types rendered
    python -B pdbtypes.py <pdb> procs <regex>          every S_GPROC32/S_LPROC32 whose name matches, per module, type rendered
    python -B pdbtypes.py <pdb> params <module> <fn>   params/locals (S_REGREL32/S_BPREL32/S_REGISTER/S_LOCAL) of one proc
    python -B pdbtypes.py <pdb> udt <regex>            every S_UDT (typedef) whose name matches (module + global streams)
    python -B pdbtypes.py <pdb> udtbytype <typeidx>    every S_UDT whose type is <typeidx> or a pointer to it
    python -B pdbtypes.py <pdb> modsyms <module>       list PROC/DATA/UDT symbols of a module (basename match)
Never writes anything.
"""
import re
import struct
import sys

PRIM = {0x03: 'void', 0x10: 'char', 0x11: 'short', 0x12: 'long', 0x13: '__int64', 0x20: 'unsigned char',
        0x21: 'unsigned short', 0x22: 'unsigned long', 0x23: 'unsigned __int64', 0x30: 'bool', 0x40: 'float',
        0x41: 'double', 0x68: '__int8', 0x69: 'unsigned __int8', 0x70: 'char', 0x71: 'wchar_t', 0x74: 'int',
        0x75: 'unsigned int', 0x7a: 'char16_t', 0x7b: 'char32_t'}


class PDB:
    def __init__(self, path):
        self.f = open(path, 'rb')
        sb = self.f.read(4096)
        assert sb.startswith(b'Microsoft C/C++ MSF 7.00\r\n\x1aDS'), 'not MSF7'
        self.bs, _fpm, _nb, ndirbytes, _u = struct.unpack_from('<5I', sb, 32)
        bs = self.bs
        ndirblocks = (ndirbytes + bs - 1) // bs
        nmap = (ndirblocks * 4 + bs - 1) // bs
        mapblocks = struct.unpack_from('<%dI' % nmap, sb, 52)
        idx = b''.join(self.blk(m) for m in mapblocks)
        dirblocks = struct.unpack_from('<%dI' % ndirblocks, idx, 0)
        d = b''.join(self.blk(b) for b in dirblocks)[:ndirbytes]
        n = struct.unpack_from('<I', d, 0)[0]
        sizes = struct.unpack_from('<%dI' % n, d, 4)
        off = 4 + 4 * n
        self.streams = []
        for s in sizes:
            if s == 0xffffffff:
                s = 0
            k = (s + bs - 1) // bs
            self.streams.append((s, struct.unpack_from('<%dI' % k, d, off)))
            off += 4 * k
        tpi = self.stream(2)
        _ver, hsz, self.tib, self.tie, trb = struct.unpack_from('<5I', tpi, 0)
        self.recs = {}
        p = hsz
        ti = self.tib
        while ti < self.tie:
            ln, kind = struct.unpack_from('<HH', tpi, p)
            self.recs[ti] = (kind, tpi[p + 4:p + 2 + ln])
            p += 2 + ln
            ti += 1
        self._mods = None
        self._fwd = None

    def blk(self, i):
        self.f.seek(i * self.bs)
        return self.f.read(self.bs)

    def stream(self, i):
        s, bl = self.streams[i]
        return b''.join(self.blk(b) for b in bl)[:s]

    # ---------- types
    @staticmethod
    def numleaf(b, o):
        v = struct.unpack_from('<H', b, o)[0]
        if v < 0x8000:
            return v, o + 2
        fmt = {0x8000: ('<b', 1), 0x8001: ('<h', 2), 0x8002: ('<H', 2), 0x8003: ('<l', 4), 0x8004: ('<L', 4),
               0x8009: ('<q', 8), 0x800a: ('<Q', 8)}.get(v)
        if not fmt:
            raise ValueError('numleaf %#x' % v)
        return struct.unpack_from(fmt[0], b, o + 2)[0], o + 2 + fmt[1]

    @staticmethod
    def cstr(b, o):
        e = b.index(b'\0', o)
        return b[o:e].decode('latin-1'), e + 1

    def udt_name(self, ti):
        k, b = self.recs[ti]
        if k in (0x1505, 0x1504):  # structure / class
            _cnt, _prop, _fl, _der, _vs = struct.unpack_from('<HHIII', b, 0)
            _sz, o = self.numleaf(b, 16)
            return 'struct ' + self.cstr(b, o)[0]
        if k == 0x1506:
            _cnt, _prop, _fl = struct.unpack_from('<HHI', b, 0)
            _sz, o = self.numleaf(b, 8)
            return 'union ' + self.cstr(b, o)[0]
        if k == 0x1507:
            _cnt, _prop, _ut, _fl = struct.unpack_from('<HHII', b, 0)
            return 'enum ' + self.cstr(b, 12)[0]
        return None

    def render(self, ti, depth=0):
        if ti < 0x1000:
            mode = (ti >> 8) & 0xf
            base = PRIM.get(ti & 0xff, 'prim%#x' % (ti & 0xff))
            return base + (' *' if mode else '')
        if ti not in self.recs:
            return '?%#x' % ti
        k, b = self.recs[ti]
        n = self.udt_name(ti)
        if n:
            return n
        if k == 0x1001:  # modifier
            t, m = struct.unpack_from('<IH', b, 0)
            q = ('const ' if m & 1 else '') + ('volatile ' if m & 2 else '')
            return q + self.render(t, depth + 1)
        if k == 0x1002:  # pointer
            t = struct.unpack_from('<I', b, 0)[0]
            tk = self.recs.get(t, (0, b''))[0] if t >= 0x1000 else 0
            if tk == 0x1008:
                return self.render_proc(t, ptr=True)
            return self.render(t, depth + 1) + ' *'
        if k == 0x1008:
            return self.render_proc(ti)
        if k == 0x1503:  # array
            et, it = struct.unpack_from('<II', b, 0)
            sz, _o = self.numleaf(b, 8)
            return '%s[%d bytes]' % (self.render(et, depth + 1), sz)
        if k == 0x1205:
            t, ln, pos = struct.unpack_from('<IBB', b, 0)
            return '%s:%d@%d' % (self.render(t), ln, pos)
        return 'kind%#x' % k

    def render_proc(self, ti, ptr=False):
        k, b = self.recs[ti]
        rv, cc, _fa, pc, al = struct.unpack_from('<IBBHI', b, 0)
        n = struct.unpack_from('<I', self.recs[al][1], 0)[0]
        args = struct.unpack_from('<%dI' % n, self.recs[al][1], 4)
        a = ', '.join(self.render(x) for x in args) or 'void?'
        return '%s (%s%s)(%s)  [proc %#x ret %#x cc %d nparm %d arglist %#x args %s]' % (
            self.render(rv), '*' if ptr else '', '', a, ti, rv, cc, pc, al, [hex(x) for x in args])

    def structs(self, name):
        out = []
        for ti, (k, b) in self.recs.items():
            if k != 0x1505:
                continue
            cnt, prop, fl, _der, _vs = struct.unpack_from('<HHIII', b, 0)
            if prop & 0x80:
                continue
            size, o = self.numleaf(b, 16)
            nm, _o = self.cstr(b, o)
            if nm == name:
                out.append((ti, size, fl))
        return out

    def members(self, fl):
        _k, fb = self.recs[fl]
        o = 0
        res = []
        while o < len(fb):
            lk = struct.unpack_from('<H', fb, o)[0]
            if lk != 0x150d:
                res.append(('?leaf%#x' % lk, None, None))
                break
            _attr, typ = struct.unpack_from('<HI', fb, o + 2)
            off, o2 = self.numleaf(fb, o + 8)
            nm, o2 = self.cstr(fb, o2)
            res.append((nm, off, typ))
            o = o2
            while o < len(fb) and fb[o] >= 0xf0:
                o += fb[o] & 0xf
        return res

    # ---------- modules / symbols
    def mods(self):
        if self._mods is None:
            dbi = self.stream(3)
            hdr = struct.unpack_from('<iIIHHHHHHIIIIIIIHHI', dbi, 0)
            self.gsym_stream = hdr[5]
            self.psym_stream = hdr[4]
            self.symrec_stream = hdr[7]
            modsz = hdr[9]
            p = 64
            end = 64 + modsz
            mods = []
            while p < end:
                sn = struct.unpack_from('<H', dbi, p + 34)[0]
                symsz = struct.unpack_from('<I', dbi, p + 36)[0]
                q = p + 64
                e = dbi.index(b'\0', q)
                name = dbi[q:e].decode('latin-1')
                q = e + 1
                e = dbi.index(b'\0', q)
                obj = dbi[q:e].decode('latin-1')
                q = e + 1
                q = (q + 3) & ~3
                mods.append((name, obj, sn, symsz))
                p = q
            self._mods = mods
        return self._mods

    def mod_symbols(self, m):
        name, _obj, sn, symsz = m
        if sn == 0xffff:
            return
        s = self.stream(sn)
        pp = 4
        while pp < symsz:
            ln, kind = struct.unpack_from('<HH', s, pp)
            yield pp, kind, s[pp + 4:pp + 2 + ln]
            pp += 2 + ln

    def symrec_symbols(self):
        self.mods()
        s = self.stream(self.symrec_stream)
        pp = 0
        while pp + 4 <= len(s):
            ln, kind = struct.unpack_from('<HH', s, pp)
            if ln < 2:
                break
            yield pp, kind, s[pp + 4:pp + 2 + ln]
            pp += 2 + ln


def modbase(m):
    return m[0].replace('/', '\\').split('\\')[-1].lower()


def main():
    pdb = PDB(sys.argv[1])
    cmd = sys.argv[2]
    if cmd == 'struct':
        for ti, size, fl in pdb.structs(sys.argv[3]):
            print('struct %s ti %#x size %d fieldlist %#x' % (sys.argv[3], ti, size, fl))
            for nm, off, typ in pdb.members(fl):
                print('  +%#05x %-40s %s  [ti %#x]' % (off or 0, nm, pdb.render(typ) if typ else '', typ or 0))
    elif cmd == 'procs':
        rx = re.compile(sys.argv[3])
        for m in pdb.mods():
            for pp, kind, b in pdb.mod_symbols(m):
                if kind in (0x1110, 0x110f, 0x1146, 0x1147):
                    nm = pdb.cstr(b, 35)[0]
                    if rx.search(nm):
                        ti = struct.unpack_from('<I', b, 24)[0]
                        print('%-28s %s %-40s %s' % (modbase(m), 'GPROC' if kind in (0x1110, 0x1147) else 'LPROC',
                                                     nm, pdb.render(ti)))
    elif cmd == 'params':
        want_mod, want_fn = sys.argv[3].lower(), sys.argv[4]
        for m in pdb.mods():
            if modbase(m) != want_mod:
                continue
            inside = 0
            for pp, kind, b in pdb.mod_symbols(m):
                if kind in (0x1110, 0x110f):
                    nm = pdb.cstr(b, 35)[0]
                    if nm == want_fn:
                        inside = 1
                        print('PROC', nm, pdb.render(struct.unpack_from('<I', b, 24)[0]))
                        continue
                if not inside:
                    continue
                if kind == 0x0006:  # S_END
                    inside -= 1
                    if inside == 0:
                        break
                    continue
                if kind in (0x1103, 0x1110, 0x110f):  # nested block
                    inside += 1
                    continue
                if kind == 0x1111:  # S_REGREL32
                    off, ti, reg = struct.unpack_from('<iIH', b, 0)
                    print('  REGREL reg%d%+d %-32s %s' % (reg, off, pdb.cstr(b, 10)[0], pdb.render(ti)))
                elif kind == 0x110b:
                    off, ti = struct.unpack_from('<iI', b, 0)
                    print('  BPREL %+d %-32s %s' % (off, pdb.cstr(b, 8)[0], pdb.render(ti)))
                elif kind == 0x1106:
                    ti, reg = struct.unpack_from('<IH', b, 0)
                    print('  REGISTER r%d %-32s %s' % (reg, pdb.cstr(b, 6)[0], pdb.render(ti)))
                elif kind == 0x113e:
                    ti, fl = struct.unpack_from('<IH', b, 0)
                    print('  LOCAL flags%#x %-32s %s' % (fl, pdb.cstr(b, 6)[0], pdb.render(ti)))
    elif cmd in ('udt', 'udtbytype'):
        if cmd == 'udt':
            rx = re.compile(sys.argv[3])
            match = lambda nm, ti: rx.search(nm)
        else:
            tgt = int(sys.argv[3], 16)
            ptrs = {ti for ti, (k, b) in pdb.recs.items() if k == 0x1002 and struct.unpack_from('<I', b, 0)[0] == tgt}
            match = lambda nm, ti: ti == tgt or ti in ptrs
        seen = set()
        for m in pdb.mods():
            for pp, kind, b in pdb.mod_symbols(m):
                if kind == 0x1108:
                    ti = struct.unpack_from('<I', b, 0)[0]
                    nm = pdb.cstr(b, 4)[0]
                    if match(nm, ti):
                        key = (nm, ti)
                        print('%-28s UDT %-40s %s' % (modbase(m), nm, pdb.render(ti)))
                        seen.add(key)
        for pp, kind, b in pdb.symrec_symbols():
            if kind == 0x1108:
                ti = struct.unpack_from('<I', b, 0)[0]
                nm = pdb.cstr(b, 4)[0]
                if match(nm, ti):
                    print('%-28s UDT %-40s %s' % ('<symrec>', nm, pdb.render(ti)))
    elif cmd == 'modsyms':
        want_mod = sys.argv[3].lower()
        K = {0x110f: 'LPROC', 0x1110: 'GPROC', 0x110c: 'LDATA', 0x110d: 'GDATA', 0x1108: 'UDT'}
        for m in pdb.mods():
            if modbase(m) != want_mod:
                continue
            print(m)
            for pp, kind, b in pdb.mod_symbols(m):
                if kind in (0x110f, 0x1110):
                    print('  %s %s %s' % (K[kind], pdb.cstr(b, 35)[0], pdb.render(struct.unpack_from('<I', b, 24)[0])))
                elif kind in (0x110c, 0x110d):
                    print('  %s %s %s' % (K[kind], pdb.cstr(b, 10)[0], pdb.render(struct.unpack_from('<I', b, 0)[0])))
                elif kind == 0x1108:
                    print('  UDT %s %s' % (pdb.cstr(b, 4)[0], pdb.render(struct.unpack_from('<I', b, 0)[0])))
    else:
        print(__doc__)


if __name__ == '__main__':
    main()
