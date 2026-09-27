"""Split a single-file unified diff into hunks; write a patch with only the selected (1-based) hunks.
usage: hunks.py <patch> list | hunks.py <patch> keep 2,3,4 <out.patch>"""
import re, sys
from pathlib import Path
b = Path(sys.argv[1]).read_bytes()
lines = b.splitlines(keepends=True)
head, hunks, cur = [], [], None
for l in lines:
    if l.startswith(b'@@'):
        cur = [l]; hunks.append(cur)
    elif cur is None:
        head.append(l)
    else:
        cur.append(l)
if sys.argv[2] == 'list':
    for i, h in enumerate(hunks, 1):
        body = b''.join(x for x in h[1:] if x[:1] in b'+-')[:160]
        print(i, h[0].strip().decode(), body.decode('latin1').replace('\n', ' | ')[:150])
else:
    keep = {int(x) for x in sys.argv[3].split(',')}
    Path(sys.argv[4]).write_bytes(b''.join(head) + b''.join(b''.join(h) for i, h in enumerate(hunks, 1) if i in keep))
    print('kept', sorted(keep), 'of', len(hunks))
