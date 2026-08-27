"""Apply an edits JSON (LF-normalized find/replace) to a real CRLF source file."""
import sys, json
edits_path, target_path = sys.argv[1], sys.argv[2]
edits = json.load(open(edits_path))
raw = open(target_path, 'rb').read()
crlf = b'\r\n' in raw
text = raw.decode('latin-1')
n = 0
for e in edits:
    find, repl = e[0], e[1]
    if crlf:
        find = find.replace('\n', '\r\n')
        repl = repl.replace('\n', '\r\n')
    assert find in text, 'NOT FOUND: %r' % find[:70]
    text = text.replace(find, repl)
    n += 1
open(target_path, 'wb').write(text.encode('latin-1'))
print('applied', n, 'edits to', target_path)
