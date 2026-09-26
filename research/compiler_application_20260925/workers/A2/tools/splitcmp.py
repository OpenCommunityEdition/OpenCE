"""A2 lab tool (read-only): byte-compare every object of two csplit output roots.

    python -B splitcmp.py <rootA> <rootB>

Prints each object that differs (or exists on one side only) and a census line.
"""
import hashlib
import os
import sys


def census(root):
    out = {}
    for dp, dn, fn in os.walk(root):
        for f in fn:
            if f.endswith('.obj'):
                p = os.path.join(dp, f)
                out[os.path.relpath(p, root).replace('\\', '/')] = hashlib.sha256(open(p, 'rb').read()).hexdigest()
    return out


A, B = census(sys.argv[1]), census(sys.argv[2])
diff = 0
for k in sorted(A.keys() | B.keys()):
    if A.get(k) != B.get(k):
        diff += 1
        print('DIFFERS  %s  (%s / %s)' % (k, 'present' if k in A else 'ABSENT', 'present' if k in B else 'ABSENT'))
print('%d objects in A, %d in B, %d differ' % (len(A), len(B), diff))
