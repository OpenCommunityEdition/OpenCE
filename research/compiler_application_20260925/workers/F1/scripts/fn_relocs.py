import sys, struct
sys.path.insert(0, 'tools')
import coff_compare as cc
path, fn = sys.argv[1], sys.argv[2]
obj = cc.load(path)
sym = cc.symbol(obj, fn)
sec = obj['sections'][sym['section'] - 1]
raw = cc._section_bytes(obj, sec)
data = obj['data']
print(path, fn, 'size', hex(sec['size']))
print(' bytes', raw.hex())
for i in range(sec['reloc_count']):
    off = sec['reloc'] + i * 10
    va, si, typ = struct.unpack_from('<LLH', data, off)
    t = obj['by_index'][si]
    add = struct.unpack_from('<l', raw, va)[0]
    print('  reloc +0x%02X type=%d -> %s (sec %d value 0x%X) inline addend 0x%X => 0x%X' % (va, typ, t['name'], t['section'], t['value'], add & 0xffffffff, (t['value'] + add) & 0xffffffff))
