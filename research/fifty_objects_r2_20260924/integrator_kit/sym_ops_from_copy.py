"""Derive sym_edit.py ops from a worker's edited symbols.json copy vs HEAD's config/symbols.json.

    python scratch/campaign/sym_ops_from_copy.py <worker_symbols.json> <out_ops.json> [<base-ref>]
The worker copy is diffed against <base-ref>:config/symbols.json (default HEAD); use the commit the
worker started from so later integrated edits are not read as reversions.
Handles in-place static add/remove, renames at the same file_offset, and inserted lines.
"""
import difflib
import json
import re
import subprocess
import sys

base = subprocess.run(['git', 'show', (sys.argv[3] if len(sys.argv) > 3 else 'HEAD') + ':config/symbols.json'], capture_output=True, text=True, encoding='utf-8').stdout
base = base.replace('\r\n', '\n').split('\n')
new = open(sys.argv[1], 'rb').read().decode('utf-8').replace('\r\n', '\n').split('\n')
R = re.compile(r'"file_offset": (\d+), "flags": \d+, "name": "([^"]+)"')
ops = []
sm = difflib.SequenceMatcher(a=base, b=new, autojunk=False)
for tag, i1, i2, j1, j2 in sm.get_opcodes():
    if tag == 'equal':
        continue
    if tag == 'replace' and (i2 - i1) == (j2 - j1):
        for a, b in zip(base[i1:i2], new[j1:j2]):
            ma, mb = R.search(a), R.search(b)
            assert ma and mb and ma.group(1) == mb.group(1), (a, b)
            off, name = int(ma.group(1)), ma.group(2)
            if ma.group(2) != mb.group(2):
                ops.append({'op': 'rename', 'file_offset': off, 'name': name, 'new': mb.group(2)})
                name = mb.group(2)
            sa, sb = '"static": true' in a, '"static": true' in b
            if sb and not sa:
                ops.append({'op': 'static', 'file_offset': off, 'name': name})
            elif sa and not sb:
                ops.append({'op': 'unstatic', 'file_offset': off, 'name': name})
    elif tag == 'insert':
        anchor = R.search(base[i1 - 1])
        prev = (int(anchor.group(1)), anchor.group(2))
        # anchor must reference the (possibly renamed) previous line in new
        pm = R.search(new[j1 - 1])
        for k, l in enumerate(new[j1:j2]):
            ops.append({'op': 'insert_after', 'file_offset': int(pm.group(1)), 'name': pm.group(2), 'line': l.strip()})
            pm = R.search(l)
    elif tag == 'replace':
        # mixed: pair the common prefix, insert the rest
        n = min(i2 - i1, j2 - j1)
        for a, b in zip(base[i1:i1 + n], new[j1:j1 + n]):
            ma, mb = R.search(a), R.search(b)
            assert ma and mb and ma.group(1) == mb.group(1), (a, b)
            off, name = int(ma.group(1)), ma.group(2)
            if ma.group(2) != mb.group(2):
                ops.append({'op': 'rename', 'file_offset': off, 'name': name, 'new': mb.group(2)})
                name = mb.group(2)
            if '"static": true' in b and '"static": true' not in a:
                ops.append({'op': 'static', 'file_offset': off, 'name': name})
            elif '"static": true' in a and '"static": true' not in b:
                ops.append({'op': 'unstatic', 'file_offset': off, 'name': name})
        assert (i2 - i1) <= (j2 - j1), 'deletions unsupported'
        pm = R.search(new[j1 + n - 1])
        for l in new[j1 + n:j2]:
            ops.append({'op': 'insert_after', 'file_offset': int(pm.group(1)), 'name': pm.group(2), 'line': l.strip()})
            pm = R.search(l)
    else:
        raise SystemExit('unsupported opcode %s at %d' % (tag, i1))
json.dump(ops, open(sys.argv[2], 'w'), indent=1)
print(len(ops), 'ops')
for o in ops:
    print(o)
