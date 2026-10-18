#!/usr/bin/env python3
"""List the raw data blobs of an Xbox map that port/mac/xbox_blob.c converts, for
tools/mac_xbox_blob_probe.c: lines "kind region offset size [fields...]" (region 0: the tag
data; n: bsp n; offsets within the region). Read with Python, on any host."""
import struct, sys, zlib

BASE = 0x803a6000

def load(path):
    d = open(path, 'rb').read()
    return d[:0x800] + zlib.decompressobj().decompress(d[0x800:])

def u32(d, o): return struct.unpack_from('<I', d, o)[0]

def main():
    d = load(sys.argv[1])
    off, size = struct.unpack_from('<II', d, 0x10)
    count = u32(d, off + 0xc)
    inst = u32(d, off) - BASE + off
    def tag_off(a): return a - BASE          # offset in the tag-data region
    def at(a): return off + a - BASE         # offset in the image
    scenario = None
    for i in range(count):
        g, _, _, _, _, a = struct.unpack_from('<6I', d, inst + i * 32)
        if g == 0x73636e72: scenario = a
        elif g == 0x75737472:                        # 'ustr'
            n, p = struct.unpack_from('<II', d, at(a))
            for k in range(n):
                sz, _, _, addr = struct.unpack_from('<IIII', d, at(p) + k * 20)
                if sz and addr: print('words16 0', tag_off(addr), sz)
        elif g == 0x686d7420:                        # 'hmt ': text_data, UTF-16
            sz, _, _, addr = struct.unpack_from('<IIII', d, at(a))
            if sz and addr: print('words16 0', tag_off(addr), sz)
        elif g == 0x616e7472:                        # 'antr'
            n, p = struct.unpack_from('<II', d, at(a) + 116)
            for k in range(n):
                e = at(p) + k * 180
                typ, fc, fs, fit = struct.unpack_from('<hhhh', d, e + 32)
                nc, = struct.unpack_from('<h', d, e + 44)
                flags, = struct.unpack_from('<H', d, e + 58)
                tr = struct.unpack_from('<II', d, e + 92); ro = struct.unpack_from('<II', d, e + 108); sc = struct.unpack_from('<II', d, e + 124)
                isz, _, _, iad = struct.unpack_from('<IIII', d, e + 72)
                dsz, _, _, dad = struct.unpack_from('<IIII', d, e + 140)
                asz, _, _, aad = struct.unpack_from('<IIII', d, e + 160)
                print('animation 0', fc, fs, fit, nc, flags, *tr, *ro, *sc, isz, tag_off(iad) if isz else 0, dsz, tag_off(dad) if dsz else 0, asz, tag_off(aad) if asz else 0)
    r = at(scenario)
    sz, _, _, ad = struct.unpack_from('<IIII', d, r + 1140)
    print('hs_syntax 0', tag_off(ad), sz)
    n, p = struct.unpack_from('<II', d, r + 876)
    for k in range(n):
        e = at(p) + k * 64
        version = d[e + 34]
        sz, _, _, ad = struct.unpack_from('<IIII', d, e + 44)
        if sz: print('events 0', tag_off(ad), sz, version)
    bn, bp = struct.unpack_from('<II', d, r + 0x5a4)
    for k in range(bn):
        fo, fs, base = struct.unpack_from('<III', d, at(bp) + k * 32)
        root = u32(d, fo)
        sz, _, _, ad = struct.unpack_from('<IIII', d, fo + (root - base) + 320)
        if sz: print('words32', k + 1, ad - base, sz)

main()
