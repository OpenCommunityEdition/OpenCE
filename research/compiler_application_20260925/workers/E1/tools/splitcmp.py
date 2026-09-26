"""E1 helper: compare every object of two or three csplit output roots by sha256 (read-only).

    python -B splitcmp.py <root_a> <root_b> [<root_c>]

Prints the object count of each root, then every relative path whose bytes differ between a and b (and b and c)."""
import hashlib
import os
import sys


def walk(root):
    out = {}
    for d, _, fs in os.walk(root):
        for f in fs:
            if f.endswith('.obj'):
                p = os.path.join(d, f)
                rel = os.path.relpath(p, root).replace(os.sep, '/')
                out[rel] = hashlib.sha256(open(p, 'rb').read()).hexdigest()
    return out


roots = sys.argv[1:]
maps = [walk(r) for r in roots]
for r, m in zip(roots, maps):
    print('%-60s %d objects' % (r, len(m)))
for i in range(len(maps) - 1):
    a, b = maps[i], maps[i + 1]
    keys = sorted(set(a) | set(b))
    diff = [k for k in keys if a.get(k) != b.get(k)]
    print('DIFF %s vs %s: %d' % (roots[i], roots[i + 1], len(diff)))
    for k in diff:
        print('   ', k)
