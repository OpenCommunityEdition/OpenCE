"""H1: read-only XBE reader (Sept-2001 cachebeta.xbe etc.), as data."""
import struct, capstone
class Xbe:
    def __init__(self, path):
        self.b = b = open(path, 'rb').read()
        assert b[:4] == b'XBEH'
        self.base = struct.unpack_from('<I', b, 0x104)[0]
        nsec = struct.unpack_from('<I', b, 0x11C)[0]
        shdr = struct.unpack_from('<I', b, 0x120)[0] - self.base
        self.secs = []
        for i in range(nsec):
            o = shdr + i * 0x38
            flags, va, vsize, raw, rsize, nameaddr = struct.unpack_from('<IIIIII', b, o)
            no = nameaddr - self.base
            name = b[no:b.index(b'\0', no)].decode('latin-1')
            self.secs.append((name, va, vsize, raw, rsize, flags))
        self.md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        self.md.detail = True
    def va2off(self, va):
        for name, sva, vs, raw, rs, fl in self.secs:
            if sva <= va < sva + rs:
                return raw + va - sva
        return None
    def sec_of(self, va):
        for name, sva, vs, raw, rs, fl in self.secs:
            if sva <= va < sva + max(vs, rs):
                return name
        return None
    def dis(self, va, n):
        off = self.va2off(va)
        return list(self.md.disasm(self.b[off:off + n], va))
