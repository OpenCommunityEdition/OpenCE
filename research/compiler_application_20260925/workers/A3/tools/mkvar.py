"""A3: build a scratch variant of a source file by exact (old -> new) replacements, preserving CRLF.

    python -B research/compiler_application_20260925/workers/A3/tools/mkvar.py <base.c> <edits.py> <out.c> [NAME ...]

<edits.py> defines EDITS = {name: [(old, new), ...]} written with '\\n' newlines; each old string must occur
exactly once (after earlier edits of the same variant). With NAME arguments, only those edit groups are applied,
in the given order; otherwise all groups in dict order. Never writes the worktree source.
"""
import os
import runpy
import sys

base, edits_py, out = sys.argv[1:4]
names = sys.argv[4:]
raw = open(base, 'rb').read()
crlf = b'\r\n' in raw
text = raw.decode('latin-1')
nl = '\r\n' if crlf else '\n'
groups = runpy.run_path(edits_py)['EDITS']
for name in (names or list(groups)):
    for old, new in groups[name]:
        o = old.replace('\n', nl)
        n = new.replace('\n', nl)
        c = text.count(o)
        if c != 1:
            sys.exit('edit %s: expected 1 occurrence, found %d: %r' % (name, c, old[:100]))
        text = text.replace(o, n)
os.makedirs(os.path.dirname(os.path.abspath(out)), exist_ok=True)
open(out, 'wb').write(text.encode('latin-1'))
print('wrote', out, 'groups', names or list(groups))
