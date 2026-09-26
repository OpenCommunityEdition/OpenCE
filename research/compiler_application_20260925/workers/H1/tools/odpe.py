"""H1: read-only helpers over the first-party /Od build (halo_cache_symbols.exe), as data."""
import struct, re, sys
import capstone
EXE = r'C:\Users\isabe\Documents\Codex\2026-07-13\i-w\research\symbol-build-h1-tags-20260906\halo_cache_symbols.exe'
B = open(EXE, 'rb').read()
pe = struct.unpack_from('<I', B, 0x3c)[0]
nsec = struct.unpack_from('<H', B, pe + 6)[0]
opt = struct.unpack_from('<H', B, pe + 20)[0]
BASE = struct.unpack_from('<I', B, pe + 24 + 28)[0]
SECS = []
for i in range(nsec):
    o = pe + 24 + opt + 40 * i
    name = B[o:o + 8].rstrip(b'\0').decode()
    vsize, va, rsize, raw = struct.unpack_from('<IIII', B, o + 8)
    SECS.append((name, BASE + va, vsize, rsize, raw))
MD = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
MD.detail = True

def va2off(va):
    for name, sva, vs, rs, raw in SECS:
        if sva <= va < sva + rs:
            return raw + va - sva
    return None

def off2va(off):
    for name, sva, vs, rs, raw in SECS:
        if raw <= off < raw + rs:
            return sva + off - raw
    return None

def text():
    for name, sva, vs, rs, raw in SECS:
        if name == '.text':
            return sva, B[raw:raw + rs]

def find_string(s):
    out = []
    for m in re.finditer(re.escape(s.encode()), B):
        out.append(off2va(m.start()))
    return out

def refs_to(va_lo, va_hi=None):
    """all offsets in .text whose 4-byte LE value lies in [va_lo, va_hi]"""
    if va_hi is None:
        va_hi = va_lo
    sva, t = text()
    out = []
    for i in range(len(t) - 3):
        v = struct.unpack_from('<I', t, i)[0]
        if va_lo <= v <= va_hi:
            out.append((sva + i, v))
    return out

def func_start(va):
    """scan back for /Od prologue 55 8b ec (push ebp; mov ebp,esp) preceded by padding cc or ret"""
    sva, t = text()
    i = va - sva
    while i > 0:
        if t[i:i + 3] == b'\x55\x8b\xec' and t[i - 1] in (0xcc, 0xc3, 0x90, 0xc2):
            return sva + i
        i -= 1
    return None

def dis(va, n=0x400, stop_ret=True):
    off = va2off(va)
    out = []
    for ins in MD.disasm(B[off:off + n], va):
        out.append(ins)
        if stop_ret and ins.mnemonic == 'ret' and False:
            break
    return out
