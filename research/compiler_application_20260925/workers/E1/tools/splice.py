"""E1 helper: splice a replacement into a CRLF source copy.
    python -B splice.py <src> <dst> <start_marker_file> <end_marker_file> <replacement_file>
Markers/replacement are LF text; the file's CRLF convention is preserved."""
import sys
src, dst, sm, em, rp = sys.argv[1:6]
raw = open(src, encoding='latin-1', newline='').read()
crlf = '\r\n' in raw
t = raw.replace('\r\n', '\n')
s = open(sm, encoding='latin-1').read().rstrip('\n')
e = open(em, encoding='latin-1').read().rstrip('\n')
r = open(rp, encoding='latin-1').read()
i = t.index(s)
j = t.index(e, i)
assert t.count(s) == 1, 'start marker not unique'
out = t[:i] + r + t[j:]
if crlf:
    out = out.replace('\n', '\r\n')
open(dst, 'w', encoding='latin-1', newline='').write(out)
print('spliced', i, j, 'crlf' if crlf else 'lf')
