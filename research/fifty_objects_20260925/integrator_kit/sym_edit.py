"""In-place config/symbols.json line surgery (never re-serialises; preserves order and CRLF).

    python scratch/campaign/sym_edit.py ops.json [--dry]

ops.json: list of operations, each keyed by the exact file_offset and current name:
  {"op": "static", "file_offset": 75280, "name": "_foo"}             -> add "static": true
  {"op": "unstatic", "file_offset": 518208, "name": "_foo"}          -> remove "static": true
  {"op": "rename", "file_offset": 2600880, "name": "_old", "new": "_new"}
  {"op": "insert_after", "file_offset": 2600880, "name": "_anchor", "line": "{ ... },"}
Each op must match exactly one line (file_offset AND name) or the script aborts.
"""
import json
import re
import sys

ops = json.load(open(sys.argv[1]))
dry = '--dry' in sys.argv
p = 'config/symbols.json'
raw = open(p, 'rb').read().decode('utf-8')
crlf = '\r\n' in raw
lines = raw.split('\r\n' if crlf else '\n')


def find(off, name):
    pat = re.compile(r'"file_offset": %d, "flags": \d+, "name": "%s"' % (off, re.escape(name)))
    hits = [i for i, l in enumerate(lines) if pat.search(l)]
    if len(hits) != 1:
        sys.exit('ABORT: %d matches for %s @%d' % (len(hits), name, off))
    return hits[0]


inserts = []
for op in ops:
    i = find(op['file_offset'], op['name'])
    line = lines[i]
    if op['op'] == 'static':
        if '"static": true' in line:
            print('already static', op['name'])
            continue
        lines[i] = re.sub(r'\s*\}(,?)\s*$', r', "static": true }\1', line)
    elif op['op'] == 'unstatic':
        lines[i] = line.replace(', "static": true', '')
    elif op['op'] == 'rename':
        lines[i] = line.replace('"name": "%s"' % op['name'], '"name": "%s"' % op['new'])
    elif op['op'] == 'insert_after':
        lines.insert(i + 1, op['line'])
        print('inserted after %s: %s' % (op['name'], op['line']))
        continue
    else:
        sys.exit('unknown op ' + op['op'])
    print('%-12s %s -> %s' % (op['op'], line.strip(), lines[i].strip()))
for i, l in sorted(inserts, reverse=True):
    lines.insert(i + 1, l)
    print('inserted after line %d: %s' % (i + 1, l))
if not dry:
    open(p, 'wb').write(('\r\n' if crlf else '\n').join(lines).encode('utf-8'))
