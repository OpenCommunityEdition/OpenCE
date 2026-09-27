"""List every relocation (any type) in every object under a root that targets one of the named symbols.
Prints object, section, the defining function of that section (if any), offset, type."""
import sys, struct, os
sys.path.insert(0, 'tools')
import coff_compare as cc

root = sys.argv[1]
names = set(sys.argv[2:])
for dp, dn, fns in os.walk(root):
    for f in sorted(fns):
        if not f.endswith('.obj'):
            continue
        p = os.path.join(dp, f)
        try:
            obj = cc.load(p)
        except Exception:
            continue
        data = obj['data']
        fn_at = {}
        for y in obj['symbols']:
            if y['section'] > 0 and y['type'] == 0x20:
                fn_at.setdefault(y['section'], []).append((y['value'], y['name']))
        for s in obj['sections']:
            if not s['reloc_count'] or (s['flags'] & 0x80):
                continue
            for i in range(s['reloc_count']):
                off = s['reloc'] + i * 10
                va, si, typ = struct.unpack_from('<LLH', data, off)
                t = obj['by_index'][si]
                if t['name'] in names:
                    owner = None
                    for v, n in sorted(fn_at.get(s['index'], [])):
                        if v <= va:
                            owner = (n, va - v)
                    rel = os.path.relpath(p, root).replace(os.sep, '/')
                    print('%s %s %s +0x%X type=%d -> %s' % (
                        rel, s['name'], ('%s+0x%X' % owner) if owner else '?', va, typ, t['name']))
