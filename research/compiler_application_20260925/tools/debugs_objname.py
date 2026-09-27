"""Compare two objects' .debug$S after masking the S_OBJNAME output path (CodeView 8 record 0x0009)."""
import struct
import sys
from pathlib import Path


def dbg(p):
    b = Path(p).read_bytes()
    n = struct.unpack_from('<H', b, 2)[0]
    for i in range(n):
        o = 20 + i * 40
        if b[o:o + 8].rstrip(b'\0') == b'.debug$S':
            size, raw = struct.unpack_from('<II', b, o + 16)
            return b[raw:raw + size]
    return b''


def records(d):
    """Split the CodeView symbol stream after the 4-byte signature into (type, body) records."""
    out, k = [], 4
    while k + 4 <= len(d):
        reclen, rtype = struct.unpack_from('<HH', d, k)
        body = d[k + 4:k + 2 + reclen]
        out.append((rtype, body))
        k += 2 + reclen
    return d[:4], out, d[k:]


def masked(d):
    sig, recs, tail = records(d)
    norm = []
    for rtype, body in recs:
        if rtype == 0x0009:  # S_OBJNAME (16-bit length-prefixed name): signature u32 + Pascal string
            norm.append((rtype, body[:4] + b'<OBJNAME>'))
        else:
            norm.append((rtype, body))
    return sig, norm, tail


def main():
    for a, f in zip(sys.argv[1::2], sys.argv[2::2]):
        da, df = dbg(a), dbg(f)
        sa, ra, ta = records(da)
        paths = [body[5:5 + body[4]].decode('latin1') for rtype, body in ra if rtype == 0x0009]
        pathf = [body[5:5 + body[4]].decode('latin1') for rtype, body in records(df)[1] if rtype == 0x0009]
        print('%s\n  records %s | S_OBJNAME %s vs %s\n  equal after masking S_OBJNAME path: %s' % (
            a, [hex(t) for t, _ in ra], paths, pathf, masked(da) == masked(df)))


main()
