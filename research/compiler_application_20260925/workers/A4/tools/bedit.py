"""Byte-preserving edit helper (keeps CRLF). Usage:
    python bedit.py <in> <out> <edits.json>
edits.json = [{"old": "...", "new": "...", "count": 1}, ...]  ("\n" in old/new is matched as the file's EOL)
Each edit must match EXACTLY `count` times (default 1) or the tool fails.
"""
import json
import sys

src, dst, spec = sys.argv[1:4]
data = open(src, 'rb').read()
eol = b'\r\n' if b'\r\n' in data else b'\n'
edits = json.load(open(spec, encoding='utf-8'))
for e in edits:
    old = e['old'].encode('latin-1').replace(b'\n', eol)
    new = e['new'].encode('latin-1').replace(b'\n', eol)
    n = data.count(old)
    want = e.get('count', 1)
    if n != want:
        sys.exit('edit %r matched %d times (want %d)' % (e['old'][:60], n, want))
    data = data.replace(old, new)
open(dst, 'wb').write(data)
print('wrote', dst, len(data), 'bytes, eol', 'CRLF' if eol == b'\r\n' else 'LF')
