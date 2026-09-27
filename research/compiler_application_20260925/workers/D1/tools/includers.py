"""D1: transitive #include consumers of a header (textual, quoted includes; resolution by path suffix).

    python -B includers.py <source_root> <header path relative to root, e.g. source/items/weapons.h> [--out FILE]

Prints every .c whose include closure contains the header (as unit paths without .c). Over-approximates when a
quoted include name is ambiguous (all suffix matches are followed), so no real consumer is missed.
"""
import os
import re
import sys

root_dir, header = sys.argv[1], sys.argv[2].replace(os.sep, '/')
out = sys.argv[sys.argv.index('--out') + 1] if '--out' in sys.argv else None
inc = re.compile(r'^\s*#\s*include\s+"([^"]+)"', re.M)
files = {}
for dp, dn, fn in os.walk(os.path.join(root_dir, 'source')):
    for f in fn:
        if f.endswith(('.c', '.h')):
            full = os.path.join(dp, f)
            rel = os.path.relpath(full, root_dir).replace(os.sep, '/')
            files[rel] = inc.findall(open(full, encoding='latin-1').read())
by_base = {}
for p in files:
    by_base.setdefault(os.path.basename(p), []).append(p)


def resolve(name):
    name = name.replace('\\', '/')
    return [p for p in by_base.get(os.path.basename(name), []) if p.endswith('/' + name) or p == name]


memo = {}


def closure(c):
    seen = set()
    st = [c]
    while st:
        x = st.pop()
        for n in files.get(x, []):
            for r in resolve(n):
                if r not in seen:
                    seen.add(r)
                    st.append(r)
    return seen


res = sorted(c[:-2] for c in files if c.endswith('.c') and header in closure(c))
print('%d transitive consumers of %s' % (len(res), header))
if out:
    open(out, 'w').write('\n'.join(res) + '\n')
else:
    print('\n'.join(res))
