import sys
sys.path.insert(0, 'tools')
import coff_compare as cc
for path in sys.argv[1:]:
    obj = cc.load(path)
    print('==', path)
    for s in obj['sections']:
        if s['flags'] & 0x80 or s['name'].startswith('.bss') or s['name'].startswith('.data'):
            align = (s['flags'] >> 20) & 0xF
            print('  section #%d %-8s size=0x%X flags=0x%08X align=%s raw=0x%X' % (
                s['index'], s['name'], s['size'], s['flags'], (1 << (align - 1)) if align else 'default', s['raw']))
            syms = sorted([y for y in obj['symbols'] if y['section'] == s['index']], key=lambda y: (y['value'], y['name']))
            for y in syms:
                print('     +0x%06X  %-50s type=0x%X storage=%d' % (y['value'], y['name'], y['type'], y['storage']))
    # COMMON (undefined with value)
    com = [y for y in obj['symbols'] if y['section'] == 0 and y['value'] and y['storage'] == 2]
    for y in com:
        print('  COMMON %s size=0x%X' % (y['name'], y['value']))
