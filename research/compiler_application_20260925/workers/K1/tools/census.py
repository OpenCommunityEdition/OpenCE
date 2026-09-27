"""Slot census: every [ebp-X] reference with access widths and counts (plus lea). usage: census.py a.txt [b.txt]"""
import sys, re, collections
def census(path):
    c = collections.defaultdict(collections.Counter)
    for ln in open(path):
        for m in re.finditer(r'(byte|word|dword|qword) ptr \[ebp (?:\+ \w+(?:\*\d)? )?([-+]) (0x[0-9a-f]+|\d+)\]', ln):
            sz, sign, off = m.groups()
            off = int(off, 0) * (-1 if sign == '-' else 1)
            c[off][sz[0]] += 1
        for m in re.finditer(r'lea\s+\w+, \[ebp (?:\+ \w+(?:\*\d)? )?- (0x[0-9a-f]+|\d+)\]', ln):
            c[-int(m.group(1), 0)]['L'] += 1
    return c
for p in sys.argv[1:]:
    c = census(p)
    print('==', p, len([k for k in c if k < 0]), 'local slots')
    for off in sorted(c):
        if off < 0 and off > -0x200:
            print('  -%-5x %s' % (-off, ' '.join('%s%d' % kv for kv in sorted(c[off].items()))))
