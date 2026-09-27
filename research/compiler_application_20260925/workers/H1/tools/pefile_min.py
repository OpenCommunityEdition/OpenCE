"""H1: minimal read-only PE reader for Win32 executables (as data)."""
import struct, re, capstone
class PE:
    def __init__(self, path):
        self.B = B = open(path, 'rb').read()
        pe = struct.unpack_from('<I', B, 0x3c)[0]
        nsec = struct.unpack_from('<H', B, pe + 6)[0]
        opt = struct.unpack_from('<H', B, pe + 20)[0]
        self.base = struct.unpack_from('<I', B, pe + 24 + 28)[0]
        self.secs = []
        for i in range(nsec):
            o = pe + 24 + opt + 40 * i
            name = B[o:o + 8].rstrip(b'\0').decode('latin-1')
            vsize, va, rsize, raw = struct.unpack_from('<IIII', B, o + 8)
            ch = struct.unpack_from('<I', B, o + 36)[0]
            self.secs.append((name, self.base + va, vsize, rsize, raw, ch))
        self.md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        self.md.detail = True
    def va2off(self, va):
        for name, sva, vs, rs, raw, ch in self.secs:
            if sva <= va < sva + rs:
                return raw + va - sva
    def off2va(self, off):
        for name, sva, vs, rs, raw, ch in self.secs:
            if raw <= off < raw + rs:
                return sva + off - raw
    def sec(self, name):
        return [s for s in self.secs if s[0] == name][0]
    def find_str(self, s):
        return [self.off2va(m.start()) for m in re.finditer(re.escape(s.encode()), self.B)]
    def refs(self, v):
        t = self.sec('.text')
        seg = self.B[t[4]:t[4] + t[3]]
        pat = struct.pack('<I', v)
        return [t[1] + m.start() for m in re.finditer(re.escape(pat), seg)]
    def dis(self, va, n):
        off = self.va2off(va)
        return list(self.md.disasm(self.B[off:off + n], va))
